#pragma once

// Pure decision rule for the medium health band (50-70): pick the heal by
// the pace of the damage, then by the healer's own mana. No core or module
// includes: callers in strategy/priest translate game state into plain
// inputs, so the rule stays testable in tools/test_heal_pace_policy.cpp.
//
// Pace signal: the same "incoming damage" value the preheal gate uses
// (attacks in reach on the target), read as percent of the target's max
// health. Mobs swing slower than once a second, so per-second pressure is
// overestimated a little - the gate errs toward the fast heal, which is
// the safe side of the choice.

namespace ai
{
    enum class HealPaceChoice
    {
        Flash,      // flash heal
        Efficient   // greater heal / heal / lesser heal, renew for light deficits
    };

    struct HealPaceState
    {
        float healthPercent;          // target's current percent
        float incomingDamagePct;      // per-second pressure as % of the target's max health
        float slowCastSeconds;        // cast time of the efficient direct heal
        float fastCastSeconds;        // cast time of flash heal
        float healerManaPercent;      // the healer's own mana
        float mediumManaPercent;      // comfort line below which mana is saved
        float lowHealthPercent;       // bottom of the medium band
        float criticalHealthPercent;  // danger line
    };

    // Flash heal when the target would land in danger before a slow cast
    // would finish and the healer can afford it. When the healer's mana is
    // below the comfort line the efficient heals win, and flash heal is
    // kept only for a target predicted to reach the critical line before
    // the slow cast lands (or already inside it).
    inline HealPaceChoice PickMediumBandHeal(HealPaceState const& state)
    {
        if (state.healthPercent <= state.criticalHealthPercent)
            return HealPaceChoice::Flash;

        float const fastPredicted = state.healthPercent - state.incomingDamagePct * state.fastCastSeconds;
        float const slowPredicted = state.healthPercent - state.incomingDamagePct * state.slowCastSeconds;
        if (fastPredicted <= state.criticalHealthPercent || slowPredicted <= state.criticalHealthPercent)
            return HealPaceChoice::Flash;

        if (state.healerManaPercent < state.mediumManaPercent)
            return HealPaceChoice::Efficient;

        // The damage outruns the slow cast: the bar would leave the medium
        // band before the heal lands.
        return slowPredicted < state.lowHealthPercent ? HealPaceChoice::Flash : HealPaceChoice::Efficient;
    }
}
