"""The glue of the bot-led run with real players in the party (1 to 4 players, the rest bots).

The policy (runtime/RemoteCommanderPolicy.h, runtime/DungeonRunPolicy.h, tools/test_player_in_party_policy.cpp) decides;
these checks pin that commands/BotCommands.cpp and ai/playerbot/PlayerbotAI.cpp hand it the facts and act on its
answers: the `inparty` bind keeps the commander in the party, the binding's state does not count the commander as a
joined player, a bot leader of a commanded group keeps the lead, a player's chat or `.bot action` is classed (pause,
chat, refusal), the run reads every player of the bound group, and the run's revive, summon and teleport still reach
bots only.
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMMANDS = (ROOT / "commands/BotCommands.cpp").read_text()
AI = (ROOT / "ai/playerbot/PlayerbotAI.cpp").read_text()
QUERY = (ROOT / "runtime/CommandedGroupQuery.h").read_text()


def strip_comments(text):
    return "\n".join(line.split("//", 1)[0].rstrip() for line in text.splitlines())


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class PlayerInPartyWiringTest(unittest.TestCase):
    def test_binding_state_does_not_count_the_commander(self):
        state = strip_comments(body(COMMANDS, "RemoteCommander::BindingState ReadBindingState(", "\n}\n"))
        self.assertIn("slot.guid.GetRawValue() != binding.commanderGuid && !BotManager::Instance().IsBot(slot.guid)", state)

    def test_inparty_bind_keeps_the_commander(self):
        bind = strip_comments(body(COMMANDS, "static bool HandleCommanderBind(", "\nstatic bool HandleCommander("))
        self.assertIn("bool const inParty = RemoteCommander::TakeBindInParty(names);", bind)
        self.assertIn("facts.inParty = inParty;", bind)
        self.assertIn("facts.partyPlayers", bind)
        self.assertIn("binding.inParty = inParty;", bind)
        # The commander is never dropped from an in-party binding's party.
        self.assertIn("if (keepParty && !inParty && lead->GetGroup() && !DropCommanderFromParty(lead->GetGroup(), commander))",
                      bind)
        full = bind.index("lead->GetGroup()->IsFull())")
        self.assertLess(bind.index("if (inParty)\n                return failBind(\"party-full\"", full),
                        bind.index("if (!DropCommanderFromParty(lead->GetGroup(), commander))", full))
        # Without the word the plan is today's; with it the commander's party is kept whoever plays in it.
        self.assertIn("RemoteCommander::BindGroupPlan const plan = inParty\n"
                      "        ? (planFacts.leadInCommanderParty ? RemoteCommander::BindGroupPlan::KeepCommanderParty", bind)
        self.assertIn(": RemoteCommander::DecideBindGroup(planFacts);", bind)
        # Other real players stay members, never bots to command; unnamed BOTS still refuse the bind.
        self.assertIn("if (inParty && !BotManager::Instance().IsBot(slot.guid))\n                continue;", bind)
        self.assertIn("BotManager::Instance().IsBot(slot.guid) &&\n"
                      "                    std::find(named.begin(), named.end(), slot.guid) == named.end())", bind)

    def test_commanded_bot_leader_keeps_the_lead(self):
        code = strip_comments(AI)
        self.assertRegex(code, r"if \(group->IsLeader\(bot->GetObjectGuid\(\)\) &&\s*"
                               r"!TortoiseBots::BotCommands::IsInCommandedGroup\(bot\)\)")

    def test_chat_order_is_classed(self):
        code = strip_comments(AI)
        self.assertIn("if (!TortoiseBots::BotCommands::NoteHumanChatOrder(bot, fromPlayer, filtered, "
                      "type == CHAT_MSG_WHISPER))", code)
        self.assertIn("bool NoteHumanChatOrder(Player* bot, Player& from, std::string const& text, bool whisper);", QUERY)
        note = strip_comments(body(COMMANDS, "bool NoteHumanChatOrder(Player* bot, Player& from,", "\n}\n"))
        self.assertIn("RemoteCommander::ClassifyRunBoundOrder(", note)
        self.assertIn("case RemoteCommander::RunOrderVerdict::PauseRun:", note)
        self.assertIn("case RemoteCommander::RunOrderVerdict::Refuse:", note)
        self.assertIn("PauseDungeonRunForPlayer(commanderRaw,", note)
        self.assertIn("RemoteCommander::RunBoundRefusalText(", note)

    def test_bot_action_of_another_player_is_refused(self):
        action = strip_comments(body(COMMANDS, "static bool HandleAction(ChatHandler* handler, char const* args)", "\n}\n"))
        self.assertIn("    if (parsed)\n        if (uint64 const owner = RunBoundForeignCommander(requester))", action)
        refuse = action.index("RunBoundForeignCommander(requester)")
        self.assertLess(refuse, action.index("\"run-bound\""))
        self.assertLess(action.index("\"run-bound\""), action.index("PauseDungeonRunForPlayer("))
        # The commander's own order still pauses.
        self.assertIn("PauseDungeonRunForPlayer(requester->GetObjectGuid().GetRawValue(), \"action \" + intent);", action)

    def test_run_reads_every_player(self):
        tick = strip_comments(body(COMMANDS, "void TickDungeonRun(uint64 commanderRaw, uint32 dt)", "\n}\n"))
        read = tick.index("ReadRunFacts(slot, lead, bots, facts);")
        players = tick.index("ReadRunPlayers(slot, commanderRaw, lead, group, facts);")
        health = tick.index("TortoiseBots::DungeonRun::ApplyPlayerHealth(facts);")
        order = tick.index("TortoiseBots::DungeonRun::Tick(slot.run, facts, dt)")
        self.assertTrue(read < players < health < order)
        fn = strip_comments(body(COMMANDS, "void ReadRunPlayers(DungeonRunSlot const& slot,", "\n}\n"))
        self.assertIn("if (BotManager::Instance().IsBot(entry.guid))\n            continue;", fn)
        self.assertIn("p.commander = entry.guid.GetRawValue() == commanderRaw;", fn)
        self.assertIn("p.onMap = member->IsInWorld() && member->GetMapId() == slot.run.plan->map", fn)
        self.assertIn("p.ghost = member->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST)", fn)
        self.assertIn("f.players.push_back(p);", fn)
        self.assertIn("f.itemPlayer", fn)
        self.assertIn("f.lootTurnPlayer", fn)

    def test_revive_summon_and_teleport_reach_bots_only(self):
        fill = strip_comments(body(COMMANDS, "void FillCommandedBots(Group* group, Player* lead,", "\n}\n"))
        self.assertIn("IsLiveHeadlessBot(member)", fill)
        self.assertIn("order.bots = CommandedBots(order.group, order.lead);", COMMANDS)
        revive = strip_comments(body(COMMANDS, "if (word == \"party\")", "SendActionAck(handler, intent, \"party\", revived"))
        self.assertIn("for (Player* member : order.bots)", revive)
        teleport = strip_comments(body(COMMANDS, "bool RunStrandedTeleport(Player* commander, std::string const& order, std::string& answer)\n{", "\n}\n"))
        self.assertEqual(teleport.count("NearTeleportTo("), 1)
        self.assertIn("lead->NearTeleportTo(", teleport)


if __name__ == "__main__":
    unittest.main()
