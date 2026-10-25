#pragma once

// Item-level cap for *generated* gear (owner rule, Oct 2026): a wandering pool
// bot was being seeded with raid-level items it could never have earned (a
// level-60 priest with item level 96, others at 71/68), because the source-tier
// gate is fail-open for items without a known source and RandomGearMaxLevel is
// a flat 100. Generated gear is now bounded by the bot's own level:
//   - pool bots:        level + AiPlayerbot.RandomGearMaxItemLevelAboveLevel
//   - standard hires:   level + AiPlayerbot.HireGearMaxItemLevelAboveLevel
// and never above the global AiPlayerbot.RandomGearMaxLevel.
//
// Gear a bot earns itself (loot, quest rewards, vendor and auction buys) is
// organic and is not capped here; neither are the curated tier 1-7 hire kits
// (tortoise_bots_hire_gear) or the owned-bot autogear cap.
//
// Pure and core-free, so tools/test_gear_ilvl_cap_policy.cpp includes it directly.

#include <cstdint>

namespace TortoiseBots
{
// The highest item level a generated item may have for a bot of `botLevel`.
// `aboveLevel` is the configured allowance over the bot's level; 0 means "no
// above-level part" and keeps the global ceiling alone (the default for the
// paths that carry their own rule: owned-bot autogear, the master-sync gear
// score path, the curated hire tiers). The result never exceeds `globalMax`,
// so a lowered RandomGearMaxLevel still wins.
inline std::uint32_t GearItemLevelCap(std::uint32_t botLevel, std::uint32_t aboveLevel, std::uint32_t globalMax)
{
    if (aboveLevel == 0)
        return globalMax;

    std::uint32_t const cap = botLevel + aboveLevel;
    return cap < globalMax ? cap : globalMax;
}
}
