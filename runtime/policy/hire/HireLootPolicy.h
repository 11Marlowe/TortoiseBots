#pragma once

// Hired companions loot by default. AiPlayerbot.HireLoot = 0 keeps a hire off
// every corpse and makes it pass on every roll. Pool bots and owned alts are
// untouched. Header-only; exercised standalone in tools/test_hire_loot_policy.cpp.

namespace TortoiseBots
{
inline bool HireMayLoot(bool isHired, bool hireLoot)
{
    return !isHired || hireLoot;
}

} // namespace TortoiseBots
