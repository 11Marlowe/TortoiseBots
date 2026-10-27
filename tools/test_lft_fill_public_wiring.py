"""The LFT level-range table lives in one policy header; the fill uses it.

runtime/policy/lfg/LftDungeonRangePolicy.h owns the authoritative
Soromeister/LFT v0.0.3.3 LFT.allDungeons range table
(LftDungeonRangeTable), its alnum-lowercase normalization
(NormalizeLftToken) and the fail-closed lookup (LftDungeonRange).
runtime/LftBotFillService.cpp only calls the lookup; the old private
DungeonLevelRange / NormalizeInstanceToken copies are gone. The role
checks bot LFG needs (GetBotRoleMask, RoleMismatchReason,
EquipBestShieldFromBags, GetCombatKitRoleMask, ClearForcedRole) and the
new ApplyForcedRole are public on LftBotFillService.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
CPP = (ROOT / "runtime/LftBotFillService.cpp").read_text()
HEADER = (ROOT / "runtime/LftBotFillService.h").read_text()


def strip_comments(text):
    return "\n".join(line.split("//", 1)[0].rstrip() for line in text.splitlines())


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class LftDungeonRangeWiringTest(unittest.TestCase):
    def test_fill_uses_the_shared_range_table(self):
        self.assertIn('#include "runtime/policy/lfg/LftDungeonRangePolicy.h"', CPP)
        self.assertIn("LftDungeonRange(raw)", CPP)
        self.assertNotIn('{"ragefirechasm"', CPP)
        self.assertNotIn("std::string NormalizeInstanceToken(", strip_comments(CPP))

    def test_role_checks_are_public(self):
        public = body(HEADER, "public:", "private:")
        for decl in (
            "uint8 GetBotRoleMask(Player const* bot) const;",
            "std::string RoleMismatchReason(Player* bot, uint8 needRole) const;",
            "bool EquipBestShieldFromBags(Player* bot) const;",
            "uint8 GetCombatKitRoleMask(Player const* bot) const;",
            "void ClearForcedRole(uint32 guidLow);",
            "void ApplyForcedRole(Player* bot, uint8 role);",
        ):
            self.assertIn(decl, public, decl)

    def test_apply_forced_role_rebuilds_the_kit(self):
        fn = body(CPP, "void LftBotFillService::ApplyForcedRole(", "\n}\n")
        self.assertIn("ai->SetForcedRole(role);", fn)
        self.assertIn("InvalidateStrategySnapshots(ai)", fn)
        self.assertIn('DoSpecificAction("auto talents")', fn)
        self.assertIn("ResetStrategies()", fn)


if __name__ == "__main__":
    unittest.main()
