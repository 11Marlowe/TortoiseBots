// Standalone regression test for the shaman tank kit policy
// (runtime/ShamanTankKitPolicy.h, issue #1125): a shaman with the tank
// role installs "tank shaman" plus the shared tank-framework pieces and the
// shaman pieces that still apply — never "dps assist", "behind", or threat
// backoff. The bare "tank" alias (LFT handoff) counts as covering the slot.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_shaman_tank_kit_policy.cpp -o /tmp/test_shaman_tank_kit
//   /tmp/test_shaman_tank_kit

#include "../runtime/ShamanTankKitPolicy.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

static bool Contains(std::vector<std::string> const& kit, char const* name)
{
    return std::find(kit.begin(), kit.end(), name) != kit.end();
}

static void TestCombatKit()
{
    std::vector<std::string> kit = ShamanTankCombatKit();
    // Spec + tank framework pieces...
    CHECK(Contains(kit, "tank shaman"));
    CHECK(Contains(kit, "tank assist"));
    CHECK(Contains(kit, "pull"));
    CHECK(Contains(kit, "pull back"));
    CHECK(Contains(kit, "close"));
    CHECK(Contains(kit, "tank face"));
    // ...plus the shared shaman pieces that still apply...
    CHECK(Contains(kit, "totems"));
    CHECK(Contains(kit, "buff"));
    CHECK(Contains(kit, "cure"));
    CHECK(Contains(kit, "boost"));
    // ...the "aoe" placeholder, without which the tank's Fire Nova / Magma
    // rows ("aoe tank shaman *") are never installed...
    CHECK(Contains(kit, "aoe"));
    // ...and never the DPS-kit pieces that fight the tank role.
    CHECK(!Contains(kit, "dps assist"));
    CHECK(!Contains(kit, "behind"));
    CHECK(!Contains(kit, "threat"));
}

static void TestNonCombatKit()
{
    std::vector<std::string> kit = ShamanTankNonCombatKit();
    CHECK(Contains(kit, "tank shaman"));
    CHECK(Contains(kit, "tank assist"));
    CHECK(Contains(kit, "totems"));
    CHECK(Contains(kit, "buff"));
    CHECK(Contains(kit, "cure"));
    CHECK(Contains(kit, "boost"));
    CHECK(!Contains(kit, "dps assist"));
}

static void TestCoverage()
{
    CHECK(ShamanTankKitCoversTank(ShamanTankCombatKit()));
    CHECK(ShamanTankKitCoversTank({ "tank" }));  // LFT handoff alias
    CHECK(ShamanTankKitCoversTank({ "tank shaman" }));
    CHECK(!ShamanTankKitCoversTank({ "enhancement", "dps assist" }));
    CHECK(!ShamanTankKitCoversTank({}));
}

static void TestPremadeIdentity()
{
    CHECK(kShamanTankPremadeClass == 7);
    CHECK(kShamanTankPremadeSpec == 3);
}

int main()
{
    TestCombatKit();
    std::printf("  [PASS] combat kit: tank shaman + framework, no dps assist/behind\n");
    TestNonCombatKit();
    std::printf("  [PASS] noncombat kit: tank shaman + shared shaman pieces\n");
    TestCoverage();
    std::printf("  [PASS] kit coverage: spec and bare tank alias cover the slot\n");
    TestPremadeIdentity();
    std::printf("  [PASS] premade identity: class 7 spec 3\n");
    std::printf("shaman tank kit policy tests passed (%d checks).\n", checks);
    return 0;
}
