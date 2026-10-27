"""A dungeon crew bot keeps its group and its slot (bot LFG part 2).

The pure decisions (GroupHygienePolicy FarAwayLeaveAllowed/RandomGroupInviteAllowed,
RunReleaseHold::Decide with the on-instance-map flag) are tested in
tools/test_group_hygiene_policy.cpp and tools/test_run_release_hold_policy.cpp;
this checks the call sites use them, since those files only build against the core:
- LeaveFarAwayAction refuses for a crew bot (Dungeon activity lease) via
  ai::FarAwayLeaveAllowed, before anything can return true;
- "invite nearby" refuses for a crew bot via ai::RandomGroupInviteAllowed,
  before the group-size check (GrouperType grouperType);
- "invite guild" is defined in the .cpp (not the header) and refuses for a
  crew bot via ai::RandomGroupInviteAllowed while keeping the
  randomBotGuildNearby gate;
- AutoReleaseSpiritAction passes the bot's instanceable map into
  RunReleaseHold::Decide, and the verdict gate still sits before the stock rules.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
LEAVE = (ROOT / "ai/playerbot/strategy/actions/LeaveGroupAction.cpp").read_text()
INVITE_H = (ROOT / "ai/playerbot/strategy/actions/InviteToGroupAction.h").read_text()
INVITE = (ROOT / "ai/playerbot/strategy/actions/InviteToGroupAction.cpp").read_text()
ACTION = (ROOT / "ai/playerbot/strategy/actions/ReleaseSpiritAction.h").read_text()


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class DungeonCrewWiringTest(unittest.TestCase):
    def test_leave_far_away_refuses_dungeon_crew(self):
        fn = body(LEAVE, "bool LeaveFarAwayAction::isUseful()", "\n    }\n}")
        self.assertIn("TortoiseBots::BotActivity::Dungeon", fn)
        guard = fn.index("ai::FarAwayLeaveAllowed(")
        self.assertLess(guard, fn.index("return true;"))

    def test_invite_nearby_refuses_dungeon_crew(self):
        fn = body(INVITE, "bool InviteNearbyToGroupAction::isUseful()", "\n    }\n}")
        guard = fn.index("ai::RandomGroupInviteAllowed(")
        self.assertLess(guard, fn.index("GrouperType grouperType"))

    def test_invite_guild_refuses_dungeon_crew(self):
        self.assertIn("virtual bool isUseful() override;", body(INVITE_H, "class InviteGuildToGroupAction", "};"))
        self.assertNotIn("randomBotGuildNearby; };", INVITE_H)
        fn = body(INVITE, "bool InviteGuildToGroupAction::isUseful()", "\n    }\n}")
        self.assertIn("ai::RandomGroupInviteAllowed(", fn)
        self.assertIn("randomBotGuildNearby", fn)
    def test_release_passes_the_instance_map(self):
        auto = body(ACTION, "class AutoReleaseSpiritAction", "class CorpseRunAction")
        useful = body(auto, "virtual bool isUseful() override", "\n        }\n")
        gate = useful.index("RunReleaseHold::Decide(")
        self.assertLess(gate, useful.index("bot->GetMap()->Instanceable()"))
        self.assertLess(useful.index("bot->GetMap()->Instanceable()"), useful.index("if (verdict =="))


if __name__ == "__main__":
    unittest.main()
