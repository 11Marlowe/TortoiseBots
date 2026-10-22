#pragma once

// Pure gate for the pet branch of "party member to heal" (issue #1146).
//
// The pet branch of the value walked the group's pets after the players and
// pushed every non-mini pet below the almost-full-health line onto the heal
// list. It never asked whether the pet was alive: the player loop above it
// skips the dead, but a pet that died in the last pull is still present
// (GetPet returns it until revive/dismiss) and sits there at 0% health - the
// top of the list. The healer then stood over the corpse casting heals into
// it, while the hurt players it should have healed went unhealed.
//
// mod-playerbots guards the same branch with pet->IsAlive() (donor
// PartyMemberToHeal.cpp:99). The hunter's revive has its own gate
// (runtime/PetRevivePolicy.h, the "safe to revive pet" trigger) and does not
// read this value, so nothing needs a dead pet back.
//
// No core includes; tested in tools/test_heal_pet_policy.cpp.

namespace ai
{
    // A pet is a heal target only while it is alive; mini pets are never
    // healable (they are companions, not combat pets).
    inline bool HealablePet(bool isMiniPet, bool isAlive)
    {
        return !isMiniPet && isAlive;
    }
}
