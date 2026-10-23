"""The run leader's approach stand, too-far legs, walk-back target and no-path resume glue.

Gnomeregan r24 slot 4 step 412, Sunken Temple r25 slot 4 step 5, Razorfen Kraul r24
slot 8 steps 299 and 331-343 (2026-10-23). The policy is tested in
tools/test_dungeon_run_policy.cpp; this pins the glue in commands/BotCommands.cpp:
- the approach stand (DungeonRun::ApproachStand): after the tick the glue runs the
  unreachable move's stand-in search once when asked (NearestReachablePoint within
  kApproachStandYd of the step's point) and hands the answer to ApproachStandFound;
- the too-far legs (DungeonRun::TooFarLegWanted): a walk's move refused beyond
  ReactDistance sends a leg along the lead bot's navmesh path (LegOnPath, never a
  shortcut) instead of the pause, and only a refused leg falls back to OrderRefused.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMMANDS = (ROOT / "commands/BotCommands.cpp").read_text()
POLICY = (ROOT / "runtime/DungeonRunPolicy.h").read_text()


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class ApproachStandWiringTest(unittest.TestCase):
    def test_glue_searches_the_stand_when_asked(self):
        tick = body(COMMANDS, "void TickDungeonRun(uint64 commanderRaw, uint32 dt)", "\n}\n")
        i = tick.index("if (slot.run.approachStandWanted)")
        self.assertLess(tick.index("std::string const order = TortoiseBots::DungeonRun::Tick(slot.run, facts, dt);"), i)
        self.assertLess(i, tick.index("if (!order.empty())"))
        block = tick[i:tick.index("if (!order.empty())", i)]
        self.assertIn("NearestReachablePoint(lead, step->x, step->y, step->z, TortoiseBots::DungeonRun::kApproachStandYd,\n"
                      "                sx, sy, sz);", block)
        self.assertIn("TortoiseBots::DungeonRun::ApproachStandFound(slot.run, facts, found, sx, sy, sz);", block)
        self.assertIn("lead->IsAlive() && lead->IsInWorld()", block)

    def test_hop_cap_asks_the_stand_before_its_pause(self):
        fn = body(POLICY, "inline void ApproachHop(Run& r, const Facts& f, const PlanStep& s,", "\n}\n")
        cap = fn[fn.index("if (++r.approachHops > kMaxApproachHops)"):]
        self.assertLess(cap.index("if (ApproachStandWanted(r, s))"), cap.index("Pause(r, why);"))
        # The stand's own states come before any hop.
        self.assertLess(fn.index("if (ApproachStand(r, s, dt, f, order, what))"), fn.index("r.hopWalk = true;"))

    def test_resume_keeps_the_stand_and_step_done_clears_it(self):
        resume = body(POLICY, "inline bool Resume(Run& r)", "\n}\n")
        self.assertIn("if (r.approachStandStep == r.step)", resume)
        self.assertIn("r.approachStandResumed = true;", resume)
        reset = body(POLICY, "inline void ResetStep(Run& r)", "\n}\n")
        self.assertNotIn("approachStand", reset)
        self.assertIn("ClearApproachStand(r);", body(POLICY, "inline void NextStep(Run& r, const std::string& note)", "\n}\n"))


class TooFarLegWiringTest(unittest.TestCase):
    def test_refused_too_far_sends_a_leg_before_the_refusal(self):
        tick = body(COMMANDS, "void TickDungeonRun(uint64 commanderRaw, uint32 dt)", "\n}\n")
        i = tick.index("TortoiseBots::DungeonRun::TooFarLegWanted(it->second.run, order, answer)")
        self.assertLess(tick.index("bool const stallDirectRefused = !accepted &&"), i)
        self.assertLess(i, tick.index("if (!accepted && !stallDirectRefused && !legTaken)"))
        block = tick[i:tick.index("if (!accepted && !stallDirectRefused && !legTaken)")]
        self.assertIn("LegOnPath(lead, run.walkX, run.walkY, run.walkZ, legYd, lx, ly, lz)", block)
        self.assertIn("std::min(TortoiseBots::DungeonRun::kTooFarLegYd, sPlayerbotAIConfig.reactDistance - 10.0f)", block)
        self.assertIn("IssueRunOrder(commander, TortoiseBots::DungeonRun::MoveOrder(lx, ly, lz, false),", block)
        # The run is looked up again after the leg's order (it may release the binding).
        self.assertIn("it = DungeonRuns().find(commanderRaw);\n                if (it == DungeonRuns().end())\n                    return;", block)
        self.assertIn("TooFarLegTaken(it->second.run, facts, lx, ly, lz, legAccepted,", block)
        self.assertIn("OrderRefused(it->second.run, order, answer + legNote +", tick)

    def test_leg_follows_the_navmesh_path(self):
        fn = body(COMMANDS, "bool LegOnPath(Player* lead,", "\n}\n")
        self.assertIn("if (type & (PATHFIND_NOPATH | PATHFIND_NOT_USING_PATH | PATHFIND_SHORTCUT))\n        return false;", fn)
        self.assertIn("for (Vector3 const& p : path.getPath())", fn)
        self.assertIn("return TortoiseBots::DungeonRun::PointAlongPath(points, maxYd, ox, oy, oz);", fn)

    def test_walk_sends_its_point_again_after_the_leg(self):
        fn = body(POLICY, "inline bool WalkTo(Run& r, const Facts& f,", "\n}\n")
        i = fn.index("if (r.legOn && (toLeg <= kArriveYd || r.walkStillMs >= kShortStallMs))")
        self.assertLess(i, fn.index("if (r.walkTries >= kWalkTries)"))
        fresh = body(fn, "if (fresh)", "return false;")
        self.assertIn("r.legOn = false;", fresh)
        self.assertIn("r.walkLegs = 0;", fresh)


if __name__ == "__main__":
    unittest.main()
