// Standalone regression test for the mimic-consumable policy (Issue #491):
// purpose-key classification, per-role ladders, highest-usable-tier picks.
// Server data, not guesses: entries + RequiredLevel verified against core
// sql/base/tw_world_item_template.sql.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_mimic_consumable_policy.cpp -o /tmp/test_mimic_consumable
//   /tmp/test_mimic_consumable

#include "../runtime/MimicConsumablePolicy.h"

#include <cstdio>
#include <cstdlib>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

// Master item entries classify into purpose keys; unknown items map to NONE.
static void TestPurposeClassification()
{
    CHECK(PurposeForMasterItem(9206) == MimicPurpose::BATTLE_PHYS_STR); // Giants
    CHECK(PurposeForMasterItem(13453) == MimicPurpose::BATTLE_PHYS_STR); // Brute Force
    CHECK(PurposeForMasterItem(13452) == MimicPurpose::BATTLE_PHYS_AGI); // Mongoose
    CHECK(PurposeForMasterItem(9187) == MimicPurpose::BATTLE_PHYS_AGI); // Greater Agility
    CHECK(PurposeForMasterItem(6373) == MimicPurpose::BATTLE_CASTER); // Firepower
    CHECK(PurposeForMasterItem(13454) == MimicPurpose::BATTLE_CASTER); // Greater Arcane
    CHECK(PurposeForMasterItem(9264) == MimicPurpose::BATTLE_CASTER); // Shadow Power
    CHECK(PurposeForMasterItem(13445) == MimicPurpose::GUARDIAN_ARMOR); // Superior Defense
    CHECK(PurposeForMasterItem(20004) == MimicPurpose::GUARDIAN_HEALTH); // Major Troll's Blood
    CHECK(PurposeForMasterItem(20007) == MimicPurpose::GUARDIAN_MANA); // Mageblood
    CHECK(PurposeForMasterItem(954) == MimicPurpose::SCROLL_STAT); // Scroll of Strength
    CHECK(PurposeForMasterItem(10309) == MimicPurpose::SCROLL_STAT); // Scroll of Agility IV
    CHECK(PurposeForMasterItem(18262) == MimicPurpose::WEAPON_IMBUE); // Elemental Sharpening
    CHECK(PurposeForMasterItem(20749) == MimicPurpose::WEAPON_IMBUE); // Brilliant Wizard Oil
    CHECK(PurposeForMasterItem(8928) == MimicPurpose::WEAPON_IMBUE); // Instant Poison VI
    CHECK(PurposeForMasterItem(13510) == MimicPurpose::FLASK); // Titans
    CHECK(PurposeForMasterItem(13512) == MimicPurpose::FLASK); // Supreme Power
    CHECK(PurposeForMasterItem(13457) == MimicPurpose::PROT_POTION); // Greater Fire Prot
    CHECK(PurposeForMasterItem(6049) == MimicPurpose::PROT_POTION); // Fire Prot
    CHECK(PurposeForMasterItem(13928) == MimicPurpose::FOOD_BUFF); // Grilled Squid
    CHECK(PurposeForMasterItem(21023) == MimicPurpose::FOOD_BUFF); // Chimaerok Chops
    CHECK(PurposeForMasterItem(1) == MimicPurpose::NONE);
    CHECK(PurposeForMasterItem(999999) == MimicPurpose::NONE);

    CHECK(IsBattleElixirPurpose(MimicPurpose::BATTLE_PHYS_STR));
    CHECK(IsBattleElixirPurpose(MimicPurpose::BATTLE_PHYS_AGI));
    CHECK(IsBattleElixirPurpose(MimicPurpose::BATTLE_CASTER));
    CHECK(!IsBattleElixirPurpose(MimicPurpose::GUARDIAN_ARMOR));
    CHECK(IsGuardianElixirPurpose(MimicPurpose::GUARDIAN_ARMOR));
    CHECK(IsGuardianElixirPurpose(MimicPurpose::GUARDIAN_HEALTH));
    CHECK(IsGuardianElixirPurpose(MimicPurpose::GUARDIAN_MANA));
    CHECK(!IsGuardianElixirPurpose(MimicPurpose::BATTLE_CASTER));
}

