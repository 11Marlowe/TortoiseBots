#include "../ai/playerbot/HealPacePolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " #x "\n"; std::exit(1); } } while (0)

using ai::HealPaceChoice;
using ai::HealPaceState;
using ai::PickMediumBandHeal;

static HealPaceState Make(float health, float incomingPct, float mana)
{
    // Live spell constants and band defaults: Heal/Greater Heal 3 s,
    // Flash Heal 1.5 s; bands 20/50/70, mana comfort 40.
    return HealPaceState{health, incomingPct, 3.0f, 1.5f, mana, 40.0f, 50.0f, 20.0f};
}

int main()
{
    // Slow or stable damage: the efficient heals, at any mana level.
    CHECK(PickMediumBandHeal(Make(60.0f, 0.0f, 80.0f)) == HealPaceChoice::Efficient);
    CHECK(PickMediumBandHeal(Make(60.0f, 2.0f, 80.0f)) == HealPaceChoice::Efficient);
    CHECK(PickMediumBandHeal(Make(60.0f, 2.0f, 30.0f)) == HealPaceChoice::Efficient);
    // Exactly at the band bottom after the predicted loss is still efficient.
    CHECK(PickMediumBandHeal(Make(65.0f, 5.0f, 80.0f)) == HealPaceChoice::Efficient);

    // Fast damage + plenty of mana: the pace rule as briefed - the bar would
    // leave the medium band before the slow cast lands.
    CHECK(PickMediumBandHeal(Make(60.0f, 10.0f, 80.0f)) == HealPaceChoice::Flash);
    CHECK(PickMediumBandHeal(Make(70.0f, 7.0f, 100.0f)) == HealPaceChoice::Flash);

    // Fast damage + low mana: mana wins, the efficient heals keep the bar up.
    CHECK(PickMediumBandHeal(Make(60.0f, 10.0f, 39.0f)) == HealPaceChoice::Efficient);
    CHECK(PickMediumBandHeal(Make(70.0f, 10.0f, 10.0f)) == HealPaceChoice::Efficient);

    // About to die: flash heal even on fumes - predicted to reach the
    // critical line before the slow cast would land.
    CHECK(PickMediumBandHeal(Make(45.0f, 10.0f, 30.0f)) == HealPaceChoice::Flash);
    CHECK(PickMediumBandHeal(Make(50.0f, 10.0f, 39.0f)) == HealPaceChoice::Flash);

    // Damage so heavy even the flash cast arrives too late: flash anyway.
    CHECK(PickMediumBandHeal(Make(60.0f, 30.0f, 10.0f)) == HealPaceChoice::Flash);

    // Critical health: always flash, unchanged.
    CHECK(PickMediumBandHeal(Make(20.0f, 0.0f, 100.0f)) == HealPaceChoice::Flash);
    CHECK(PickMediumBandHeal(Make(15.0f, 0.0f, 5.0f)) == HealPaceChoice::Flash);

    // Mana exactly at the comfort line is still comfortable.
    CHECK(PickMediumBandHeal(Make(60.0f, 10.0f, 40.0f)) == HealPaceChoice::Flash);

    std::cout << "heal pace policy: OK\n";
}
