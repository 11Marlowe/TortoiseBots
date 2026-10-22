#pragma once

#include <cstdint>

// Rage/cooldown gates for the warrior trigger rows, so an ability the bot
// cannot pay for is never armed and the rotation never stalls on it.

namespace ai
{
    // Vanilla 1.12 rage costs / thresholds.
    constexpr uint32_t WARRIOR_DISARM_RAGE_COST = 20;
    constexpr uint32_t WARRIOR_BATTLE_SHOUT_RAGE_COST = 10;
    // Bloodrage only tops up below this rage.
    constexpr uint32_t WARRIOR_BLOODRAGE_RAGE_FLOOR = 20;

    // Disarm is only worth casting on a target that carries a weapon;
    // without one the disarm aura does nothing and the rage is wasted.
    inline bool ShouldDisarm(bool targetHasWeapon, uint32_t rage)
    {
        return targetHasWeapon && rage >= WARRIOR_DISARM_RAGE_COST;
    }

    // Bloodrage costs health, so only spend it when hurt is acceptable.
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
