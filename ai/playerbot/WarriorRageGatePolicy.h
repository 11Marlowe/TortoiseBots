#pragma once

#include <cstdint>

// Rage/cooldown gates for the warrior rows that used to fire while
// impossible (issue 1157). The spell audit of 33 dungeon runs measured
// "disarm" 99.6 % IMPOSSIBLE at a median of 6 rage (it costs 20),
// "battle shout" 79 % IMPOSSIBLE at median 1 (it costs 10) and
// "bloodrage" 94 % IMPOSSIBLE while its 60 s cooldown ran: the rows were
// armed every combat tick and failed in the engine's isPossible, starving
// the threat rotation. These gates live in the triggers so an impossible
// row is never armed.

namespace ai
{
    // Vanilla 1.12 ability rage costs / thresholds.
    constexpr uint32_t WARRIOR_DISARM_RAGE_COST = 20;
    constexpr uint32_t WARRIOR_BATTLE_SHOUT_RAGE_COST = 10;
    // Bloodrage triggers below this rage, mirroring the existing trigger.
    constexpr uint32_t WARRIOR_BLOODRAGE_RAGE_FLOOR = 20;

    // Disarm is only worth casting on a target that carries a weapon;
    // without one the disarm aura does nothing and the rage is wasted.
    inline bool ShouldDisarm(bool targetHasWeapon, uint32_t rage)
    {
        return targetHasWeapon && rage >= WARRIOR_DISARM_RAGE_COST;
    }

    // Bloodrage: health >= medium (it costs health), rage < 20, and the
    // 60 s cooldown actually off — otherwise the row is armed and IMPOSSIBLE
    // every tick while the cooldown runs.
    inline bool ShouldBloodrage(bool spellReady, bool healthAtLeastMedium, uint32_t rage)
    {
        return spellReady && healthAtLeastMedium && rage < WARRIOR_BLOODRAGE_RAGE_FLOOR;
    }

    // Battle Shout is only affordable at 10+ rage; the buff-missing and
    // Blessing-of-Might comparisons live in BattleShoutPolicy.h.
    inline bool ShouldBattleShout(uint32_t rage)
    {
        return rage >= WARRIOR_BATTLE_SHOUT_RAGE_COST;
    }
} // namespace ai
