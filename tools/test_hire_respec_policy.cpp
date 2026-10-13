// Standalone regression test for issue #490 PR 3: a respec gives a tier-kit
// hire the same tier's kit for its new spec. Guards the premade-path-name ->
// recruiter-menu-index mapping (HireSpecIndexForPathName), including the
// druid Bear/Cat split on the shared feral path via the forced role.
//
// Build and run (from the repo root):
//   g++ -std=c++17 -Wall -Wextra tools/test_hire_respec_policy.cpp -o /tmp/test_hire_respec
//   /tmp/test_hire_respec

#include "../runtime/HireRespecPolicy.h"

#include "../runtime/HireGearPolicy.h"
#include <string>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

// Each class's premade path names land on their own menu index.
static void TestRoundTrip()
{
    CHECK(HireSpecIndexForPathName(kHireClassWarrior, "arms", kHireRoleDps) == 1);
    CHECK(HireSpecIndexForPathName(kHireClassWarrior, "protection", kHireRoleTank) == 0);
    CHECK(HireSpecIndexForPathName(kHireClassPaladin, "retribution", kHireRoleDps) == 2);
    CHECK(HireSpecIndexForPathName(kHireClassPaladin, "holy", kHireRoleHealer) == 1);
    CHECK(HireSpecIndexForPathName(kHireClassPriest, "shadow", kHireRoleDps) == 2);
    CHECK(HireSpecIndexForPathName(kHireClassPriest, "discipline", kHireRoleHealer) == 1);
    CHECK(HireSpecIndexForPathName(kHireClassShaman, "enhancement", kHireRoleDps) == 2);
    CHECK(HireSpecIndexForPathName(kHireClassMage, "frost", kHireRoleDps) == 0);
    // Druid non-feral paths resolve by name.
    CHECK(HireSpecIndexForPathName(kHireClassDruid, "restoration", kHireRoleHealer) == 1);
    CHECK(HireSpecIndexForPathName(kHireClassDruid, "balance", kHireRoleDps) == 2);
}

// Feral is one premade path but two kits: the forced role picks Bear vs Cat.
static void TestDruidFeralSplit()
{
    CHECK(HireSpecIndexForPathName(kHireClassDruid, "feral", kHireRoleTank) == 0);
    CHECK(HireSpecIndexForPathName(kHireClassDruid, "feral", kHireRoleDps) == 3);
    CHECK(HireSpecIndexForPathName(kHireClassDruid, "feral cat", kHireRoleDps) == 3);
    CHECK(HireSpecIndexForPathName(kHireClassDruid, "feral bear", kHireRoleTank) == 0);
    // Resolved indices reach the right gear keys.
    CHECK(std::string(HireGearSpecKey(kHireClassDruid,
        HireSpecIndexForPathName(kHireClassDruid, "feral", kHireRoleTank))) == "bear");
    CHECK(std::string(HireGearSpecKey(kHireClassDruid,
        HireSpecIndexForPathName(kHireClassDruid, "feral", kHireRoleDps))) == "cat");
}

// Unknown class or name: -1, so the caller keeps the current kit.
static void TestUnknown()
{
    CHECK(HireSpecIndexForPathName(6, "feral", kHireRoleDps) == -1);
    CHECK(HireSpecIndexForPathName(kHireClassWarrior, "frost", kHireRoleDps) == -1);
    CHECK(HireSpecIndexForPathName(kHireClassWarrior, "", kHireRoleDps) == 1);
    CHECK(HireSpecIndexForPathName(kHireClassWarrior, nullptr, kHireRoleTank) == 0);
}

int main()
{
    TestRoundTrip();
    TestDruidFeralSplit();
    TestUnknown();
    std::printf("hire respec policy OK (%d checks)\n", checks);
    return 0;
}
