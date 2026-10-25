#pragma once

// Holy/discipline Fade on climbing threat. Pure decision rule with no core
// types: the trigger in ai/playerbot/strategy/priest/PriestTriggers.{h,cpp}
// translates game state into these plain inputs, so the gating is
// unit-tested on its own (tools/test_priest_fade_policy.cpp).
//
// The "medium threat" value is the priest's threat as a percentage of the
// tank's, but it reads 100 for an in-combat mob the tank has no threat on
// yet. Without the tankHoldsMob gate the climbing-threat Fade fires at pull
// start, before the tank lands a hit. A mob genuinely on the priest is shed
// by the separate "target of attacker" row, so this gate loses nothing.

namespace TortoiseBots
{

struct PriestFadeInputs
{
    bool mediumThreat = false; // existing "medium threat" trigger (>= 60% of tank threat)
    bool tankHoldsMob = false; // the tank has threat on the current target
};

inline bool ShouldPriestFadeOnThreat(PriestFadeInputs const& in)
{
    return in.mediumThreat && in.tankHoldsMob;
}

} // namespace TortoiseBots
