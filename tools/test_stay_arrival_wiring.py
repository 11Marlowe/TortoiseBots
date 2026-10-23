"""The stay walk, its trigger and the run leader's deadband share one arrival rule.

Crescent Grove r26 step 272 (2026-10-23): a tauren lead bot stood 5.67 yd from its
point on the ground and 6.6 yd below it. The walk's isUseful measured the ground
distance minus the bounding radius (4.7 yd: USELESS, every `cmd move` refused), the
trigger 3D minus the radius (active every tick) and the run leader's deadband the raw
ground distance (not arrived): the run paused four times. All three now call
TortoiseBots::StayPointReached (runtime/StayArrivalPolicy.h, raw 3D distance), so they
cannot drift apart again. The other MoveToPositionAction qualifiers (guard, return,
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

BOT_TO_STAY = ("TortoiseBots::StayPointReached(bot->GetPositionX() - stayPosition.x, "
               "bot->GetPositionY() - stayPosition.y,\n        bot->GetPositionZ() - stayPosition.z, "
               "ai->GetRange(\"follow\"))")


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
        self.assertIn('#include "../../../../runtime/StayArrivalPolicy.h"', POSITION)
        fn = body(POSITION, "bool ReturnToStayPositionAction::isUseful()")
        self.assertIn('PositionEntry stayPosition = AI_VALUE(PositionMap&, "position")["stay"];', fn)
        self.assertIn("stayPosition.isSet()", fn)
        self.assertIn("!" + BOT_TO_STAY, fn)
        self.assertIn("ai->CanMove()", fn)
        self.assertNotIn('"distance"', fn)

    def test_other_qualifiers_keep_their_rule(self):
        fn = body(POSITION, "bool MoveToPositionAction::isUseful()")
        self.assertIn('AI_VALUE2(float, "distance", std::string("position_") + qualifier)', fn)
        self.assertNotIn("StayPointReached", fn)

    def test_trigger_uses_the_shared_rule(self):
        self.assertIn('#include "../../../../runtime/StayArrivalPolicy.h"', TRIGGERS)
        fn = body(TRIGGERS, "bool ReturnToStayPositionTrigger::IsActive()")
        self.assertIn("return !" + BOT_TO_STAY + ";", fn)
        self.assertNotIn("GetDistance(", fn)

    def test_run_leader_deadband_uses_the_shared_rule(self):
        self.assertIn('#include "StayArrivalPolicy.h"', RUN)
        fn = body(RUN, "inline bool InFollowDeadband(")
        self.assertIn("return StayPointReached(lx - x, ly - y, lz - z, kFollowDeadbandYd);", fn)
        self.assertNotIn("sqrt", fn)

    def test_only_these_callers(self):
        callers = []
        for path in list((ROOT / "ai").rglob("*.cpp")) + list((ROOT / "runtime").rglob("*.h")) + \
                list((ROOT / "commands").rglob("*.cpp")):
            if path.name == "StayArrivalPolicy.h":
                continue
            n = len(re.findall(r"\bStayPointReached\(", path.read_text(errors="replace")))
            if n:
                callers.append((path.name, n))
        self.assertEqual(sorted(callers), [("DungeonRunPolicy.h", 1), ("GenericTriggers.cpp", 1),
                                           ("PositionAction.cpp", 1)])


if __name__ == "__main__":
    unittest.main()
