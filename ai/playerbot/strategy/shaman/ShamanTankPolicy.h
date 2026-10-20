#pragma once

#include <cstdint>

// Pure policy for the tank shaman combat loop (issue #1125).
// Every function takes plain scalars so tools/test_shaman_tank_policy.cpp
// pins the exact logic without a server: triggers compose these gates with
// HasSpell / aura / attacker-count reads only.

namespace ai
{
    // Pull mana gate: a shaman tank does not pull below 80% own mana
    // (owner decision). The caller (ShouldPullTrigger) already checked the
    // healer loop; this is the tank's own mana only. Non-mana bots (never a
    // shaman) are exempt so the gate cannot strand them.
    inline bool TankShamanPullManaReady(bool hasManaPool, std::uint8_t manaPct)
    {
        if (!hasManaPool)
            return true;
        return manaPct >= 80;
    }

    // Water Shield swap: Lightning Shield for the pull, Water Shield once
    // mana falls to about two thirds (community shaman tank guide). Both
    // rows stay queued like the elemental pair; the trigger picks the phase.
    inline bool TankShamanWantsWaterShield(bool knowsWaterShield, std::uint8_t manaPct)
    {
        return knowsWaterShield && manaPct <= 66;
    }

    // Chain Lightning pack gate: 2+ mobs.
    // Attacker count here means mobs on the bot; the pull path uses the
    // possible-targets count instead (see TankShamanPullWantsChainLightning).
    inline bool ChainLightningPackShouldFire(std::uint8_t attackerCount)
    {
        return attackerCount >= 2;
    }

    // Chain Lightning pull branch: fire CL on any pull of 2+ mobs, single
    // Lightning Bolt otherwise. Count is possible attack targets at pull
    // time (pack standing ahead), not mobs already on the bot.
    inline bool TankShamanPullWantsChainLightning(std::uint8_t possibleTargets)
    {
        return possibleTargets >= 2;
    }

    // Earth Shock fight gate: any elite (creature
    // rank above normal — elite, rare-elite, world boss and rare, anywhere)
    // or any raid-instance fight burns Earth Shock on cooldown (~40% of
    // threat); normal 5-man/open-world trash spends it only when threat is
    // needed (someone catching up or a mob loose).
    inline bool TankShamanEarthShockFreeFire(bool bossOrElite, bool inInstance, bool inRaid)
    {
        // inInstance stays in the signature (dungeon trash vs open-world
        // trash both stay economical), but only the raid leg matters now
        // that every elite counts anywhere.
        (void)inInstance;
        return bossOrElite || inRaid;
    }

    // Elite-anywhere rank read: creature ranks above normal count, wherever
    // the fight happens (open-world elites included). Mirrors the
    // GrindTargetValue `rank > CREATURE_ELITE_NORMAL` idiom.
    inline bool TankShamanTargetIsElite(int creatureRank, int normalRank)
    {
        return creatureRank > normalRank;
    }

    // Threat-lead decision for economical trash mode: shock when another
    // group member is catching up on the target (their threat at or above
    // the gated fraction of ours) or when a pack mob is loose (attacking a
    // non-tank while we hold the current target). Fractions are 0-100
    // percents; the strategy passes 70 (catch-up means 70%+ of our threat).
    inline bool TankShamanThreatNeedsShock(float ownThreat, float memberThreat, std::uint8_t catchUpPct, bool mobLoose)
    {
        if (mobLoose)
            return true;
        if (ownThreat <= 0.0f)
            return memberThreat > 0.0f;
        return memberThreat >= ownThreat * catchUpPct / 100.0f;
    }

    // Tank earth pick: exactly one earth totem per fight, so
    // the slot never flaps. Fights (elite/boss/raid) hold Strength of
    // Earth; trash holds Stoneclaw (threat-needed or wide pack). Stoneskin
    // is never the tank pick on a fight — it only survives as the low-level
    // node fallback before Strength of Earth is trained.
    inline bool TankEarthTotemIsStrength(bool fightTarget)
    {
        return fightTarget;
    }

    // Tank Stoneclaw: grouped threat-gated drop (opposite of the solo-only
    // panic rule in ShamanStoneclawPolicy.h). Manual earth orders keep
    // precedence; any explicit non-stoneclaw earth order wins. Fights
    // (boss/elite/raid) hold Strength of Earth instead of Stoneclaw; trash
    // fires on arrival while the earth slot is empty and threat is needed
    // (another member catching up or a loose mob), or whenever the pack is
    // wide enough that the totem earns its GCD (3+ attackers).
    inline bool TankStoneclawShouldDrop(bool inGroup, bool manualStoneclaw, bool otherManualEarthOrder,
        bool hasEarthTotem, bool threatNeeded, std::uint8_t attackerCount, bool fightTarget = false)
    {
        if (otherManualEarthOrder)
            return false;
        if (manualStoneclaw)
            return !hasEarthTotem;
        if (fightTarget)
            return false;
        if (!inGroup)
            return false;
        if (hasEarthTotem)
            return false;
        if (threatNeeded)
            return true;
        return attackerCount >= 3;
    }

    // Fire Nova vs Magma order: Nova first whenever the party AoEs (3+
    // attackers, the trigger already gates the count); Magma only as the
    // last resort when the pack still lives after Nova (nova totem already
    // down this fight is the proxy) and mana can take the burn, or when the
    // pack is wide (4+) and Nova alone will not hold it.
    inline bool TankMagmaShouldFollow(bool novaTotemDown, bool partyAoeing, std::uint8_t attackerCount, bool manaForMagma)
    {
        if (!manaForMagma)
            return false;
        if (novaTotemDown && partyAoeing)
            return true;
        return attackerCount >= 4 && partyAoeing;
    }
}
