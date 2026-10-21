#pragma once

// Warrior rage hygiene (audit F2, PLAN-SPELL-AUDIT W2).
//
// Dungeon tanks sat under 20 rage in 81 % of combat samples, yet three
// buff/debuff rows were tried on every tick: Disarm (20 rage) was the newest
// verdict in 8 % of tank combat and impossible in 99.6 % of it, Battle Shout
// (10 rage) 79 % impossible, Bloodrage 94 % impossible (60 s cooldown, tried
// every rage-starved tick). An impossible row that is tried last leaves the
// tick without a usable ability. These pure rules decide when each row is
// worth trying at all; the triggers in WarriorTriggers call them.
//
// Pure, no engine types, so tools/test_warrior_rage_policy.cpp can pin them.

#include <cstdint>

namespace ai
{
    // Spell costs in whole rage points (the "rage" value is power / 10).
    constexpr uint32_t kDisarmRageCost = 20;
    constexpr uint32_t kBattleShoutRageCost = 10;
    // Bloodrage is a rage generator: only wanted below this much rage.
    constexpr uint32_t kBloodrageRageCeiling = 20;

    // Disarm needs its full rage cost, a target that carries a main-hand
    // weapon (players count as armed; a beast has nothing to strip) and the
    // spell off its cooldown.
    inline bool ShouldDisarm(uint32_t rage, bool targetArmed, bool spellReady)
    {
        return spellReady && targetArmed && rage >= kDisarmRageCost;
    }

    // Bloodrage costs health, so it needs the medium health band, and it is
    // pointless unless rage is starved or while the spell is cooling down.
    inline bool ShouldBloodrage(uint32_t healthPct, uint32_t mediumHealthPct,
                                uint32_t rage, bool spellReady)
    {
        return spellReady && healthPct >= mediumHealthPct && rage < kBloodrageRageCeiling;
    }

    // Battle Shout is impossible below its cost.
    inline bool ShouldBattleShoutForRage(uint32_t rage)
    {
        return rage >= kBattleShoutRageCost;
    }
}
