// Standalone regression test for issue #1125: AiPlayerbot.ShamanTankEnabled.
// With the flag off, the 7.3 shaman tank must be absent from every entry point
// the policy helpers gate:
//  - the recruiter spec menu row (HireSpecOffered),
//  - any hire that resolves to the tank role for a shaman (ShamanTankRoleOffered),
//  - the random pool roll weight of the 7.3 premade (PremadePathRollWeight).
// Every other row, class and role must be unaffected by the flag.
//
// Build and run (from the repo root):
//   g++ -std=c++17 -Wall -Wextra tools/test_shaman_tank_flag_policy.cpp -o /tmp/test_shaman_tank_flag
//   /tmp/test_shaman_tank_flag

#include "../runtime/HireSpecPolicy.h"
#include "../runtime/policy/class/ShamanTankKitPolicy.h"

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

static void TestRecruiterRow()
{
    CHECK(HireSpecOffered(kHireClassShaman, 3, 10, true) == true);
    CHECK(HireSpecOffered(kHireClassShaman, 3, 60, true) == true);
    CHECK(HireSpecOffered(kHireClassShaman, 3, 60, false) == false);
    CHECK(HireSpecOffered(kHireClassShaman, 3, 9, false) == false);
    CHECK(HireSpecOffered(kHireClassShaman, 2, 60, false) == true);
    CHECK(HireSpecOffered(kHireClassShaman, 0, 1, false) == true);
    CHECK(HireSpecOffered(kHireClassWarrior, 0, 1, false) == true);
}

static void TestTankRoleHire()
{
    CHECK(ShamanTankRoleOffered(kHireClassShaman, kHireRoleTank, true) == true);
    CHECK(ShamanTankRoleOffered(kHireClassShaman, kHireRoleTank, false) == false);
    CHECK(ShamanTankRoleOffered(kHireClassShaman, kHireRoleDps, false) == true);
    CHECK(ShamanTankRoleOffered(kHireClassShaman, kHireRoleHealer, false) == true);
    CHECK(ShamanTankRoleOffered(kHireClassWarrior, kHireRoleTank, false) == true);
    CHECK(ShamanTankRoleOffered(kHireClassDruid, kHireRoleTank, false) == true);
}

static void TestPoolRollWeight()
{
    constexpr uint8_t kShaman = kShamanTankPremadeClass;
    constexpr int kTankPath = kShamanTankPremadeSpec;
    CHECK(PremadePathRollWeight(kShaman, kTankPath, 40, true) == 40);
    CHECK(PremadePathRollWeight(kShaman, kTankPath, 40, false) == 0);
    CHECK(PremadePathRollWeight(kShaman, kTankPath - 1, 40, false) == 40);
    CHECK(PremadePathRollWeight(kShaman + 1, kTankPath, 40, false) == 40);
    CHECK(PremadePathRollWeight(kShaman, kTankPath, -5, true) == 0);
    CHECK(PremadePathRollWeight(kShaman, kTankPath - 1, -5, false) == 0);
    // Owned bots roll evenly: weight 1 per path, the tank still 0 with the flag off.
    CHECK(PremadePathRollWeight(kShaman, kTankPath, 1, true) == 1);
    CHECK(PremadePathRollWeight(kShaman, kTankPath, 1, false) == 0);
    CHECK(PremadePathRollWeight(kShaman, kTankPath - 1, 1, false) == 1);
}

int main()
{
    TestRecruiterRow();
    TestTankRoleHire();
    TestPoolRollWeight();
    std::printf("shaman tank flag policy: %d checks passed\n", checks);
    return 0;
}
