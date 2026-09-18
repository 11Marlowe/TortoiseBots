#include "HireCost.h"

namespace TortoiseBots
{
namespace HireCost
{

uint32_t ForHireIndex(CostConfig const& config, uint32_t hireIndex, uint32_t level)
{
    if (hireIndex == 0)
        hireIndex = 1;
    if (level == 0)
        level = 1;
    if (level > 60)
        level = 60;

    float mult = 1.0f;
    uint32_t base = config.baseCopper;
    if (hireIndex >= 5)
        base = config.raidFlatCopper;
    else if (hireIndex == 2)
        mult = config.mult2;
    else if (hireIndex == 3)
        mult = config.mult3;
    else if (hireIndex >= 4)
        mult = config.mult4;

    double scaled = static_cast<double>(base) * static_cast<double>(mult) *
        static_cast<double>(level) / 60.0;
    if (scaled < 0.0)
        return 0;
    if (scaled > 100000000.0)
        return 100000000;
    return static_cast<uint32_t>(scaled);
}

uint32_t ForNextHire(CostConfig const& config, uint32_t ownedCount, uint32_t level)
{
    return ForHireIndex(config, ownedCount + 1, level);
}

} // namespace HireCost
} // namespace TortoiseBots
