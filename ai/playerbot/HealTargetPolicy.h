#pragma once

// Pure gates for the branches of "party member to heal" that the alive test
// of the player loop does not reach (issue #1146).
//
// The player loop of the value skips the dead before anything else. Three
// other paths into the same heal list had no such test:
//
// - The pet branch pushed every non-mini pet below the almost-full-health
//   line. A pet that died in the last pull is still present (GetPet returns
//   it until revive/dismiss) and sits there at 0% health - the top of the
//   list. The healer then stood over the corpse casting heals into it, while
//   the hurt players it should have healed went unhealed. mod-playerbots
//   guards the same branch with pet->IsAlive() (donor PartyMemberToHeal.cpp:99).
//
// - The selection branch and the rpg-target branch push whatever friendly
//   unit is below full health; a friendly corpse is at 0% health and passes
//   that test, so the same "heal the body" pick was reachable for a dead
//   party member's body or a dead quest companion.
//
// The hunter's revive has its own gate (runtime/PetRevivePolicy.h, the
// "safe to revive pet" trigger) and does not read this value, so nothing
// needs a dead target back.
//
// No core includes; tested in tools/test_heal_target_policy.cpp.

namespace ai
{
    // A pet is a heal target only while it is alive; mini pets are never
    // healable (they are companions, not combat pets).
    inline bool HealablePet(bool isMiniPet, bool isAlive)
    {
        return !isMiniPet && isAlive;
    }

    // A unit picked off the bot's selection or its rpg target is a heal
    // target only while it is alive: below-full-health is not a life sign.
    inline bool HealableTarget(bool isAlive)
    {
        return isAlive;
    }
}
