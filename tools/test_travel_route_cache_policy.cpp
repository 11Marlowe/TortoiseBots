#include "../ai/playerbot/TravelRouteCachePolicy.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using namespace ai;

namespace
{
    // Bot at (sx,0,0) on map 0 asks for (1000,0,0); the route it got ends at (200,0,0).
    TravelRouteCacheEntry Stamped(uint32_t nowMs, std::string const& fail = "")
    {
        return TravelRouteCacheStamp(nowMs, 0, 0.f, 0.f, 0.f, 0, 1000.f, 0.f, 0.f, 0, 200.f, 0.f, 0.f, fail);
    }

    TravelRouteCacheVerdict Ask(TravelRouteCacheEntry const& e, uint32_t nowMs,
        float sx = 0.f, uint32_t endMap = 0, float ex = 1000.f, std::size_t points = 5, float backX = 200.f)
    {
        return TravelRouteCacheDecide(e, nowMs, 0, sx, 0.f, 0.f, endMap, ex, 0.f, 0.f, points, 0, backX, 0.f, 0.f);
    }
}

int main()
{
    std::cout << "Starting TortoiseBots travel route cache policy tests...\n";

    // Nothing cached: miss.
    CHECK(Ask(TravelRouteCacheEntry(), 1000) == TravelRouteCacheVerdict::MissNoEntry);
    std::cout << "  [PASS] empty cache misses\n";

    // Same question, young answer, bot where it was, route still ahead: reuse. A partial route
    // (back 200 yd of 1000) is exactly the case the old "ends near the destination" check missed.
    TravelRouteCacheEntry e = Stamped(10000);
    CHECK(Ask(e, 10000) == TravelRouteCacheVerdict::Reuse);
    CHECK(Ask(e, 10100) == TravelRouteCacheVerdict::Reuse);
    CHECK(Ask(e, 10000 + TRAVEL_ROUTE_CACHE_MS - 1) == TravelRouteCacheVerdict::Reuse);
    CHECK(Ask(e, 10100, 10.f) == TravelRouteCacheVerdict::Reuse);        // walked 10 yd
    CHECK(Ask(e, 10100, 0.f, 0, 1003.f) == TravelRouteCacheVerdict::Reuse); // goal jittered 3 yd
    std::cout << "  [PASS] same destination reuses a partial route inside the window\n";

    // Destination changed (past the tolerance) or on another map: miss.
    CHECK(Ask(e, 10100, 0.f, 0, 1006.f) == TravelRouteCacheVerdict::MissDestination);
    CHECK(Ask(e, 10100, 0.f, 1, 1000.f) == TravelRouteCacheVerdict::MissDestination);
    CHECK(TravelRouteCacheDecide(e, 10100, 1, 0.f, 0.f, 0.f, 0, 1000.f, 0.f, 0.f, 5, 0, 200.f, 0.f, 0.f)
        == TravelRouteCacheVerdict::MissDestination); // bot changed map
    std::cout << "  [PASS] other destination or map misses\n";

    // Expiry: the window is a hard limit measured from the rebuild (hits do not extend it),
    // and it survives the 32-bit millisecond wrap.
    CHECK(Ask(e, 10000 + TRAVEL_ROUTE_CACHE_MS) == TravelRouteCacheVerdict::MissExpired);
    CHECK(Ask(e, 10000 + TRAVEL_ROUTE_CACHE_MS + 5000) == TravelRouteCacheVerdict::MissExpired);
    TravelRouteCacheEntry wrap = Stamped(0xFFFFFF00u);
    CHECK(Ask(wrap, 0xFFFFFF00u + 100u) == TravelRouteCacheVerdict::Reuse);
    CHECK(Ask(wrap, 0xFFFFFF00u + TRAVEL_ROUTE_CACHE_MS) == TravelRouteCacheVerdict::MissExpired);
    std::cout << "  [PASS] expiry after the window, wrap-safe\n";

    // Moved far from where the route was planned: miss, so a long partial walk keeps extending.
    CHECK(Ask(e, 10100, TRAVEL_ROUTE_CACHE_START_RADIUS) == TravelRouteCacheVerdict::Reuse);
    CHECK(Ask(e, 10100, TRAVEL_ROUTE_CACHE_START_RADIUS + 1.f) == TravelRouteCacheVerdict::MissMoved);
    std::cout << "  [PASS] moving far invalidates\n";

    // Consumed route (nothing or one point left) and a swapped route both miss.
    CHECK(Ask(e, 10100, 0.f, 0, 1000.f, 1) == TravelRouteCacheVerdict::MissConsumed);
    CHECK(Ask(e, 10100, 0.f, 0, 1000.f, 0) == TravelRouteCacheVerdict::MissConsumed);
    CHECK(Ask(e, 10100, 0.f, 0, 1000.f, 5, 400.f) == TravelRouteCacheVerdict::MissRouteSwap);
    std::cout << "  [PASS] consumed or replaced route misses\n";

    // A rebuild replaces the entry: new window, new start.
    TravelRouteCacheEntry second = TravelRouteCacheStamp(30000, 0, 25.f, 0.f, 0.f, 0, 1000.f, 0.f, 0.f, 0, 300.f, 0.f, 0.f, "");
    CHECK(TravelRouteCacheDecide(second, 30100, 0, 25.f, 0.f, 0.f, 0, 1000.f, 0.f, 0.f, 4, 0, 300.f, 0.f, 0.f)
        == TravelRouteCacheVerdict::Reuse);
    std::cout << "  [PASS] restamp starts a fresh window\n";

    // Failure semantics. The cache has no way to skip a retry: the caller counts one failed
    // move per turn whatever the verdict, and a hit replays the route verdict the rebuild
    // left (LastRouteFail, which feeds the TravelMoveFailed row and the off-graph rescue).
    // Model one wedged bot: a turn per second, IncRetry adds 2 per failed move and the target drops past 10.
    TravelRouteCacheEntry cache;
    std::string routeFailSeen; // what LastRouteFail() holds when the caller reads it
    uint32_t retry = 0, rebuilds = 0, hits = 0, dropTurn = 0;
    for (uint32_t turn = 0; turn < 20 && !dropTurn; ++turn)
    {
        uint32_t const now = 100000 + turn * 1000;
        TravelRouteCacheVerdict const v = Ask(cache, now);
        if (v == TravelRouteCacheVerdict::Reuse)
        {
            ++hits;
            routeFailSeen = cache.routeFail; // replayed
        }
        else
        {
            ++rebuilds;
            routeFailSeen = "startwalk"; // the fresh rebuild's verdict
            cache = Stamped(now, routeFailSeen);
        }
        CHECK(routeFailSeen == "startwalk");  // identical verdict on hit and miss
        retry += 2;                            // IncRetry(true), every turn
        if (retry > 10)                        // TravelTarget::IsMaxRetry: the 6th failed move
            dropTurn = turn + 1;
    }
    CHECK(dropTurn == 6);          // same drop turn as with no cache at all (6 x +2 > 10)
    CHECK(rebuilds == 1 && hits == 5);
    std::cout << "  [PASS] failures counted per turn, verdict replayed on hits\n";

    // Over a long wedge the rebuild rate falls from every turn to one per window.
    cache = TravelRouteCacheEntry();
    rebuilds = 0;
    for (uint32_t turn = 0; turn < 60; ++turn)
    {
        uint32_t const now = 200000 + turn * 1000;
        if (Ask(cache, now) != TravelRouteCacheVerdict::Reuse)
        {
            ++rebuilds;
            cache = Stamped(now);
        }
    }
    CHECK(rebuilds == 8); // 60 turns / 8 s window, rounded up
    std::cout << "  [PASS] 60 one-second turns rebuild 8 times instead of 60\n";

    std::cout << "All travel route cache policy tests passed.\n";
    return 0;
}
