// Standalone policy test for the mount/dismount-loop fix (#1097,
// ai/playerbot/MountDismountPolicy.h). Runtime proof (headless GM observer,
// Oct 2026): each mount cast lands (spell_go + aura_applied) and the aura is
// removed 1-4 s later with the bot unmoved - by buff upkeep (Blessing of
// Wisdom re-cast on the dismount tick) or by the combat dismount rule on a
// dps pick the travelling bot never engages - then the next tick casts
// again. The motion gate (ca0af148, #553) stays: the fix is on the dismount
// side, in two rules plus a stable mount choice.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_mount_dismount_policy.cpp -o /tmp/test_mount_dismount_policy
//   /tmp/test_mount_dismount_policy

#include "../ai/playerbot/MountDismountPolicy.h"

#include <cstdio>
#include <cstdlib>

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

// (1) The loop shape: travelling bot, not in combat, dps pick nearby that
// fights nobody and no order on it. Must NOT dismount.
static void TestUnengagedPickKeepsMount()
{
    CHECK(!ai::CombatDismountAllowed(false, false, false));
}

// A real fight dismounts: bot in combat, or the target fights the bot, or
// an explicit attack/pull order names it.
static void TestRealFightDismounts()
{
    CHECK(ai::CombatDismountAllowed(true, false, false));
    CHECK(ai::CombatDismountAllowed(false, true, false));
    CHECK(ai::CombatDismountAllowed(false, false, true));
}

// (2) Out-of-combat upkeep waits while mounted and travelling; combat,
// and non-travelling bots, keep today's behavior.
static void TestMountedTravelDefersBuffs()
{
    CHECK(!ai::BuffCastAllowedWhileMounted(false, true));
    CHECK(ai::BuffCastAllowedWhileMounted(true, true));
    CHECK(ai::BuffCastAllowedWhileMounted(false, false));
}

// (3) Stable choice: the current mount wins ties, else the fastest.
static void TestStableMountChoice()
{
    CHECK(ai::StableMountSpell(13819, 13819, true) == 13819);
    CHECK(ai::StableMountSpell(13819, 46449, true) == 13819);
    CHECK(ai::StableMountSpell(13819, 46449, false) == 46449);
    CHECK(ai::StableMountSpell(0, 46449, false) == 46449);
    CHECK(ai::StableMountSpell(0, 0, false) == 0);
}

int main()
{
    std::printf("Starting mount-dismount policy tests...\n");
    TestUnengagedPickKeepsMount();
    std::printf("  [PASS] unengaged pick keeps mount\n");
    TestRealFightDismounts();
    std::printf("  [PASS] real fight dismounts\n");
    TestMountedTravelDefersBuffs();
    std::printf("  [PASS] mounted travel defers buffs\n");
    TestStableMountChoice();
    std::printf("  [PASS] stable mount choice\n");
    std::printf("All mount-dismount policy checks PASSED (%d assertions)!\n", checks);
    return 0;
}
