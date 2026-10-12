#pragma once

// Issue #490 PR 2: hiring a companion at a gear tier. Pure, core-free choice
// policy for the hire wizard's "Choose equipment" step and `.bot hire <tier>`:
// tier display names, tier-word parsing, step visibility, gossip sender
// encoding and the unlock-chain PrevQuestId. Unit-tested standalone
// (tools/test_hire_tier_request_policy.cpp). Prices and unlock resolution
// live in the contract header (HireUnlockPolicy.h).

#include <cstdint>
#include <string>

#include "HireUnlockPolicy.h"

namespace TortoiseBots
{

// Gossip sender ids for the new wizard steps. The spec step packs its whole
// selection into 32 bits, so the tier rides in the sender instead: 520+tier
// picks the tier, 530+tier confirms the hire at that tier, 540 steps back.
// All above the world-buff branch (505-509), disjoint from it.
constexpr uint32_t kHireTierStepSenderBase = 520;
constexpr uint32_t kHireTierConfirmSenderBase = 530;
constexpr uint32_t kHireTierBackSender = 540;

// Display names, tiers 0..7. nullptr above the max.
inline char const* HireTierName(uint8_t tier)
{
    switch (tier)
    {
        case 0: return "Standard";
        case 1: return "Pre-raid";
        case 2: return "Zul'Gurub";
        case 3: return "Molten Core";
        case 4: return "Blackwing Lair";
        case 5: return "Ahn'Qiraj";
        case 6: return "Naxxramas";
        case 7: return "Karazhan";
        default: return nullptr;
    }
}

// Menu kit label ("Standard kit", "Molten Core kit", ...). Empty above max.
inline std::string HireTierKitName(uint8_t tier)
{
    char const* name = HireTierName(tier);
    if (!name)
        return std::string();
    return std::string(name) + " kit";
}

// `.bot hire` tier word -> tier 0..7. word is lower case (HandleHire lowercases
// tokens, like HireSpecIndexForWord). Accepts t<N>/tier<N> and the tier names
// with short aliases (mc, bwl, zg, aq, naxx, kara, ...). Bare numbers are NOT
// tier words: "3" is a class id to parseClass. False when the word names no
// tier; every accepted word is a no-op in today's command, so old input is
// unaffected.
inline bool ParseHireTierWord(std::string const& word, uint8_t& tierOut)
{
    if (word.size() == 2 && word[0] == 't' && word[1] >= '0' && word[1] <= '7')
    {
        tierOut = static_cast<uint8_t>(word[1] - '0');
        return true;
    }
    if (word.compare(0, 4, "tier") == 0 && word.size() == 5 &&
        word[4] >= '0' && word[4] <= '7')
    {
        tierOut = static_cast<uint8_t>(word[4] - '0');
        return true;
    }
    if (word == "standard")
        tierOut = 0;
    // No bare "pre": a warrior's "pre" means Protection (HireSpecIndexForWord
    // matches the path prefix), so the tier needs "pre-raid"/"preraid".
    else if (word == "pre-raid" || word == "preraid")
        tierOut = 1;
    else if (word == "zul'gurub" || word == "zulgurub" || word == "zg")
        tierOut = 2;
    else if (word == "molten core" || word == "moltencore" || word == "molten" ||
        word == "mc" || word == "core")
        tierOut = 3;
    else if (word == "blackwing lair" || word == "blackwinglair" ||
        word == "blackwing" || word == "bwl" || word == "lair")
        tierOut = 4;
    else if (word == "ahn'qiraj" || word == "ahnqiraj" || word == "qiraj" ||
        word == "aq" || word == "aq40")
        tierOut = 5;
    else if (word == "naxxramas" || word == "naxx")
        tierOut = 6;
    else if (word == "karazhan" || word == "kara")
        tierOut = 7;
    else
        return false;
    return true;
}

// The "Choose equipment" step is skipped entirely when tiers are disabled,
// the hire is below 60, or the player unlocks nothing above tier 0, so those
// players see the exact old wizard.
inline bool HireTierStepVisible(bool tiersEnabled, uint32_t hireLevel, uint8_t highestTier)
{
    return tiersEnabled && hireLevel == kHireGearHireLevel && highestTier >= 1 &&
        highestTier <= kHireGearTierMax;
}

inline uint32_t HireTierStepSender(uint8_t tier)
{
    return kHireTierStepSenderBase + tier;
}

inline bool DecodeHireTierStepSender(uint32_t sender, uint8_t& tierOut)
{
    if (sender < kHireTierStepSenderBase ||
        sender > kHireTierStepSenderBase + kHireGearTierMax)
        return false;
    tierOut = static_cast<uint8_t>(sender - kHireTierStepSenderBase);
    return true;
}

inline uint32_t HireTierConfirmSender(uint8_t tier)
{
    return kHireTierConfirmSenderBase + tier;
}

inline bool DecodeHireTierConfirmSender(uint32_t sender, uint8_t& tierOut)
{
    if (sender < kHireTierConfirmSenderBase ||
        sender > kHireTierConfirmSenderBase + kHireGearTierMax)
        return false;
    tierOut = static_cast<uint8_t>(sender - kHireTierConfirmSenderBase);
    return true;
}

// PrevQuestId for an unlock quest: the chain (tier N needs tier N-1's quest,
// tier 1 none), or 0 for every quest when all quests are available at once.
// Written into quest_template at startup, live after the next restart.
inline int32_t HireTierChainPrevQuest(uint8_t tier, bool allQuestsAvailable)
{
    if (allQuestsAvailable || tier <= 1 || tier > kHireGearTierMax)
        return 0;
    return static_cast<int32_t>(HireUnlockQuestForTier(tier - 1));
}

} // namespace TortoiseBots
