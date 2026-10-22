"""Wiring pins for `.bot run reloadplans` (owner goal 2026-10-22: fix dungeon plan blockers by a hot reload instead of an
image rebuild and a server restart).

The pure rules (a refused row or a plan gone keeps the old set, the per-plan changes, the replaced table kept alive while
a run's plan points into it) live in ai/playerbot/DungeonPlanReloadPolicy.h and are unit-tested in
tools/test_dungeon_plan_reload_policy.cpp. These pins prove the live code uses them: the command is registered under
`.bot run`, GameMaster-gated before anything else is read, reads the rows through the SAME loaders the startup uses,
hands every run's plan pointer to the commit, and rebuilds the boss spawn cache after a swap.
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
CMD = (ROOT / "commands/BotCommands.cpp").read_text()
MGR = (ROOT / "ai/playerbot/DungeonPlanMgr.cpp").read_text()
TACTICS = (ROOT / "ai/playerbot/BossTacticsMgr.cpp").read_text()
DOC_CMD = (ROOT / "docs/play/commands/commander.md").read_text()
DOC_RUN = (ROOT / "docs/play/dungeon-run-leader.md").read_text()


def body(text, start, end="\n}\n"):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class ReloadPlansWiring(unittest.TestCase):
    def test_registered_under_bot_run_before_the_run_leader_checks(self):
        run = body(CMD, "static bool HandleDungeonRun(", "\nvoid UpdateRemoteCommanders(")
        hook = 'if (verb == "reloadplans")\n        return HandleReloadPlans(handler, intent);'
        self.assertIn(hook, run)
        # before the in-game requirement and the DungeonSelfClear/RemoteCommander gate: a console GM may reload too
        self.assertLess(run.index(hook), run.index("Player* commander = Requester(handler);"))
        self.assertLess(run.index(hook), run.index("sPlayerbotAIConfig.dungeonSelfClear"))
        self.assertIn("/ reloadplans (GameMaster)", run)
        # `.bot run` reaches HandleDungeonRun
        self.assertIn('if (cmd == "run")\n        return HandleDungeonRun(handler, subArgs);', CMD)

    def test_gm_gated_first(self):
        fn = body(CMD, "static bool HandleReloadPlans(")
        gate = "if (requester ? !IsBotAdministrator(requester) : !IsGmCliHandler(handler))"
        self.assertIn(gate, fn)
        self.assertIn('SendActionError(handler, intent, "forbidden", "Only a GameMaster may reload the dungeon plans.");', fn)
        self.assertLess(fn.index(gate), fn.index("ai::ReloadDungeonPlans("))
        self.assertLess(fn.index(gate), fn.index("RebuildBossSpawnCache("))
        cli = body(CMD, "static bool IsGmCliHandler(")
        self.assertIn("if (!handler || handler->GetSession())\n        return false;", cli)
        self.assertIn("cli && cli->GetAccessLevel() >= SEC_GAMEMASTER", cli)
        # the GM rank: account security >= SEC_GAMEMASTER (3)
        admin = (ROOT / "commands/BotCommandContext.cpp").read_text()
        self.assertIn("requester->GetSession()->GetSecurity() >= SEC_GAMEMASTER", admin)

    def test_every_runs_plan_is_handed_to_the_reload(self):
        fn = body(CMD, "static bool HandleReloadPlans(")
        self.assertRegex(fn, r"for \(auto const& entry : DungeonRuns\(\)\)\s*if \(entry\.second\.run\.plan\)\s*"
                             r"inUse\.push_back\(entry\.second\.run\.plan\);")
        self.assertIn("ai::ReloadDungeonPlans(inUse)", fn)

    def test_refusal_answers_and_changes_nothing_else(self):
        fn = body(CMD, "static bool HandleReloadPlans(")
        refused = fn[fn.index("if (!report.outcome.swapped)"):]
        refused = refused[:refused.index("return true;")]
        self.assertIn('SendActionError(handler, intent, "refused", why.c_str());', refused)
        self.assertIn("report.outcome.reason", refused)
        # the spawn cache is rebuilt only after a swap
        self.assertLess(fn.index("if (!report.outcome.swapped)"), fn.index("RebuildBossSpawnCache(ms, maps)"))

    def test_answer_prints_the_boot_line_and_the_changes(self):
        fn = body(CMD, "static bool HandleReloadPlans(")
        self.assertIn('"%s, reloaded (dungeon bosses %u -> %u, boss tactic rows %u -> %u)", report.summary.c_str()', fn)
        self.assertIn("for (std::string const& change : report.outcome.changes)", fn)
        self.assertIn('"Boss spawn cache rebuilt in %u ms for %u maps"', fn)
        self.assertIn('"Boss spawn cache NOT rebuilt: ', fn)
        self.assertIn('SendActionAck(handler, intent, "plans", uint32(plans.PlanCount()));', fn)

    def test_reload_reads_through_the_startup_loaders(self):
        load = body(MGR, "void ai::LoadDungeonPlans()")
        reload = body(MGR, "PlanReloadReport ai::ReloadDungeonPlans(")
        # one row reader for both
        self.assertIn("ReadDungeonPlanRows(table, counts);", load)
        self.assertIn("ReadDungeonPlanRows(*next, counts);", reload)
        self.assertEqual(MGR.count("WorldDatabase.Query("), 4)
        for select in ("FROM `ai_dungeon_plan`", "FROM `ai_dungeon_leg`", "FROM `ai_dungeon_step`", "FROM `ai_dungeon_boss`"):
            self.assertIn(select, body(MGR, "static void ReadDungeonPlanRows("))
        # the boot line is the policy's, so the reload prints the same one
        self.assertIn('sLog.outString(">> %s", PlanLoadSummary(table, counts).c_str());', load)
        self.assertIn("report.summary = PlanLoadSummary(*next, counts);", reload)
        # boss tactics through the startup reader too
        self.assertIn("uint32 const skippedTactics = ReadBossTacticRows(*nextTactics);", reload)
        self.assertIn("ReadBossTacticRows(table);", body(TACTICS, "void ai::LoadBossTactics()"))
        self.assertEqual(TACTICS.count("WorldDatabase.Query("), 1)
        # the decision is the tested commit, into the holders the getters read
        self.assertIn("CommitPlanReload(DungeonPlanHolder(), std::move(next), counts, BossTacticsHolder(), "
                      "std::move(nextTactics),\n        skippedTactics, inUse);", reload)
        self.assertIn("return DungeonPlanHolder().Current();", body(MGR, "DungeonPlanTable& ai::GetDungeonPlans()"))
        self.assertIn("return BossTacticsHolder().Current();", body(TACTICS, "BossTacticTable& ai::GetBossTactics()"))
        # never reloads before the startup load sealed the set
        self.assertRegex(reload, r"if \(!GetDungeonPlans\(\)\.Sealed\(\)\)")

    def test_every_refused_row_is_counted_for_the_reload(self):
        rows = body(MGR, "static void ReadDungeonPlanRows(")
        self.assertIn("++skippedPlans;", rows)
        self.assertIn("++counts.badUnits;", rows)
        self.assertIn("++skippedSteps;", rows)
        self.assertIn("++skippedBosses;", rows)
        self.assertEqual(rows.count("counts.Note("), 4)
        self.assertIn("std::uint32_t& skippedSteps = counts.skippedSteps;", rows)

    def test_cache_rebuild_clears_first(self):
        fn = body(CMD, "static bool RebuildBossSpawnCache(uint32& ms, uint32& maps)\n{")
        self.assertLess(fn.index("cache.built = false;"), fn.index("BuildBossSpawnCache();"))
        self.assertLess(fn.index("cache.byMap.clear();"), fn.index("BuildBossSpawnCache();"))
        self.assertIn("cache.buildMs = uint32(ms);", body(CMD, "void BuildBossSpawnCache()"))

    def test_documented(self):
        self.assertIn("`.bot run reloadplans`", DOC_CMD)
        self.assertIn("`.bot run reloadplans`", DOC_RUN)


if __name__ == "__main__":
    unittest.main()
