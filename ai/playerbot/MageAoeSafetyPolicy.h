#pragma once

// Pure decision rules for the mage point-blank spells (issue #1164):
// Arcane Explosion existed only as an arcane-strategy row on the
// current-target "enemy too close for spell" trigger, so frost/fire never
// burst a pack standing on the tank; and the emergency Frost Nova row keyed
// on the current target's distance, which in a group is usually the tank's
// mob - the cooldown burned on a pack a tank already holds.
//
// mod-playerbots has no pack-on-tank logic to port (its Arcane Explosion
// lives only in the arcane rotation and its Frost Nova keys on "enemy is
// close", GenericMageStrategy.cpp:104 - the same current-target read); the
// rules below are new work in this repo's AoeTrigger family. Callers
// translate game state into plain inputs: the splash count and CC verdict
// come from AoeTrigger/AoeCountValue, the holder classification from each
// mob's swing victim (ai::IsTank for the tank test), exactly like
// BlinkBackTrigger's "only a mob that is attacking the mage" read.
// Unit-tested without the server in tools/test_mage_aoe_safety_policy.cpp.

namespace ai
{
    // Who a mob is currently held by, read from its melee victim. A mob
    // engaged but not yet swinging (pull in flight, chase) has no victim.
    enum class MobHolder
    {
        Tank,
        Bot,
        OtherPlayer,
        Nobody
    };

    inline MobHolder ClassifyMobHolder(bool hasVictim, bool victimIsTank, bool victimIsBot)
    {
        if (!hasVictim)
            return MobHolder::Nobody;
        if (victimIsTank)
            return MobHolder::Tank;
        if (victimIsBot)
            return MobHolder::Bot;
        return MobHolder::OtherPlayer;
    }

    // --- Arcane Explosion (frost/fire rows, issue #1164) ---

    struct ArcaneExplosionGate
    {
        int mobsInSplash;         // densest attacker cluster within AE range of the mage
        int minMobs;              // pack threshold, matches the aoe family (3)
        bool breakableCcInSplash; // AoeTrigger interlock verdict over the cluster
        bool safeSplash;          // every splash mob held by a tank or by the mage
    };

    // Arcane Explosion hits everything around the mage: cast it on a real
    // pack, never through a breakable CC, and only while nothing in the
    // splash is loose - a mob on another member or not yet held would ride
    // the splash threat off its holder.
    inline bool ShouldArcaneExplosion(ArcaneExplosionGate const& gate)
    {
        if (gate.mobsInSplash < gate.minMobs)
            return false;
        if (gate.breakableCcInSplash)
            return false;
        return gate.safeSplash;
    }

    // --- Frost Nova emergency row (issue #1164) ---

    // Nova roots everything within 10yd of the mage: spend the cooldown only
    // when a mob in reach is not sitting on a tank. A mob chewing on the
    // mage, on another member, or not yet held all qualify; the tank's held
    // pack does not.
    inline bool ShouldFrostNova(bool mobInNovaRange, MobHolder holder)
    {
        if (!mobInNovaRange)
            return false;
        return holder != MobHolder::Tank;
    }
} // namespace ai
