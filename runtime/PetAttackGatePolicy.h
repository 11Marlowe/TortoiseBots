#pragma once

// Pet manners in a party with a live tank (audit F5, 2026-10-21): a hunter or
// warlock pet was the top of a mob's threat list in 11 % of mob-frames and
// hunters ordered pet Cower 1,011 times in 33 runs. Growl is already off with
// a live tank (runtime/PetTauntPolicy.h), so the lead was plain damage threat:
// the "pet attack" row (and the pet send inside AttackAction::Attack) put the
// pet on the mob the moment the bot picked it, before the tank had touched it.
//
// Pure rules, fed by plain facts from the action layer and unit-tested on
// their own (tools/test_pet_attack_gate_policy.cpp):
//  - PetAttackHeldForTank: with a live tank in the party, the pet is not sent
//    in until the tank really holds the mob: the mob's current victim is a
//    living tank (any tank of the group, so an off-tank counts), or the best
//    tank's threat on it has reached the core's switch margin over the
//    victim's (110 % in melee reach, 130 % outside; the same margins as the
//    tank's taunt-the-attacker row, ai/playerbot/strategy/values/
//    TauntAttackerPolicy.h on fix/tank-taunts-attacker, copied here so this
//    branch does not depend on it). Token threat does not release the pet: a
//    shout, a heal or a body pull gives a tank a few points while the mob
//    still hits someone else. A mob on the hunter, an enemy player (no threat
//    list, the tank's threat reads 0), a tankless party and a solo hunter
//    never hold the pet. The hold needs no timer: if the tank never gets the
//    mob, the hunter's own shots pull it onto the hunter, which releases the
//    pet. Explicit orders (the player's "pet attack" or ".bot attack") are
//    not held; only the automatic rows are.
//  - PetCowerOnLead: Cower on the pet's first lead over the tank, not after
//    the pet has become the mob's victim.

namespace TortoiseBots
{

// The core's aggro switch margins (threat needed over the victim's).
inline constexpr float kPetTankMarginMelee = 1.1f;
inline constexpr float kPetTankMarginRanged = 1.3f;

struct PetAttackGateInputs
{
    bool grouped = false;            // the bot is in a group
    bool liveTank = false;           // a living tank other than the bot is in the group (pet taunts stood down)
    bool targetIsPlayer = false;     // an enemy player: never held
    bool targetOnBot = false;        // the mob's victim is the bot
    bool victimIsLivingTank = false; // the mob's victim is a living tank of the group
    float tankThreat = -1.0f;        // best tank's threat on the mob; -1 = no tank value
    float victimThreat = 0.0f;       // the current victim's threat on the mob (0 = no victim / none)
    bool tankInMelee = false;        // that tank is in melee reach of the mob
};

// The tank takes the mob by itself: its threat reaches the switch margin over
// the victim's. A victim without threat on the mob counts as taken once the
// tank has any threat at all.
inline bool PetTankHoldsByThreat(PetAttackGateInputs const& in)
{
    if (!(in.tankThreat > 0.0f))
        return false;
    if (!(in.victimThreat > 0.0f))
        return true;
    float const margin = in.tankInMelee ? kPetTankMarginMelee : kPetTankMarginRanged;
    return in.tankThreat >= in.victimThreat * margin;
}

inline bool PetAttackHeldForTank(PetAttackGateInputs const& in)
{
    if (!in.grouped || !in.liveTank || in.targetIsPlayer)
        return false;
    if (in.targetOnBot || in.victimIsLivingTank)
        return false;
    return !PetTankHoldsByThreat(in);
}

// True when the pet's threat on the mob is above the tank's and the tank has
// threat on it at all (a tank at 0 has not engaged: the pet is not "leading"
// anybody). A mob already on the hunter is left alone: the pet was released
// for it and a Cower would only waste the pet's focus.
inline bool PetCowerOnLead(float petThreat, float tankThreat, bool mobOnBot)
{
    if (mobOnBot)
        return false;
    return tankThreat > 0.0f && petThreat > tankThreat;
}

} // namespace TortoiseBots
