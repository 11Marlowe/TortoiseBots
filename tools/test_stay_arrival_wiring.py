"""The stay walk and its trigger share one arrival rule and one back-off.

Crescent Grove r26 step 272 (2026-10-23): a tauren lead bot stood 5.67 yd from its
point on the ground and 6.6 yd below it. The walk's isUseful measured the ground
distance minus the bounding radius (4.7 yd: USELESS, every `cmd move` refused), the
trigger 3D minus the radius (active every tick) and the run leader's deadband the raw
ground distance (not arrived): the run paused four times. All three now call
TortoiseBots::StayPointReached (runtime/StayArrivalPolicy.h, raw 3D distance) on the
stay point's map; the walk and its trigger also share a back-off after failed walks.
The run leader's deadband was removed (on the 3D rule its stalled arrival covers it). The other MoveToPositionAction qualifiers (guard, return,
pull) keep their own rule.
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
ACTIONS = ROOT / "ai/playerbot/strategy/actions"
POSITION = (ACTIONS / "PositionAction.cpp").read_text()
POSITION_H = (ACTIONS / "PositionAction.h").read_text()
TRIGGERS = (ROOT / "ai/playerbot/strategy/triggers/GenericTriggers.cpp").read_text()
RUN = (ROOT / "runtime/DungeonRunPolicy.h").read_text()
POLICY = (ROOT / "runtime/StayArrivalPolicy.h").read_text()

LAST_MOVE = (ROOT / "ai/playerbot/strategy/values/LastMovementValue.h").read_text()
COMMANDS = (ROOT / "commands/BotCommands.cpp").read_text()

REACHED = ("bool const reached = TortoiseBots::StayPointReached(bot->GetPositionX() - stayPosition.x, "
           "bot->GetPositionY() - stayPosition.y,\n        {indent}bot->GetPositionZ() - stayPosition.z, ai->GetRange(\"follow\"));")
BACKOFF = ('TortoiseBots::StayWalkBackoff& backoff = AI_VALUE(LastMovement&, "last movement").stayWalk;',
           "TortoiseBots::StayWalkTrack(backoff, stayPosition.x, stayPosition.y, stayPosition.z, stayPosition.mapId);",
           "return TortoiseBots::StayWalkDue(backoff, reached, WorldTimer::getMSTime());")


def body(text, start, end="\n}\n"):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class StayArrivalWiringTest(unittest.TestCase):
    def test_rule_is_raw_3d(self):
        fn = body(POLICY, "inline bool StayPointReached(")
        self.assertIn("return std::sqrt(dx * dx + dy * dy + dz * dz) <= range;", fn)

    def test_stay_walk_is_useful_by_the_shared_rule(self):
        cls = body(POSITION_H, "class ReturnToStayPositionAction : public MoveToPositionAction", "\n    };\n")
        self.assertIn("virtual bool isUseful() override;", cls)
        self.assertIn("virtual bool Execute(Event& event) override;", cls)
        self.assertIn('#include "../../../../runtime/StayArrivalPolicy.h"', POSITION)
        fn = body(POSITION, "bool ReturnToStayPositionAction::isUseful()")
        self.assertIn('PositionEntry stayPosition = AI_VALUE(PositionMap&, "position")["stay"];', fn)
        # The map check the old "distance" value made (DistanceValue.h: 0 on another map).
        self.assertIn("if (!stayPosition.isSet() || stayPosition.mapId != bot->GetMapId() || !ai->CanMove())\n        return false;", fn)
        self.assertIn(REACHED.format(indent=""), fn)
        for line in BACKOFF:
            self.assertIn(line, fn)
        self.assertNotIn('"distance"', fn)

    def test_stay_walk_feeds_the_backoff(self):
        fn = body(POSITION, "bool ReturnToStayPositionAction::Execute(Event& event)")
        self.assertIn("bool const moved = MoveToPositionAction::Execute(event);", fn)
        self.assertIn('TortoiseBots::StayWalkResult(AI_VALUE(LastMovement&, "last movement").stayWalk, moved, WorldTimer::getMSTime());', fn)
        self.assertIn("return moved;", fn)
        # The back-off lives with the movement state, survives clear() and copies.
        self.assertIn('#include "../../../../runtime/StayArrivalPolicy.h"', LAST_MOVE)
        self.assertIn("TortoiseBots::StayWalkBackoff stayWalk;", LAST_MOVE)
        self.assertIn("stayWalk = other.stayWalk;", LAST_MOVE)
        self.assertNotIn("stayWalk", body(LAST_MOVE, "void clear()", "\n        }\n"))

    def test_other_qualifiers_keep_their_rule(self):
        fn = body(POSITION, "bool MoveToPositionAction::isUseful()")
        self.assertIn('AI_VALUE2(float, "distance", std::string("position_") + qualifier)', fn)
        self.assertNotIn("StayPointReached", fn)

    def test_trigger_uses_the_shared_rule(self):
        self.assertIn('#include "../../../../runtime/StayArrivalPolicy.h"', TRIGGERS)
        fn = body(TRIGGERS, "bool ReturnToStayPositionTrigger::IsActive()")
        self.assertIn("if (stayPosition.isSet() && stayPosition.mapId == bot->GetMapId())", fn)
        self.assertIn(REACHED.format(indent="    "), fn)
        for line in BACKOFF:
            self.assertIn(line, fn)
        self.assertNotIn("GetDistance(", fn)

    def test_run_leader_sees_a_resting_walk_as_failing(self):
        # A walk resting after failures still reads as a failing move for the stall's fallback.
        fn = body(COMMANDS, "void ReadRunFacts(DungeonRunSlot const& slot, Player* lead,")
        self.assertIn("f.leadMoveFailed = ai::PositionMoveFailing(lastMove.positionMoveFailMs, WorldTimer::getMSTime()) ||\n"
                      "            TortoiseBots::StayWalkBackingOff(lastMove.stayWalk, WorldTimer::getMSTime());", fn)

    def test_run_leader_has_no_separate_deadband(self):
        self.assertNotIn("InFollowDeadband", RUN.replace("separate deadband rule, InFollowDeadband, was removed", ""))
        self.assertNotIn("plainWalk", RUN)
        self.assertIn('static_assert(kStalledArriveYd >= 5.0f,', RUN)

    def test_only_these_callers(self):
        callers = []
        for path in list((ROOT / "ai").rglob("*.cpp")) + list((ROOT / "runtime").rglob("*.h")) + \
                list((ROOT / "commands").rglob("*.cpp")):
            if path.name == "StayArrivalPolicy.h":
                continue
            n = len(re.findall(r"\bStayPointReached\(", path.read_text(errors="replace")))
            if n:
                callers.append((path.name, n))
        self.assertEqual(sorted(callers), [("GenericTriggers.cpp", 1), ("PositionAction.cpp", 1)])


if __name__ == "__main__":
    unittest.main()
