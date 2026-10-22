#pragma once

// Hunter responses to mobs on the hunter itself (issues #1167/#1162).
// Pure decision rules, no core includes: the triggers in
// strategy/hunter/HunterTriggers.h translate game state into these plain
// inputs, so the rules stay testable in tools/test_hunter_kite_policy.cpp
// without the server.
//
// Donor comparison (mod-playerbots, read-only reference): the donor answers a
// mob that is too close for auto shot with disengage 35 then flee 34
// (GenericHunterStrategy.cpp:83-84). It has no rule for two or more mobs on
// the hunter - the reported death case (a hunter standing in melee, trading
// Raptor/Mongoose with a pack until dead) is new here. The rules:
//
//  - 2+ mobs on the hunter while the ranged kit is usable: stop trading,
//    Wing Clip the nearest attacker and step out to shooting range. Below
//    kHunterKiteEngageYd the kite owns the fight (the melee switch and the
//    raptor/mongoose chain stand down); past it the normal ranged kit
//    resumes and the hunter shoots the mobs chasing it.
//  - Hunter's Mark follows the kill: with a living tank in the group, only
//    the mob the tank is fighting gets marked, not whichever mob is chewing
//    on the hunter.
#include <cstdint>

namespace ai
{
    // Attackers on the hunter needed before it stops trading and kites.
    inline constexpr uint8_t kHunterKiteMinAttackers = 2;
    // The kite is "in melee trouble" while the nearest attacker is inside
    // this range (edge-to-edge combat distance): the 8 yd dead zone edge plus
    // a small margin, so a hunter that just stepped out is not re-clipped the
    // same second. Past it the ranged kit is assumed to be shooting.
    inline constexpr float kHunterKiteEngageYd = 10.0f;

    // 2+ mobs on the hunter and a usable ranged kit: clip the nearest and
    // step out. hasRangedKit covers strategy + a loaded ranged weapon - a
    // hunter without a working bow gains nothing from stepping away, it
    // should keep trading in the close kit.
    inline bool ShouldHunterKite(uint8_t attackerCount, bool hasRangedKit, float nearestAttackerYd)
    {
        return hasRangedKit && attackerCount >= kHunterKiteMinAttackers &&
            nearestAttackerYd < kHunterKiteEngageYd;
    }

    // Hunter's Mark goes on the party's kill. Without a living tank in the
    // group (solo hunter, or the tank is down) every target is the hunter's
    // own kill: mark freely.
    inline bool ShouldHuntersMarkTarget(bool groupHasTank, bool targetIsTankTarget)
    {
        return !groupHasTank || targetIsTankTarget;
    }
}