// Spec maps to role: tanks, healers, melee, casters, hunters.
static void TestRoleForSpec()
{
    CHECK(RoleForSpec(1, SPEC_WARRIOR_PROTECTION) == MimicRole::TANK);
    CHECK(RoleForSpec(1, SPEC_WARRIOR_ARMS) == MimicRole::MELEE_DPS);
    CHECK(RoleForSpec(1, SPEC_WARRIOR_FURY) == MimicRole::MELEE_DPS);
    CHECK(RoleForSpec(2, SPEC_PALADIN_HOLY) == MimicRole::HEALER);
    CHECK(RoleForSpec(2, SPEC_PALADIN_PROTECTION) == MimicRole::TANK);
    CHECK(RoleForSpec(2, SPEC_PALADIN_RETRIBUTION) == MimicRole::MELEE_DPS);
    CHECK(RoleForSpec(3, SPEC_HUNTER_MM) == MimicRole::RANGED_AGI);
    CHECK(RoleForSpec(4, SPEC_ROGUE_COMBAT) == MimicRole::MELEE_DPS);
    CHECK(RoleForSpec(5, SPEC_PRIEST_HOLY) == MimicRole::HEALER);
    CHECK(RoleForSpec(5, SPEC_PRIEST_SHADOW) == MimicRole::CASTER_DPS);
    CHECK(RoleForSpec(7, SPEC_SHAMAN_ENHANCEMENT) == MimicRole::MELEE_DPS);
    CHECK(RoleForSpec(7, SPEC_SHAMAN_ELEMENTAL) == MimicRole::CASTER_DPS);
    CHECK(RoleForSpec(7, SPEC_SHAMAN_RESTORATION) == MimicRole::HEALER);
    CHECK(RoleForSpec(8, SPEC_MAGE_FIRE) == MimicRole::CASTER_DPS);
    CHECK(RoleForSpec(9, SPEC_WARLOCK_DESTRUCTION) == MimicRole::CASTER_DPS);
    CHECK(RoleForSpec(11, SPEC_DRUID_RESTORATION) == MimicRole::HEALER);
    CHECK(RoleForSpec(11, SPEC_DRUID_BALANCE) == MimicRole::CASTER_DPS);
    CHECK(RoleForSpec(11, SPEC_DRUID_FERAL) == MimicRole::MELEE_DPS);
}

// Battle ladders: highest usable tier at req <= level; healers take none.
static void TestBattleLadders()
{
    // Warrior arms: STR ladder 1/20/38/45.
    CHECK(ResolveBattleElixir(1, SPEC_WARRIOR_ARMS, 1) == 2454);
    CHECK(ResolveBattleElixir(1, SPEC_WARRIOR_ARMS, 20) == 3391);
    CHECK(ResolveBattleElixir(1, SPEC_WARRIOR_ARMS, 38) == 9206);
    CHECK(ResolveBattleElixir(1, SPEC_WARRIOR_ARMS, 60) == 13453);
    // Tank: same STR battle.
    CHECK(ResolveBattleElixir(1, SPEC_WARRIOR_PROTECTION, 60) == 13453);
    // Rogue: AGI ladder ending in Mongoose 13452(46).
    CHECK(ResolveBattleElixir(4, SPEC_ROGUE_COMBAT, 2) == 2457);
    CHECK(ResolveBattleElixir(4, SPEC_ROGUE_COMBAT, 27) == 8949);
    CHECK(ResolveBattleElixir(4, SPEC_ROGUE_COMBAT, 46) == 13452);
    CHECK(ResolveBattleElixir(4, SPEC_ROGUE_COMBAT, 60) == 13452);
    // Hunter: AGI ladder.
    CHECK(ResolveBattleElixir(3, SPEC_HUNTER_MM, 60) == 13452);
    // Fire mage: Firepower 6373(18) -> Arcane 9155(37) -> Greater Fire 21546(40).
    CHECK(ResolveBattleElixir(8, SPEC_MAGE_FIRE, 18) == 6373);
    CHECK(ResolveBattleElixir(8, SPEC_MAGE_FIRE, 37) == 9155);
    CHECK(ResolveBattleElixir(8, SPEC_MAGE_FIRE, 40) == 21546);
    CHECK(ResolveBattleElixir(8, SPEC_MAGE_FIRE, 60) == 13454);
    // Frost mage skips to Greater Arcane.
    CHECK(ResolveBattleElixir(8, SPEC_MAGE_FROST, 28) == 17708);
    CHECK(ResolveBattleElixir(8, SPEC_MAGE_FROST, 47) == 13454);
    // Shadow priest: Shadow Power 9264(40).
    CHECK(ResolveBattleElixir(5, SPEC_PRIEST_SHADOW, 40) == 9264);
    CHECK(ResolveBattleElixir(5, SPEC_PRIEST_SHADOW, 47) == 13454);
    // Holy priest: no battle elixir.
    CHECK(ResolveBattleElixir(5, SPEC_PRIEST_HOLY, 60) == 0);
    // Holy paladin / resto shaman / resto druid: none.
    CHECK(ResolveBattleElixir(2, SPEC_PALADIN_HOLY, 60) == 0);
    CHECK(ResolveBattleElixir(7, SPEC_SHAMAN_RESTORATION, 60) == 0);
    CHECK(ResolveBattleElixir(11, SPEC_DRUID_RESTORATION, 60) == 0);
    // Too low for anything: level 1 mage takes nothing.
    CHECK(ResolveBattleElixir(8, SPEC_MAGE_FIRE, 1) == 0);
}

