"""Wiring pins for pull-release logging (#265 round 2 logging).

EndPullWindow is the single release point: it logs exactly once and clears
the reason, guarded by 'a window was running'. Every caller passes an
explicit reason. These pins fail first on integ/20d f3711f42, where only
ReleasePullHoldAction logs and the direct EndPullWindow callers
(SyncPullWindow, ResetStrategies, preset reload) neither log nor clear,
leaving pullReleaseReason_ stale for the next logged release.

Run: flock /tmp/tb-test.lock nice -n 10 python3 -m unittest tools/test_pull_release_logging.py
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
PULL_ACTIONS = (ROOT / "ai/playerbot/strategy/actions/PullActions.cpp").read_text()
PULL_TRIGGERS = (ROOT / "ai/playerbot/strategy/triggers/PullTriggers.cpp").read_text()
PLAYERBOT_AI_CPP = (ROOT / "ai/playerbot/PlayerbotAI.cpp").read_text()
PLAYERBOT_AI_H = (ROOT / "ai/playerbot/PlayerbotAI.h").read_text()
DB_STORE = (ROOT / "ai/playerbot/PlayerbotDbStore.cpp").read_text()


def body(text, start, end="\n}\n"):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


def args_at(text, start_marker):
    i = text.index(start_marker)
    depth = text.index("(", i) + 1
    level = 1
    j = depth
    while level:
        level += (text[j] == "(") - (text[j] == ")")
        j += 1
    return text[depth:j - 1]


class PullReleaseLogging(unittest.TestCase):
    def test_release_action_defers_to_end_pull_window(self):
        # The single log line lives in EndPullWindow, not in the action: the
        # action only forwards the trigger's reason (defaulting to "order").
        action = body(PULL_ACTIONS, "bool ReleasePullHoldAction::Execute(Event& event)")
        self.assertNotIn("pull-release", action)
        self.assertIn("EndPullWindow(", action)

    def test_end_pull_window_logs_once_and_clears(self):
        # Exactly one pull-release line, inside the running-window guard, and
        # the reason is cleared there so consecutive windows never share one.
        fn = body(PLAYERBOT_AI_CPP, "void PlayerbotAI::EndPullWindow(")
        self.assertEqual(fn.count("pull-release"), 1)
        self.assertIn("PullReleaseReason()", fn)
        self.assertIn('SetPullReleaseReason(nullptr)', fn)
        self.assertLess(fn.index("pullWindow_"), fn.index("pull-release"))

    def test_every_end_pull_window_caller_passes_a_reason(self):
        # Every in-repo EndPullWindow(...) call carries an explicit reason
        # argument; the bare reset/reload/order calls must not come back.
        decl = "void EndPullWindow("
        self.assertIn(decl, PLAYERBOT_AI_H)
        for path, text in [
            ("PlayerbotAI.cpp", PLAYERBOT_AI_CPP),
            ("PullActions.cpp", PULL_ACTIONS),
            ("PlayerbotDbStore.cpp", DB_STORE),
        ]:
            for token in ("EndPullWindow()", "EndPullWindow ();"):
                self.assertNotIn(token, text, f"bare EndPullWindow call left in {path}")
        # At least the order/reset/reload reasons flow through the callers.
        joined = PLAYERBOT_AI_CPP + PULL_ACTIONS + DB_STORE
        for reason in ('"order"', '"reset"', '"reload"'):
            self.assertIn(reason, joined)

    def test_strategy_removal_releases_with_order(self):
        # An attack/flee/explicit strategy order that drops the combat gate
        # ends the window with "order" (SyncPullWindow's direct caller path).
        sync = body(PLAYERBOT_AI_CPP, "void PlayerbotAI::SyncPullWindow(BotState type)")
        self.assertIn('"order"', sync)
        self.assertIn("EndPullWindow(", sync)

    def test_reset_and_reload_log_once(self):
        reset = body(PLAYERBOT_AI_CPP, "void PlayerbotAI::ResetStrategies(bool autoLoad)")
        self.assertIn('"reset"', reset)
        self.assertIn("EndPullWindow(", reset)
        self.assertIn('"reload"', DB_STORE)
        self.assertIn("EndPullWindow(", DB_STORE)

    def test_consecutive_windows_never_share_a_reason(self):
        # A stale reason must not survive a release: the next logged release
        # reports its own reason, not the previous window's. The clear lives
        # in EndPullWindow itself so the direct callers cannot skip it.
        end = body(PLAYERBOT_AI_CPP, "void PlayerbotAI::EndPullWindow(")
        clear = end.index("SetPullReleaseReason(nullptr)")
        self.assertLess(end.index("pull-release"), clear)
        release = body(PULL_ACTIONS, "bool ReleasePullHoldAction::Execute(Event& event)")
        self.assertNotIn("SetPullReleaseReason", release)

    def test_trigger_reasons_still_flow_through(self):
        # The trigger still names each rule; EndPullWindow only resolves the
        # empty-reason default. Re-pin so the centralisation cannot drop them.
        trigger = body(PULL_TRIGGERS, "bool PullHoldExpiredTrigger::IsActive()")
        for reason in ('"expired"', '"cap"', '"tank-first"', '"hit"', '"tank-low"', '"tank-dead"'):
            if reason in ('"cap"', '"tank-first"', '"hit"', '"tank-low"', '"tank-dead"'):
                self.assertIn(reason, ROOT.joinpath("ai/playerbot/PullJoinWindowPolicy.h").read_text())
            else:
                self.assertIn(reason, trigger)

    def test_chained_pull_takes_the_latest_kind(self):
        # A run-leader pull chained onto a fight-start hold with no Clear
        # between takes the latest Start's flags: fightStart is overwritten
        # unconditionally, so the logged kind is "pull", never a stale
        # "fight-start". EndPullWindow reads the pre-Clear snapshot's fields
        # (the live accessors return cleared state after Clear()).
        policy = (ROOT / "ai/playerbot/PullJoinWindowPolicy.h").read_text()
        start = policy[policy.index("void Start("):policy.index("void Restamp(")]
        self.assertIn("fightStart = fightStartIn;", start)
        self.assertLess(start.index("start = now;"), start.index("fightStart = fightStartIn;"))
        end = body(PLAYERBOT_AI_CPP, "void PlayerbotAI::EndPullWindow(")
        self.assertIn("fight-start", end)
        self.assertIn("window.fightStart", end)
        self.assertIn("window.leadKeyed", end)


if __name__ == "__main__":
    unittest.main()
