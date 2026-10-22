#pragma once

// Pure decision rules for the hunter's area shots, no core includes: the
// triggers in strategy/hunter/HunterTriggers.h translate game state into
// these plain inputs, so the rules stay testable in
// tools/test_hunter_aoe_policy.cpp without the server.
#include <cstdint>

namespace ai
{
    // Multi-Shot pays for itself only when the splash hits a second live
    // enemy; a lone target gets the single-target kit.
    inline constexpr uint8_t kHunterMultiShotMinTargets = 2;

    // Splash radius of every 1.12 Multi-Shot rank (Spell.dbc EffectRadius).
    inline constexpr float kHunterMultiShotSplashYd = 15.0f;

    // The caller counts fighting, reachable enemies inside the splash radius
    // of the shot's target; crowd-controlled ones never count. Any breakable
    // CC inside the splash vetoes the shot entirely - the splash would shatter
    // it. Unbreakable CC (stun, fear, roots) survives the damage, so only the
    // count is reduced, not vetoed.
    inline bool ShouldHunterMultiShot(uint8_t splashTargets, bool breakableCcInSplash)
    {
        return !breakableCcInSplash && splashTargets >= kHunterMultiShotMinTargets;
    }
}
