#pragma once

// Pet manners in a party with a live tank (audit F5, 2026-10-21): a hunter or
// warlock pet was the top of a mob's threat list in 11 % of mob-frames and
// hunters ordered pet Cower 1,011 times in 33 runs. Growl is already off with
// a live tank (runtime/PetTauntPolicy.h), so the lead was plain damage threat:
// the "pet attack" row (and the pet send inside AttackAction::Attack) put the
// pet on the mob the moment the bot picked it, before the tank had touched it.
//
// Two pure rules, fed by plain facts from the action layer and unit-tested on
// their own (tools/test_pet_attack_gate_policy.cpp):
//  - PetAttackHeldForTank: with a live tank in the party, the pet is not sent
//    in until the tank holds the mob (it is the mob's victim, or has threat on
//    it). A mob already on the hunter releases the pet at once, and so does a
//    group with no living tank (the pet is the fallback tank) or a solo
//    hunter. The hold needs no timer: if the tank never gets the mob, the
//    hunter's own shots pull it onto the hunter, which releases the pet.
//    Explicit orders (the player's "pet attack" or ".bot attack") are not
//    held; only the automatic rows are.
//  - PetThreatLeadsTank: Cower on the pet's first lead over the tank, not after
//    the pet has become the mob's victim.

namespace TortoiseBots
{

struct PetAttackGateInputs
{
    bool grouped = false;       // the bot is in a group
    bool liveTank = false;      // a living tank other than the bot is in the group (pet taunts stood down)
    bool targetOnBot = false;   // the mob's victim is the bot
    bool targetOnTank = false;  // the mob's victim is a tank
    float tankThreat = -1.0f;   // best tank's threat on the mob; -1 = no tank value
};

inline bool PetAttackHeldForTank(PetAttackGateInputs const& in)
{
    if (!in.grouped || !in.liveTank)
        return false;
    if (in.targetOnBot || in.targetOnTank)
        return false;
    return !(in.tankThreat > 0.0f);
}

// True when the pet's threat on the mob is above the tank's. A tank value of
// -1 (no tank / unreadable) is never a lead; a pet with no threat never leads.
inline bool PetThreatLeadsTank(float petThreat, float tankThreat)
{
    return tankThreat >= 0.0f && petThreat > 0.0f && petThreat > tankThreat;
}

} // namespace TortoiseBots
