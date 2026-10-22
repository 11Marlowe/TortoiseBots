// Standalone regression test for the pet engage hold + threat-lead cower
// (runtime/PetEngagePolicy.h): in a bot-led party (no real
// player master) the pet stays parked through the tank's ~2 s lead window
// before it opens, and the hunter cowers on the pet's first threat lead over
// the tank instead of waiting for the pet to hold aggro.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_pet_engage_policy.cpp -o /tmp/test_pet_engage
//   /tmp/test_pet_engage

#include "../runtime/PetEngagePolicy.h"

#include <cstddef>
#include <cstdint>
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

static PetEngageInputs EngageInputs(bool grouped, bool tank, bool master, bool inCombat, uint32_t elapsed)
{
    PetEngageInputs in;
    in.grouped = grouped;
    in.tankInGroup = tank;
    in.playerMaster = master;
    in.inCombat = inCombat;
    in.combatElapsedSeconds = elapsed;
    return in;
}

static PetThreatLeadInputs CowerInputs(bool aggro, float petThreat, float tankThreat)
{
    PetThreatLeadInputs in;
    in.petHasAggro = aggro;
    in.petThreat = petThreat;
    in.tankThreat = tankThreat;
    return in;
}

// The measured bug: bot-led party (no master), live tank, combat just
// started — the pet must stay parked through the lead window.
static void TestMasterlessPartyHoldsDuringTankLeadWindow()
{
    CHECK(ShouldHoldPetForTankLead(EngageInputs(true, true, false, true, 0)));
    CHECK(ShouldHoldPetForTankLead(EngageInputs(true, true, false, true, 1)));
}

// Once the window has run, the pet engages.
static void TestMasterlessPartyReleasesAfterWindow()
{
    CHECK(!ShouldHoldPetForTankLead(EngageInputs(true, true, false, true, kPetTankLeadSeconds)));
    CHECK(!ShouldHoldPetForTankLead(EngageInputs(true, true, false, true, kPetTankLeadSeconds + 30)));
}

// Pre-combat the pet attack row does not fire, and the policy must not hold
// on a stale clock either.
static void TestNoHoldOutsideCombat()
{
    CHECK(!ShouldHoldPetForTankLead(EngageInputs(true, true, false, false, 0)));
}

// Solo pets and tankless groups are the fallback holders: they engage at
// once, with or without a master.
static void TestNoHoldWithoutTank()
{
    CHECK(!ShouldHoldPetForTankLead(EngageInputs(false, false, false, true, 0)));
    CHECK(!ShouldHoldPetForTankLead(EngageInputs(true, false, false, true, 0)));
}

// A real player master keeps the "wait for attack" clock: no extra hold.
static void TestNoHoldWithPlayerMaster()
{
    CHECK(!ShouldHoldPetForTankLead(EngageInputs(true, true, true, true, 0)));
}

// A hostile player target never waits, same rule as the wait clock.
static void TestNoHoldAgainstEnemyPlayer()
{
    PetEngageInputs in = EngageInputs(true, true, false, true, 0);
    in.enemyPlayer = true;
    CHECK(!ShouldHoldPetForTankLead(in));
}

// The lead window is exactly 2 s.
static void TestLeadWindowIsTwoSeconds()
{
    CHECK(kPetTankLeadSeconds == 2);
}

// The pet already holds aggro — cower now.
static void TestCowerOnAggro()
{
    CHECK(ShouldPetCower(CowerInputs(true, 0.0f, 0.0f)));
    CHECK(ShouldPetCower(CowerInputs(true, 0.0f, 500.0f)));
}

// The new half: the pet leads the tank's threat before the mob turns.
static void TestCowerOnThreatLead()
{
    CHECK(ShouldPetCower(CowerInputs(false, 300.0f, 250.0f)));
    // Tank engaged but no lead yet (or nothing on the pet): no cower.
    CHECK(!ShouldPetCower(CowerInputs(false, 100.0f, 250.0f)));
    CHECK(!ShouldPetCower(CowerInputs(false, 0.0f, 0.0f)));
    CHECK(!ShouldPetCower(CowerInputs(false, 250.0f, 250.0f)));
}

// Tankless group (or dead tank): the pet is the holder, it never cowers —
// except when it already holds aggro, where cowering is ordered anyway.
static void TestNoThreatLeadCowerWithoutTank()
{
    CHECK(!ShouldPetCower(CowerInputs(false, 300.0f, -1.0f)));
    CHECK(ShouldPetCower(CowerInputs(true, 300.0f, -1.0f)));
}

int main()
{
    TestMasterlessPartyHoldsDuringTankLeadWindow();
    TestMasterlessPartyReleasesAfterWindow();
    TestNoHoldOutsideCombat();
    TestNoHoldWithoutTank();
    TestNoHoldWithPlayerMaster();
    TestNoHoldAgainstEnemyPlayer();
    TestLeadWindowIsTwoSeconds();
    TestCowerOnAggro();
    TestCowerOnThreatLead();
    TestNoThreatLeadCowerWithoutTank();

    std::printf("ok: %d checks\n", checks);
    return 0;
}
