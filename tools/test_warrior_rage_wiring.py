"""Warrior rage hygiene wiring pins (audit F2, PLAN-SPELL-AUDIT W2).

The Disarm, Bloodrage and Battle Shout triggers must keep asking
WarriorRagePolicy.h before the engine tries their rows, and the live Protection
rows must keep using those triggers. The rules themselves are pinned in
tools/test_warrior_rage_policy.cpp.

Run: python3 -m unittest tools/test_warrior_rage_wiring.py
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
WAR = ROOT / "ai/playerbot/strategy/warrior"


def body(text, header):
    start = text.index(header)
    end = text.index("\n}\n", start)
    return text[start:end]


class WarriorRageWiringTest(unittest.TestCase):
    def setUp(self):
        self.cpp = (WAR / "WarriorTriggers.cpp").read_text()
        self.hdr = (WAR / "WarriorTriggers.h").read_text()

    def test_policy_header_included(self):
        self.assertIn('#include "playerbot/WarriorRagePolicy.h"', self.hdr)

    def test_disarm_trigger_gated(self):
        self.assertIn('DEBUFF_TRIGGER_A(DisarmDebuffTrigger, "disarm");', self.hdr)
        fn = body(self.cpp, "bool DisarmDebuffTrigger::IsActive()")
        self.assertIn('if (!ShouldDisarm(AI_VALUE2(uint8, "rage", "self target"), armed, '
                      'sServerFacade.IsSpellReady(bot, spellId)))\n        return false;', fn)
        self.assertIn("    return DebuffTrigger::IsActive();", fn)
        self.assertIn('"rage", "self target"', fn)
        self.assertIn("IsSpellReady(bot, spellId)", fn)
        self.assertIn("UNIT_VIRTUAL_ITEM_DISPLAY + 0", fn)
        self.assertIn("DebuffTrigger::IsActive()", fn)

    def test_bloodrage_trigger_gated(self):
        fn = body(self.cpp, "bool BloodrageBuffTrigger::IsActive()")
        self.assertIn("    return ShouldBloodrage(AI_VALUE2(uint8, \"health\", \"self target\"), ", fn)
        self.assertIn("IsSpellReady(bot, spellId)", fn)
        self.assertIn("sPlayerbotAIConfig.mediumHealth", fn)

    def test_battle_shout_trigger_gated(self):
        start = self.hdr.index("class BattleShoutTrigger")
        fn = self.hdr[start:self.hdr.index("class BerserkerRageBuffTrigger")]
        self.assertIn('if (!ShouldBattleShoutForRage(AI_VALUE2(uint8, "rage", "self target")))\n'
                      '                return false;', fn)
        # the rage gate comes before any aura / blessing look-up
        self.assertLess(fn.index("ShouldBattleShoutForRage"), fn.index("battleShoutIds"))

    def test_protection_rows_still_use_the_gated_triggers(self):
        text = (WAR / "ProtectionWarriorStrategy.cpp").read_text()
        pat = re.compile(r'TriggerNode\(\s*"([^"]+)",\s*NextAction::array\(0, new NextAction\("([^"]+)", ([^)]+)\), NULL\)\)')
        rows = [(m.group(1), m.group(2)) for m in pat.finditer(text)]
        self.assertIn(("disarm", "disarm"), rows)
        self.assertIn(("bloodrage", "bloodrage"), rows)
        ctx = (WAR / "WarriorAiObjectContext.cpp").read_text()
        self.assertIn('creators["disarm"] = [](PlayerbotAI* ai) { return new DisarmDebuffTrigger(ai); };', ctx)
        self.assertIn('creators["bloodrage"] = [](PlayerbotAI* ai) { return new BloodrageBuffTrigger(ai); };', ctx)
        self.assertIn('creators["battle shout"] = [](PlayerbotAI* ai) { return new BattleShoutTrigger(ai); };', ctx)
        strat = (WAR / "WarriorStrategy.cpp").read_text()
        self.assertIn('"battle shout",\n        NextAction::array(0, new NextAction("battle shout", ACTION_HIGH + 1)', strat)

    def test_policy_registered_in_verify_all(self):
        sh = (ROOT / "tools/verify_all.sh").read_text()
        self.assertIn("test_warrior_rage_policy", sh)
        self.assertIn("tools/test_warrior_rage_wiring.py", sh)


if __name__ == "__main__":
    unittest.main()
