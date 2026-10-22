#include "../ai/playerbot/HealPacePolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " #x "\n"; std::exit(1); } } while (0)

using ai::HealPaceChoice;
using ai::HealPaceState;
using ai::PickPacedHeal;

// Production domain: the caller's band gate passes only targets with
// CURRENT health in [criticalHealth, mediumHealth) = [20, 70) by default.
// Live spell constants and band defaults: Heal/Greater Heal 3 s; bands
// 20/50/70, mana comfort 40.
static HealPaceState Make(float health, float incomingPct, float mana)
{
    return HealPaceState{health, incomingPct, 3.0f, mana, 40.0f, 50.0f, 20.0f};
}

int main()
{
    // Slow or stable damage: the efficient heals, at any mana level.
    CHECK(PickPacedHeal(Make(60.0f, 0.0f, 80.0f)) == HealPaceChoice::Efficient);
    CHECK(PickPacedHeal(Make(60.0f, 2.0f, 80.0f)) == HealPaceChoice::Efficient);
    CHECK(PickPacedHeal(Make(60.0f, 2.0f, 30.0f)) == HealPaceChoice::Efficient);
    CHECK(PickPacedHeal(Make(45.0f, 0.0f, 80.0f)) == HealPaceChoice::Efficient);
    CHECK(PickPacedHeal(Make(45.0f, 2.0f, 30.0f)) == HealPaceChoice::Efficient);
    // Exactly at the low band line after the predicted loss is still
    // efficient.
    CHECK(PickPacedHeal(Make(65.0f, 5.0f, 80.0f)) == HealPaceChoice::Efficient);

    // Fast damage + plenty of mana: the bar would leave the band before the
    // slow cast lands, in the medium band...
    CHECK(PickPacedHeal(Make(60.0f, 10.0f, 80.0f)) == HealPaceChoice::Flash);
    CHECK(PickPacedHeal(Make(70.0f, 7.0f, 100.0f)) == HealPaceChoice::Flash);
    // ...and in the low band, where the same pick keeps the fast heal
    // instead of dropping to the slow greater heal as the bar crosses 50.
    CHECK(PickPacedHeal(Make(48.0f, 10.0f, 80.0f)) == HealPaceChoice::Flash);
    CHECK(PickPacedHeal(Make(30.0f, 5.0f, 100.0f)) == HealPaceChoice::Flash);

    // Fast damage + low mana: mana wins, the efficient heals keep the bar up.
    CHECK(PickPacedHeal(Make(60.0f, 10.0f, 39.0f)) == HealPaceChoice::Efficient);
    CHECK(PickPacedHeal(Make(48.0f, 8.0f, 10.0f)) == HealPaceChoice::Efficient);

    // About to die: flash heal even on fumes - predicted to reach the
    // critical line before the slow cast would land.
    CHECK(PickPacedHeal(Make(45.0f, 10.0f, 30.0f)) == HealPaceChoice::Flash);
    CHECK(PickPacedHeal(Make(50.0f, 10.0f, 39.0f)) == HealPaceChoice::Flash);
    // Damage so heavy even a top-rank cast arrives too late: flash anyway.
    CHECK(PickPacedHeal(Make(60.0f, 20.0f, 10.0f)) == HealPaceChoice::Flash);

    // Band gate inputs, defensive only (the caller filters the band):
    // at the critical line the fast heal wins at any mana.
    CHECK(PickPacedHeal(Make(20.0f, 0.0f, 100.0f)) == HealPaceChoice::Flash);
    CHECK(PickPacedHeal(Make(20.0f, 0.0f, 5.0f)) == HealPaceChoice::Flash);

    // Mana exactly at the comfort line is still comfortable.
    CHECK(PickPacedHeal(Make(60.0f, 10.0f, 40.0f)) == HealPaceChoice::Flash);

    std::cout << "heal pace policy: OK\n";
}