// Guardian ladders per role.
static void TestGuardianLadders()
{
    // Prot warrior: armor ladder to Superior Defense 13445(43).
    CHECK(ResolveGuardianElixir(1, SPEC_WARRIOR_PROTECTION, 1) == 5997);
    CHECK(ResolveGuardianElixir(1, SPEC_WARRIOR_PROTECTION, 16) == 3389);
    CHECK(ResolveGuardianElixir(1, SPEC_WARRIOR_PROTECTION, 43) == 13445);
    // Arms warrior: health ladder to Major Troll's Blood 20004(53).
    CHECK(ResolveGuardianElixir(1, SPEC_WARRIOR_ARMS, 2) == 2458);
    CHECK(ResolveGuardianElixir(1, SPEC_WARRIOR_ARMS, 25) == 3825);
    CHECK(ResolveGuardianElixir(1, SPEC_WARRIOR_ARMS, 53) == 20004);
    // Holy paladin: mana ladder to Sages 13447(44).
    CHECK(ResolveGuardianElixir(2, SPEC_PALADIN_HOLY, 10) == 3383);
    CHECK(ResolveGuardianElixir(2, SPEC_PALADIN_HOLY, 40) == 20007);
    CHECK(ResolveGuardianElixir(2, SPEC_PALADIN_HOLY, 44) == 13447);
    // Hunter: mana line for shot rotations.
    CHECK(ResolveGuardianElixir(3, SPEC_HUNTER_MM, 40) == 20007);
}

// Scrolls resolve to the role stat at the highest usable rank.
static void TestScrolls()
{
    CHECK(ResolveScroll(1, SPEC_WARRIOR_ARMS, 60) == 10310); // STR IV
    CHECK(ResolveScroll(1, SPEC_WARRIOR_PROTECTION, 60) == 10307); // STA IV
    CHECK(ResolveScroll(4, SPEC_ROGUE_COMBAT, 60) == 10309); // AGI IV
    CHECK(ResolveScroll(8, SPEC_MAGE_FIRE, 60) == 10308); // INT IV
    CHECK(ResolveScroll(5, SPEC_PRIEST_HOLY, 45) == 10306); // SPI IV
    CHECK(ResolveScroll(5, SPEC_PRIEST_HOLY, 20) == 1712); // SPI II at 20
    CHECK(ResolveScroll(11, SPEC_DRUID_RESTORATION, 60) == 10306);
}

// Flasks at 50+: Titans for physical, Wisdom for healers, Supreme for casters.
static void TestFlasks()
{
    CHECK(ResolveFlask(1, SPEC_WARRIOR_ARMS, 49) == 0);
    CHECK(ResolveFlask(1, SPEC_WARRIOR_ARMS, 50) == 13510);
    CHECK(ResolveFlask(1, SPEC_WARRIOR_PROTECTION, 60) == 13510);
    CHECK(ResolveFlask(4, SPEC_ROGUE_COMBAT, 60) == 13510);
    CHECK(ResolveFlask(2, SPEC_PALADIN_HOLY, 60) == 13511);
    CHECK(ResolveFlask(5, SPEC_PRIEST_HOLY, 60) == 13511);
    CHECK(ResolveFlask(8, SPEC_MAGE_FIRE, 60) == 13512);
    CHECK(ResolveFlask(9, SPEC_WARLOCK_DESTRUCTION, 60) == 13512);
    CHECK(ResolveFlask(11, SPEC_DRUID_BALANCE, 60) == 13512);
    CHECK(ResolveFlask(2, SPEC_PALADIN_HOLY, 49) == 0);
}

