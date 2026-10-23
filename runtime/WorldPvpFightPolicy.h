#pragma once

// Open-world PvP fight timers: how long a seeker chases a fleeing target,
// how long it may stay around a player it killed, and how long it then
// ignores that player.
//
// Pure and core-free, so tools/test_world_pvp_fight_policy.cpp includes it
// directly.

#include <cstdint>

namespace TortoiseBots
{

// Seconds a seeker chases a fleeing target before giving up and resuming
// its normal tasks.
inline std::uint32_t WorldPvpPursuitSeconds()
{
    return 60;
}

// Seconds a seeker may stay around a player it killed before moving on.
inline std::uint32_t WorldPvpCampSeconds()
{
    return 5 * 60;
}

// Seconds a seeker ignores a player it camped out after moving on.
inline std::uint32_t WorldPvpCampIgnoreSeconds()
{
    return 10 * 60;
}

// True when a pursuit started at `pursuitStartMs` has run past its limit at
// `nowMs`.
inline bool WorldPvpPursuitExpired(std::uint64_t pursuitStartMs, std::uint64_t nowMs)
{
    if (nowMs < pursuitStartMs)
        return false;
    return nowMs - pursuitStartMs >= std::uint64_t(WorldPvpPursuitSeconds()) * 1000;
}

// True when an established pursuit of a target stays valid: the 60 s limit
// has not passed and the target is still inside sight range. The 20 yd/LOS
// acquisition rule does not apply anymore: a chase would end at once
// otherwise.
inline bool WorldPvpPursuitHolds(std::uint64_t pursuitStartMs, std::uint64_t nowMs, bool withinSight)
{
    if (pursuitStartMs == 0)
        return false;
    if (nowMs < pursuitStartMs)
        return false;
    if (!withinSight)
        return false;
    return !WorldPvpPursuitExpired(pursuitStartMs, nowMs);
}

// True when a camp that started at `campStartMs` has run past its limit at
// `nowMs`.
inline bool WorldPvpCampExpired(std::uint64_t campStartMs, std::uint64_t nowMs)
{
    if (nowMs < campStartMs)
        return false;
    return nowMs - campStartMs >= std::uint64_t(WorldPvpCampSeconds()) * 1000;
}

// True when an ignore set at `ignoreStartMs` still holds at `nowMs`.
inline bool WorldPvpIgnoreFresh(std::uint64_t ignoreStartMs, std::uint64_t nowMs)
{
    if (nowMs < ignoreStartMs)
        return true;
    return nowMs - ignoreStartMs < std::uint64_t(WorldPvpCampIgnoreSeconds()) * 1000;
}

}
