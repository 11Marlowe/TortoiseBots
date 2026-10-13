#pragma once

// Pure policy for the issue #490 (bug part) hire gear-source guard.
//
// Header-only and free of core/database types on purpose: the automation
// paths (equip audit, quest-reward pick) read the cached source flags, the
// rule only decides. Exercised standalone in
// tools/test_hire_source_guard_policy.cpp.
//
// Scope (orchestrator decision): hired bots only. Pool bots may wear PvP
// rewards they earn in the organic world; owned alts are geared by player.

#include <cstdint>

namespace TortoiseBots
{

// Source-flag bits (mirror ItemSourceFlag in ai/playerbot/RandomItemMgr.h;
// duplicated so this header stays core-free).
constexpr uint8_t kSourceFlagRep = 1 << 0;
constexpr uint8_t kSourceFlagPvp = 1 << 1;

// Automation gate: may a hired bot auto-equip / auto-pick this item?
// Each flag obeys its own knob: a PvP-flagged item needs
// RandomGearAllowPvP, a rep-flagged item needs RandomGearAllowReputation.
// Non-hires always pass (pool/owned behaviour untouched).
inline bool MayHireAutoTakeSource(uint8_t sourceFlags, bool isHired,
    bool allowPvP, bool allowReputation)
{
    if (!isHired)
        return true;
    if ((sourceFlags & kSourceFlagPvp) && !allowPvP)
        return false;
    if ((sourceFlags & kSourceFlagRep) && !allowReputation)
        return false;
    return true;
}

// Quest-choice filter: drop gated choice indexes when at least one ungated
// choice exists; all-gated (or empty) falls back to the plain ranking so
// the quest still completes. Returns the surviving indexes.
template <typename IdVec>
inline IdVec FilterHireGatedRewardChoices(IdVec const& bestIds,
    uint8_t const* choiceFlags, uint32_t choiceCount, bool isHired,
    bool allowPvP, bool allowReputation)
{
    IdVec kept;
    for (uint32_t index : bestIds)
    {
        if (index >= choiceCount)
            continue;
        if (isHired && !MayHireAutoTakeSource(choiceFlags[index], true, allowPvP, allowReputation))
            continue;
        kept.push_back(index);
    }
    if (isHired && kept.empty() && !bestIds.empty())
        return bestIds;
    return kept;
}

}
