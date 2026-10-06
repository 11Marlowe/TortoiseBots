#pragma once

#include <cstdint>

// Consumable-mimicry decision policy (Issue #491): pure mapping rules over
// durable facts (master item entry, bot class/spec/level) so they stay unit
// tested on their own (tools/test_mimic_consumable_policy.cpp). Server data,
// not guesses: item entries and req levels below verified against core
// sql/base/tw_world_item_template.sql (itemlevel=-1 consumables; the field
// after itemlevel is RequiredLevel). Aura IDs mirror
// UseConsumableAction::knownFlaskAuras / knownFoodBuffAuras and
// EmoteAction. Stones/oils/poisons reuse runtime/ClassConsumablePolicy.h
// entries (verified there); this file owns the elixir/flask/scroll/
// protection-potion/food ladders only.
//
// Class numbers mirror SharedDefines.h. Spec numbers mirror
// ai/playerbot/PlayerTalentSpec.h.

namespace TortoiseBots
{

inline constexpr uint32_t MIMIC_CLASS_WARRIOR_ID = 1;
inline constexpr uint32_t MIMIC_CLASS_PALADIN_ID = 2;
inline constexpr uint32_t MIMIC_CLASS_HUNTER_ID = 3;
inline constexpr uint32_t MIMIC_CLASS_ROGUE_ID = 4;
inline constexpr uint32_t MIMIC_CLASS_PRIEST_ID = 5;
inline constexpr uint32_t MIMIC_CLASS_SHAMAN_ID = 7;
inline constexpr uint32_t MIMIC_CLASS_MAGE_ID = 8;
inline constexpr uint32_t MIMIC_CLASS_WARLOCK_ID = 9;
inline constexpr uint32_t MIMIC_CLASS_DRUID_ID = 11;

// PlayerTalentSpec values (ai/playerbot/PlayerTalentSpec.h).
inline constexpr uint32_t SPEC_WARRIOR_ARMS = 1;
inline constexpr uint32_t SPEC_WARRIOR_FURY = 2;
inline constexpr uint32_t SPEC_WARRIOR_PROTECTION = 3;
inline constexpr uint32_t SPEC_PALADIN_HOLY = 4;
inline constexpr uint32_t SPEC_PALADIN_PROTECTION = 5;
inline constexpr uint32_t SPEC_PALADIN_RETRIBUTION = 6;
inline constexpr uint32_t SPEC_HUNTER_BM = 7;
inline constexpr uint32_t SPEC_HUNTER_MM = 8;
inline constexpr uint32_t SPEC_HUNTER_SV = 9;
inline constexpr uint32_t SPEC_ROGUE_ASSA = 10;
inline constexpr uint32_t SPEC_ROGUE_COMBAT = 11;
inline constexpr uint32_t SPEC_ROGUE_SUB = 12;
inline constexpr uint32_t SPEC_PRIEST_DISC = 13;
inline constexpr uint32_t SPEC_PRIEST_HOLY = 14;
inline constexpr uint32_t SPEC_PRIEST_SHADOW = 15;
inline constexpr uint32_t SPEC_SHAMAN_ELEMENTAL = 19;
inline constexpr uint32_t SPEC_SHAMAN_ENHANCEMENT = 20;
inline constexpr uint32_t SPEC_SHAMAN_RESTORATION = 21;
inline constexpr uint32_t SPEC_MAGE_ARCANE = 22;
inline constexpr uint32_t SPEC_MAGE_FIRE = 23;
inline constexpr uint32_t SPEC_MAGE_FROST = 24;
inline constexpr uint32_t SPEC_WARLOCK_AFFLICTION = 25;
inline constexpr uint32_t SPEC_WARLOCK_DEMONOLOGY = 26;
inline constexpr uint32_t SPEC_WARLOCK_DESTRUCTION = 27;
inline constexpr uint32_t SPEC_DRUID_BALANCE = 31;
inline constexpr uint32_t SPEC_DRUID_FERAL = 32;
inline constexpr uint32_t SPEC_DRUID_RESTORATION = 33;

enum class MimicPurpose : uint8_t
{
    NONE = 0,
    BATTLE_PHYS_STR,   // STR / AP elixirs
    BATTLE_PHYS_AGI,   // AGI / crit elixirs
    BATTLE_CASTER,     // spell damage elixirs
    GUARDIAN_ARMOR,    // armor / defense elixirs
    GUARDIAN_HEALTH,   // HP / HP5 elixirs
    GUARDIAN_MANA,     // INT / MP5 / spirit elixirs
    SCROLL_STAT,       // single-use stat scrolls
    WEAPON_IMBUE,      // stones, oils, poisons
    FLASK,             // persistent flasks
    PROT_POTION,       // elemental absorption shields
    FOOD_BUFF          // Well Fed stat food
};

enum class MimicRole : uint8_t
{
    MELEE_DPS = 0,
    TANK,
    CASTER_DPS,
    HEALER,
    RANGED_AGI // hunter: agility battle, mana guardian
};

// Spec -> role. Feral covers both cat (melee) and bear (tank); the bot
// resolves bear via form at cast time, the policy defaults feral to melee.
inline MimicRole RoleForSpec(uint32_t playerClass, uint32_t spec)
{
    switch (playerClass)
    {
        case MIMIC_CLASS_WARRIOR_ID:
            return spec == SPEC_WARRIOR_PROTECTION ? MimicRole::TANK : MimicRole::MELEE_DPS;
        case MIMIC_CLASS_PALADIN_ID:
            if (spec == SPEC_PALADIN_PROTECTION)
                return MimicRole::TANK;
            return spec == SPEC_PALADIN_HOLY ? MimicRole::HEALER : MimicRole::MELEE_DPS;
        case MIMIC_CLASS_HUNTER_ID:
            return MimicRole::RANGED_AGI;
        case MIMIC_CLASS_ROGUE_ID:
            return MimicRole::MELEE_DPS;
        case MIMIC_CLASS_PRIEST_ID:
            return (spec == SPEC_PRIEST_DISC || spec == SPEC_PRIEST_HOLY) ? MimicRole::HEALER : MimicRole::CASTER_DPS;
        case MIMIC_CLASS_SHAMAN_ID:
            if (spec == SPEC_SHAMAN_RESTORATION)
                return MimicRole::HEALER;
            return spec == SPEC_SHAMAN_ELEMENTAL ? MimicRole::CASTER_DPS : MimicRole::MELEE_DPS;
        case MIMIC_CLASS_MAGE_ID:
        case MIMIC_CLASS_WARLOCK_ID:
            return MimicRole::CASTER_DPS;
        case MIMIC_CLASS_DRUID_ID:
            if (spec == SPEC_DRUID_RESTORATION)
                return MimicRole::HEALER;
            return spec == SPEC_DRUID_BALANCE ? MimicRole::CASTER_DPS : MimicRole::MELEE_DPS;
        default:
            break;
    }
    return MimicRole::MELEE_DPS;
}

// Master item entry -> purpose key. Entries verified in core item_template.
inline MimicPurpose PurposeForMasterItem(uint32_t itemId)
{
    switch (itemId)
    {
        // battle_phys_str: Lion 2454(1) Ogre 3391(20) Giants 9206(38) Brute 13453(45)
        case 2454:
        case 3391:
        case 9206:
        case 13453:
            return MimicPurpose::BATTLE_PHYS_STR;
        // battle_phys_agi: Minor 2457(2) Lesser 3390(18) Agility 8949(27)
        // Greater 9187(38) Mongoose 13452(46)
        case 2457:
        case 3390:
        case 8949:
        case 9187:
        case 13452:
            return MimicPurpose::BATTLE_PHYS_AGI;
        // battle_caster: Firepower 6373(18) Greater Fire 21546(40)
        // Arcane 9155(37) Greater Arcane 13454(47) Frost 17708(28)
        // Shadow 9264(40) Greater Nature 50237(48)
        case 6373:
        case 21546:
        case 9155:
        case 13454:
        case 17708:
        case 9264:
        case 50237:
            return MimicPurpose::BATTLE_CASTER;
        // guardian_armor: Minor Defense 5997(1) Defense 3389(16)
        // Greater 8951(29) Superior 13445(43)
        case 5997:
        case 3389:
        case 8951:
        case 13445:
            return MimicPurpose::GUARDIAN_ARMOR;
        // guardian_health: Minor Fort 2458(2) Fortitude 3825(25)
        // Mighty Troll 3826(26) Major Troll 20004(53)
        case 2458:
        case 3825:
        case 3826:
        case 20004:
            return MimicPurpose::GUARDIAN_HEALTH;
        // guardian_mana: Wisdom 3383(10) Greater Intellect 9179(37)
        // Sages 13447(44) Mageblood 20007(40)
        case 3383:
        case 9179:
        case 13447:
        case 20007:
            return MimicPurpose::GUARDIAN_MANA;
        // scroll_stat: STR 954/2289/4426/10310, AGI 3012/1477/4425/10309,
        // STA 1180/1711/4422/10307, INT 955/2290/4419/10308,
        // SPI 1181/1712/4424/10306, PROT 3013/1478/4421/10305
        case 954:
        case 2289:
        case 4426:
        case 10310:
        case 3012:
        case 1477:
        case 4425:
        case 10309:
        case 1180:
        case 1711:
        case 4422:
        case 10307:
        case 1181:
        case 1712:
        case 4424:
        case 10306:
        case 955:
        case 2290:
        case 4419:
        case 10308:
        case 3013:
        case 1478:
        case 4421:
        case 10305:
            return MimicPurpose::SCROLL_STAT;
        // weapon_imbue: stones 2862/2863/2871/7964/12404/18262/23122,
        // 3239/3240/3241/7965/12643; oils 20744/20746/20750/20749/23123,
        // 20745/20747/20748; poisons 6947/6949/6950/8926/8927/8928,
        // 2892/2893/8984/8985/20844, 3775/3776
        case 2862:
        case 2863:
        case 2871:
        case 7964:
        case 12404:
        case 18262:
        case 23122:
        case 3239:
        case 3240:
        case 3241:
        case 7965:
        case 12643:
        case 20744:
        case 20746:
        case 20750:
        case 20749:
        case 23123:
        case 20745:
        case 20747:
        case 20748:
        case 6947:
        case 6949:
        case 6950:
        case 8926:
        case 8927:
        case 8928:
        case 2892:
        case 2893:
        case 8984:
        case 8985:
        case 20844:
        case 3775:
        case 3776:
            return MimicPurpose::WEAPON_IMBUE;
        // flask: Titans 13510(60) Wisdom 13511(50) Supreme 13512(60) Chromatic 13513(50)
        case 13510:
        case 13511:
        case 13512:
        case 13513:
            return MimicPurpose::FLASK;
        // prot_potion: Fire 6049(23)/13457(48) Frost 6050(38)/13456(48)
        // Nature 6052(38)/13458(48) Shadow 6048(27)/13459(48)
        // Holy 6051(10)/13460(48) Arcane 13461(48)
        case 6049:
        case 13457:
        case 6050:
        case 13456:
        case 6052:
        case 13458:
        case 6048:
        case 13459:
        case 6051:
        case 13460:
        case 13461:
            return MimicPurpose::PROT_POTION;
        // food_buff: Squid 13928(35) Nightfin 13931(35) Dumplings 20452(45)
        // Chimaerok Chops 21023(55); 21024 is the raw ingredient, not food
        case 13928:
        case 13931:
        case 20452:
        case 21023:
            return MimicPurpose::FOOD_BUFF;
        default:
            break;
    }
    return MimicPurpose::NONE;
}

inline bool IsBattleElixirPurpose(MimicPurpose purpose)
{
    return purpose == MimicPurpose::BATTLE_PHYS_STR || purpose == MimicPurpose::BATTLE_PHYS_AGI ||
           purpose == MimicPurpose::BATTLE_CASTER;
}

inline bool IsGuardianElixirPurpose(MimicPurpose purpose)
{
    return purpose == MimicPurpose::GUARDIAN_ARMOR || purpose == MimicPurpose::GUARDIAN_HEALTH ||
           purpose == MimicPurpose::GUARDIAN_MANA;
}

// Tiered entry with the req level from core item_template. Highest usable
// tier wins: reqLevel <= botLevel. Broken custom items (55046/55048: no
// spell on the prototype) are filtered at cast time via Spells[0].SpellId.
struct MimicTier
{
    uint32_t itemId;
    uint8_t reqLevel;
};

// Battle elixir ladders per role.
inline constexpr MimicTier BATTLE_STR_LADDER[] = { { 2454, 1 }, { 3391, 20 }, { 9206, 38 }, { 13453, 45 } };
inline constexpr MimicTier BATTLE_AGI_LADDER[] = { { 2457, 2 }, { 3390, 18 }, { 8949, 27 }, { 9187, 38 }, { 13452, 46 } };
inline constexpr MimicTier BATTLE_FIRE_LADDER[] = { { 6373, 18 }, { 9155, 37 }, { 21546, 40 }, { 13454, 47 } };
inline constexpr MimicTier BATTLE_FROST_LADDER[] = { { 17708, 28 }, { 9155, 37 }, { 13454, 47 } };
inline constexpr MimicTier BATTLE_ARCANE_LADDER[] = { { 9155, 37 }, { 13454, 47 } };
inline constexpr MimicTier BATTLE_SHADOW_LADDER[] = { { 9155, 37 }, { 9264, 40 }, { 13454, 47 } };
inline constexpr MimicTier BATTLE_NATURE_LADDER[] = { { 9155, 37 }, { 13454, 47 }, { 50237, 48 } };

// Guardian elixir ladders per role.
inline constexpr MimicTier GUARDIAN_ARMOR_LADDER[] = { { 5997, 1 }, { 3389, 16 }, { 8951, 29 }, { 13445, 43 } };
inline constexpr MimicTier GUARDIAN_HEALTH_LADDER[] = { { 2458, 2 }, { 3825, 25 }, { 3826, 26 }, { 20004, 53 } };
inline constexpr MimicTier GUARDIAN_MANA_LADDER[] = { { 3383, 10 }, { 9179, 37 }, { 20007, 40 }, { 13447, 44 } };

// Scroll ladders (I-IV RequiredLevel from core item_template).
inline constexpr MimicTier SCROLL_STR_LADDER[] = { { 954, 10 }, { 2289, 25 }, { 4426, 40 }, { 10310, 55 } };
inline constexpr MimicTier SCROLL_AGI_LADDER[] = { { 3012, 10 }, { 1477, 25 }, { 4425, 40 }, { 10309, 55 } };
inline constexpr MimicTier SCROLL_STA_LADDER[] = { { 1180, 5 }, { 1711, 20 }, { 4422, 35 }, { 10307, 50 } };
inline constexpr MimicTier SCROLL_INT_LADDER[] = { { 955, 5 }, { 2290, 20 }, { 4419, 35 }, { 10308, 50 } };
inline constexpr MimicTier SCROLL_SPI_LADDER[] = { { 1181, 1 }, { 1712, 15 }, { 4424, 30 }, { 10306, 45 } };
inline constexpr MimicTier SCROLL_PROT_LADDER[] = { { 3013, 1 }, { 1478, 15 }, { 4421, 30 }, { 10305, 45 } };

// Weapon imbue ladders. Stones: sharpening 2862(1)/2863(5)/2871(15)/
// 7964(25)/12404(35)/18262(50)/23122(50); weight 3239(1)/3240(5)/3241(15)/
// 7965(25)/12643(35). Oils: wizard 20744(5)/20746(30)/20750(40)/20749(45);
// mana 20745(20)/20747(40)/20748(45). Poisons: instant MH
// 6947(20)/6949(28)/6950(36)/8926(44)/8927(52)/8928(60); deadly OH
// 2892(30)/2893(38)/8984(46)/8985(54)/20844(60).
inline constexpr MimicTier SHARPENING_LADDER[] = { { 2862, 1 }, { 2863, 5 }, { 2871, 15 }, { 7964, 25 }, { 12404, 35 }, { 18262, 50 }, { 23122, 50 } };
inline constexpr MimicTier WEIGHTSTONE_LADDER[] = { { 3239, 1 }, { 3240, 5 }, { 3241, 15 }, { 7965, 25 }, { 12643, 35 } };
inline constexpr MimicTier WIZARD_OIL_LADDER[] = { { 20744, 5 }, { 20746, 30 }, { 20750, 40 }, { 20749, 45 } };
inline constexpr MimicTier MANA_OIL_LADDER[] = { { 20745, 20 }, { 20747, 40 }, { 20748, 45 } };
inline constexpr MimicTier INSTANT_POISON_LADDER[] = { { 6947, 20 }, { 6949, 28 }, { 6950, 36 }, { 8926, 44 }, { 8927, 52 }, { 8928, 60 } };
inline constexpr MimicTier DEADLY_POISON_LADDER[] = { { 2892, 30 }, { 2893, 38 }, { 8984, 46 }, { 8985, 54 }, { 20844, 60 } };

// Flask items (RequiredLevel 50 for all four in core item_template).
inline constexpr uint32_t FLASK_TITANS_ITEM_ID = 13510;
inline constexpr uint32_t FLASK_WISDOM_ITEM_ID = 13511;
inline constexpr uint32_t FLASK_SUPREME_ITEM_ID = 13512;
inline constexpr uint32_t FLASK_CHROMATIC_ITEM_ID = 13513;
inline constexpr uint8_t FLASK_TITANS_REQ = 50;
inline constexpr uint8_t FLASK_WISDOM_REQ = 50;
inline constexpr uint8_t FLASK_SUPREME_REQ = 50;
inline constexpr uint8_t FLASK_CHROMATIC_REQ = 50;

// Flask auras mirror UseConsumableAction::knownFlaskAuras.
inline constexpr uint32_t FLASK_TITANS_AURA_ID = 17626;
inline constexpr uint32_t FLASK_WISDOM_AURA_ID = 17627;
inline constexpr uint32_t FLASK_SUPREME_AURA_ID = 17628;
inline constexpr uint32_t FLASK_CHROMATIC_AURA_ID = 17629;

// Well-fed food items + buff auras (auras mirror knownFoodBuffAuras).
inline constexpr MimicTier FOOD_LADDER[] = { { 13928, 35 }, { 13931, 35 }, { 20452, 45 }, { 21023, 55 } };
inline constexpr uint32_t FOOD_SQUID_AURA_ID = 18230;
inline constexpr uint32_t FOOD_CHIMAEROK_AURA_ID = 25660;
inline constexpr uint32_t FOOD_DUMPLINGS_AURA_ID = 24800;
inline constexpr uint32_t FOOD_NIGHTFIN_AURA_ID = 18233;

// Protection potions echo the master's element (lesser 10-28, greater 48).
inline constexpr MimicTier PROT_FIRE_LADDER[] = { { 6049, 23 }, { 13457, 48 } };
inline constexpr MimicTier PROT_FROST_LADDER[] = { { 6050, 28 }, { 13456, 48 } };
inline constexpr MimicTier PROT_NATURE_LADDER[] = { { 6052, 28 }, { 13458, 48 } };
inline constexpr MimicTier PROT_SHADOW_LADDER[] = { { 6048, 17 }, { 13459, 48 } };
inline constexpr MimicTier PROT_HOLY_LADDER[] = { { 6051, 10 }, { 13460, 48 } };
inline constexpr MimicTier PROT_ARCANE_LADDER[] = { { 13461, 48 } };

template <typename T, uint32_t N>
inline uint32_t BestTierForLevel(T const (&ladder)[N], uint32_t botLevel)
{
    uint32_t best = 0;
    for (auto const& tier : ladder)
        if (botLevel >= tier.reqLevel)
            best = tier.itemId;
    return best;
}

// Battle elixir for the bot's role/spec. Healers take none (0).
inline uint32_t ResolveBattleElixir(uint32_t playerClass, uint32_t spec, uint32_t botLevel)
{
    MimicRole role = RoleForSpec(playerClass, spec);
    switch (role)
    {
        case MimicRole::MELEE_DPS:
            if (playerClass == MIMIC_CLASS_ROGUE_ID)
                return BestTierForLevel(BATTLE_AGI_LADDER, botLevel);
            if (playerClass == MIMIC_CLASS_SHAMAN_ID)
                return BestTierForLevel(BATTLE_STR_LADDER, botLevel);
            if (playerClass == MIMIC_CLASS_DRUID_ID)
                return BestTierForLevel(BATTLE_AGI_LADDER, botLevel);
            // Warrior / paladin / default melee: STR ladder.
            return BestTierForLevel(BATTLE_STR_LADDER, botLevel);
        case MimicRole::TANK:
            return BestTierForLevel(BATTLE_STR_LADDER, botLevel);
        case MimicRole::CASTER_DPS:
            if (playerClass == MIMIC_CLASS_MAGE_ID)
            {
                if (spec == SPEC_MAGE_FROST)
                    return BestTierForLevel(BATTLE_FROST_LADDER, botLevel);
                if (spec == SPEC_MAGE_ARCANE)
                    return BestTierForLevel(BATTLE_ARCANE_LADDER, botLevel);
                return BestTierForLevel(BATTLE_FIRE_LADDER, botLevel);
            }
            if (playerClass == MIMIC_CLASS_WARLOCK_ID)
            {
                if (spec == SPEC_WARLOCK_DESTRUCTION)
                    return BestTierForLevel(BATTLE_FIRE_LADDER, botLevel);
                return BestTierForLevel(BATTLE_SHADOW_LADDER, botLevel);
            }
            if (playerClass == MIMIC_CLASS_PRIEST_ID)
                return BestTierForLevel(BATTLE_SHADOW_LADDER, botLevel);
            if (playerClass == MIMIC_CLASS_SHAMAN_ID)
                return BestTierForLevel(BATTLE_NATURE_LADDER, botLevel);
            if (playerClass == MIMIC_CLASS_DRUID_ID)
                return BestTierForLevel(BATTLE_NATURE_LADDER, botLevel);
            return BestTierForLevel(BATTLE_ARCANE_LADDER, botLevel);
        case MimicRole::RANGED_AGI:
            return BestTierForLevel(BATTLE_AGI_LADDER, botLevel);
        case MimicRole::HEALER:
            return 0;
        default:
            break;
    }
    return 0;
}

// Guardian elixir for the bot's role/spec.
inline uint32_t ResolveGuardianElixir(uint32_t playerClass, uint32_t spec, uint32_t botLevel)
{
    MimicRole role = RoleForSpec(playerClass, spec);
    switch (role)
    {
        case MimicRole::TANK:
            return BestTierForLevel(GUARDIAN_ARMOR_LADDER, botLevel);
        case MimicRole::MELEE_DPS:
            return BestTierForLevel(GUARDIAN_HEALTH_LADDER, botLevel);
        case MimicRole::RANGED_AGI:
            // Hunters sustain mana for shot rotations.
            return BestTierForLevel(GUARDIAN_MANA_LADDER, botLevel);
        case MimicRole::CASTER_DPS:
        case MimicRole::HEALER:
            return BestTierForLevel(GUARDIAN_MANA_LADDER, botLevel);
        default:
            break;
    }
    return 0;
}

// Scroll for the bot's role: STR/AGI for melee, STA for tanks,
// INT for casters, SPI for healers.
inline uint32_t ResolveScroll(uint32_t playerClass, uint32_t spec, uint32_t botLevel)
{
    MimicRole role = RoleForSpec(playerClass, spec);
    switch (role)
    {
        case MimicRole::TANK:
            return BestTierForLevel(SCROLL_STA_LADDER, botLevel);
        case MimicRole::MELEE_DPS:
            if (playerClass == MIMIC_CLASS_ROGUE_ID || playerClass == MIMIC_CLASS_HUNTER_ID ||
                playerClass == MIMIC_CLASS_DRUID_ID)
                return BestTierForLevel(SCROLL_AGI_LADDER, botLevel);
            return BestTierForLevel(SCROLL_STR_LADDER, botLevel);
        case MimicRole::RANGED_AGI:
            return BestTierForLevel(SCROLL_AGI_LADDER, botLevel);
        case MimicRole::CASTER_DPS:
            return BestTierForLevel(SCROLL_INT_LADDER, botLevel);
        case MimicRole::HEALER:
            return BestTierForLevel(SCROLL_SPI_LADDER, botLevel);
        default:
            break;
    }
    return 0;
}

// Flask for the bot's role at 50+: Titans for melee/tank/ranged,
// Distilled Wisdom for healers, Supreme Power for casters.
inline uint32_t ResolveFlask(uint32_t playerClass, uint32_t spec, uint32_t botLevel)
{
    MimicRole role = RoleForSpec(playerClass, spec);
    if (role == MimicRole::HEALER)
        return botLevel >= FLASK_WISDOM_REQ ? FLASK_WISDOM_ITEM_ID : 0;
    if (role == MimicRole::CASTER_DPS)
        return botLevel >= FLASK_SUPREME_REQ ? FLASK_SUPREME_ITEM_ID : 0;
    return botLevel >= FLASK_TITANS_REQ ? FLASK_TITANS_ITEM_ID : 0;
}

inline uint32_t ResolveFood(uint32_t botLevel)
{
    return BestTierForLevel(FOOD_LADDER, botLevel);
}

// Same-element protection potion at the bot's level, or 0.
inline uint32_t ResolveProtPotion(uint32_t masterItemId, uint32_t botLevel)
{
    switch (masterItemId)
    {
        case 6049:
        case 13457:
            return BestTierForLevel(PROT_FIRE_LADDER, botLevel);
        case 6050:
        case 13456:
            return BestTierForLevel(PROT_FROST_LADDER, botLevel);
        case 6052:
        case 13458:
            return BestTierForLevel(PROT_NATURE_LADDER, botLevel);
        case 6048:
        case 13459:
            return BestTierForLevel(PROT_SHADOW_LADDER, botLevel);
        case 6051:
        case 13460:
            return BestTierForLevel(PROT_HOLY_LADDER, botLevel);
        case 13461:
            return BestTierForLevel(PROT_ARCANE_LADDER, botLevel);
        default:
            break;
    }
    return 0;
}

struct MimicResolution
{
    MimicPurpose purpose = MimicPurpose::NONE;
    uint32_t battleItemId = 0;   // elixir or flask-instead-of-elixir choice
    uint32_t guardianItemId = 0; // elixir or flask-instead-of-elixir choice
    uint32_t singleItemId = 0;   // scroll / imbue / flask / prot / food
    bool skipWeaponImbue = false;
};

// Top-level mimic decision: master item -> bot's own equivalent.
// Flask masters make bots flask (both elixir slots collapse into the
// flask); single-slot masters resolve only their own slot so the bot
// keeps whatever it already has in the other slot.
inline MimicResolution ResolveMimicItem(uint32_t masterItemId, uint32_t playerClass, uint32_t spec, uint32_t botLevel)
{
    MimicResolution out;
    out.purpose = PurposeForMasterItem(masterItemId);
    if (out.purpose == MimicPurpose::NONE || botLevel == 0)
    {
        out.purpose = MimicPurpose::NONE;
        return out;
    }

    switch (out.purpose)
    {
        case MimicPurpose::BATTLE_PHYS_STR:
        case MimicPurpose::BATTLE_PHYS_AGI:
        case MimicPurpose::BATTLE_CASTER:
            out.battleItemId = ResolveBattleElixir(playerClass, spec, botLevel);
            break;
        case MimicPurpose::GUARDIAN_ARMOR:
        case MimicPurpose::GUARDIAN_HEALTH:
        case MimicPurpose::GUARDIAN_MANA:
            out.guardianItemId = ResolveGuardianElixir(playerClass, spec, botLevel);
            break;
        case MimicPurpose::SCROLL_STAT:
            out.singleItemId = ResolveScroll(playerClass, spec, botLevel);
            break;
        case MimicPurpose::WEAPON_IMBUE:
            // Rogues use poisons (handled by the action); shaman respect
            // class imbues; feral druids skip stones in animal forms
            // (stat-sticks). Melee take stones, casters/healers take oils.
            if (playerClass == MIMIC_CLASS_ROGUE_ID || playerClass == MIMIC_CLASS_SHAMAN_ID ||
                playerClass == MIMIC_CLASS_DRUID_ID)
            {
                out.skipWeaponImbue = (playerClass != MIMIC_CLASS_ROGUE_ID);
                out.singleItemId = 0;
                break;
            }
            if (playerClass == MIMIC_CLASS_WARRIOR_ID || playerClass == MIMIC_CLASS_PALADIN_ID ||
                playerClass == MIMIC_CLASS_HUNTER_ID)
            {
                out.singleItemId = BestTierForLevel(SHARPENING_LADDER, botLevel);
                break;
            }
            // Casters take wizard oil, healers take mana oil.
            if (RoleForSpec(playerClass, spec) == MimicRole::HEALER)
                out.singleItemId = BestTierForLevel(MANA_OIL_LADDER, botLevel);
            else
                out.singleItemId = BestTierForLevel(WIZARD_OIL_LADDER, botLevel);
            break;
        case MimicPurpose::FLASK:
            out.singleItemId = ResolveFlask(playerClass, spec, botLevel);
            break;
        case MimicPurpose::PROT_POTION:
            out.singleItemId = ResolveProtPotion(masterItemId, botLevel);
            break;
        case MimicPurpose::FOOD_BUFF:
            out.singleItemId = ResolveFood(botLevel);
            break;
        default:
            out.purpose = MimicPurpose::NONE;
            break;
    }
    return out;
}

} // namespace TortoiseBots
