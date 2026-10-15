#pragma once

// What a bot casts at a hostile totem, and what it crowd-controls (issue #999:
// Zul'Farrak's Greater Healing Ward 8179 is a totem).
//
// The spell rule mirrors the core exactly: Totem::IsImmuneToSpellEffect in
// tortoise-wow src/game/Objects/Totem.cpp:190-235. A totem is immune, effect
// by effect, to
//   - SPELL_EFFECT_ATTACK_ME (114), HEAL (10), HEAL_MAX_HEALTH (67),
//     HEAL_MECHANICAL (75) and ENERGIZE (30);
//   - for a spell that is not positive (SpellEntry::IsPositiveSpell): every
//     aura it applies (SpellEntry::IsSpellAppliesAura: an aura effect,
//     Spells::IsEffectAppliesAura - APPLY_AURA 6, APPLY_AREA_AURA_PARTY 35,
//     _PET 119, _RAID 132, _FRIEND 128, _ENEMY 129, _OWNER 133, APPLY_AURA_PET
//     134 - with an aura name), so DoTs, curses, stings, Hunter's Mark, Faerie
//     Fire, stuns, slows and crowd control do nothing;
//   - for a positive spell: the periodic regeneration auras
//     (SpellEntry::IsPeriodicRegenerateEffect: PERIODIC_ENERGIZE 24,
//     PERIODIC_HEAL 8, PERIODIC_HEALTH_FUNNEL 62).
// Two cases of that function never meet a bot casting at someone else's
// hostile totem and are left out: the shaman Mana Spring / Healing Stream
// family exception and a totem's own self cast. Anything else (direct damage,
// weapon strikes, auto attack, auto shot, wands, PERSISTENT_AREA_AURA 27,
// which the function does not list) is not immune by it; the creature's own
// immunities (Creature::IsImmuneToSpellEffect) are the core's to answer.
// A spell whose every effect is immune is not cast at a hostile totem
// (CastSpellAction::isUseful); the bot falls through to its next action.
//
// Numbers are the core's (SpellDefines.h, SpellAuraDefines.h), copied so this
// stays standalone (tools/test_totem_spell_policy.cpp).

#include <cstddef>
#include <cstdint>

namespace TortoiseBots {

inline bool EffectAppliesAura(std::uint32_t effect)
{
    switch (effect)
    {
        case 6: case 35: case 119: case 132: case 128: case 129: case 133: case 134:
            return true;
        default:
            return false;
    }
}

// Totem::IsImmuneToSpellEffect for one effect of a spell another unit casts at it.
inline bool TotemImmuneToEffect(std::uint32_t effect, std::uint32_t auraName, bool positiveSpell)
{
    switch (effect)
    {
        case 114: // ATTACK_ME
        case 10:  // HEAL
        case 67:  // HEAL_MAX_HEALTH
        case 75:  // HEAL_MECHANICAL
        case 30:  // ENERGIZE
            return true;
        default:
            break;
    }
    if (!positiveSpell)
        return EffectAppliesAura(effect) && auraName != 0;
    return auraName == 24 || auraName == 8 || auraName == 62;
}

// True when the spell has an effect a hostile totem is not immune to (a spell
// without effects is left to the cast checks).
inline bool SpellWorksOnTotem(const std::uint32_t* effects, const std::uint32_t* auraNames, std::size_t count,
    bool positiveSpell)
{
    bool any = false;
    for (std::size_t i = 0; i < count; ++i)
    {
        if (effects[i] == 0)
            continue;
        any = true;
        if (!TotemImmuneToEffect(effects[i], auraNames[i], positiveSpell))
            return true;
    }
    return !any;
}

// Crowd control (CcTargetValue): a totem is never a candidate. A unit an NPC
// summoned or created (a mob's pet, a summoned skeleton) stays one, except
// while the bot's party has a run going and the run's plan lists its entry
// kill_first (the healing ward is killed, not polymorphed).
inline bool CcLeavesOut(bool totem, bool npcSummoned, bool inRun, bool runKillFirst)
{
    return totem || (inRun && npcSummoned && runKillFirst);
}

} // namespace TortoiseBots
