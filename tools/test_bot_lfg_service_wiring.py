"""Bot LFG service wiring (part 2, Task 7).

Pins that runtime/BotLfgService.cpp forms bot LFG groups from the proven
plans (ai_bot_lfg_plan), scans eligible bots through BotLfg::CheckCandidate,
picks the plan with BotLfg::PickPlan, composes with BotLfg::ComposeWithOpen,
takes the Dungeon lease before inviting, ends with release-or-disband, and
ticks from host/BotLfgAdapter.cpp; plus the four BotCommands exports.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
CPP = (ROOT / "runtime/BotLfgService.cpp").read_text()
CMAKE = (ROOT / "TortoiseBots.cmake").read_text()
MODULE = (ROOT / "host/Module.cpp").read_text()
CMD = (ROOT / "commands/Commander.cpp").read_text()


def strip_comments(text):
    return "\n".join(line.split("//", 1)[0].rstrip() for line in text.splitlines())


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


SVC = strip_comments(CPP)


class BotLfgServiceWiringTest(unittest.TestCase):
    def test_build_and_tick_registration(self):
        self.assertIn('"${TORTOISEBOTS_ROOT}/runtime/BotLfgService.cpp"', CMAKE)
        self.assertIn('"${TORTOISEBOTS_ROOT}/host/BotLfgAdapter.cpp"', CMAKE)
        self.assertIn("new BotLfgAdapter();", MODULE)

    def test_enabled(self):
        fn = body(SVC, "bool BotLfgService::Enabled() const", "\n}\n")
        self.assertIn(
            "RemoteCommander::BindingKindEnabled(RemoteCommander::BindingKind::System, "
            "BotCommands::CurrentCommanderGates())",
            fn)
        self.assertIn("sPlayerbotAIConfig.enabled", fn)

    def test_load_plans(self):
        fn = body(SVC, "uint32 BotLfgService::LoadPlans()", "\n}\n")
        for snippet in ("FROM `ai_bot_lfg_plan` ", "LftDungeonRange(",
                        "ai::GetDungeonPlans().Find(", "sMapStorage.LookupEntry<MapEntry>(",
                        "Instanceable()"):
            self.assertIn(snippet, fn)

    def test_scan_candidates(self):
        fn = body(SVC, "std::vector<BotLfgService::Scanned> BotLfgService::ScanCandidates()", "\n}\n")
        for snippet in ("BotLfg::CheckCandidate(", "BotCommands::IsLfgWorldBot(",
                        "BotCommands::IsBotBound(", "IsAvailableForBackground(", "sLFTMgr.IsQueued(",
                        "IsHardcore()", "RoleMismatchReason("):
            self.assertIn(snippet, fn)

    def test_try_form_order(self):
        fn = body(SVC, "bool BotLfgService::TryForm(", "\n}\n")
        self.assertLess(fn.index("BotLfg::PickPlan("), fn.index("BotLfg::ComposeWithOpen("))
        self.assertLess(fn.index("BotLfg::ComposeWithOpen("), fn.index("TryAcquire("))
        self.assertLess(fn.index("TryAcquire("), fn.index("BotCommands::JoinBotToLeadGroup("))
        self.assertIn("BotActivity::Dungeon, 0)", fn)
        self.assertNotIn("BotLfg::TargetGroups(", fn)
        self.assertIn("sPlayerbotAIConfig.botLfgMaxGroups", fn)

    def test_end_releases_or_disbands(self):
        fn = body(SVC, "void BotLfgService::End(", "\n}\n")
        for snippet in ("if (!release)", "RemoveMember(", "Release(", "BotActivity::Dungeon",
                        "ClearForcedRole("):
            self.assertIn(snippet, fn)
        self.assertLess(fn.index("if (!release)"), fn.index("RemoveMember("))

    def test_update_ticks_then_forms(self):
        fn = body(SVC, "void BotLfgService::Update(", "\n}\n")
        for snippet in ("BotLfg::TargetGroups(", "BotLfg::FormDue(",
                        "sPlayerbotAIConfig.botLfgOnlyWithPlayersOnline", "TickGroups("):
            self.assertIn(snippet, fn)
        self.assertLess(fn.index("TickGroups("), fn.index("BotLfg::FormDue("))

    def test_shutdown(self):
        fn = body(SVC, "void BotLfgService::Shutdown()", "\n}\n")
        self.assertIn("BotLfgLifecycle::EndReason::Shutdown", fn)

    def test_commander_exports(self):
        anchor = CMD.index("bool JoinLeadGroup(")
        anchor = CMD.index("} // namespace", anchor)
        tail = CMD[anchor:]
        for snippet in ("bool IsLfgWorldBot(Player* bot)", "return IsWorldBot(bot);",
                        "bool JoinBotToLeadGroup(Player* lead, Player* member)",
                        "return JoinLeadGroup(lead, member);", "bool IsBotBound(Player* bot)",
                        "uint64 GroupBindingKey(uint32 groupId)"):
            self.assertIn(snippet, tail, snippet)


if __name__ == "__main__":
    unittest.main()
