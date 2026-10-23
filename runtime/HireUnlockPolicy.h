// Issue #490 PR 2: hire-gear tier unlock quests at the six capital
// <Mercenary Hire> recruiters. Pure, core-free catalog and math: quest ids,
// invisible counter entries, the boss list per counter, unlock resolution
// and the per-tier hire price. Unit-tested standalone
// (tools/test_hire_unlock_policy.cpp). The SQL migration, the kill adapter,
// the recruiter gossip and `.bot hire` all read this header, so ids and
// boss lists can never disagree.

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "runtime/policy/hire/HireGearPolicy.h"

namespace TortoiseBots
{

// One quest per tier 1..7: 90100 (tier 1) .. 90106 (tier 7). Faction-neutral
// (RequiredRaces 0), raid type, strict PrevQuestId chain unless
// HireGearAllQuestsAvailable. Unlock = quest rewarded.
constexpr uint32_t kHireUnlockQuestFirst = 90100;
constexpr uint32_t kHireUnlockQuestLast = kHireUnlockQuestFirst + kHireGearTierMax - 1;

inline uint32_t HireUnlockQuestForTier(uint8_t tier)
{
    if (tier < 1 || tier > kHireGearTierMax)
        return 0;
    return kHireUnlockQuestFirst + tier - 1;
}

// 0 when the quest is not a hire-unlock quest.
inline uint8_t HireUnlockTierForQuest(uint32_t questId)
{
    if (questId < kHireUnlockQuestFirst || questId > kHireUnlockQuestLast)
        return 0;
    return static_cast<uint8_t>(questId - kHireUnlockQuestFirst + 1);
}

// Gold fee per tier (copper), written as negative RewOrReqMoney in the
// migration. Index = tier; [0] unused.
constexpr uint32_t kHireUnlockFeeCopper[kHireGearTierMax + 1] = {
    0, 500000, 1000000, 2000000, 3000000, 4000000, 5000000, 7500000 };

// One boss (encounter) that counts toward a counter. mapId drives
// HireGearOptionalMaps: bosses on an optional map are credited
// automatically when the quest is taken.
struct HireUnlockBoss
{
    uint32_t entry;
    uint32_t mapId;
    char const* name;
};

// One quest-log counter: an invisible credit creature (never spawned) used
// as a ReqCreatureOrGOId slot. Count = bossCount; each boss credits once.
struct HireUnlockCounter
{
    uint8_t tier;
    uint8_t slot;            // ReqCreatureOrGOId slot 0..3 in the tier quest
    uint32_t creditEntry;
    char const* label;       // quest-log objective text
    HireUnlockBoss const* bosses;
    uint8_t bossCount;
};

// Map ids.
constexpr uint32_t kMapBlackrockDepths = 230;
constexpr uint32_t kMapBlackrockSpire = 229;
constexpr uint32_t kMapScholomance = 289;
constexpr uint32_t kMapStratholme = 329;
constexpr uint32_t kMapDireMaul = 429;
constexpr uint32_t kMapKarazhanCrypt = 800;
constexpr uint32_t kMapZulGurub = 309;
constexpr uint32_t kMapRuinsOfAhnQiraj = 509;
constexpr uint32_t kMapLowerKarazhan = 532;
constexpr uint32_t kMapMoltenCore = 409;
constexpr uint32_t kMapOnyxiasLair = 249;
constexpr uint32_t kMapTimbermawHold = 819;
constexpr uint32_t kMapBlackwingLair = 469;
constexpr uint32_t kMapEmeraldSanctum = 807;
constexpr uint32_t kMapTempleOfAhnQiraj = 531;
constexpr uint32_t kMapNaxxramas = 533;
constexpr uint32_t kMapTowerOfKarazhan = 814;

// Multi-boss encounters credited as one boss. The encounter is keyed by its
// first entry; the kill adapter credits it when the LAST member dies (every
// other member of the group is dead on the same map instance).
constexpr uint32_t kHireUnlockFourHorsemen[] = { 16062, 16063, 16064, 16065 };

// High Priest Thekal (14599) fakes his first death: the P1 corpse rezzes
// after 10 s and the fight continues as entry 14509 (SetEntry in
// UpdateAI_corpse, boss_thekal.cpp). The real, lootable death fires
// OnCreatureKill(14509), so that entry aliases to the same encounter.
constexpr uint32_t kHireUnlockThekalPhaseTwo = 14509;

// Linked credit (trigger creature entry -> also-credited encounter key):
// bosses that never die, or may leave without dying, count when a linked
// boss dies. Majordomo Executus (12018) is never killed (defeat = friendly
// + evade), but Ragnaros (11502) can only be summoned after Domo falls, so
// a Ragnaros kill also credits Majordomo. Erennius (60747) despawns
// friendly when Solnius (60748) dies first, so a Solnius kill also credits
// Erennius (a no-op when Erennius already died first: dedup is per player).
struct HireUnlockLinkedCredit
{
    uint32_t triggerEntry;
    uint32_t alsoCreditEncounter;
};

constexpr HireUnlockLinkedCredit kHireUnlockLinkedCredits[] = {
    { 11502, 12018 },
    { 60748, 60747 },
};

// The also-credited encounter for a killed creature entry, or 0.
inline uint32_t HireUnlockLinkedCreditFor(uint32_t creatureEntry)
{
    for (HireUnlockLinkedCredit const& link : kHireUnlockLinkedCredits)
        if (link.triggerEntry == creatureEntry)
            return link.alsoCreditEncounter;
    return 0;
}

// Tower of Karazhan King's Council (King/Knight/Rook/Bishop): the loot is on
// the King, so the council counts on the King's death alone (59967).

constexpr HireUnlockBoss kHireUnlockBossesDungeons[] = {
    { 9019, kMapBlackrockDepths, "Emperor Dagran Thaurissan" },
    { 1853, kMapScholomance, "Darkmaster Gandling" },
    { 10440, kMapStratholme, "Baron Rivendare" },
    { 10813, kMapStratholme, "Balnazzar" },
    { 10363, kMapBlackrockSpire, "General Drakkisath" },
    { 9568, kMapBlackrockSpire, "Overlord Wyrmthalak" },
    { 11492, kMapDireMaul, "Alzzin the Wildshaper" },
    { 11486, kMapDireMaul, "Prince Tortheldrin" },
    { 11501, kMapDireMaul, "King Gordok" },
    { 91928, kMapKarazhanCrypt, "Alarus" },
};

constexpr HireUnlockBoss kHireUnlockBossesZulGurub[] = {
    { 14507, kMapZulGurub, "High Priest Venoxis" },
    { 14517, kMapZulGurub, "High Priestess Jeklik" },
    { 14510, kMapZulGurub, "High Priestess Mar'li" },
    { 14599, kMapZulGurub, "High Priest Thekal" },
    { 14515, kMapZulGurub, "High Priestess Arlokk" },
    { 11380, kMapZulGurub, "Jin'do the Hexxer" },
    { 11382, kMapZulGurub, "Bloodlord Mandokir" },
    { 14834, kMapZulGurub, "Hakkar" },
};

constexpr HireUnlockBoss kHireUnlockBossesRuinsAq[] = {
    { 15348, kMapRuinsOfAhnQiraj, "Kurinnaxx" },
    { 15341, kMapRuinsOfAhnQiraj, "General Rajaxx" },
    { 15340, kMapRuinsOfAhnQiraj, "Moam" },
    { 15370, kMapRuinsOfAhnQiraj, "Buru the Gorger" },
    { 15369, kMapRuinsOfAhnQiraj, "Ayamiss the Hunter" },
    { 15339, kMapRuinsOfAhnQiraj, "Ossirian the Unscarred" },
};

constexpr HireUnlockBoss kHireUnlockBossesLowerKara[] = {
    { 61221, kMapLowerKarazhan, "Brood Queen Araxxna" },
    { 61223, kMapLowerKarazhan, "Clawlord Howlfang" },
    { 61224, kMapLowerKarazhan, "Grizikil" },
    { 61222, kMapLowerKarazhan, "Lord Blackwald II" },
    { 61225, kMapLowerKarazhan, "Moroes" },
};

constexpr HireUnlockBoss kHireUnlockBossesMoltenCore[] = {
    { 12118, kMapMoltenCore, "Lucifron" },
    { 11982, kMapMoltenCore, "Magmadar" },
    { 12259, kMapMoltenCore, "Gehennas" },
    { 12057, kMapMoltenCore, "Garr" },
    { 12056, kMapMoltenCore, "Baron Geddon" },
    { 12264, kMapMoltenCore, "Shazzrah" },
    { 12098, kMapMoltenCore, "Sulfuron Harbinger" },
    { 11988, kMapMoltenCore, "Golemagg the Incinerator" },
    { 52145, kMapMoltenCore, "Incindis" },
    { 57642, kMapMoltenCore, "Sorcerer-Thane Thaurissan" },
    { 65020, kMapMoltenCore, "Basalthar" },
    { 65021, kMapMoltenCore, "Smoldaris" },
    { 12018, kMapMoltenCore, "Majordomo Executus" },
    { 11502, kMapMoltenCore, "Ragnaros" },
};

constexpr HireUnlockBoss kHireUnlockBossesOnyxia[] = {
    { 10184, kMapOnyxiasLair, "Onyxia" },
};

constexpr HireUnlockBoss kHireUnlockBossesTimbermaw[] = {
    { 2139, kMapTimbermawHold, "Loktanag the Vile" },
    { 62935, kMapTimbermawHold, "Ormanos the Cracked" },
    { 62936, kMapTimbermawHold, "Rotgrowl" },
    { 62937, kMapTimbermawHold, "Kodiak" },
    { 62938, kMapTimbermawHold, "Archdruid Kronn" },
    { 62941, kMapTimbermawHold, "Chieftain Partath" },
    { 62946, kMapTimbermawHold, "Trioch the Devourer" },
};

constexpr HireUnlockBoss kHireUnlockBossesBlackwing[] = {
    { 12435, kMapBlackwingLair, "Razorgore the Untamed" },
    { 13020, kMapBlackwingLair, "Vaelastrasz the Corrupt" },
    { 12017, kMapBlackwingLair, "Broodlord Lashlayer" },
    { 11983, kMapBlackwingLair, "Firemaw" },
    { 14601, kMapBlackwingLair, "Ebonroc" },
    { 11981, kMapBlackwingLair, "Flamegor" },
    { 14020, kMapBlackwingLair, "Chromaggus" },
    { 11583, kMapBlackwingLair, "Nefarian" },
};

constexpr HireUnlockBoss kHireUnlockBossesEmerald[] = {
    { 60747, kMapEmeraldSanctum, "Erennius" },
    { 60748, kMapEmeraldSanctum, "Solnius" },
};

constexpr HireUnlockBoss kHireUnlockBossesTempleAq[] = {
    { 15263, kMapTempleOfAhnQiraj, "The Prophet Skeram" },
    { 15516, kMapTempleOfAhnQiraj, "Battleguard Sartura" },
    { 15510, kMapTempleOfAhnQiraj, "Fankriss the Unyielding" },
    { 15509, kMapTempleOfAhnQiraj, "Princess Huhuran" },
    { 15275, kMapTempleOfAhnQiraj, "Emperor Vek'nilash" },
    { 15276, kMapTempleOfAhnQiraj, "Emperor Vek'lor" },
    { 15727, kMapTempleOfAhnQiraj, "C'Thun" },
};

constexpr HireUnlockBoss kHireUnlockBossesNaxxramas[] = {
    { 15956, kMapNaxxramas, "Anub'Rekhan" },
    { 15953, kMapNaxxramas, "Grand Widow Faerlina" },
    { 15952, kMapNaxxramas, "Maexxna" },
    { 15954, kMapNaxxramas, "Noth the Plaguebringer" },
    { 15936, kMapNaxxramas, "Heigan the Unclean" },
    { 16011, kMapNaxxramas, "Loatheb" },
    { 16061, kMapNaxxramas, "Instructor Razuvious" },
    { 16060, kMapNaxxramas, "Gothik the Harvester" },
    { 16062, kMapNaxxramas, "The Four Horsemen" },
    { 16028, kMapNaxxramas, "Patchwerk" },
    { 15931, kMapNaxxramas, "Grobbulus" },
    { 15932, kMapNaxxramas, "Gluth" },
    { 15928, kMapNaxxramas, "Thaddius" },
    { 15989, kMapNaxxramas, "Sapphiron" },
    { 15990, kMapNaxxramas, "Kel'Thuzad" },
};

constexpr HireUnlockBoss kHireUnlockBossesTowerKara[] = {
    { 61939, kMapTowerOfKarazhan, "Keeper Gnarlmoon" },
    { 61946, kMapTowerOfKarazhan, "Ley-Watcher Incantagos" },
    { 61951, kMapTowerOfKarazhan, "Anomalus" },
    { 61958, kMapTowerOfKarazhan, "Echo of Medivh" },
    { 59967, kMapTowerOfKarazhan, "King's Council" },
    { 59981, kMapTowerOfKarazhan, "Sanv Tas'dal" },
    { 59991, kMapTowerOfKarazhan, "Kruul" },
    { 59961, kMapTowerOfKarazhan, "Rupturan the Broken" },
};

#define TB_HIRE_UNLOCK_N(arr) static_cast<uint8_t>(sizeof(arr) / sizeof((arr)[0]))

// Counter credit entries 95110-95121 (never spawned, credit-only rows in
// the migration, next to the world-buff 95100/95101).
constexpr HireUnlockCounter kHireUnlockCounters[] = {
    { 1, 0, 95110, "Dungeon lords slain", kHireUnlockBossesDungeons, TB_HIRE_UNLOCK_N(kHireUnlockBossesDungeons) },
    { 2, 0, 95111, "Zul'Gurub bosses slain", kHireUnlockBossesZulGurub, TB_HIRE_UNLOCK_N(kHireUnlockBossesZulGurub) },
    { 2, 1, 95112, "Ruins of Ahn'Qiraj bosses slain", kHireUnlockBossesRuinsAq, TB_HIRE_UNLOCK_N(kHireUnlockBossesRuinsAq) },
    { 2, 2, 95113, "Lower Karazhan Halls bosses slain", kHireUnlockBossesLowerKara, TB_HIRE_UNLOCK_N(kHireUnlockBossesLowerKara) },
    { 3, 0, 95114, "Molten Core bosses slain", kHireUnlockBossesMoltenCore, TB_HIRE_UNLOCK_N(kHireUnlockBossesMoltenCore) },
    { 3, 1, 95115, "Onyxia slain", kHireUnlockBossesOnyxia, TB_HIRE_UNLOCK_N(kHireUnlockBossesOnyxia) },
    { 3, 2, 95116, "Timbermaw Hold bosses slain", kHireUnlockBossesTimbermaw, TB_HIRE_UNLOCK_N(kHireUnlockBossesTimbermaw) },
    { 4, 0, 95117, "Blackwing Lair bosses slain", kHireUnlockBossesBlackwing, TB_HIRE_UNLOCK_N(kHireUnlockBossesBlackwing) },
    { 4, 1, 95118, "Emerald Sanctum bosses slain", kHireUnlockBossesEmerald, TB_HIRE_UNLOCK_N(kHireUnlockBossesEmerald) },
    { 5, 0, 95119, "Temple of Ahn'Qiraj bosses slain", kHireUnlockBossesTempleAq, TB_HIRE_UNLOCK_N(kHireUnlockBossesTempleAq) },
    { 6, 0, 95120, "Naxxramas bosses slain", kHireUnlockBossesNaxxramas, TB_HIRE_UNLOCK_N(kHireUnlockBossesNaxxramas) },
    { 7, 0, 95121, "Tower of Karazhan bosses slain", kHireUnlockBossesTowerKara, TB_HIRE_UNLOCK_N(kHireUnlockBossesTowerKara) },
};

#undef TB_HIRE_UNLOCK_N

// Encounter key for a killed creature: grouped members map to the group's
// key entry (Four Horsemen -> 16062); any other entry maps to itself.
inline uint32_t HireUnlockEncounterKey(uint32_t creatureEntry)
{
    for (uint32_t member : kHireUnlockFourHorsemen)
        if (member == creatureEntry)
            return kHireUnlockFourHorsemen[0];
    if (creatureEntry == kHireUnlockThekalPhaseTwo)
        return 14599;
    return creatureEntry;
}

inline bool IsHireUnlockGroupedEncounter(uint32_t encounterKey)
{
    return encounterKey == kHireUnlockFourHorsemen[0];
}

// The counter a killed creature feeds, or nullptr. Looks up by encounter
// key, so all four Horsemen resolve to the Naxxramas counter.
inline HireUnlockCounter const* HireUnlockCounterForKill(uint32_t creatureEntry, HireUnlockBoss const** bossOut = nullptr)
{
    uint32_t key = HireUnlockEncounterKey(creatureEntry);
    for (HireUnlockCounter const& counter : kHireUnlockCounters)
        for (uint8_t i = 0; i < counter.bossCount; ++i)
            if (counter.bosses[i].entry == key)
            {
                if (bossOut)
                    *bossOut = &counter.bosses[i];
                return &counter;
            }
    return nullptr;
}

// Highest tier the player may hire at: the start tier, or the highest
// rewarded unlock quest if higher. rewardedMask bit (tier - 1) = the tier's
// quest is rewarded (on this character, or on any character of the account
// when HireGearUnlockAccountWide). Unlocks need not be contiguous in the
// mask (AllQuestsAvailable), so the highest set bit wins.
inline uint8_t HighestUnlockedHireTier(uint8_t startTier, uint32_t rewardedMask)
{
    uint8_t best = startTier > kHireGearTierMax ? kHireGearTierMax : startTier;
    for (uint8_t tier = kHireGearTierMax; tier >= 1; --tier)
        if (rewardedMask & (1u << (tier - 1)))
        {
            if (tier > best)
                best = tier;
            break;
        }
    return best;
}

// Hire fee multiplier per tier 0..7 (HireGearTierCostMultipliers).
constexpr float kHireTierCostMultDefault[kHireGearTierMax + 1] = { 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 6.0f };

// Parses "1,1.5,2,3,4,5,6,6" into out[0..7]. Any malformed, negative or
// short list falls back to the defaults as a whole (returns false), so a
// typo never makes a tier free or partially configured.
inline bool ParseHireTierCostMultipliers(std::string const& text, float (&out)[kHireGearTierMax + 1])
{
    for (uint8_t i = 0; i <= kHireGearTierMax; ++i)
        out[i] = kHireTierCostMultDefault[i];
    float parsed[kHireGearTierMax + 1];
    size_t count = 0;
    size_t pos = 0;
    while (pos <= text.size())
    {
        size_t comma = text.find(',', pos);
        if (comma == std::string::npos)
            comma = text.size();
        std::string token = text.substr(pos, comma - pos);
        size_t first = token.find_first_not_of(" \t");
        size_t last = token.find_last_not_of(" \t");
        if (first == std::string::npos || count > kHireGearTierMax)
            return false;
        token = token.substr(first, last - first + 1);
        char* end = nullptr;
        float value = std::strtof(token.c_str(), &end);
        if (!end || *end != '\0' || !(value >= 0.0f))
            return false;
        parsed[count++] = value;
        pos = comma + 1;
    }
    if (count != kHireGearTierMax + 1)
        return false;
    for (uint8_t i = 0; i <= kHireGearTierMax; ++i)
        out[i] = parsed[i];
    return true;
}

// Hire fee at a gear tier: the base quote (HireCost::ForNextHire) times the
// tier multiplier, rounded to whole copper.
inline uint32_t HireCostAtTier(uint32_t baseCopper, uint8_t tier, float const (&mults)[kHireGearTierMax + 1])
{
    if (tier > kHireGearTierMax)
        tier = kHireGearTierMax;
    double cost = static_cast<double>(baseCopper) * mults[tier] + 0.5;
    if (cost > 4294967295.0)
        return 4294967295u;
    return static_cast<uint32_t>(cost);
}

} // namespace TortoiseBots
