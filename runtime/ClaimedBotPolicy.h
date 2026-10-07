#pragma once

// Pure policy for claimed guild bots (Issue #489).
//
// Header-only and free of core/database types on purpose: the eligibility,
// gear progression gates, and vendor rules are exercised standalone in
// tools/test_claimed_bot_policy.cpp.

#include <cstdint>

namespace TortoiseBots
{

// WoW standard item qualities:
// 0: Poor, 1: Common, 2: Uncommon, 3: Rare, 4: Epic, 5: Legendary
constexpr uint32_t kQualityPoor = 0;
constexpr uint32_t kQualityCommon = 1;
constexpr uint32_t kQualityUncommon = 2;
constexpr uint32_t kQualityRare = 3;
constexpr uint32_t kQualityEpic = 4;
constexpr uint32_t kQualityLegendary = 5;

constexpr uint32_t kMaxVanillaPlayerLevel = 60;

// Eligibility gate: May a bot be claimed into a guild?
inline bool MayClaimBot(bool isUnguilded, bool isPoolBot, bool isAlreadyClaimed, bool isOpposingFaction)
{
    if (!isUnguilded)
        return false;
    if (!isPoolBot)
        return false;
    if (isAlreadyClaimed)
        return false;
    if (isOpposingFaction)
        return false;
    return true;
}

// Gear upgrade gate: May this bot automatically equip an upgrade it found?
// Regular pool bots can equip upgrades. Claimed bots below level 60 can equip
// upgrades as they level. Claimed bots at level 60 are hard-locked: only their
// player master can distribute and equip gear on them.
inline bool MayClaimedBotSelfEquip(uint32_t botLevel, bool isClaimed)
{
    if (!isClaimed)
        return true;
    if (botLevel >= kMaxVanillaPlayerLevel)
        return false;
    return true;
}

// Bag vendor / liquidation gate: May this bot automatically sell an item to a vendor?
// Regular pool bots sell vendor trash according to their normal routines.
// Claimed bots at level 60 never auto-vendor items on their own.
// Claimed bots below level 60 can auto-vendor poor/common/uncommon leveling trash,
// but Rare (blue) and Epic (purple) items are preserved in bags.
inline bool MayClaimedBotAutoVendor(uint32_t botLevel, uint32_t itemQuality, bool isClaimed)
{
    if (!isClaimed)
        return true;
    if (botLevel >= kMaxVanillaPlayerLevel)
        return false;
    return itemQuality < kQualityRare;
}

// Reset exemption: Claimed bots are protected alt-bots and must never be deleted
// or re-randomized by the managed random bot pool reset.
inline bool IsClaimedBotExemptFromReset(bool isClaimed)
{
    return isClaimed;
}

} // namespace TortoiseBots
