#pragma once

// When has a bot reached its stay point? One rule for the three places that ask:
// the stay walk's isUseful (ReturnToStayPositionAction, PositionAction.cpp), its
// trigger (ReturnToStayPositionTrigger::IsActive, GenericTriggers.cpp) and the run
// leader's follow deadband (InFollowDeadband, DungeonRunPolicy.h).
//
// Crescent Grove r26 step 272 (2026-10-23, MOVE-USELESS): a tauren lead bot
// (bounding radius about 0.97 yd) stood 5.67 yd from its point on the ground and
// 6.6 yd below it (8.7 yd in 3D). The walk's isUseful measured the ground distance
// minus the bounding radius, rounded to 0.1 (4.7 yd, inside the 5.0 yd follow
// range: USELESS, so every `cmd move` was refused), the trigger measured 3D minus
// the radius (7.7 yd: active every tick) and the run leader's deadband measured the
// raw ground distance (5.67 yd: not arrived). The three disagreed and the run
// paused four times. The rule here is the raw 3D distance: no bounding radius (so
// it does not depend on the bot's race) and no rounding.
// Pure decision logic, unit-tested in tools/test_stay_arrival_policy.cpp; the call
// sites are pinned by tools/test_stay_arrival_wiring.py.

#include <cmath>

namespace TortoiseBots
{

// dx, dy, dz: the bot's position minus the stay point. range: the follow range
// (PlayerbotAI::GetRange("follow"); AiPlayerbot.RaidFollowDistance, 5.0 yd, inside
// a dungeon). True when the bot is within range of the point in 3D.
inline bool StayPointReached(float dx, float dy, float dz, float range)
{
    return std::sqrt(dx * dx + dy * dy + dz * dz) <= range;
}

} // namespace TortoiseBots
