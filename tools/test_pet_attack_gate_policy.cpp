// Standalone regression test for the pet attack hold and the early Cower
// (runtime/PetAttackGatePolicy.h, audit F5: pets were top of a mob's threat
// list in 11 % of mob-frames and hunters ordered pet Cower 1,011 times).
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra -Werror tools/test_pet_attack_gate_policy.cpp -o /tmp/test_pet_attack_gate
//   /tmp/test_pet_attack_gate

#include "../runtime/PetAttackGatePolicy.h"

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

// A grouped hunter with a live tank, a fresh pull: nobody has threat yet.
static PetAttackGateInputs FreshPull()
{
    PetAttackGateInputs in;
    in.grouped = true;
    in.liveTank = true;
    return in;
}

// The pet waits for the tank: a live tank with no threat on the mob yet.
static void TestHoldUntilTankHasThreat()
{
    PetAttackGateInputs in = FreshPull();
    CHECK(PetAttackHeldForTank(in));

    // A tank that is there but has no threat yet (0, not "no tank value"):
    // still held.
    in.tankThreat = 0.0f;
    CHECK(PetAttackHeldForTank(in));

    // The tank's first hit on a mob nobody else holds: released.
    in.tankThreat = 1.0f;
    CHECK(!PetAttackHeldForTank(in));
}

// A mob whose current victim is a living tank is held by the tank, whatever
// the threat numbers say yet; with several tanks any of them counts.
static void TestVictimIsAnyLivingTank()
{
    PetAttackGateInputs in = FreshPull();
    in.victimIsLivingTank = true;
    CHECK(!PetAttackHeldForTank(in));

    // The off-tank holds it while the main tank's threat is still token.
    in.tankThreat = 5.0f;
    in.victimThreat = 800.0f;
    CHECK(!PetAttackHeldForTank(in));
}

// Token threat does not release the pet while the mob hits someone else.
static void TestTokenThreatHolds()
{
    PetAttackGateInputs in = FreshPull();
    in.tankThreat = 5.0f;     // a shout, a heal, a body pull
    in.victimThreat = 800.0f; // the healer it is chewing on
    CHECK(PetAttackHeldForTank(in));
    in.tankInMelee = true;
    CHECK(PetAttackHeldForTank(in));
}

// The switch margins: 110 % in melee reach, 130 % outside.
static void TestSwitchMargins()
{
    PetAttackGateInputs in = FreshPull();
    in.victimThreat = 1000.0f;

    in.tankInMelee = true;
    in.tankThreat = 1099.0f;
    CHECK(PetAttackHeldForTank(in));
    in.tankThreat = 1100.0f;
    CHECK(!PetAttackHeldForTank(in));

    in.tankInMelee = false;
    in.tankThreat = 1299.0f;
    CHECK(PetAttackHeldForTank(in));
    in.tankThreat = 1300.0f;
    CHECK(!PetAttackHeldForTank(in));

    // Level with the victim is not a hold on the mob.
    in.tankInMelee = true;
    in.tankThreat = 1000.0f;
    CHECK(PetAttackHeldForTank(in));
}

// Never held where the pet is the tank, the mob is on the hunter, or the
// target is an enemy player.
static void TestNoHoldWhenPetIsTheFallback()
{
    PetAttackGateInputs in = FreshPull();
    in.liveTank = false; // no tank (or a dead one): the pet holds aggro
    CHECK(!PetAttackHeldForTank(in));

    in = FreshPull();
    in.grouped = false; // solo hunter: the pet is the tank
    CHECK(!PetAttackHeldForTank(in));

    // A mob already on the hunter: the pet helps.
    in = FreshPull();
    in.targetOnBot = true;
    CHECK(!PetAttackHeldForTank(in));

    // An enemy player has no threat list (the tank's threat reads 0 forever)
    // and a grouped battleground hunter has a "live tank" at any distance.
    in = FreshPull();
    in.targetIsPlayer = true;
    CHECK(!PetAttackHeldForTank(in));
}

// "No tank value" (-1) is not threat: the pet waits only while nothing holds
// the mob.
static void TestNegativeTankThreatStillHolds()
{
    PetAttackGateInputs in = FreshPull();
    in.tankThreat = -1.0f;
    CHECK(PetAttackHeldForTank(in));
    in.victimIsLivingTank = true;
    CHECK(!PetAttackHeldForTank(in));
}

// Early Cower: the pet is ahead of the tank before it takes the mob.
static void TestPetCowerOnLead()
{
    CHECK(PetCowerOnLead(120.0f, 100.0f, false));
    CHECK(!PetCowerOnLead(100.0f, 100.0f, false)); // level is not a lead
    CHECK(!PetCowerOnLead(80.0f, 100.0f, false));
    CHECK(!PetCowerOnLead(30.0f, 0.0f, false));    // tank has not engaged yet
    CHECK(!PetCowerOnLead(0.0f, 0.0f, false));
    CHECK(!PetCowerOnLead(30.0f, -1.0f, false));   // no tank value
    // The mob is on the hunter: no Cower.
    CHECK(!PetCowerOnLead(120.0f, 100.0f, true));
}

int main()
{
    TestHoldUntilTankHasThreat();
    TestVictimIsAnyLivingTank();
    TestTokenThreatHolds();
    TestSwitchMargins();
    TestNoHoldWhenPetIsTheFallback();
    TestNegativeTankThreatStillHolds();
    TestPetCowerOnLead();
    std::printf("test_pet_attack_gate_policy: %d checks passed\n", checks);
    return 0;
}