// Protection potions echo the master's element at the bot's tier.
static void TestProtPotions()
{
    CHECK(ResolveProtPotion(6049, 23) == 6049); // Fire lesser usable at 23
    CHECK(ResolveProtPotion(6049, 22) == 0); // not yet
    CHECK(ResolveProtPotion(13457, 60) == 13457); // Greater at 48+
    CHECK(ResolveProtPotion(13457, 47) == 6049); // falls back to lesser
    CHECK(ResolveProtPotion(13456, 60) == 13456); // Frost greater
    CHECK(ResolveProtPotion(6051, 10) == 6051); // Holy lesser at 10
    CHECK(ResolveProtPotion(13461, 48) == 13461); // Arcane has greater only
    CHECK(ResolveProtPotion(13461, 47) == 0);
    CHECK(ResolveProtPotion(9999, 60) == 0); // unknown master item
}

// Top-level resolution: slot isolation (battle master -> battle slot only).
static void TestResolveMimicItem()
{
    // Warrior master drinks Giants (Band 3, req 38); rogue bot answers the
    // Band 3 AGI equivalent (Greater Agility), not Mongoose (Band 4).
    MimicResolution r = ResolveMimicItem(9206, 4, SPEC_ROGUE_COMBAT, 60);
    CHECK(r.purpose == MimicPurpose::BATTLE_PHYS_STR);
    CHECK(r.battleItemId == 9187);

    // Mage master drinks Greater Arcane; holy paladin bot has no battle answer.

    // Healers take mana oil, casters wizard oil, hunters stones.
    CHECK(ResolveMimicItem(20750, 5, SPEC_PRIEST_HOLY, 60).singleItemId == 20748); // mana oil
    CHECK(ResolveMimicItem(20746, 8, SPEC_MAGE_FIRE, 60).singleItemId == 20746); // Band 3 oil -> Band 3 wizard oil
    CHECK(ResolveMimicItem(18262, 3, SPEC_HUNTER_MM, 60).singleItemId == 23122); // stone
    CHECK(ResolveMimicItem(18262, 2, SPEC_PALADIN_RETRIBUTION, 60).singleItemId == 23122); // ret: stone
    CHECK(ResolveMimicItem(18262, 9, SPEC_WARLOCK_DESTRUCTION, 60).singleItemId == 20749); // lock: wizard

    // Guardian master -> guardian slot only.
    MimicResolution g = ResolveMimicItem(13445, 1, SPEC_WARRIOR_PROTECTION, 60);
    CHECK(g.purpose == MimicPurpose::GUARDIAN_ARMOR);
    CHECK(g.guardianItemId == 13445);
    CHECK(g.battleItemId == 0);

    // Scroll / flask / prot / food resolve to single items (capped by band).
    CHECK(ResolveMimicItem(10310, 1, SPEC_WARRIOR_ARMS, 60).singleItemId == 10310);
    CHECK(ResolveMimicItem(13510, 8, SPEC_MAGE_FIRE, 60).singleItemId == 13512);
    CHECK(ResolveMimicItem(13457, 1, SPEC_WARRIOR_ARMS, 60).singleItemId == 13457);
    CHECK(ResolveMimicItem(21023, 1, SPEC_WARRIOR_PROTECTION, 60).singleItemId == 21023);

    // Anti-abuse: level 60 master drinking Band 1 grants Band 1 only.
    CHECK(ResolveMimicItem(2454, 4, SPEC_ROGUE_COMBAT, 60).battleItemId == 2457); // Lion -> Minor Agility
    CHECK(ResolveMimicItem(6888, 1, SPEC_WARRIOR_ARMS, 60).singleItemId == 6316); // Band 1 master -> top Band 1 food
    CHECK(ResolveMimicItem(2862, 1, SPEC_WARRIOR_ARMS, 60).singleItemId == 2871); // Band 1 master -> top Band 1 stone
    CHECK(ResolveMimicItem(13452, 4, SPEC_ROGUE_COMBAT, 60).battleItemId == 13452); // Mongoose -> Mongoose
    CHECK(ResolveMimicItem(21023, 1, SPEC_WARRIOR_PROTECTION, 60).singleItemId == 21023);
    CHECK(EffectiveMimicLevel(60, 2454) == 15);
    CHECK(EffectiveMimicLevel(60, 13452) == 49);
    CHECK(EffectiveMimicLevel(30, 13452) == 30);

    // New purposes classify.
    CHECK(PurposeForMasterItem(21151) == MimicPurpose::ALCOHOL_BUFF);
    CHECK(PurposeForMasterItem(12820) == MimicPurpose::SPECIAL_RAID);
    CHECK(PurposeForMasterItem(12460) == MimicPurpose::SPECIAL_RAID);
    CHECK(PurposeForMasterItem(20079) == MimicPurpose::SPECIAL_RAID);
    CHECK(PurposeForMasterItem(9172) == MimicPurpose::UTILITY);
    CHECK(PurposeForMasterItem(5634) == MimicPurpose::UTILITY);
    CHECK(PurposeForMasterItem(6888) == MimicPurpose::FOOD_BUFF);
    CHECK(PurposeForMasterItem(2682) == MimicPurpose::FOOD_BUFF);
    // Alcohol mirrors directly; Firewater/Juju mirror; Zanza by role.
    CHECK(ResolveMimicItem(21151, 1, SPEC_WARRIOR_ARMS, 60).singleItemId == 21151);
    CHECK(ResolveMimicItem(12820, 1, SPEC_WARRIOR_ARMS, 60).singleItemId == 12820);
    CHECK(ResolveMimicItem(12460, 1, SPEC_WARRIOR_ARMS, 60).singleItemId == 12460);
    CHECK(ResolveMimicItem(20079, 1, SPEC_WARRIOR_ARMS, 60).singleItemId == 20079);
    CHECK(ResolveMimicItem(20079, 8, SPEC_MAGE_FIRE, 60).singleItemId == 20080);
    CHECK(ResolveMimicItem(9172, 1, SPEC_WARRIOR_ARMS, 60).singleItemId == 9172);
    CHECK(ResolveMimicItem(3823, 1, SPEC_WARRIOR_ARMS, 30).singleItemId == 3823);
    CHECK(ResolveMimicItem(5634, 1, SPEC_WARRIOR_ARMS, 60).singleItemId == 5634);
    // Role food: rogue Band 3 gets Squid, mage Tuber at 45, melee keeps Dumplings (Chimaerok is Band 5).
    CHECK(ResolveMimicItem(13928, 4, SPEC_ROGUE_COMBAT, 60).singleItemId == 13928);
    CHECK(ResolveMimicItem(18254, 8, SPEC_MAGE_FIRE, 60).singleItemId == 18254);
    CHECK(ResolveMimicItem(20452, 1, SPEC_WARRIOR_ARMS, 60).singleItemId == 20452);

    // Unknown master item / zero level: no-op.
    CHECK(ResolveMimicItem(9999, 1, SPEC_WARRIOR_ARMS, 60).purpose == MimicPurpose::NONE);
    CHECK(ResolveMimicItem(9206, 1, SPEC_WARRIOR_ARMS, 0).purpose == MimicPurpose::NONE);

    // Rogue weapon imbue is handled via poisons, not stones/oils.
    MimicResolution w = ResolveMimicItem(18262, 4, SPEC_ROGUE_COMBAT, 60);
    CHECK(w.purpose == MimicPurpose::WEAPON_IMBUE);
    CHECK(w.singleItemId == 0);
    CHECK(!w.skipWeaponImbue);
    // Lowbie rogue (level 10) gets top usable stone (2863, req 5).
    CHECK(ResolveMimicItem(2862, 4, SPEC_ROGUE_COMBAT, 10).singleItemId == 2863);
    // Shaman / feral skip stones (class imbues / stat-sticks).
    CHECK(ResolveMimicItem(18262, 7, SPEC_SHAMAN_ENHANCEMENT, 60).skipWeaponImbue);
    CHECK(ResolveMimicItem(18262, 11, SPEC_DRUID_FERAL, 60).skipWeaponImbue);
    // Warrior takes the top sharpening stone.
    CHECK(ResolveMimicItem(18262, 1, SPEC_WARRIOR_ARMS, 60).singleItemId == 23122);
}
int main()
{
    TestPurposeClassification();
    TestRoleForSpec();
    TestBattleLadders();
    TestGuardianLadders();
    TestScrolls();
    TestFlasks();
    TestProtPotions();
    TestResolveMimicItem();
    std::printf("mimic_consumable_policy: %d checks passed\n", checks);
    return 0;
}
