#pragma once

// Pure decision rules for the mage point-blank spells: which splashes
// Arcane Explosion may burst. Callers translate game state into plain
// inputs (splash count and the density/CC verdict from the AoE scan,
// holder from each mob's swing victim), so the rules stay testable without
// a server.

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

    // A splash is safe to burst only when every mob in it is on a tank or
    // already on the mage: a mob on another member would ride the splash
    // threat off its holder.
    inline bool IsSplashSafeForExplosion(MobHolder holder)
    {
        return holder == MobHolder::Tank || holder == MobHolder::Bot;
    }

    struct ArcaneExplosionGate
    {
        int mobsInSplash;      // densest attacker cluster within AE range of the mage
        int minMobs;           // pack threshold
        bool aoeInterlockSafe; // density + breakable-CC verdict of the shared aoe trigger
        bool safeSplash;       // every splash mob IsSplashSafeForExplosion
    };

    inline bool ShouldArcaneExplosion(ArcaneExplosionGate const& gate)
    {
        if (gate.mobsInSplash < gate.minMobs)
            return false;
        if (!gate.aoeInterlockSafe)
            return false;
        return gate.safeSplash;
    }
} // namespace ai
