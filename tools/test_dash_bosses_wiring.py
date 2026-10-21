"""Wiring pins for the dashboard's boss list (owner request 2026-10-21: every boss
on the dashboard map, alive or dead, all the time).

The pure rules live in runtime/CommanderTelemetry.h (BossState, MergeBosses and
the `bosses` fields of BuildPayload, unit-tested in
tools/test_commander_telemetry.cpp). These pins prove the live code calls them:
the commands layer fills the list for every standing binding from a spawn cache
built once, the state is read on the lead's own map (its instance), and the
emitter copies the list into the datagram.
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
CMD = (ROOT / "commands/BotCommands.cpp").read_text()
EMIT = (ROOT / "runtime/ObservabilityEmitter.cpp").read_text()


def body(text, start, end="\n}\n"):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class BossListWiring(unittest.TestCase):
    def test_map_data_fills_the_boss_list(self):
        fn = body(CMD, "static void FillCommanderMapData(")
        self.assertIn("FillCommanderBosses(lead, ref);", fn)

    def test_the_list_is_read_on_the_leads_own_dungeon_map(self):
        fn = body(CMD, "static void FillCommanderBosses(")
        self.assertIn("Map* map = lead->GetMap();", fn)
        self.assertRegex(fn, r"if \(!map \|\| !map->IsDungeon\(\)\)\s*return;")
        self.assertIn("ref.bossesReported = true;", fn)
        self.assertIn("ref.bossesMap = lead->GetMapId();", fn)
        # the state of each spawn: the creature on THIS map (instance), else the instance's respawn time
        self.assertIn("map->GetCreature(", fn)
        self.assertIn("map->GetPersistentState()", fn)
        self.assertIn("persistent->GetCreatureRespawnTime(spawn.dbGuid)", fn)
        # the decision is the tested pure helper, handed the creature's own state and the respawn time against now
        self.assertIn("CommanderTelemetry::BossSpawnState(creature != nullptr, creature && creature->IsAlive(), respawnAt, static_cast<int64_t>(now))", fn)
        self.assertNotIn("CommanderTelemetry::BossState(", fn)
        self.assertIn("ref.bosses = CommanderTelemetry::MergeBosses(", fn)

    def test_no_grid_or_world_scan_per_report(self):
        fn = body(CMD, "static void FillCommanderBosses(")
        for scan in ("GetCreatureListWithEntryInGrid", "DoCreatureData", "Cell::Visit", "VisitAll"):
            self.assertNotIn(scan, fn)

    def test_the_spawn_cache_is_built_once_after_the_plans_are_sealed(self):
        fn = body(CMD, "static std::vector<DashBossSpawn> const* DashBossSpawnsOn(")
        self.assertIn("if (!plans.Sealed())", fn)
        self.assertIn("sObjectMgr.DoCreatureData(", fn)
        self.assertEqual(fn.count("sObjectMgr.DoCreatureData("), 1)
        self.assertRegex(fn, r"static bool built = false;")
        self.assertIn("built = true;", fn)
        # the same boss rule as the mob rank letter (RankKind): a plan boss / ai_dungeon_boss row, world-boss rank, instance bind
        self.assertIn("plans.IsBossEntry(", fn)
        self.assertIn("CREATURE_ELITE_WORLDBOSS", fn)
        self.assertIn("CREATURE_FLAG_EXTRA_INSTANCE_BIND", fn)
        # plan boss steps mark the run's bosses, and a boss step with no spawn row still gets a place
        self.assertIn("ai::PlanStepKind::Boss", fn)

    def test_the_emitter_copies_the_list(self):
        self.assertIn("info.bossesReported = ref.bossesReported;", EMIT)
        self.assertIn("info.bossesMap = ref.bossesMap;", EMIT)
        self.assertIn("info.bosses = ref.bosses;", EMIT)


if __name__ == "__main__":
    unittest.main()
