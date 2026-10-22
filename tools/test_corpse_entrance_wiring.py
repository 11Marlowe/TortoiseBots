"""The corpse run's door (Scarlet Monastery r20-19z39 slot 7, CorpseEntrancePolicy.h).

The ghosts of a Library wipe walked to the door nearest the GHOST (the
Graveyard wing's) and were raised in a wing with no way back. The pure rule
(the destination nearest the corpse wins) is tested in
tools/test_corpse_entrance_policy.cpp; this pins where
ai/playerbot/strategy/actions/ReviveFromCorpseAction.cpp calls it, since that
file only builds against the core:
- FindInstanceEntranceTrigger hands every door into the corpse's map to
  PickCorpseEntrance with the CORPSE's position, and the ghost's distance only
  as the tie-break;
- the walk to the door (the ghost's own map only) and the 300 s timeout (any
  map) both use it, the timeout keeping GetMapEntranceTrigger as the fallback
  only.
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
ACTION = (ROOT / "ai/playerbot/strategy/actions/ReviveFromCorpseAction.cpp").read_text()


def body(text, start, end):
    i = text.index(start)
    j = text.index(end, i + len(start))
    return text[i:j]


class CorpseEntranceWiringTest(unittest.TestCase):
    def setUp(self):
        self.find = body(ACTION, "static bool FindInstanceEntranceTrigger(", "bool ReviveFromCorpseAction::Execute(")
        self.corpse = body(ACTION, "bool FindCorpseAction::Execute(", "bool FindCorpseAction::isUseful()")

    def test_the_policy_header_is_included(self):
        self.assertIn('#include "playerbot/CorpseEntrancePolicy.h"', ACTION)

    def test_the_pick_is_by_the_corpse(self):
        self.assertIn("PickCorpseEntrance(candidates, corpsePos.getX(), corpsePos.getY(), corpsePos.getZ())",
                      self.find)
        # The candidates carry the trigger's destination, the ghost's distance to the trigger only on its own map.
        for field in ("c.destX = at->destination.x;", "c.destY = at->destination.y;", "c.destZ = at->destination.z;"):
            self.assertIn(field, self.find)
        self.assertRegex(self.find, r"c\.ghostSqYd = onGhostMap \? botPos\.sqDistance2d\(")
        # No nearest-to-the-ghost pick of its own any more.
        self.assertNotIn("bestDist", self.find)

    def test_only_doors_into_the_corpse_map_and_the_ghost_map_filter(self):
        self.assertIn("if (at->destination.mapId != corpseMapId)", self.find)
        self.assertIn("if (!onGhostMap && !anyMap)", self.find)

    def test_the_walk_uses_the_ghost_map_doors(self):
        self.assertIn(
            "FindInstanceEntranceTrigger(corpsePos.GetMapId(), botPos.GetMapId(), botPos, corpsePos, false, "
            "entranceTrigger, entranceTeleport)", self.corpse)

    def test_the_timeout_uses_any_door_and_falls_back(self):
        timeout = body(self.corpse, "if (corpseInDungeon && deadTime >= 5 * MINUTE)",
                       "if (botPos.GetMapId() != corpsePos.GetMapId())")
        pick = timeout.index("FindInstanceEntranceTrigger(dungeonMapId, botPos.GetMapId(), botPos, corpsePos, true,")
        fallback = timeout.index("entranceTeleport = sObjectMgr.GetMapEntranceTrigger(dungeonMapId);")
        self.assertLess(pick, fallback)
        self.assertRegex(timeout, r"if \(!FindInstanceEntranceTrigger\([^;]*\)\)\s*entranceTeleport = sObjectMgr\.GetMapEntranceTrigger")
        # The teleport goes to the picked destination.
        self.assertIn("bot->TeleportTo(entranceTeleport->destination.mapId", timeout)
        self.assertEqual(len(re.findall(r"GetMapEntranceTrigger", ACTION)), 1)


if __name__ == "__main__":
    unittest.main()
