"""The core's straight swim answer reaches the bot's own move (SwimPathPolicy.h).

Blackfathom Deeps 2026-10-22/23: a lead bot standing in a deep pool got the
core's straight swim path (NORMAL|UNDERWATER 0x81, NORMAL|NOT_USING_PATH 0x11)
from the PathFinder. WorldPosition::getPathStepFrom kept only type ==
PATHFIND_NORMAL (or INCOMPLETE), threw that answer away, and the move failed
(dispatch-short) every 2 s, while `cmd move` had acked it. The pure rule and
the water test are tested in tools/test_swim_path_policy.cpp; this pins the
glue:
- getPathStepFrom decides by ClassifyPathStepType / KeepPathStep, never by an
  equality test on PATHFIND_NORMAL, and asks the water and sight test
  (SwimPathFits) only for a swim answer;
- SwimPathFits samples every leg in water or ocean only (no magma, no slime),
  asks the line of sight with dynamic objects, and refuses a bot on a
  transport or on another map;
- the policy's bits are static_asserted against the core's.
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
WORLDPOS = (ROOT / "ai/playerbot/WorldPosition.cpp").read_text()
MOVES = (ROOT / "ai/playerbot/strategy/actions/MovementActions.cpp").read_text()


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class SwimPathWiringTest(unittest.TestCase):
    def setUp(self):
        self.step = body(WORLDPOS, "std::vector<WorldPosition> WorldPosition::getPathStepFrom(const WorldPosition& startPos, std::unique_ptr<PathFinder>& pathfinder",
                         "\n}\n")
        self.fits = body(WORLDPOS, "bool SwimPathFits(", "\n    }\n}\n")

    def test_step_keeps_by_the_policy(self):
        self.assertIn("ClassifyPathStepType(static_cast<uint32>(type), forceNormalPath)", self.step)
        self.assertIn("verdict == PathStepVerdict::KeepIfSwimLine && SwimPathFits(retvec, bot)", self.step)
        self.assertIn("if (KeepPathStep(static_cast<uint32>(type), forceNormalPath, swimLine, swimLine))\n        return retvec;", self.step)
        # The old equality test is gone from the final decision.
        self.assertNotIn("|| type == PATHFIND_NORMAL)", self.step)

    def test_step_keeps_the_underwater_incomplete_branch(self):
        self.assertIn("if (type == PATHFIND_INCOMPLETE)", self.step)
        self.assertIn("lastPoint.isUnderWater() && end.isUnderWater() && lastPoint.IsInLineOfSight(end)", self.step)

    def test_fits_samples_water_only_and_asks_sight(self):
        self.assertIn("MAP_LIQUID_TYPE_WATER | MAP_LIQUID_TYPE_OCEAN", self.fits)
        self.assertNotIn("MAP_ALL_LIQUIDS", self.fits)
        self.assertIn("ai::SwimLineInWater(a.getX(), a.getY(), a.getZ(), b.getX(), b.getY(), b.getZ(), liquidAt)", self.fits)
        self.assertIn("            if (!map->isInLineOfSight(a.getX(), a.getY(), a.getZ() + 0.5f, b.getX(), b.getY(), b.getZ() + 0.5f, true))\n                return false;", self.fits)
        self.assertIn("            if (!ai::SwimLineInWater(", self.fits)
        # Every leg, not just the first.
        self.assertIn("for (size_t i = 1; i < path.size(); ++i)", self.fits)

    def test_fits_refuses_transport_and_other_map(self):
        self.assertIn("if (!bot || bot->GetTransport() || path.size() < 2)\n            return false;", self.fits)
        self.assertIn("a.GetMapId() != map->GetId() || b.GetMapId() != map->GetId()", self.fits)

    def test_bits_are_asserted_against_the_core(self):
        for name in ("NORMAL", "SHORTCUT", "INCOMPLETE", "NOPATH", "NOT_USING_PATH", "DEST_FORCED", "FLYPATH", "UNDERWATER"):
            self.assertRegex(WORLDPOS, r"ai::kPathType\w+ == PATHFIND_%s\b" % name)
        for name in ("NO_WATER", "ABOVE_WATER", "WATER_WALK", "IN_WATER", "UNDER_WATER"):
            self.assertRegex(WORLDPOS, r"ai::kLiquid\w+ == LIQUID_MAP_%s\b" % name)

    def test_no_other_bot_move_path_tests_type_by_equality(self):
        # getPathStepFrom was the bot's route filter; the module's own move code
        # must not grow a second equality filter on the route.
        self.assertNotRegex(MOVES, r"getPathType\(\)\s*==\s*PATHFIND_NORMAL\s*\)\s*\n\s*return")


if __name__ == "__main__":
    unittest.main()
