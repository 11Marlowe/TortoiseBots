#pragma once

// Which spells can do anything to a hostile totem (issue #999: Zul'Farrak's
// Greater Healing Ward 8179 is a totem). The core's Totem::IsImmuneToSpellEffect
// (Totem.cpp) makes a totem immune, for a spell cast at it by another unit, to
// every negative aura (a DoT, a curse, a sting, a slow, crowd control), to
// heals and energize, and to a taunt. Direct damage, weapon strikes, melee,
// auto shot and wands still work. A spell whose every effect is one of the
// immune kinds is not cast at a totem (CastSpellAction::isUseful); the bot
// falls through to its next action (a strike, a shot, a direct-damage spell).
//
// The effect numbers are the core's SpellEffects (SpellDefines.h), copied as
// plain numbers so this stays standalone (tools/test_totem_spell_policy.cpp).

#include <cstddef>
#include <cstdint>

namespace TortoiseBots {

constexpr std::uint32_t kEffectNone = 0;

inline bool TotemImmuneEffect(std::uint32_t effect)
{
    switch (effect)
    {
        case 6:   // APPLY_AURA
        case 27:  // PERSISTENT_AREA_AURA (Rain of Fire, Blizzard, Consecration)
        case 35:  // APPLY_AREA_AURA_PARTY
        case 119: // APPLY_AREA_AURA_PET
        case 128: // APPLY_AREA_AURA_FRIEND
        case 129: // APPLY_AREA_AURA_ENEMY
        case 132: // APPLY_AREA_AURA_RAID
        case 133: // APPLY_AREA_AURA_OWNER
        case 134: // APPLY_AURA_PET
        case 10:  // HEAL
        case 67:  // HEAL_MAX_HEALTH
        case 75:  // HEAL_MECHANICAL
        case 30:  // ENERGIZE
        case 114: // ATTACK_ME (a taunt)
            return true;
        default:
            return false;
    }
}

// True when the spell has an effect a hostile totem is not immune to.
inline bool SpellWorksOnTotem(const std::uint32_t* effects, std::size_t count)
{
    bool any = false;
    for (std::size_t i = 0; i < count; ++i)
    {
        if (effects[i] == kEffectNone)
            continue;
        any = true;
        if (!TotemImmuneEffect(effects[i]))
            return true;
    }
    return !any; // a spell without effects is left to the cast checks
}

} // namespace TortoiseBots
