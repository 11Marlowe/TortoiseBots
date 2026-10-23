#pragma once

#include <algorithm>
#include <cstdint>

namespace ai
{
    struct HealingCastState
    {
        bool preparing;
        bool singleTargetDirect;
        std::uint32_t remainingMs;
        std::uint32_t health;
        std::uint32_t maxHealth;
        std::uint64_t incomingDamage;
        std::uint64_t estimatedHeal;
    };

    inline bool ShouldCancelWastefulHeal(HealingCastState const& cast)
    {
        // Once the spell has fired (including a channel), cancelling saves no
        // cast mana. Never reason about an area/HoT spell from one target alone.
        if (!cast.preparing || !cast.singleTargetDirect || !cast.remainingMs || !cast.health || !cast.maxHealth)
            return false;
        std::uint64_t health = std::min(cast.health, cast.maxHealth);
        health = cast.incomingDamage >= health ? 0 : health - cast.incomingDamage;
        if (health == cast.maxHealth)
            return true;
        // Keep the existing conservative boundary: above 90% and over half of
        // the estimated heal wasted. Predicted damage can make a preheal useful.
        if (health * 100 <= std::uint64_t(cast.maxHealth) * 90)
            return false;
        return cast.estimatedHeal > 2 * (cast.maxHealth - health);
    }
}
