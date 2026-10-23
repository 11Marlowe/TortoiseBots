"""The glue of the player-led (hybrid) dungeon run (runtime/PlayerLedPolicy.h, 2026-10-24).

The policy (tools/test_player_led_policy.cpp) decides; these checks pin that commands/BotCommands.cpp and the config
hand it the facts and act on its answers. S1: `.bot run start ... player [tank <Bot>]` makes its own binding of the
commander's party (the commander stays leader, no follow target changes, no form-up on the tank), the binding's state
reads whether the commander leads, a refused start releases the binding again, and a stopped, done or switched-off
player-led run releases its binding. `.bot run mode` names the mode.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMMANDS = (ROOT / "commands/BotCommands.cpp").read_text()
CONFIG = (ROOT / "ai/playerbot/PlayerbotAIConfig.cpp").read_text()
TEMPLATE = (ROOT / "ai/playerbot/aiplayerbot.conf.dist.in").read_text()


def strip_comments(text):
    return "\n".join(line.split("//", 1)[0].rstrip() for line in text.splitlines())


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class PlayerLedBindWiringTest(unittest.TestCase):
    def test_keys_default_off_and_hot_reload(self):
        keys = body(CONFIG, "void PlayerbotAIConfig::LoadDungeonRunKeys()", "\n}\n")
        self.assertIn('dungeonRunPlayerLed = config.GetBoolDefault("AiPlayerbot.DungeonRunPlayerLed", false);', keys)
        self.assertIn('config.GetFloatDefault("AiPlayerbot.DungeonRunPlayerLedPullYd", 40.0f)', keys)
        self.assertIn("AiPlayerbot.DungeonRunPlayerLed = 0", TEMPLATE)
        self.assertIn("AiPlayerbot.DungeonRunPlayerLedPullYd = 40", TEMPLATE)

    def test_binding_state_reads_the_commanders_lead(self):
        state = strip_comments(body(COMMANDS, "RemoteCommander::BindingState ReadBindingState(", "\n}\n"))
        self.assertIn("state.commanderIsLeader = group->IsLeader(::ObjectGuid(binding.commanderGuid));", state)

    def test_player_led_bind(self):
        bind = strip_comments(body(COMMANDS, "static bool BindPlayerLed(", "\n}\n"))
        self.assertIn("PL::CheckBind(facts)", bind)
        self.assertIn("sPlayerbotAIConfig.dungeonRunPlayerLed && sPlayerbotAIConfig.dungeonSelfClear", bind)
        self.assertIn("sPlayerbotAIConfig.remoteCommander", bind)
        self.assertIn("facts.commanderLeads = group->IsLeader(commander->GetObjectGuid());", bind)
        self.assertIn("binding.playerLed = true;", bind)
        self.assertIn("binding.inParty = true;", bind)
        self.assertIn("binding.leadGuid = tank->GetObjectGuid().GetRawValue();", bind)
        # The commander keeps the lead and the bots keep following him: no lead change, no form-up on the tank,
        # no follow target pointed at the tank, nobody leaves or joins the party.
        for absent in ("ChangeLeader", "FormUpOnLead", "PointFollowerAtLead", "RemoveMember", "JoinLeadGroup",
                       "DropCommanderFromParty"):
            self.assertNotIn(absent, bind)
        # Hires stay hires while bound, as with `bind ... inparty`.
        self.assertIn("hires.BeginCommand(", bind)
        self.assertIn("hires.ConfirmCommand(", bind)

    def test_run_start_player_word(self):
        run = strip_comments(body(COMMANDS, "static bool HandleDungeonRun(", "\n}\n"))
        self.assertIn("TortoiseBots::PlayerLed::TakeStart(rawArg)", run)
        self.assertIn("BindPlayerLed(handler, commander, playerLedStart.tank, intent)", run)
        self.assertIn("arg = playerLedStart.rest;", run)
        self.assertIn("slot.playerLed.on = playerLedStart.playerLed;", run)
        # A refused start after the bind releases it; a started run keeps it.
        self.assertIn("playerLedGuard.commanderRaw = commanderRaw;", run)
        self.assertIn("playerLedGuard.commanderRaw = 0;", run)
        guard = strip_comments(body(COMMANDS, "struct PlayerLedStartGuard", "\n};\n"))
        self.assertIn("ReleaseBinding(commanderRaw, RemoteCommander::ReleaseReason::Unbound, false);", guard)
        # The player stands in the plan's dungeon.
        self.assertIn("playerLedStart.playerLed && plan->map != commander->GetMapId()", run)
        # `.bot run mode`.
        self.assertIn('verb != "mode"', run)
        self.assertIn("TortoiseBots::PlayerLed::ModeText(", run)

    def test_player_led_binding_ends_with_the_run(self):
        run = strip_comments(body(COMMANDS, "static bool HandleDungeonRun(", "\n}\n"))
        stop = run[run.index('if (verb == "stop")'):]
        stop = stop[:stop.index("return true;")]
        self.assertIn("if (playerLed)", stop)
        self.assertIn("ReleaseBinding(commanderRaw, RemoteCommander::ReleaseReason::Unbound, false);", stop)
        tick = strip_comments(body(COMMANDS, "void TickDungeonRun(uint64 commanderRaw, uint32 dt)", "\n}\n"))
        self.assertIn("slot.playerLed.on && (!sPlayerbotAIConfig.dungeonRunPlayerLed ||", tick)
        self.assertGreaterEqual(tick.count("ReleaseBinding(commanderRaw, RemoteCommander::ReleaseReason::Unbound, false);"), 2)
        update = strip_comments(body(COMMANDS, "static void UpdateDungeonRuns(uint32 diff)", "\n}\n"))
        self.assertIn("ReleaseBinding(commanderRaw, RemoteCommander::ReleaseReason::Disabled, true);", update)


class PlayerLedOrderWiringTest(unittest.TestCase):
    def test_player_orders_do_not_pause_a_player_led_run(self):
        pause = strip_comments(body(COMMANDS, "static void PauseDungeonRunForPlayer(uint64 commanderRaw, std::string const& order)\n{", "\n}\n"))
        led = pause.index("it->second.playerLed.on")
        self.assertLess(led, pause.index("TortoiseBots::DungeonRun::PauseForPlayer("))
        self.assertIn("TortoiseBots::PlayerLed::NotePlayerOrder(it->second.playerLed, it->second.run, ProtocolSafe(order))", pause)
        # The run's own orders never reach it.
        self.assertLess(pause.index("if (s_runIssuing)"), led)

    def test_leading_players_chat_is_an_order(self):
        chat = strip_comments(body(COMMANDS, "bool NoteHumanChatOrder(Player* bot, Player& from, std::string const& text, bool whisper)", "\n}\n"))
        led = chat.index("if (fromCommander && run->second.playerLed.on)")
        self.assertLess(led, chat.index("RemoteCommander::ClassifyRunBoundOrder("))
        branch = chat[led:chat.index("RemoteCommander::ClassifyRunBoundOrder(")]
        self.assertIn("PauseDungeonRunForPlayer(commanderRaw,", branch)
        self.assertIn("return true;", branch)

    def test_every_order_hook_still_reaches_the_note(self):
        # `.bot action`, `.bot cmd` and the chat hook all go through PauseDungeonRunForPlayer.
        code = strip_comments(COMMANDS)
        self.assertIn('PauseDungeonRunForPlayer(requester->GetObjectGuid().GetRawValue(), "action " + intent);', code)
        self.assertIn('PauseDungeonRunForPlayer(order.commander->GetObjectGuid().GetRawValue(), "cmd " + verb);', code)


if __name__ == "__main__":
    unittest.main()
