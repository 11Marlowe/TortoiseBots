"""A stalled walk whose lead bot's own move failed tries a direct move and a dry stand-in.

Blackfathom Deeps 2026-10-22/23: the lead bot stood in a deep pool, its own move
FAILED every 2 s, the run leader re-sent the same `cmd move` three times and
paused; a stall never reached the stand-in of the unreachable-move fix (that
runs on a refusal only). The policy is tested in tools/test_dungeon_run_policy.cpp
(TestStalledWalkWithAFailingMoveTriesDirectThenADryStandIn); this pins the glue:
- WalkTo asks StallFallback before its retry and pause;
- after the tick the glue runs the dry stand-in search once when asked
  (NearestReachablePoint with dryOnly) and hands the answer to StallStandIn;
- a refused stall direct move goes to StallDirectRefused, not to the pause;
- the dry search refuses a wet point and a swim-line path;
- the direct limits are asserted against the direct step's.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMMANDS = (ROOT / "commands/BotCommands.cpp").read_text()
POLICY = (ROOT / "runtime/DungeonRunPolicy.h").read_text()


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class StallFallbackWiringTest(unittest.TestCase):
    def test_walk_asks_the_fallback_before_retrying(self):
        fn = body(POLICY, "inline bool WalkTo(Run& r, const Facts& f,", "\n}\n")
        i = fn.index("if (StallFallback(r, f, x, y, z, direct, order))\n            return false;")
        self.assertLess(i, fn.index("if (r.walkTries >= kWalkTries)"))
        self.assertLess(fn.index("if (r.walkStillMs >= kWalkRetryMs)\n    {"), i)
        # A fresh walk starts the fallback over.
        fresh = body(fn, "if (fresh)", "return false;")
        self.assertIn("r.stallFallback = 0;", fresh)
        self.assertIn("r.stallStandInWanted = false;", fresh)

    def test_glue_searches_a_dry_stand_in_when_asked(self):
        i = COMMANDS.index("    DropUnattackableCombat(lead, bots, slot.run.combatDrops);\n")
        block = COMMANDS[i:COMMANDS.index("if (!order.empty())", i)]
        self.assertLess(COMMANDS.index("std::string const order = TortoiseBots::DungeonRun::Tick(slot.run, facts, dt);"), i)
        self.assertIn("if (slot.run.stallStandInWanted)", block)
        self.assertIn("NearestReachablePoint(lead, step->x, step->y, step->z, TortoiseBots::DungeonRun::MoveFallbackYd(*step),\n"
                      "                dx, dy, dz, true);", block)
        self.assertIn("TortoiseBots::DungeonRun::StallStandIn(slot.run, found, dx, dy, dz);", block)

    def test_refused_direct_does_not_pause(self):
        self.assertIn("bool const stallDirectRefused = !accepted &&\n"
                      "            TortoiseBots::DungeonRun::StallDirectRefused(it->second.run, order, answer);\n"
                      "        if (!accepted && !stallDirectRefused)", COMMANDS)

    def test_dry_search_refuses_water_and_swim_lines(self):
        fn = body(COMMANDS, "bool NearestReachablePoint(Player* lead,", "\n}\n")
        self.assertIn("bool dryOnly = false)", fn)
        self.assertIn("if (dryOnly && map->GetTerrain()->getLiquidStatus(px, py, ground + 0.5f, MAP_ALL_LIQUIDS, nullptr) != LIQUID_MAP_NO_WATER)\n"
                      "            return false;", fn)
        self.assertIn("if (dryOnly && (type & (PATHFIND_UNDERWATER | PATHFIND_NOT_USING_PATH | PATHFIND_SHORTCUT)))\n"
                      "            return false;", fn)
        # The refusal path's own search stays as it was (not dry only).
        self.assertIn("NearestReachablePoint(lead, step->x, step->y, step->z, TortoiseBots::DungeonRun::MoveFallbackYd(*step),\n"
                      "                    dx, dy, dz);", COMMANDS)

    def test_direct_limits_follow_the_direct_step(self):
        self.assertIn("static_assert(TortoiseBots::DungeonRun::kStallDirectMaxYd == RemoteCommander::kDirectMaxDistance &&\n"
                      "    TortoiseBots::DungeonRun::kStallDirectMaxVertical == RemoteCommander::kDirectMaxVertical,", COMMANDS)


if __name__ == "__main__":
    unittest.main()
