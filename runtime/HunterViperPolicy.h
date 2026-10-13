#pragma once

#include <cstdint>

// Hunter Aspect of the Viper on low mana (opt-in, AiPlayerbot.HunterViperOnLowMana,
// default 0). Pure decision rules with no core types: the hunter triggers in
// ai/playerbot/strategy/hunter/HunterTriggers.{h,cpp} translate game state into
// these plain inputs, so the hysteresis is unit-tested on its own
// (tools/test_hunter_viper_policy.cpp).
//
// Thresholds follow mod-playerbots (79bd428): HunterAspectOfTheViperTrigger
// fires below LowMana / 2 (AiPlayerbot.LowMana, default 15, so below 7 %) and
// HunterAspectOfTheDragonhawkTrigger keeps the combat aspect off while the
// Viper is up until mana reaches 60 %. Between the two the hunter simply stays
// in the aspect it has, so it cannot flip every tick.
//
// Turtle 1.18.1: Aspect of the Viper is spell 45651 (taught by learn spell
// 47338 at trainer level 56). Below that level HasSpell is false and neither
// rule is reached.

namespace TortoiseBots
{

// Mana percent at which the Viper hands back to the combat aspect (donor: >= 60).
inline constexpr uint8_t HUNTER_VIPER_EXIT_MANA_PCT = 60;

// Mana percent below which the hunter switches to the Viper (donor: LowMana / 2).
inline constexpr uint8_t HunterViperEnterManaPct(uint32_t lowManaConfig)
{
    return static_cast<uint8_t>(lowManaConfig / 2);
}

struct HunterViperInputs
{
    bool toggleOn = false;          // AiPlayerbot.HunterViperOnLowMana
    bool inViper = false;           // bot has the Viper aura
    bool movementAspect = false;    // Cheetah or Pack is up (escape or flag run): leave it alone
    bool playerAspectOrder = false; // a player-given "aspect <x>" strategy is set
    uint8_t manaPct = 100;
    uint32_t lowManaConfig = 15;    // AiPlayerbot.LowMana
};

// Cast Aspect of the Viper now.
inline bool ShouldSwitchToViper(HunterViperInputs const& in)
{
    if (!in.toggleOn || in.inViper || in.movementAspect || in.playerAspectOrder)
        return false;
    return in.manaPct < HunterViperEnterManaPct(in.lowManaConfig);
}

// Keep the combat aspect (Hawk) from being recast over a Viper that has not
// recovered yet. False with the toggle off, so today's behaviour is untouched.
inline bool ViperHoldsCombatAspect(HunterViperInputs const& in)
{
    if (!in.toggleOn || !in.inViper || in.playerAspectOrder)
        return false;
    return in.manaPct < HUNTER_VIPER_EXIT_MANA_PCT;
}

} // namespace TortoiseBots
