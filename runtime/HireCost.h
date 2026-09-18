#pragma once

#include <cstdint>

// Issue #192: single companion-hiring cost formula. Party hires 1-4 use the
// escalating multipliers; raid hires 5+ use the flat bulk rate. Both scale
// linearly with level (level/60), so a level 20 hire costs a third of level
// 60. Pure arithmetic on plain data: no player, DB, session, or config
// access, safe to call from gossip and chat paths alike and trivially
// unit-testable without core headers.
namespace TortoiseBots
{
namespace HireCost
{

struct CostConfig
{
    uint32_t baseCopper = 15000;
    float mult2 = 1.66f;
    float mult3 = 2.66f;
    float mult4 = 4.66f;
    uint32_t raidFlatCopper = 10000;
};

uint32_t ForHireIndex(CostConfig const& config, uint32_t hireIndex, uint32_t level);
uint32_t ForNextHire(CostConfig const& config, uint32_t ownedCount, uint32_t level);

} // namespace HireCost
} // namespace TortoiseBots
