#pragma once

// Shared interrupt-target rule (fury Pummel / combat Kick / shield bash /
// war stomp parity row, task G).
//
// A target counts as worth interrupting only while it has a spell the core
// itself would let an interrupt land on: a generic spell still on its cast
// bar (PREPARING with a real cast time — the state Spell::Update counts
// down) or already channeling (CASTING), or any channeled spell in flight,
// and that spell is silence-prevented. This mirrors the core's own
// Spell::EffectInterruptCast gate (PREPARING-with-cast-time or CASTING +
// PreventionType SILENCE + the damage/channel interrupt flag), minus the
// travel-time nuance a trigger cannot see.
//
// Why not IsNonMeleeSpellCasted(true): that helper also returns true for a
// DELAYED spell (instant cast with a missile in flight — Hateful Strike,
// Slimebolt — nothing on a bar to interrupt) and for the autorepeat slot,
// so every melee boss read as "casting" and Pummel/Kick fired on cooldown
// into thin air (Patchwerk 2026-10-14: 64 Pummels / 19 Kicks, 10 rage +
// GCD each, zero casts to interrupt). mod-playerbots asks the same
// question the same way (PlayerbotAI::IsInterruptableSpellCasting,
// 79bd4281): cast-in-progress first, interrupt effect second.
//
// Pure rule on core spell state, no engine types: the trigger passes the
// target's generic/channeled Spell pointers (or nulls) plus the
// already-resolved interrupt spell's flags, and gets a verdict.

#include <cstdint>

class Spell;

namespace ai
{
    // Spell states from game/Spells/Spell.h (numeric to stay core-free).
    enum InterruptSpellState : uint32_t
    {
        INTERRUPT_SPELL_PREPARING = 1,
        INTERRUPT_SPELL_CASTING = 2,
    };

    // Effect ids from game/Spells/SpellDefines.h, aura ids from
    // game/Spells/SpellAuraDefines.h (verified 1.18.1: INTERRUPT_CAST =
    // 68, APPLY_AURA = 6, MOD_SILENCE = 27).
    enum InterruptEffect : uint32_t
    {
        INTERRUPT_EFFECT_INTERRUPT_CAST = 68,
        INTERRUPT_EFFECT_APPLY_AURA = 6,
        INTERRUPT_AURA_MOD_SILENCE = 27,
    };

    struct InterruptTargetSpell
    {
        // The spell object on the target, if any (generic slot first,
        // channeled slot second). Null = slot empty.
        Spell const* spell = nullptr;
        // True when this is the channeled slot (CASTING = channel phase).
        bool channeledSlot = false;
        // Spell::getState() of the object.
        uint32_t state = 0;
        // Spell::GetCastTime() — 0 for instants.
        int32_t castTime = 0;
        // SpellEntry of the spell being cast.
        uint32_t preventionType = 0; // SPELL_PREVENTION_TYPE_SILENCE = 1
        uint32_t interruptFlags = 0; // SPELL_INTERRUPT_FLAG_DAMAGE = 0x02
        uint32_t channelInterruptFlags = 0; // CHANNEL_FLAG_INTERRUPT = 0x04
    };

    struct InterruptSpellInfo
    {
        uint32_t interruptFlags = 0; // of the interrupt spell (pummel/kick/...)
        uint32_t preventionType = 0;
        // Effect triples of the interrupt spell (3 max, EFFECT_INDEX_0..2).
        static const int MAX_EFFECTS = 3;
        uint32_t effect[MAX_EFFECTS] = {0, 0, 0};
        uint32_t effectAura[MAX_EFFECTS] = {0, 0, 0};
        // Whether the target is immune to effect i (IsImmuneToSpellEffect).
        bool effectImmune[MAX_EFFECTS] = {false, false, false};
    };

    // True when the target has an interruptible cast in progress AND the
    // interrupt spell can land on it. Either slot may be null.
    inline bool ShouldInterruptTargetCasting(InterruptTargetSpell const* genericSpell,
        InterruptTargetSpell const* channeledSpell, InterruptSpellInfo const& interrupt)
    {
        // The victim's cast must be real: a generic spell still on its bar
        // or channeling, or a channeled spell in flight. DELAYED instants
        // (missile in flight) and finished/idle slots are not on a bar.
        bool casting = false;
        InterruptTargetSpell const* victim = nullptr;
        if (genericSpell && genericSpell->spell &&
            ((genericSpell->state == INTERRUPT_SPELL_PREPARING && genericSpell->castTime > 0) ||
                genericSpell->state == INTERRUPT_SPELL_CASTING))
        {
            casting = true;
            victim = genericSpell;
        }
        else if (channeledSpell && channeledSpell->spell &&
            (channeledSpell->state == INTERRUPT_SPELL_PREPARING ||
                channeledSpell->state == INTERRUPT_SPELL_CASTING))
        {
            casting = true;
            victim = channeledSpell;
        }
        if (!casting || !victim)
            return false;

        // The victim's spell must be silence-prevented with the matching
        // interrupt flag (core EffectInterruptCast gate): generic spells
        // carry INTERRUPT_FLAG_DAMAGE, channeled ones CHANNEL_FLAG_INTERRUPT.
        static const uint32_t SILENCE = 1;
        static const uint32_t INTERRUPT_FLAG_DAMAGE = 0x02;
        static const uint32_t CHANNEL_FLAG_INTERRUPT = 0x04;
        if (victim->preventionType != SILENCE)
            return false;
        if (victim->channeledSlot)
        {
            if (!(victim->channelInterruptFlags & CHANNEL_FLAG_INTERRUPT))
                return false;
        }
        else if (!(victim->interruptFlags & INTERRUPT_FLAG_DAMAGE))
            return false;

        // The interrupt spell itself must be able to interrupt (donor's
        // three-way check, unchanged): combat-interrupt + silence, an
        // interrupt-cast effect the target is not immune to, or a silence
        // aura effect.
        static const uint32_t INTERRUPT_FLAG_COMBAT = 0x10;
        for (int i = 0; i < InterruptSpellInfo::MAX_EFFECTS; ++i)
        {
            if ((interrupt.interruptFlags & INTERRUPT_FLAG_COMBAT) && interrupt.preventionType == SILENCE)
                return true;
            if (interrupt.effect[i] == INTERRUPT_EFFECT_INTERRUPT_CAST && !interrupt.effectImmune[i])
                return true;
            if (interrupt.effect[i] == INTERRUPT_EFFECT_APPLY_AURA &&
                interrupt.effectAura[i] == INTERRUPT_AURA_MOD_SILENCE)
                return true;
        }
        return false;
    }
}
