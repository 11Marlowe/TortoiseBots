#include "../ai/playerbot/HunterKitePolicy.h"

#include <cstdint>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using ai::kHunterKiteMinAttackers;
using ai::kHunterKiteEngageYd;
using ai::ShouldHunterKite;
using ai::ShouldHuntersMarkTarget;

int main()
{
    std::cout << "Starting TortoiseBots hunter kite tests...\n";

    CHECK(kHunterKiteMinAttackers == 2);
    // Dead zone edge (8 yd) plus margin: the kite holds until the hunter is
    // clear enough to shoot, and one step out of the dead zone is enough.
    CHECK(kHunterKiteEngageYd > 8.0f);

    // Kite: 2+ mobs on the hunter, ranged kit usable.
    {
        // Two mobs glued on the hunter: clip and step out.
        CHECK(ShouldHunterKite(2, true, 3.0f));
        // Three at the dead zone edge: still kite.
        CHECK(ShouldHunterKite(3, true, 8.0f));
        // Past the engage edge the ranged kit shoots instead.
        CHECK(!ShouldHunterKite(2, true, 10.0f));
        CHECK(!ShouldHunterKite(2, true, 25.0f));
        // A single mob on the hunter is the dead-zone/melee chain case.
        CHECK(!ShouldHunterKite(1, true, 3.0f));
        // No mobs on the hunter: nothing to answer.
        CHECK(!ShouldHunterKite(0, true, 3.0f));
        // Without a usable ranged kit stepping away gains nothing.
        CHECK(!ShouldHunterKite(3, false, 3.0f));
        std::cout << "  [PASS] kites 2+ attackers with a ranged kit, inside the engage edge\n";
    }

    // Hunter's Mark follows the tank's target.
    {
        // Tank in the group: mark only what the tank is fighting.
        CHECK(!ShouldHuntersMarkTarget(true, false));
        CHECK(ShouldHuntersMarkTarget(true, true));
        // No living tank (solo or tank down): mark anything.
        CHECK(ShouldHuntersMarkTarget(false, false));
        CHECK(ShouldHuntersMarkTarget(false, true));
        std::cout << "  [PASS] mark is tank-target gated with a tank, free without one\n";
    }

    std::cout << "All TortoiseBots hunter kite tests passed!\n";
    return 0;
}
