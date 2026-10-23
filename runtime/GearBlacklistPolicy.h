#pragma once

// Owner-configured gear exclusion list (`AiPlayerbot.RandomGearBlacklist`):
// a blocked item id is never handed out by any auto-gear path. The blacklist
// wins over the whitelist, and a bot already wearing a blocked item treats the
// slot as empty on the next gear pass; a real player's own gear is never
// touched.
//
// Pure policy: no core/engine headers, unit-tested standalone
// (tools/test_gear_blacklist_policy.cpp).

#include <cstddef>
#include <cstdint>
#include <list>

namespace TortoiseBots
{

inline bool GearBlacklistBlocks(std::list<uint32_t> const& blacklist, uint32_t itemId)
{
    for (uint32_t blocked : blacklist)
        if (blocked == itemId)
            return true;
    return false;
}

// The whitelist seeds an item regardless of the scoring rules, unless the
// blacklist holds the same id.
inline bool GearWhitelistApplies(std::list<uint32_t> const& blacklist, bool whitelisted, uint32_t itemId)
{
    if (!whitelisted)
        return false;
    return !GearBlacklistBlocks(blacklist, itemId);
}

// A bot already wearing a blocked item re-gears the slot: the worn piece
// counts as empty, so it is replaced, or removed when nothing passes.
// A real player's worn gear is never treated as empty.
inline bool GearWornCountsAsEmpty(bool isRealPlayer, std::list<uint32_t> const& blacklist, uint32_t wornItemId)
{
    if (isRealPlayer)
        return false;
    return GearBlacklistBlocks(blacklist, wornItemId);
}

} // namespace TortoiseBots
