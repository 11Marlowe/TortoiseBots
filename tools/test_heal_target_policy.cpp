// Standalone regression test for the alive gates of "party member to heal"
// (ai/playerbot/HealTargetPolicy.h, issue #1146): a dead pet still standing
// next to its master - and a friendly corpse reachable through the selection
// or rpg-target branches - used to be picked as a heal target and the healer
// cast into the body while hurt players went unhealed.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_heal_target_policy.cpp -o /tmp/test_heal_target
//   /tmp/test_heal_target

#include "../ai/playerbot/HealTargetPolicy.h"

#include <cstdio>
#include <cstdlib>

using ai::HealablePet;
using ai::HealableTarget;

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

// Issue #1146 follow-up: a friendly corpse at 0% health passes the
// below-full-health test of the selection and rpg-target branches, so the
// alive check is the only thing keeping the healer off the body.
static void TestDeadTargetIsNotHealable()
{
    CHECK(!HealableTarget(false));
    CHECK(HealableTarget(true));
}

int main()
{
    std::printf("Starting TortoiseBots heal target policy tests...\n");
    TestDeadPetIsNotHealable();
    TestAlivePetIsHealable();
    TestMiniPetIsNeverHealable();
    TestDeadTargetIsNotHealable();
    std::printf("  [PASS] %d checks\n", checks);
    return 0;
}
