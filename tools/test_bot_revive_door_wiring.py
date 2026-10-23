"""A bot released inside a dungeon revives at the door nearest its corpse (SM-REVIVE-PATH-2026-10-23).

The core's Player::RepopAtGraveyard revives a managed bot that releases in a
dungeon at once, still on the dungeon map, at GetMapEntranceTrigger: the
lowest trigger id, so Scarlet Monastery always put it in the Graveyard wing.
Our core fork asks the module first through the PlayerScript hook
GetBotRepopEntrance(bot, mapId, corpse x/y/z, entrance&). The pure answer
(RepopEntranceRules + PickCorpseEntrance) is tested in
tools/test_corpse_entrance_policy.cpp; this pins the glue, which only builds
against the core:
- BotPlayerAdapter enables PLAYERHOOK_GET_BOT_REPOP_ENTRANCE and overrides the
  hook, answering only for its own bots through ai::PickRepopEntrance;
- PickRepopEntrance builds the candidates with the corpse position and the
  repop rules (the dungeon map as the bot's map, ghost_entrance_map as the
  entrance map) and answers nothing when no door is eligible;
- with TORTOISE_CORE=<core checkout> set, the core side is checked too: the
  hook's signature matches, it is called inside the managed-bot branch before
  the revive, and with no answer the old trigger is kept.
"""
import os
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
ADAPTER_H = (ROOT / "host/BotPlayerAdapter.h").read_text()
ADAPTER = (ROOT / "host/BotPlayerAdapter.cpp").read_text()
ACTION = (ROOT / "ai/playerbot/strategy/actions/ReviveFromCorpseAction.cpp").read_text()
ACTION_H = (ROOT / "ai/playerbot/strategy/actions/ReviveFromCorpseAction.h").read_text()
POLICY = (ROOT / "ai/playerbot/CorpseEntrancePolicy.h").read_text()
GATE = (ROOT / "tools/verify_all.sh").read_text()


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class BotReviveDoorWiringTest(unittest.TestCase):
    def test_the_hook_is_registered(self):
        ctor = body(ADAPTER, "BotPlayerAdapter::BotPlayerAdapter()", "\n{\n")
        self.assertIn("PLAYERHOOK_GET_BOT_REPOP_ENTRANCE", ctor)
        self.assertRegex(ADAPTER_H, r"bool GetBotRepopEntrance\(Player\* bot, uint32 mapId, float x, float y, float z,"
                                    r"\s*AreaTriggerTeleport const\*& entrance\) override;")

    def test_the_hook_answers_for_its_own_bots_only(self):
        fn = body(ADAPTER, "bool BotPlayerAdapter::GetBotRepopEntrance(", "\n}\n")
        guard = fn.index("if (!bot || !BotManager::Instance().IsBot(bot->GetObjectGuid()))")
        answer = fn.index("return ai::PickRepopEntrance(bot, mapId, x, y, z, entrance);")
        self.assertLess(guard, answer)
        self.assertIn('#include "../ai/playerbot/strategy/actions/ReviveFromCorpseAction.h"', ADAPTER)
        self.assertIn("bool PickRepopEntrance(Player* bot, uint32 dungeonMapId,", ACTION_H)

    def test_the_answer_uses_the_corpse_and_the_repop_rules(self):
        fn = body(ACTION, "bool ai::PickRepopEntrance(", "\n}\n")
        self.assertIn("outAt = nullptr;", fn)
        self.assertIn("WorldPosition const corpsePos(dungeonMapId, corpseX, corpseY, corpseZ);", fn)
        self.assertRegex(fn, r"FindEntranceTriggerByRules\(dungeonMapId, WorldPosition\(bot\), corpsePos,\s*"
                             r"RepopEntranceRules\(dungeonMapId, dungeonMap->ghostEntranceMap\), atEntry, outAt\)")
        # No eligible door: false, nothing set.
        self.assertRegex(fn, r"if \(!FindEntranceTriggerByRules\([^;]*\)\)\s*return false;")
        self.assertIn("[BOT REPOP]", fn)
        # The rules: the bot stands on the dungeon map, the doors are the timeout's.
        rules = body(POLICY, "inline CorpseEntranceRules RepopEntranceRules(", "\n    }\n")
        for line in ("rules.destMap = dungeonMap;", "rules.ghostMap = dungeonMap;",
                     "rules.entranceMap = entranceMap;", "rules.timeout = true;"):
            self.assertIn(line, rules)

    def test_policy_test_is_in_the_gate(self):
        self.assertIn("python3 -m unittest tools/test_bot_revive_door_wiring.py", GATE)
        self.assertIn("test_corpse_entrance_policy", GATE)


@unittest.skipUnless(os.environ.get("TORTOISE_CORE"), "set TORTOISE_CORE=<core checkout> to check the core side")
class CoreRepopHookTest(unittest.TestCase):
    def setUp(self):
        core = pathlib.Path(os.environ["TORTOISE_CORE"]) / "src/game"
        self.objects = (core / "ScriptObjects.h").read_text()
        self.mgr = (core / "ScriptMgr.cpp").read_text()
        self.player = (core / "Objects/Player.cpp").read_text()

    def test_the_core_hook_matches(self):
        self.assertIn("PLAYERHOOK_GET_BOT_REPOP_ENTRANCE,", self.objects)
        self.assertRegex(self.objects, r"virtual bool GetBotRepopEntrance\(Player\* /\*bot\*/, uint32 /\*mapId\*/, "
                                       r"float /\*x\*/, float /\*y\*/, float /\*z\*/,\s*"
                                       r"AreaTriggerTeleport const\*& /\*entrance\*/\) \{ return false; \}")
        self.assertIn("ForEachEnabledHook(PLAYERHOOK_GET_BOT_REPOP_ENTRANCE,", self.mgr)

    def test_the_core_asks_before_the_revive_and_keeps_its_trigger(self):
        fn = body(self.player, "void Player::RepopAtGraveyard()", "\n}\n")
        branch = body(fn, "if (!IsAlive() && repopAtEntrance && GetMap() && GetMap()->IsDungeon())", "return;")
        default = branch.index("AreaTriggerTeleport const* entrance = sObjectMgr.GetMapEntranceTrigger(GetMapId());")
        ask = branch.index("sScriptMgr.GetBotRepopEntrance(this, GetMapId(), x, y, z, entrance);")
        revive = branch.index("ResurrectPlayer(1.0f);")
        self.assertLess(default, ask)
        self.assertLess(ask, revive)
        self.assertIn("if (entrance)", branch[ask:revive])
        self.assertIn("TeleportTo(entrance->destination, TeleOptions);", branch)


if __name__ == "__main__":
    unittest.main()
