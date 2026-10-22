#pragma once

#include <cstdint>

// Pet engage hold + threat-lead cower (issue #1161): a bot-led party
// (commander-driven, no real player master) never runs the "wait for attack"
// clock — that one requires a real player master — so hunter and warlock
// pets engaged the moment the "pet attack" row fired and held top threat on
// the tank's pull. Two halves:
//
// - Engage hold: with a live tank in the group, the pet stays parked for the
//   tank's lead window (~2 s from the group's combat start, the issue's
//   "about 2 s (or 2 Sunders)") before it opens.
// - Threat-lead cower: the hunter stands its pet down (Cower) on the pet's
//   first threat lead over the tank, before the mob turns (PET-8b ordered it
//   only after the pet already held aggro).
//
// Pure decision logic; the live input gathering (group walk + IsTank, the
// PET-3 read, combat clock, threat reads) lives in the action/trigger layer.
// Unit-tested on its own (tools/test_pet_engage_policy.cpp).

namespace TortoiseBots
{

// Tank lead window in seconds: how long after the group's combat start the
// pet stays parked so the tank can build a threat lead.
constexpr uint32_t kPetTankLeadSeconds = 2;

struct PetEngageInputs
{
    bool grouped = false;       // the pet's owner is in a group
    bool tankInGroup = false;   // a live member fills the tank role (PET-3 read)
    bool playerMaster = false;  // a real player master is present
    bool inCombat = false;      // the owner's combat clock is running
    bool enemyPlayer = false;   // the target is a hostile player (PvP: no tank lead to wait for)
    uint32_t combatElapsedSeconds = 0;
};

// True while the pet must stay out of the fight. Solo pets and tankless
// groups return false: the pet is the fallback holder (PET-3 leaves its
// taunts on). A real player master returns false: its party owns the
// "wait for attack" clock, which already holds the pet there. The same
// clock's own rule: a hostile player target never waits.
inline bool ShouldHoldPetForTankLead(PetEngageInputs const& inputs)
{
    if (!inputs.grouped || !inputs.tankInGroup)
        return false;
    if (inputs.playerMaster || inputs.enemyPlayer)
        return false;
    if (!inputs.inCombat)
        return false;
    return inputs.combatElapsedSeconds < kPetTankLeadSeconds;
}

struct PetThreatLeadInputs
{
    bool petHasAggro = false;  // the mob's current victim is the pet (PET-8b)
    float petThreat = 0.0f;    // the pet's threat on the current target
    float tankThreat = 0.0f;   // highest tank threat on it; < 0 means no live tank
};

// Cower when the pet already holds aggro (PET-8b), or — first, so the mob
// never turns — when its threat leads the tank's. Without a live tank the
// pet is the holder and must not cower.
inline bool ShouldPetCower(PetThreatLeadInputs const& inputs)
{
    if (inputs.petHasAggro)
        return true;
    if (inputs.tankThreat < 0.0f)
        return false;
    return inputs.petThreat > 0.0f && inputs.petThreat > inputs.tankThreat;
}

} // namespace TortoiseBots
