#pragma once

// Pure decision rule for the direct-heal pick across the low and medium
// health bands (critical line to almost-full): pick by the pace of the
// damage, then by the healer's own mana. No core or module includes:
// callers in strategy/priest translate game state into plain inputs, so the
// rule stays testable in tools/test_heal_pace_policy.cpp.
//
// The pace signal is the "incoming damage" value the preheal gate reads:
// ONE melee swing of every attacker currently in melee reach of the target,
// summed. Melee autos only - spells, ranged attacks and damage-over-time
// effects are not counted. Read as percent of the target's max health it is
// the hurt the next swing window brings; scaling it over the slow cast's
// duration approximates where the bar sits when the heal would land. Mob
// swing timers (~2 s) make the per-cast estimate pessimistic, so the gate
// errs toward the fast heal - the safe side of the choice.

namespace ai
{
    enum class HealPaceChoice
    {
        Flash,      // flash heal
        Efficient   // greater heal / heal / lesser heal, renew for light deficits
    };

    struct HealPaceState
    {
        float healthPercent;          // target's CURRENT percent (band gate input)
        float incomingDamagePct;      // one swing per in-reach attacker, as % of max health
        float slowCastSeconds;        // cast time of the efficient direct heal
        float healerManaPercent;      // the healer's own mana
        float mediumManaPercent;      // comfort line below which mana is saved
        float lowHealthPercent;       // band line between low and medium
        float criticalHealthPercent;  // danger line
    };

    // Flash heal when the target would land in the critical band before a
    // slow cast would finish and the healer can afford it. When the
    // healer's mana is below the comfort line the efficient heals win, and
    // flash heal is kept only for a target predicted to reach the critical
    // line even then.
    inline HealPaceChoice PickPacedHeal(HealPaceState const& state)
    {
        // Defensive: callers gate on the band above the critical line; a
        // target already inside it takes the fast heal regardless.
        if (state.healthPercent <= state.criticalHealthPercent)
            return HealPaceChoice::Flash;

        float const slowPredicted = state.healthPercent - state.incomingDamagePct * state.slowCastSeconds;
        if (slowPredicted <= state.criticalHealthPercent)
            return HealPaceChoice::Flash;

        if (state.healerManaPercent < state.mediumManaPercent)
            return HealPaceChoice::Efficient;

        // The damage outruns the slow cast: the bar would leave the target's
        // band before the heal lands (out of the medium band below the low
        // line; out of the low band only into the critical one).
        float const bandFloor = state.healthPercent >= state.lowHealthPercent
            ? state.lowHealthPercent
            : state.criticalHealthPercent;
        return slowPredicted < bandFloor ? HealPaceChoice::Flash : HealPaceChoice::Efficient;
    }
}
