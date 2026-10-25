"""The system binding glue: a server-owned binding of a bot-only party
(runtime/SystemRunPolicy.h), whose replies go to the server log.

The policy decides; these checks pin that commands/Commander.cpp wires it:
the reply path (ServerLogChatHandler/CommanderReplyHandler), the world-bot
gate (IsWorldBot), the commanded set, the run line, the tick's joined-player
pause, the bind entry point, telemetry, the chat refusal name and the trace
anchor.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMMANDER = (ROOT / "commands" / "Commander.cpp").read_text()
HANDLERS = (ROOT / "commands" / "BotCommandHandlers.h").read_text()


def strip_comments(text):
    return "\n".join(line.split("//", 1)[0].rstrip() for line in text.splitlines())


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


COMMANDS = strip_comments(COMMANDER)


class SystemReplyWiringTest(unittest.TestCase):
    def test_log_handler(self):
        self.assertIn("class ServerLogChatHandler final : public ChatHandler", COMMANDS)
        self.assertIn("void SendSysMessage(char const* str) override", COMMANDS)

    def test_run_line_goes_to_the_log(self):
        run = body(COMMANDS, "void SendRunLine(", "\n}\n")
        self.assertIn("IsSystemBinding(", run)
        self.assertIn("TB_LOG_BASIC(", run)
        self.assertLess(run.index("IsSystemBinding("),
                        run.index("ChatHandler(commander).PSendSysMessage"))

    def test_issue_run_order_uses_the_log_handler(self):
        order = body(COMMANDS, "bool IssueRunOrder(", "\n}\n")
        self.assertIn("ServerLogChatHandler", order)
        self.assertNotIn("ChatHandler handler(commander);", order)

    def test_reply_sites_use_the_handler(self):
        for start in ("void TaskReplyAck(", "void TaskReplyErr(", "void RitualReply(",
                      "void NoteEnterResult("):
            with self.subTest(start=start):
                site = body(COMMANDS, start, "\n}\n")
                self.assertIn("CommanderReplyHandler(", site)
                self.assertNotIn("ChatHandler handler(commander);", site)

    def test_release_notifies_through_the_handler(self):
        release = body(COMMANDS, "uint32 ReleaseBinding(", "\n}\n")
        self.assertIn("CommanderReplyHandler(commanderRaw, removed.system)", release)

    def test_reply_handler_ignores_bot_sessions(self):
        handler = body(COMMANDS, "std::unique_ptr<ChatHandler> CommanderReplyHandler(", "\n}\n")
        self.assertIn("BotManager::Instance().IsBot(", handler)


class SystemBindingWiringTest(unittest.TestCase):
    def test_commanded_bots_are_world_bots(self):
        fill = body(COMMANDS, "void FillCommandedBots(", "\n}\n")
        self.assertIn("IsWorldBot(", fill)

    def test_world_bot_gate(self):
        gate = body(COMMANDS, "bool IsWorldBot(Player* bot)\n{", "\n}\n")
        self.assertIn("ownerAccountId", gate)
        self.assertIn("HireLifecycle::Instance().IsHired(", gate)
        self.assertIn("ClaimLifecycle::Instance().IsClaimed(", gate)
        self.assertIn("AltBotService::Instance().IsAutonomous(", gate)
        self.assertIn("HasNetworkTransport()", gate)

    def test_owner_checks_follow_the_binding(self):
        self.assertEqual(COMMANDS.count("CanControlBot(order.commander"), 1)

    def test_tick_pauses_for_a_joined_player(self):
        tick = body(COMMANDS, "void TickDungeonRun(uint64 commanderRaw, uint32 dt)", "\n}\n")
        self.assertLess(tick.index("TortoiseBots::SystemRun::DecideJoinStep("),
                        tick.index("if (slot.run.state != TortoiseBots::DungeonRun::State::Running || !commander)"))
        self.assertIn("TortoiseBots::SystemRun::kPlayerJoinedNote", tick)
        self.assertIn("slot.systemPausedForPlayer", tick)

    def test_bind_entry_point(self):
        bind = body(COMMANDS, "uint64 BindSystemParty(", "\n}\n")
        self.assertLess(bind.index("TortoiseBots::SystemRun::CheckBind(facts)"),
                        bind.index("CommanderBindings().Bind(binding)"))
        self.assertIn("binding.system = true;", bind)
        self.assertIn("binding.commanderGuid = binding.leadGuid;", bind)
        self.assertIn("BindSystemParty(Player* lead, std::vector<Player*> const& joiners, std::string& code, std::string& message)", HANDLERS)

    def test_trace_follows_the_anchor_binding(self):
        trace = body(COMMANDS, "static bool TraceContext(", "\n}\n")
        self.assertIn("->system", trace)


class SystemSurfaceWiringTest(unittest.TestCase):
    def test_telemetry_names_the_system_commander(self):
        self.assertIn('ref.commander = "system"', COMMANDS)

    def test_chat_refusal_names_the_system_run(self):
        self.assertIn('"the server\'s dungeon run"', COMMANDS)


if __name__ == "__main__":
    unittest.main()
