#pragma once

// Pure decision rules for the hunter's answer to mobs on the hunter itself,
// no core includes: the triggers in strategy/hunter/HunterTriggers.h translate
// game state into these plain inputs, so the rules stay testable in
// tools/test_hunter_kite_policy.cpp without the server.
#include <cstdint>

namespace ai
{
    // Attackers on the hunter needed before it stops trading and kites.
    inline constexpr uint8_t kHunterKiteMinAttackers = 2;
    // The kite is "in melee trouble" while the nearest attacker is inside
    // this range: the 8 yd dead zone edge plus a small margin, so a hunter
    // that just stepped out is not re-clipped the same second. Past it the
    // ranged kit is assumed to be shooting.
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
