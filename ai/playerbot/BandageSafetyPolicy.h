#pragma once

// Pure decision rules for the First Aid bandage gate (issue #1168): a bot
// started a bandage with a hostile already in melee reach. The channel
// breaks on the mob's next swing, so the bot paid the bandage's cooldown
// (Recently Bandaged) and often died anyway (audit run r7-19z25: hunter at
// 25% hp, dead two seconds after the cast started).
//
// The attacker-count and damage-over-time gates predate this file and only
// see mobs that already threaten the bot. A hostile standing within melee
// reach without threatening it yet - a tank-held mob parked on the bot, a
// chaser mid-run - slipped through, so the gate order below is the whole
// refusal list the action applies, in the order it asks.
//
// No core includes: the action translates the world into flags, so
// tools/test_bandage_safety_policy.cpp can test the rules.

#include <cstdint>

namespace ai
{
    // Verdict of the bandage gate. Order matters only for diagnostics: the
    // first refusing gate is the one reported.
    enum BandageGate : uint32_t
    {
        BANDAGE_OK = 0,
        BANDAGE_RECENTLY_BANDAGED = 1,
        BANDAGE_ATTACKER_ON_BOT = 2,
        BANDAGE_PERIODIC_DAMAGE = 3,
        BANDAGE_HOSTILE_IN_MELEE_REACH = 4,
    };

    // Bandage only with no attacker on the bot, no damage over time on it,
    // and no hostile within melee reach of it.
    inline BandageGate BandageGateCheck(bool recentlyBandaged, bool hasMyAttacker,
        bool hasPeriodicDamage, bool hasHostileInMeleeReach)
    {
        if (recentlyBandaged)
            return BANDAGE_RECENTLY_BANDAGED;
        if (hasMyAttacker)
            return BANDAGE_ATTACKER_ON_BOT;
        if (hasPeriodicDamage)
            return BANDAGE_PERIODIC_DAMAGE;
        if (hasHostileInMeleeReach)
            return BANDAGE_HOSTILE_IN_MELEE_REACH;
        return BANDAGE_OK;
    }
}
