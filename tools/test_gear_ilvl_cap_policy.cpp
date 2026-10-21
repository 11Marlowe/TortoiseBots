// Standalone test for the generated-gear item-level cap: pool bots and
// standard hires cap at their own level plus the configured allowance, the
// global RandomGearMaxLevel always wins when it is lower, and 0 disables the
// above-level part (owned-bot autogear, master sync, curated hire tiers).
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_gear_ilvl_cap_policy.cpp -o /tmp/test_gear_ilvl_cap_policy
//   /tmp/test_gear_ilvl_cap_policy

#include "../runtime/GearItemLevelCapPolicy.h"

#include <cstdio>
#include <cstdlib>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

int main()
{
    // Pool bots (AiPlayerbot.RandomGearMaxItemLevelAboveLevel = 1): the owner
    // rule "max ilvl about 60-61 for a level-60 pool bot".
    CHECK(GearItemLevelCap(60, 1, 100) == 61);
    CHECK(GearItemLevelCap(1, 1, 100) == 2);
    CHECK(GearItemLevelCap(10, 1, 100) == 11);
    CHECK(GearItemLevelCap(42, 1, 100) == 43);

    // Standard (tier 0) hires (AiPlayerbot.HireGearMaxItemLevelAboveLevel = 5):
    // a little above the pool rule, still below the tier-2 curated kit (71).
    CHECK(GearItemLevelCap(60, 5, 100) == 65);
    CHECK(GearItemLevelCap(20, 5, 100) == 25);
    CHECK(GearItemLevelCap(60, 5, 100) > GearItemLevelCap(60, 1, 100));

    // The global ceiling always wins when it is the lower bound.
    CHECK(GearItemLevelCap(60, 5, 62) == 62);
    CHECK(GearItemLevelCap(60, 1, 61) == 61);
    CHECK(GearItemLevelCap(1, 1, 1) == 1);

    // 0 keeps the global ceiling alone: owned-bot autogear with no explicit
    // ilvl cap, the master-sync path and the curated hire tiers.
    CHECK(GearItemLevelCap(60, 0, 100) == 100);
    CHECK(GearItemLevelCap(1, 0, 100) == 100);
    CHECK(GearItemLevelCap(60, 0, 0) == 0);

    // A cap that equals the global ceiling is not raised by the allowance.
    CHECK(GearItemLevelCap(60, 40, 100) == 100);

    std::printf("test_gear_ilvl_cap_policy: %d checks passed\n", checks);
    return 0;
}
