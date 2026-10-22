#pragma once

// Pure policy for the tank's "taunt the attacker of a non-tank" row
// (PLAN-SPELL-AUDIT F1, 2026-10-21). In 33 dungeon runs 31 of 53 priest
// deaths had a mob on the priest in the 5 s before and only 8 saw a Taunt:
// "lose aggro" reads only the tank's current target, and "taunt on snare
// target" only a mob still running (SnareTargetValue, which keeps that job).
// A mob that has arrived and swings at the healer was nobody's Taunt.
//
// Kept core-free so the standalone g++ policy test can include it; the
// caller (AttackerOfNonTankValue.cpp) fills one candidate per attacker of
// the group from the game state.

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ai
{
    // At most one Taunt per mob in this window (Taunt's own cooldown is 10 s
    // too; the per-mob memory keeps a second tank, a shorter talent cooldown
    // or a reset from Taunting the same mob back and forth).
    constexpr std::uint32_t TauntAttackerRepeatMs = 10000;

    // Challenging Shout instead of a single Taunt at this many eligible mobs
    // within its radius of the tank.
    constexpr int ChallengingShoutMinAttackers = 3;
    // Challenging Shout (1161) hits enemies within 10 yd of the warrior.
    constexpr float ChallengingShoutRadiusYd = 10.0f;

    // Threat a tank needs over the victim's to take a mob back by itself
    // (the core's aggro switch: 110 % in melee reach, 130 % outside).
    constexpr float TauntPullBackMelee = 1.1f;
    constexpr float TauntPullBackRanged = 1.3f;

    struct TauntAttackerCandidate
    {
        bool victimInGroup = false;       // its victim is a player of the tank's group (not a pet)
        bool victimIsSelf = false;        // ... is the tank itself
        bool victimIsTank = false;        // ... is another tank
        bool victimIsHealer = false;
        float victimHealthPct = 100.0f;
        bool attackerIsPlayer = false;    // an enemy player: no threat list, Taunt does nothing
        bool crowdControlled = false;     // breakable CC, fear or banish on it now
        bool evading = false;
        bool heldCcMark = false;          // CcGuard::IsHeldCcMark
        bool heldMarkTauntAllowed = false;// CcHeldMarkTauntAllowed for the tank's Taunt
        float distance = 0.0f;            // from the tank (2d, as SnareTargetValue)
        bool inRange = false;             // within Taunt's own spell range: the row never moves the tank
        bool inSight = false;
        bool tankInMelee = false;         // the tank is in melee reach of it
        float tankThreat = 0.0f;          // the tank's threat on it
        float victimThreat = 0.0f;        // its victim's threat on it
        bool tauntedRecently = false;     // this tank Taunted it < TauntAttackerRepeatMs ago
    };

    // `now - last` in unsigned arithmetic, so a getMSTime wrap still counts.
    inline bool TauntedRecently(bool known, std::uint32_t lastMs, std::uint32_t nowMs)
    {
        return known && std::uint32_t(nowMs - lastMs) < TauntAttackerRepeatMs;
    }

    // The tank takes the mob back without a Taunt: its threat already reaches
    // the core's switch margin over the victim's. Merely being top of the
    // list is not enough (between 100 % and 110 % the mob stays on the
    // victim; Taunt moves it). A victim with no threat on the mob (scripted
    // or fixated attack) counts as "unaided" too: Taunt would not hold it.
    inline bool TankWinsBackUnaided(TauntAttackerCandidate const& c)
    {
        if (c.victimThreat <= 0.0f)
            return true;
        float const margin = c.tankInMelee ? TauntPullBackMelee : TauntPullBackRanged;
        return c.tankThreat >= c.victimThreat * margin;
    }

    // Everything but where the mob stands from the tank and the repeat window.
    inline bool TauntAttackerOnNonTank(TauntAttackerCandidate const& c)
    {
        if (!c.victimInGroup || c.victimIsSelf || c.victimIsTank)
            return false;
        if (c.attackerIsPlayer || c.crowdControlled || c.evading)
            return false;
        if (c.heldCcMark && !c.heldMarkTauntAllowed)
            return false;
        return !TankWinsBackUnaided(c);
    }

    inline bool TauntAttackerEligible(TauntAttackerCandidate const& c)
    {
        return TauntAttackerOnNonTank(c) && c.inRange && c.inSight && !c.tauntedRecently;
    }

    // Healer victims first, then the lowest victim health, then the nearest mob.
    inline bool TauntAttackerBetter(TauntAttackerCandidate const& a, TauntAttackerCandidate const& b)
    {
        if (a.victimIsHealer != b.victimIsHealer)
            return a.victimIsHealer;
        if (a.victimHealthPct != b.victimHealthPct)
            return a.victimHealthPct < b.victimHealthPct;
        return a.distance < b.distance;
    }

    // Index of the mob to Taunt, -1 for none.
    inline int TauntAttackerPick(std::vector<TauntAttackerCandidate> const& list)
    {
        int best = -1;
        for (std::size_t i = 0; i < list.size(); ++i)
        {
            if (!TauntAttackerEligible(list[i]))
                continue;
            if (best < 0 || TauntAttackerBetter(list[i], list[static_cast<std::size_t>(best)]))
                best = static_cast<int>(i);
        }
        return best;
    }

    // Mobs on non-tanks within the shout's radius of the tank (Taunt range,
    // sight and the single-Taunt repeat window do not apply to an area).
    inline int TauntAttackerShoutCount(std::vector<TauntAttackerCandidate> const& list, float radius)
    {
        int n = 0;
        for (TauntAttackerCandidate const& c : list)
            if (TauntAttackerOnNonTank(c) && c.distance <= radius)
                ++n;
        return n;
    }

    inline bool ChallengingShoutOnAttackers(std::vector<TauntAttackerCandidate> const& list, float radius)
    {
        return TauntAttackerShoutCount(list, radius) >= ChallengingShoutMinAttackers;
    }

    // The row runs for a tank in a group whose Taunt can be cast now
    // (stance, cooldown and rage as the core sees them).
    inline bool TauntAttackerGate(bool isTank, bool inGroup, bool spellReady)
    {
        return isTank && inGroup && spellReady;
    }
}
