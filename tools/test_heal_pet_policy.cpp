// Standalone regression test for the pet gate of "party member to heal"
// (ai/playerbot/HealPetPolicy.h, issue #1146): a dead pet still standing next
// to its master used to be picked as a heal target and the healer cast into
// the corpse while hurt players went unhealed.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_heal_pet_policy.cpp -o /tmp/test_heal_pet
//   /tmp/test_heal_pet

#include "../ai/playerbot/HealPetPolicy.h"

#include <cstdio>
#include <cstdlib>

using ai::HealablePet;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

// Issue #1146: the dead combat pet must drop out of the heal list.
static void TestDeadPetIsNotHealable()
{
    CHECK(!HealablePet(false, false));
}

// A living, wounded combat pet is the branch's reason to exist.
static void TestAlivePetIsHealable()
{
    CHECK(HealablePet(false, true));
}

// A mini pet is never a heal target, alive or not (pre-existing rule).
static void TestMiniPetIsNeverHealable()
{
    CHECK(!HealablePet(true, true));
    CHECK(!HealablePet(true, false));
}

int main()
{
    std::printf("Starting TortoiseBots heal pet policy tests...\n");
    TestDeadPetIsNotHealable();
    TestAlivePetIsHealable();
    TestMiniPetIsNeverHealable();
    std::printf("  [PASS] %d checks\n", checks);
    return 0;
}
