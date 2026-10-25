// Standalone regression test for the mage point-blank spell gates:
// frost/fire mages burst a pack inside Arcane Explosion range only when the
// splash is safe, and Frost Nova is left for mobs attacking the mage itself
// (checked in the trigger, not here).
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_mage_aoe_safety_policy.cpp -o /tmp/test_mage_aoe_safety
//   /tmp/test_mage_aoe_safety

#include "../ai/playerbot/policy/class/MageAoeSafetyPolicy.h"

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
    gate.aoeInterlockSafe = true;
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

    // --- splash safety ---
    CHECK(IsSplashSafeForExplosion(MobHolder::Tank));
    CHECK(IsSplashSafeForExplosion(MobHolder::Bot));
    // A mob on another member, or not yet held, would ride the splash threat
    // away from its holder.
    CHECK(!IsSplashSafeForExplosion(MobHolder::OtherPlayer));
    CHECK(!IsSplashSafeForExplosion(MobHolder::Nobody));

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
        gate.aoeInterlockSafe = false;
        CHECK(!ShouldArcaneExplosion(gate));
    }

    // A mob loose on another member: no burst.
    {
        ArcaneExplosionGate gate = Base();
        gate.safeSplash = false;
        CHECK(!ShouldArcaneExplosion(gate));
    }

    std::printf("mage aoe safety policy: %d checks passed\n", checks);
    return 0;
}
