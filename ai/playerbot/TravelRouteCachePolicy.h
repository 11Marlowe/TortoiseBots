#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

// Pure policy for the per-bot travel route cache (MovementAction::ResolveMovePath).
//
// Problem: a route that ends short of its destination (node route not found, so
// getFullPath returns the partial direct navmesh path; or an "incomplete"
// navmesh path) never passes ResolveMovePath's "last path still ends near the
// destination" check, so the whole route (up to 25 node A* runs, navmesh legs,
// tile loads) was rebuilt on every turn until the bot dropped the target.
// Profile 2026-10-13, 500 bots: ~41% of the module pass.
//
// Rule: remember what the last rebuild was asked (start, destination) and what it
// produced (the end of the route), and reuse the route being walked (lastPath)
// while the same question is asked again, the route is still ahead of the bot,
// and the answer is young. A rebuild always replaces the entry.
//
// What the cache does NOT touch: the retry counting. A reused route flows through
// the same makeShortCut / special-movement / ClipPath / dispatch code as a fresh
// one, so a failed or phantom move still fails in the same turn and
// MoveToTravelTargetAction::Execute still counts it (IncRetry) at the same
// cadence. The only extra state a fresh rebuild left behind is
// TravelNodeMap::LastRouteFail() (read by the TravelMoveFailed row and the
// off-graph rescue), so the entry carries it and a hit replays it (up to
// TRAVEL_ROUTE_CACHE_MS / TRAVEL_ROUTE_CACHE_START_RADIUS old).
//
// One real difference: when a fresh rebuild would now come back EMPTY (no node
// route and no navmesh path), ResolveMovePath would dispatch the NOPATH portal
// point that turn and the move fails; a hit keeps walking the old lastPath for up
// to the window instead, so the retry count can differ in that case.

namespace ai
{
    // A cached route is trusted this long. A fresh rebuild after it may find a
    // route that was missing (tiles loaded, a graph change); until then the bot
    // keeps walking what it has.
    constexpr uint32_t TRAVEL_ROUTE_CACHE_MS = 8000;

    // The destination may drift this far (yards) and still be "the same". Travel
    // targets hold one stable approach point per bot, so this only absorbs float
    // noise and slowly moving goals.
    constexpr float TRAVEL_ROUTE_CACHE_END_TOLERANCE = 5.0f;

    // The bot may wander this far (yards) from where the route was planned before
    // the plan is considered stale. A partial route is rebuilt every time the bot
    // has moved this far, so a long partial walk keeps extending.
    constexpr float TRAVEL_ROUTE_CACHE_START_RADIUS = 30.0f;

    // The back of the route being walked must still be the cached route's back
    // within this many yards, else another movement replaced lastPath.
    constexpr float TRAVEL_ROUTE_CACHE_BACK_TOLERANCE = 1.0f;

    struct TravelRouteCacheEntry
    {
        bool valid = false;
        // Where the route was planned from and for.
        uint32_t startMap = 0;
        float startX = 0.0f, startY = 0.0f, startZ = 0.0f;
        uint32_t endMap = 0;
        float endX = 0.0f, endY = 0.0f, endZ = 0.0f;
        // Where the route that was produced ends (it may stop short of the
        // destination). A different back means lastPath is not this route any more.
        uint32_t backMap = 0;
        float backX = 0.0f, backY = 0.0f, backZ = 0.0f;
        uint32_t stampMs = 0;
        // TravelNodeMap::LastRouteFail() after the rebuild ("" when none).
        std::string routeFail;
    };

    enum class TravelRouteCacheVerdict : uint8_t
    {
        Reuse = 0,
        MissNoEntry,     // nothing cached yet / cleared
        MissDestination, // asked for another destination or map
        MissExpired,     // older than TRAVEL_ROUTE_CACHE_MS
        MissMoved,       // bot is far from where the route was planned
        MissConsumed,    // nothing (or a single point) left to walk
        MissRouteSwap,   // lastPath is no longer the cached route
    };

    inline float TravelRouteCacheDist(float ax, float ay, float az, float bx, float by, float bz)
    {
        float const dx = ax - bx, dy = ay - by, dz = az - bz;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    // lastPathPoints / lastBack*: the route the bot holds now (lastMove.lastPath).
    inline TravelRouteCacheVerdict TravelRouteCacheDecide(
        TravelRouteCacheEntry const& entry, uint32_t nowMs,
        uint32_t startMap, float startX, float startY, float startZ,
        uint32_t endMap, float endX, float endY, float endZ,
        std::size_t lastPathPoints,
        uint32_t lastBackMap, float lastBackX, float lastBackY, float lastBackZ)
    {
        if (!entry.valid)
            return TravelRouteCacheVerdict::MissNoEntry;

        if (entry.endMap != endMap || entry.startMap != startMap ||
            TravelRouteCacheDist(entry.endX, entry.endY, entry.endZ, endX, endY, endZ) > TRAVEL_ROUTE_CACHE_END_TOLERANCE)
            return TravelRouteCacheVerdict::MissDestination;

        // Unsigned subtraction: correct across the 32-bit millisecond wrap.
        if (static_cast<uint32_t>(nowMs - entry.stampMs) >= TRAVEL_ROUTE_CACHE_MS)
            return TravelRouteCacheVerdict::MissExpired;

        if (TravelRouteCacheDist(entry.startX, entry.startY, entry.startZ, startX, startY, startZ) > TRAVEL_ROUTE_CACHE_START_RADIUS)
            return TravelRouteCacheVerdict::MissMoved;

        // Fewer than two points is a consumed route: a fresh rebuild may reach
        // further, and the dispatch would refuse a one-point route anyway.
        if (lastPathPoints < 2)
            return TravelRouteCacheVerdict::MissConsumed;

        // Another movement replaced lastPath since (cutTo only trims the front,
        // so the back of the cached route does not change while it is walked).
        if (entry.backMap != lastBackMap ||
            TravelRouteCacheDist(entry.backX, entry.backY, entry.backZ, lastBackX, lastBackY, lastBackZ) > TRAVEL_ROUTE_CACHE_BACK_TOLERANCE)
            return TravelRouteCacheVerdict::MissRouteSwap;

        return TravelRouteCacheVerdict::Reuse;
    }

    inline TravelRouteCacheEntry TravelRouteCacheStamp(
        uint32_t nowMs,
        uint32_t startMap, float startX, float startY, float startZ,
        uint32_t endMap, float endX, float endY, float endZ,
        uint32_t backMap, float backX, float backY, float backZ,
        std::string const& routeFail)
    {
        TravelRouteCacheEntry entry;
        entry.valid = true;
        entry.startMap = startMap;
        entry.startX = startX; entry.startY = startY; entry.startZ = startZ;
        entry.endMap = endMap;
        entry.endX = endX; entry.endY = endY; entry.endZ = endZ;
        entry.backMap = backMap;
        entry.backX = backX; entry.backY = backY; entry.backZ = backZ;
        entry.stampMs = nowMs;
        entry.routeFail = routeFail;
        return entry;
    }
}
