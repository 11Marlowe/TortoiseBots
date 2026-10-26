"""Wiring pins for stealthed pull-hold release (issue #1165).

A held melee bot goes into stealth during the pull's join window. The
stealthed multipliers veto every action not on their allow-list, so unless
"release pull hold" is allowed, a stealthed rogue (or prowling feral druid)
can never drop its own anchor hold on time while the rest of the party
fights. These pins prove both allow-lists carry the release action, and that
the action name still matches the trigger row that fires it.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
STRAT = ROOT / "ai/playerbot/strategy"
ROGUE = (STRAT / "rogue/RogueStrategy.cpp").read_text()
DRUID = (STRAT / "druid/DpsFeralDruidStrategy.cpp").read_text()
COMBAT = (STRAT / "generic/CombatStrategy.cpp").read_text()


def multiplier_body(text, class_name):
    start = text.index("class " + class_name)
    end = text.index("return 0.0f;", start)
    return text[start:end]


class StealthedPullRelease(unittest.TestCase):
    def test_stealthed_rogue_may_release_its_pull_hold(self):
        body = multiplier_body(ROGUE, "RogueStealthedStrategyMultiplier")
        self.assertIn('"release pull hold"', body)

    def test_prowling_druid_may_release_its_pull_hold(self):
        body = multiplier_body(DRUID, "DpsFeralDruidStealthedStrategyMultiplier")
        self.assertIn('"release pull hold"', body)

    def test_release_action_name_matches_the_trigger_row(self):
        row = COMBAT[COMBAT.index('"pull hold expired"'):COMBAT.index('"pull hold expired"') + 600]
        self.assertIn('NextAction("release pull hold"', row)


if __name__ == "__main__":
    unittest.main()
