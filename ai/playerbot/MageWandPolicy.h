#pragma once

#include <cstdint>

// Pure decision rule for "a mage with mana casts instead of wanding".
//
// Turtle core rule (tortoise-wow `Unit::_UpdateAutoRepeatSpell`): every wand
// shot (spell category 351, SPELLCATEGORY_RANGED_WEAPON) puts the global
// cooldown (category 133) on the shooter for the wand's whole swing time,
// and the next shot renews it. Wand Shoot (5019) itself has no GCD category,
// so it stays castable while Frostbolt, Fireball and every other mage spell
// answer SPELL_FAILED_NOT_READY. A mage bot whose nuke fell through to its
// "shoot" alternative once (a GCD tick, a cast refused while moving) then
// never saw its nuke possible again: it wanded at full mana until movement
// broke the wand or the fight ended.
//
// The donor (mod-playerbots FrostMageStrategy default list: frostbolt 5.4
// above shoot 5.1) wands only when the nuke cannot be cast. This rule keeps
// that intent under the Turtle GCD: stop the wand while the nuke is blocked
// by nothing but the GCD (or is castable outright), keep it while the nuke
// is blocked by anything else (no mana, silence, not known).
//
// No core includes: the mage trigger translates game state into these plain
// inputs, so the rule stays testable in tools/test_mage_wand_policy.cpp.

namespace ai
{
    // Spell category of wand Shoot (5019) in Turtle 1.18.1 spell data.
    constexpr std::uint32_t MAGE_WAND_SHOOT_CATEGORY = 351;

    // How a nuke probe (PlayerbotAI::CanCastSpell with its result) came out.
    enum class MageNukeProbe
    {
        Castable,           // the cast would go out now
        GlobalCooldownOnly, // refused with SPELL_FAILED_NOT_READY: the GCD
        Blocked             // anything else: mana, silence, unknown spell...
    };

    struct MageWandState
    {
        bool wandAutoRepeat;     // current auto-repeat spell is a wand shot
        MageNukeProbe bestNuke;  // best probe over the mage's nukes
        bool keepWandForThreat;  // raid row "has aggro -> shoot" owns this
        bool noManaRowActive;    // "no mana" (< 5%) -> shoot row owns this
    };

    inline bool ShouldStopWandForNuke(MageWandState const& state)
    {
        if (!state.wandAutoRepeat || state.keepWandForThreat || state.noManaRowActive)
            return false;
        return state.bestNuke != MageNukeProbe::Blocked;
    }

    // Combine two nuke probes: the better outcome wins.
    inline MageNukeProbe BetterNukeProbe(MageNukeProbe a, MageNukeProbe b)
    {
        if (a == MageNukeProbe::Castable || b == MageNukeProbe::Castable)
            return MageNukeProbe::Castable;
        if (a == MageNukeProbe::GlobalCooldownOnly || b == MageNukeProbe::GlobalCooldownOnly)
            return MageNukeProbe::GlobalCooldownOnly;
        return MageNukeProbe::Blocked;
    }

    // After stopping the wand, the GCD of its last shot runs out when the
    // next shot would have fired. Sleep that long (plus the react delay) so
    // the next combat tick finds the nuke castable instead of falling back
    // to "shoot" and restarting the wand. Never longer than one swing.
    inline std::uint32_t MageWandStopWaitMs(std::uint32_t rangedTimerMs, std::uint32_t swingMs, std::uint32_t reactDelayMs)
    {
        std::uint32_t const wait = (swingMs && rangedTimerMs > swingMs) ? swingMs : rangedTimerMs;
        return wait + reactDelayMs;
    }
}
