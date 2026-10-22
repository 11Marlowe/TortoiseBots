// Standalone regression test for the off-target taunt policy (issue #1156):
// which engaged hostile the tank taunts when a mob sits on a non-tank group
// member — healer victims first, closest first, tanks' own mobs and out-of-
// range / player attackers skipped, held CC marks refused unless the owner's
// taunt exception applies.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_attacker_of_non_tank_policy.cpp -o /tmp/test_attacker_of_non_tank_policy
//   /tmp/test_attacker_of_non_tank_policy

#include "../ai/playerbot/strategy/values/AttackerOfNonTankPolicy.h"

#include <cstdio>
#include <cstdlib>

using namespace ai;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

static AttackerOfNonTankCandidate Mob(float distance, bool healerVictim)
{
    AttackerOfNonTankCandidate c;
    c.victimIsGroupPlayer = true;
    c.victimIsHealer = healerVictim;
    c.inRange = true;
    c.distance = distance;
    return c;
}

int main()
{
    // The core case from the issue: a mob that has arrived and swings at the
    // healer while the tank fights its own target.
    {
        std::vector<AttackerOfNonTankCandidate> candidates = { Mob(12.0f, true) };
        CHECK(AttackerOfNonTankPick(candidates) == 0);
    }

    // The healer's attacker outranks a mob on a DPS, regardless of distance.
    {
        std::vector<AttackerOfNonTankCandidate> candidates = { Mob(5.0f, false), Mob(25.0f, true) };
        CHECK(AttackerOfNonTankPick(candidates) == 1);
    }
    {
        std::vector<AttackerOfNonTankCandidate> candidates = { Mob(5.0f, true), Mob(1.0f, false) };
        CHECK(AttackerOfNonTankPick(candidates) == 0);
    }

    // Two mobs on the healer: the closer one.
    {
        std::vector<AttackerOfNonTankCandidate> candidates = { Mob(20.0f, true), Mob(8.0f, true) };
        CHECK(AttackerOfNonTankPick(candidates) == 1);
    }

    // No non-tank victim needs help: the tank holding its mob, nothing else.
    {
        AttackerOfNonTankCandidate tankHeld = Mob(3.0f, false);
        tankHeld.victimIsTank = true;
        std::vector<AttackerOfNonTankCandidate> candidates = { tankHeld };
        CHECK(AttackerOfNonTankPick(candidates) == 1);
    }

    // Out of taunt range: not picked, the tank never chases the healer's mob.
    {
        AttackerOfNonTankCandidate far = Mob(45.0f, true);
        far.inRange = false;
        std::vector<AttackerOfNonTankCandidate> candidates = { far };
        CHECK(AttackerOfNonTankPick(candidates) == 1);
    }

    // A held CC mark is refused like every other row: the CC bot's job.
    {
        AttackerOfNonTankCandidate mark = Mob(10.0f, true);
        mark.heldCcMark = true;
        std::vector<AttackerOfNonTankCandidate> candidates = { mark };
        CHECK(AttackerOfNonTankPick(candidates) == 1);
    }

    // Owner exception: a held mark that is not crowd-controlled yet and is
    // hitting the healer may be taunted (CcHeldMarkTauntAllowed). The caller
    // folds that into heldMarkTauntAllowed; a DPS victim never qualifies.
    {
        AttackerOfNonTankCandidate mark = Mob(10.0f, true);
        mark.heldCcMark = true;
        mark.heldMarkTauntAllowed = true;
        std::vector<AttackerOfNonTankCandidate> candidates = { mark };
        CHECK(AttackerOfNonTankPick(candidates) == 0);
    }

    // A held mark on a DPS victim stays refused even with the taunt flag set
    // (the caller can only set it for healer victims; the policy double-checks
    // the combination the caller may not produce).
    {
        AttackerOfNonTankCandidate mark = Mob(10.0f, false);
        mark.heldCcMark = true;
        mark.heldMarkTauntAllowed = true;
        std::vector<AttackerOfNonTankCandidate> candidates = { mark };
        CHECK(AttackerOfNonTankPick(candidates) == 1);
    }

    // Empty scan / nothing fighting: no pick.
    {
        std::vector<AttackerOfNonTankCandidate> candidates;
        CHECK(AttackerOfNonTankPick(candidates) == candidates.size());
        CHECK(AttackerOfNonTankPick(candidates) == 0);
    }

    std::printf("ok: %d checks\n", checks);
    return 0;
}
