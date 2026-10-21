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
    in.targetOnBot = false;
    in.targetOnTank = false;
    in.tankThreat = 0.0f;
    return in;
}

// The pet waits for the tank: a live tank with no threat on the mob yet.
static void TestHoldUntilTankHasThreat()
{
    PetAttackGateInputs in = FreshPull();
    CHECK(PetAttackHeldForTank(in));

    // The tank's first hit lands: threat > 0 releases the pet.
    in.tankThreat = 1.0f;
    CHECK(!PetAttackHeldForTank(in));

    // The mob already runs at the tank: released even before the threat reads.
    in = FreshPull();
    in.targetOnTank = true;
    CHECK(!PetAttackHeldForTank(in));
}

// Never held where the pet is the tank or the mob is already on the hunter.
static void TestNoHoldWhenPetIsTheFallback()
{
    PetAttackGateInputs in = FreshPull();
    in.liveTank = false; // no tank (or a dead one): the pet holds aggro
    CHECK(!PetAttackHeldForTank(in));

    in = FreshPull();
    in.grouped = false; // solo hunter: the pet is the tank
    CHECK(!PetAttackHeldForTank(in));

    // A mob already on the hunter: the pet helps, holding it would only
    // leave the hunter alone with the mob.
    in = FreshPull();
    in.targetOnBot = true;
    CHECK(!PetAttackHeldForTank(in));
}

// "No tank value" (-1) is not "tank threat 0": with a live tank flag and the
// threat value reading -1 (tank not safe to read) the pet still waits only
// while nothing holds the mob.
static void TestNegativeTankThreatStillHolds()
{
    PetAttackGateInputs in = FreshPull();
    in.tankThreat = -1.0f;
    CHECK(PetAttackHeldForTank(in));
    in.targetOnTank = true;
    CHECK(!PetAttackHeldForTank(in));
}

// Early Cower: the pet is ahead of the tank before it takes the mob.
static void TestPetThreatLeadsTank()
{
    CHECK(PetThreatLeadsTank(120.0f, 100.0f));
    CHECK(!PetThreatLeadsTank(100.0f, 100.0f));   // level is not a lead
    CHECK(!PetThreatLeadsTank(80.0f, 100.0f));
    CHECK(PetThreatLeadsTank(30.0f, 0.0f));       // tank has none yet
    CHECK(!PetThreatLeadsTank(0.0f, 0.0f));       // nobody has any
    CHECK(!PetThreatLeadsTank(30.0f, -1.0f));     // no tank value: not a lead
}

int main()
{
    TestHoldUntilTankHasThreat();
    TestNoHoldWhenPetIsTheFallback();
    TestNegativeTankThreatStillHolds();
    TestPetThreatLeadsTank();
    std::printf("test_pet_attack_gate_policy: %d checks passed\n", checks);
    return 0;
}
