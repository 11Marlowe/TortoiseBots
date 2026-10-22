#pragma once

#include <cstdint>

// Pet engage hold + threat-lead cower. The "wait for attack" clock requires
// a real player master, so a party run by bots alone had no hold and its
// pets engaged before the tank had any threat lead. Two halves:
//
// - Engage hold: with a live tank in the group, the pet stays parked for the
//   tank's lead window (~2 s from the group's combat start) before it opens.
// - Threat-lead cower: the hunter stands its pet down (Cower) on the pet's
//   first threat lead over the tank, before the mob turns on the pet.
//
// Pure decision logic; the live input gathering (group walk + IsTank,
// combat clock, threat reads) lives in the action/trigger layer.

namespace TortoiseBots
{

// Tank lead window in seconds: how long after the group's combat start the
// pet stays parked so the tank can build a threat lead.
constexpr uint32_t kPetTankLeadSeconds = 2;

struct PetEngageInputs
{
    bool grouped = false;       // the pet's owner is in a group
    bool tankInGroup = false;   // a live member fills the tank role
    bool playerMaster = false;  // a real player master is present
    bool inCombat = false;      // the owner's combat clock is running
    bool enemyPlayer = false;   // the target is a hostile player (PvP: no tank lead to wait for)
    uint32_t combatElapsedSeconds = 0;
};

// True while the pet must stay out of the fight. Solo pets and tankless
// groups return false: the pet is the fallback holder (its taunts stay on).
// A real player master returns false: its party owns the
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
    bool petHasAggro = false;  // the mob's current victim is the pet
    float petThreat = 0.0f;    // the pet's threat on the current target
    float tankThreat = 0.0f;   // highest tank threat on it; < 0 means no live tank
};

// Cower when the pet already holds aggro, or — first, so the mob never
// turns — when its threat leads the tank's. Without a live tank the
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
