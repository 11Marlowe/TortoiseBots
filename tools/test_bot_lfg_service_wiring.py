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
        for snippet in ("release = release || RealPlayerInGroup(group);", "if (!release)", "RemoveMember(",
                        "Release(", "BotActivity::Dungeon", "ClearForcedRole("):
            self.assertIn(snippet, fn)
        self.assertLess(fn.index("release = release || RealPlayerInGroup(group);"), fn.index("RemoveMember("))
        self.assertLess(fn.index("if (!release)"), fn.index("RemoveMember("))

    def test_real_player_lookup_by_group_id(self):
        fn = body(SVC, "bool BotLfgService::RealPlayerInGroup(", "\n}\n")
        self.assertIn("sObjectMgr.GetGroupById(group.groupId)", fn)
        self.assertNotIn("FindPlayer(group.leader)", fn)

    def test_end_now_erases_at_once(self):
        fn = body(SVC, "bool BotLfgService::EndNow(", "\n}\n")
        self.assertIn("m_groups.erase(std::remove_if(", fn)

    def test_form_stamp_has_no_bit_trick(self):
        self.assertNotIn("m_nowMs | 1", SVC)
        fn = body(SVC, "bool BotLfgService::TryForm(", "\n}\n")
        self.assertIn("FormStamp{m_nowMs, true}", fn)
        self.assertIn("found->second.formed", SVC)
        self.assertIn("m_nowMs - found->second.atMs", SVC)

    def test_missing_grace(self):
        tick = body(SVC, "void BotLfgService::TickGroups(bool enabled)", "\n}\n")
        for snippet in ("LeaderMissing(group, leader, grp, ours)", "MemberMissing(group,"):
            self.assertIn(snippet, tick)
        self.assertNotIn("facts.leaderLost = !leader || !leader->IsInWorld()", tick)
        track = body(SVC, "bool BotLfgService::TrackOutOfWorld(", "\n}\n")
        self.assertIn("BotLfgLifecycle::MissingGraceExpired(", track)
        self.assertIn("group.missingSinceMs.erase(raw)", track)

    def test_load_plans_skips_when_off(self):
        fn = body(SVC, "uint32 BotLfgService::LoadPlans()", "\n}\n")
        self.assertIn("!sPlayerbotAIConfig.botLfgGroups", fn)
        self.assertIn("!ai::GetDungeonPlans().Sealed()", fn)

    def test_shutdown_first_and_idempotent(self):
        fn = body(SVC, "void BotLfgService::Shutdown()", "\n}\n")
        self.assertIn("BotLfgLifecycle::EndReason::Shutdown", fn)
        self.assertIn("if (m_shutDown)", fn)
        host = strip_comments((ROOT / "host/BotHostAdapter.cpp").read_text())
        shutdown = body(host, "void BotHostAdapter::OnShutdown()", "\n}\n")
        self.assertLess(shutdown.index("BotLfgService::Instance().Shutdown();"),
                        shutdown.index("BotCommandQueue::Instance().Clear();"))
        self.assertLess(shutdown.index("BotLfgService::Instance().Shutdown();"),
                        shutdown.index("BotManager::Instance().Shutdown();"))


    def test_commander_exports(self):
        anchor = CMD.index("bool JoinLeadGroup(")
        anchor = CMD.index("} // namespace", anchor)
        tail = CMD[anchor:]
        for snippet in ("bool IsLfgWorldBot(Player* bot)", "return IsWorldBot(bot);",
                        "bool JoinBotToLeadGroup(Player* lead, Player* member)",
                        "return JoinLeadGroup(lead, member);", "bool IsBotBound(Player* bot)",
                        "uint64 GroupBindingKey(uint32 groupId)"):
            self.assertIn(snippet, tail, snippet)


class BotLfgLifecycleWiringTest(unittest.TestCase):
    def test_tick_groups_drives_lifecycle(self):
        fn = body(SVC, "void BotLfgService::TickGroups(bool enabled)", "\n}\n")
        self.assertIn("BotLfgLifecycle::DecideStep(facts)", fn)
        self.assertLess(fn.index("facts.realPlayerInGroup = RealPlayerInGroup(group);"),
                        fn.index("BotLfgLifecycle::DecideStep(facts)"))
        self.assertLess(fn.index("BotCommands::GroupBindingKey(group.groupId)"),
                        fn.index("BotLfgLifecycle::DecideStep(facts)"))
        self.assertLess(fn.index("BotLfgLifecycle::FillDue("),
                        fn.index("BotLfgLifecycle::DecideStep(facts)"))
        for snippet in ("case BotLfgLifecycle::Step::FillWithBot:",
                        "case BotLfgLifecycle::Step::Replace:",
                        "case BotLfgLifecycle::Step::ToReady:",
                        "case BotLfgLifecycle::Step::End:",
                        "End(group, decision.reason, decision.release);"):
            self.assertIn(snippet, fn)
        self.assertIn("sPlayerbotAIConfig.botLfgFillAfterMinutes * 60000", fn)

    def test_add_member_takes_lease_before_invite(self):
        fn = body(SVC, "bool BotLfgService::AddMember(", "\n}\n")
        for snippet in ("TryAcquire(", "BotCommands::JoinBotToLeadGroup(", "ApplyForcedRole("):
            self.assertIn(snippet, fn)
        self.assertLess(fn.index("TryAcquire("), fn.index("JoinBotToLeadGroup("))

    def test_end_now_gm_disband(self):
        fn = body(SVC, "bool BotLfgService::EndNow(", "\n}\n")
        for snippet in ("BotLfgLifecycle::EndReason::GmDisband", "RealPlayerInGroup("):
            self.assertIn(snippet, fn)

    def test_fill_now_marks_force_fill(self):
        fn = body(SVC, "bool BotLfgService::FillNow(", "\n}\n")
        self.assertIn("forceFill = true", fn)

    def test_reload_plans_reports_bot_lfg(self):
        fn = body(CMD, "static bool HandleReloadPlans(", "\n}\n")
        self.assertIn("BotLfgService::Instance().LoadPlans()", fn)
        self.assertLess(fn.index("report.outcome.swapped"), fn.index("BotLfgService::Instance().LoadPlans()"))

class BotLfgCommandWiringTest(unittest.TestCase):
    def test_dungeon_run_dispatches_lfg(self):
        fn = body(strip_comments(CMD), "bool HandleDungeonRun(", "\n}\n")
        needle = 'if (verb == "lfg")\n        return HandleBotLfgRun(handler, rawArg, intent);'
        self.assertIn(needle, fn)
        self.assertLess(fn.index(needle), fn.index('if (verb != "start"'))

    def test_bot_lfg_run_handler(self):
        fn = body(strip_comments(CMD), "static bool HandleBotLfgRun(", "\n}\n")
        self.assertLess(fn.index("IsBotAdministrator("), fn.index("BotLfgService::Instance().Enabled()"))
        self.assertLess(fn.index("BotLfgService::Instance().Enabled()"), fn.index("BotLfgLifecycle::ParseArgs("))
        for snippet in ("IsGmCliHandler(handler)", "FormNow(", "FillNow(", "EndNow(",
                        'SendActionAck(handler, intent, "groups"'):
            self.assertIn(snippet, fn)

    def test_template_mentions_commands(self):
        tpl = (ROOT / "ai/playerbot/aiplayerbot.conf.dist.in").read_text()
        self.assertNotIn("forming the groups is not built yet", tpl)
