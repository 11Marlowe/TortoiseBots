// Standalone test for the shaman tank detection predicate
// (runtime/ShamanTankDetectionPolicy.h): all three conditions (shield,
// Rockbiter, key tank talent) are required; each missing one reads non-tank;
// forced role wins either way; a detected shaman tank co-tanks.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_shaman_tank_detection_policy.cpp -o /tmp/test_shaman_tank_detection
//   /tmp/test_shaman_tank_detection

#include "../runtime/ShamanTankDetectionPolicy.h"

#include <cstdio>
#include <cstdlib>

#define CHECK(cond) do { \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

using TortoiseBots::IsShamanTank;
using TortoiseBots::ShamanTankDetectionInputs;

static ShamanTankDetectionInputs FullTank()
{
    ShamanTankDetectionInputs inputs;
    inputs.shieldEquipped = true;
    inputs.rockbiterOnMainHand = true;
    inputs.totemicAlignment = true;
    return inputs;
}

int main()
{
    // All three conditions (Totemic Alignment path): tank.
    CHECK(IsShamanTank(FullTank()));
    // Spirit Armor path instead of Totemic Alignment: tank.
    {
        ShamanTankDetectionInputs inputs = FullTank();
        inputs.totemicAlignment = false;
        inputs.spiritArmor = true;
        CHECK(IsShamanTank(inputs));
    }
    // No shield: not tank.
    {
        ShamanTankDetectionInputs inputs = FullTank();
        inputs.shieldEquipped = false;
        CHECK(!IsShamanTank(inputs));
    }
    // No Rockbiter (windfury/flametongue upkeep): not tank.
    {
        ShamanTankDetectionInputs inputs = FullTank();
        inputs.rockbiterOnMainHand = false;
        CHECK(!IsShamanTank(inputs));
    }
    // Gear without either key talent (plain Enhancement): not tank.
    {
        ShamanTankDetectionInputs inputs = FullTank();
        inputs.totemicAlignment = false;
        CHECK(!IsShamanTank(inputs));
    }
    // Forced tank wins without any gear or talent.
    {
        ShamanTankDetectionInputs inputs;
        inputs.forcedTank = true;
        CHECK(IsShamanTank(inputs));
    }
    // Forced non-tank (ordered DPS) wins over full tank gear.
    {
        ShamanTankDetectionInputs inputs = FullTank();
        inputs.forcedNonTank = true;
        CHECK(!IsShamanTank(inputs));
    }
    // Co-tank: another tank present neither demotes the shaman ...
    {
        ShamanTankDetectionInputs inputs = FullTank();
        inputs.otherTankPresent = true;
        CHECK(IsShamanTank(inputs));
    }
    // ... nor does its own absence matter.
    {
        ShamanTankDetectionInputs inputs = FullTank();
        inputs.otherTankPresent = false;
        CHECK(IsShamanTank(inputs));
    }

    std::puts("All shaman tank detection checks PASSED!");
    return 0;
}
