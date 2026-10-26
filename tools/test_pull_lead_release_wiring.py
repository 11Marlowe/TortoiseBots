"""Wiring pins for the tank-first pull hold (turtle-agent#265).

The pure rule lives in ai/playerbot/PullJoinWindowPolicy.h (PullLeadReleases,
unit-tested in tools/test_pull_join_window_policy.cpp). These pins prove the
live rows call it: a rule nobody calls changes nothing in the game.

 - The hold release waits for the tank's lead on a run leader's pull
   (PullHoldExpiredTrigger checks the lead key, then the release rule, and the
   timer expiry stays the first gate).
 - The run leader's party hold is lead-keyed; the chat pullback hold is not.
 - The landing stamps the window landed only where the spec says: an ordinary
   pull marks it at the shot, a pullback only when the tank is back.

Run: python3 -m unittest tools/test_pull_lead_release_wiring.py
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
PULL_TRIGGERS = (ROOT / "ai/playerbot/strategy/triggers/PullTriggers.cpp").read_text()
COMMANDS = (ROOT / "commands/BotCommandContext.cpp").read_text()
PULL_ACTIONS = (ROOT / "ai/playerbot/strategy/actions/PullActions.cpp").read_text()


def body(text, start, end="\n}\n"):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class PullLeadReleaseWiring(unittest.TestCase):
    def test_hold_expiry_checks_the_tank_lead(self):
        fn = body(PULL_TRIGGERS, "bool PullHoldExpiredTrigger::IsActive()")
        self.assertIn("PullWindowLeadKeyed()", fn)
        self.assertIn("ai::PullLeadReleases(", fn)
        self.assertLess(fn.index("PullWindowExpired("), fn.index("PullLeadReleases("))

    def test_run_leader_party_hold_is_lead_keyed(self):
        fn = body(COMMANDS, "void PausePartyDpsForPull(")
        self.assertIn("StartPullWindow(static_cast<uint8>(windowSeconds), context.runLeader,", fn)

    def test_landing_marks_only_where_the_spec_says(self):
        self.assertIn("RestampPullParty(bot, 0, false, !strategy->IsCommandPullback())", PULL_ACTIONS)
        self.assertIn("RestampPullParty(bot, strategy->GetCommandJoinDelay(), true, true)", PULL_ACTIONS)

    def test_chat_pullback_hold_stays_plain_timer(self):
        fn = body(PULL_ACTIONS, "void ai::PausePartyPullHold(Player* tank, uint32 joinDelaySeconds)")
        start = fn.index("StartPullWindow(")
        call = fn[start:fn.index(";", start)]
        self.assertNotIn(",", call)


class BotOnlyRunHoldWiring(unittest.TestCase):
    def test_bot_only_run_parties_keep_the_hold(self):
        combat = (ROOT / "ai/playerbot/strategy/generic/CombatStrategy.cpp").read_text()
        fn = body(combat, "bool WaitForAttackStrategy::ShouldWait(PlayerbotAI* ai)")
        self.assertIn("IsInRunningDungeonRun(bot)", fn)


if __name__ == "__main__":
    unittest.main()
