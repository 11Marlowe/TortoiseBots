"""Bots fire scripted area triggers (issue #984), wired the way the brief says.

ai/playerbot/BotAreaTriggerPolicy.h holds the pure parts (tested in
tools/test_bot_area_trigger_policy.cpp). This checks the glue: the per-bot
check sits behind AiPlayerbot.BotAreaTriggers and its own timer, the index is
filtered to scripted, non-teleport, non-tavern, non-BG triggers on instance
maps, only the script runs (sScriptMgr.OnAreaTrigger, never the opcode
handler), no grid visit happens, and the key's code default equals the
template.
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
PB = ROOT / "ai/playerbot"
AI = (PB / "PlayerbotAI.cpp").read_text()
MGR = (PB / "BotAreaTriggers.cpp").read_text()
CONF_CPP = (PB / "PlayerbotAIConfig.cpp").read_text()
TEMPLATE = (PB / "aiplayerbot.conf.dist.in").read_text()
CMAKE = (ROOT / "TortoiseBots.cmake").read_text()


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class BotAreaTriggerWiringTest(unittest.TestCase):
    def test_update_ai_gates_and_throttles_the_check(self):
        fn = body(AI, "void PlayerbotAI::UpdateAI(uint32 elapsed, bool minimal)", "\n}\n")
        block = body(fn, "if (sPlayerbotAIConfig.botAreaTriggers)", "\n    }\n")
        self.assertIn("areaTriggerCheckDelay = ai::kBotAreaTriggerCheckMs;", block)
        self.assertIn("ai::UpdateBotAreaTriggers(bot, areaTriggerState);", block)
        self.assertEqual(AI.count("UpdateBotAreaTriggers("), 1)

    def test_index_filter_uses_every_exclusion(self):
        fn = body(MGR, "BotAreaTriggerIndex const& GetIndex()", "\n    }\n}\n")
        self.assertIn("sScriptMgr.GetAreaTriggerScriptId(at.id) != 0", fn)
        self.assertIn("map && map->IsDungeon()", fn)
        self.assertIn("sObjectMgr.GetAreaTriggerTeleport(at.id) != nullptr", fn)
        self.assertIn("sObjectMgr.IsTavernAreaTrigger(at.id)", fn)
        self.assertIn("sObjectMgr.GetBattlegroundEntranceTrigger(at.id) != nullptr", fn)
        self.assertIn("if (built)", fn)

    def test_only_the_script_runs_and_no_world_scan(self):
        self.assertIn("sScriptMgr.OnAreaTrigger(bot, at);", MGR)
        self.assertNotIn("HandleAreaTriggerOpcode", MGR)
        self.assertNotIn("CMSG_AREATRIGGER", MGR)
        self.assertNotRegex(MGR, r"Cell::|Visit\(|GridNotifiers|WorldDatabase|CharacterDatabase")
        fn = body(MGR, "void ai::UpdateBotAreaTriggers(", "\n}\n")
        for guard in ("bot->GetGroup()", "bot->IsAlive()", "bot->IsTaxiFlying()",
                      "bot->IsBeingTeleported()", "bot->GetMap()->IsDungeon()"):
            self.assertIn(guard, fn)

    def test_config_default_matches_template(self):
        self.assertIn('botAreaTriggers = config.GetBoolDefault("AiPlayerbot.BotAreaTriggers", true);', CONF_CPP)
        self.assertRegex(TEMPLATE, r"(?m)^AiPlayerbot\.BotAreaTriggers = 1$")
        self.assertIn("# 1 - grouped bots in instances fire scripted area triggers (Default)", TEMPLATE)

    def test_source_is_built(self):
        self.assertIn('"${TORTOISEBOTS_ROOT}/ai/playerbot/BotAreaTriggers.cpp"', CMAKE)


if __name__ == "__main__":
    unittest.main()
