// Standalone regression test for the mage point-blank spell gates
// (issue #1164): frost/fire mages burst a pack inside Arcane Explosion
// range only when the splash is safe, and Frost Nova no longer burns its
// emergency cooldown on a pack a tank already holds.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_mage_aoe_safety_policy.cpp -o /tmp/test_mage_aoe_safety
//   /tmp/test_mage_aoe_safety

#include "../ai/playerbot/MageAoeSafetyPolicy.h"

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

static ArcaneExplosionGate Base()
{
    ArcaneExplosionGate gate;
    gate.mobsInSplash = 3;
    gate.minMobs = 3;
    gate.breakableCcInSplash = false;
    gate.safeSplash = true;
    return gate;
}

int main()
{
    // --- holder classification (from the mob's swing victim) ---
    CHECK(ClassifyMobHolder(false, false, false) == MobHolder::Nobody);
    CHECK(ClassifyMobHolder(true, true, false) == MobHolder::Tank);
    CHECK(ClassifyMobHolder(true, false, true) == MobHolder::Bot);
    CHECK(ClassifyMobHolder(true, false, false) == MobHolder::OtherPlayer);
    // A victim cannot be both tank and bot; tank wins the classification.
    CHECK(ClassifyMobHolder(true, true, true) == MobHolder::Tank);

    // --- Arcane Explosion gate ---
    CHECK(ShouldArcaneExplosion(Base()));

    // Two mobs is no pack: single-target logic owns the fight.
    {
        ArcaneExplosionGate gate = Base();
        gate.mobsInSplash = 2;
        CHECK(!ShouldArcaneExplosion(gate));
    }

    // Four mobs still fire.
    {
        ArcaneExplosionGate gate = Base();
        gate.mobsInSplash = 4;
        CHECK(ShouldArcaneExplosion(gate));
    }

    // A sheep, sap or trap inside the splash forbids the burst.
    {
        ArcaneExplosionGate gate = Base();
        gate.breakableCcInSplash = true;
        CHECK(!ShouldArcaneExplosion(gate));
    }

    // A mob loose on another member (or not yet held) rides the splash
    // threat away from its holder: no burst.
    {
        ArcaneExplosionGate gate = Base();
        gate.safeSplash = false;
        CHECK(!ShouldArcaneExplosion(gate));
    }

    // --- Frost Nova gate ---
    // The #1164 bug case: the mob in reach is the tank's held pack.
    CHECK(!ShouldFrostNova(true, MobHolder::Tank));
    // Not in reach: never.
    CHECK(!ShouldFrostNova(false, MobHolder::Tank));
    CHECK(!ShouldFrostNova(false, MobHolder::Bot));

    // A mob chewing on the mage, on another member, or not yet held is a
    // peel situation.
    CHECK(ShouldFrostNova(true, MobHolder::Bot));
    CHECK(ShouldFrostNova(true, MobHolder::OtherPlayer));
    CHECK(ShouldFrostNova(true, MobHolder::Nobody));

    std::printf("mage aoe safety policy: %d checks passed\n", checks);
    return 0;
}
