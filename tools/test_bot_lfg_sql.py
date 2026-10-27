"""The proven-plan table ai_bot_lfg_plan lists what bot LFG groups may pick.

runtime/BotLfgService.cpp forms groups only from rows of ai_bot_lfg_plan;
each row means the plan has a full bot-only clear. The level band comes
from the plan's lft_code key in the LFT addon range table
(runtime/policy/lfg/LftDungeonRangePolicy.h). The flag is its own table
so a plan re-export (DELETE and INSERT of the ai_dungeon_plan row) never
clears it.
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
MIGRATION = ROOT / "data/sql/world/20261027120000_world.sql"

EXPECTED = {
    "ragefire_chasm": ("rfc", 2), "wailing_caverns": ("wc", 3), "deadmines": ("deadmines", 3),
    "shadowfang_keep": ("sfk", 3), "stormwind_stockade": ("stockade", 1), "blackfathom_deeps": ("bfd", 3),
    "gnomeregan": ("gnomer", 3), "razorfen_kraul": ("rfk", 3), "crescent_grove": ("tcg", 3),
    "scarlet_library": ("smlib", 3), "scarlet_armory": ("smarmory", 3), "scarlet_cathedral": ("smcath", 3),
    "razorfen_downs": ("rfd", 3), "uldaman": ("ulda", 3), "gilneas_city": ("gilneas", 3), "zulfarrak": ("zf", 3),
    "maraudon_orange": ("maraorange", 3), "maraudon_purple": ("marapurple", 3), "sunken_temple": ("st", 3),
    "hateforge_quarry": ("hfq", 3), "blackrock_depths": ("brd", 3), "lower_blackrock_spire": ("lbrs", 3),
    "dire_maul_east": ("dme", 3), "dire_maul_north": ("dmn", 3), "dire_maul_west": ("dmw", 3),
    "scholomance": ("scholo", 3), "stratholme_live": ("stratlive", 3), "stratholme_undead": ("stratud", 3),
    "karazhan_crypt": ("kc", 3), "black_morass": ("cotbm", 3), "stormwind_vault": ("swv", 1),
}


class BotLfgPlanTableTest(unittest.TestCase):
    def test_table_and_columns(self):
        text = MIGRATION.read_text()
        self.assertIn("CREATE TABLE IF NOT EXISTS `ai_bot_lfg_plan` ", text)
        for column in ("`plan` varchar(32) NOT NULL", "`lft_code` varchar(32) NOT NULL",
                       "`teams` tinyint unsigned NOT NULL DEFAULT 3",
                       "`comment` varchar(255) NOT NULL DEFAULT ''", "PRIMARY KEY (`plan`)"):
            self.assertIn(column, text, column)

    def test_exactly_the_proven_plans(self):
        text = MIGRATION.read_text()
        start = text.index("REPLACE INTO `ai_bot_lfg_plan` ")
        found = {plan: (code, int(teams)) for plan, code, teams, _comment in
                 re.findall(r"\('(\w+)', '(\w+)', (\d), '([^']*)'\)", text[start:])}
        self.assertEqual(found, EXPECTED)

    def test_plans_and_codes_exist(self):
        plans = set()
        for path in (ROOT / "data/sql/world").glob("*.sql"):
            plans.update(re.findall(r"INSERT INTO `ai_dungeon_plan` \([^)]*\) VALUES\s*\('(\w+)'",
                                    path.read_text()))
        for plan in EXPECTED:
            self.assertIn(plan, plans, plan)
        header = (ROOT / "runtime/policy/lfg/LftDungeonRangePolicy.h").read_text()
        codes = set(re.findall(r'\{"(\w+)", \{\d+, \d+\}\}', header))
        for plan, (code, _teams) in EXPECTED.items():
            self.assertIn(code, codes, plan)

    def test_left_out_plans_are_absent(self):
        text = MIGRATION.read_text()
        start = text.index("REPLACE INTO `ai_bot_lfg_plan` ")
        statement = text[start:]
        for plan in ("scarlet_graveyard", "dragonmaw_retreat", "frostmane_hollow",
                     "stormwrought_ruins", "windhorn_canyon"):
            self.assertNotRegex(statement, r"\('%s', " % plan, plan)

    def test_idempotent(self):
        text = MIGRATION.read_text()
        self.assertNotIn("DELETE FROM", text)
        self.assertNotIn("DROP TABLE", text)


if __name__ == "__main__":
    unittest.main()
