// Standalone regression test for the stay point arrival rule
// (runtime/StayArrivalPolicy.h): the raw 3D distance, no bounding radius, no
// rounding, shared by the stay walk, its trigger and the run leader's deadband.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_stay_arrival_policy.cpp -o /tmp/test_stay_arrival
//   /tmp/test_stay_arrival

#include "../runtime/StayArrivalPolicy.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

static constexpr float kRange = 5.0f; // AiPlayerbot.RaidFollowDistance

// A ground distance split over x and y (as a real offset is), dz up or down.
static bool Reached(float ground, float dz, float range = kRange)
{
    float const dx = ground * 0.6f;
    float const dy = ground * 0.8f;
    return StayPointReached(dx, dy, dz, range);
}

int main()
{
    // Crescent Grove r26 step 272: a tauren lead bot (radius about 0.97 yd) 5.67 yd
    // from the point on the ground and 6.6 yd below it (8.7 yd in 3D). The old walk
    // rule (ground minus the radius, 4.7 yd) called it reached and refused every move;
    // the rule does not reach it, so the walk is useful and the run leader does not
    // count it as arrived.
    CHECK(!Reached(5.67f, 6.6f));
    CHECK(!Reached(5.67f, -6.6f));
    CHECK(!StayPointReached(-91.7f - -88.0f, -62.1f - -66.4f, 273.4f - 280.0f, kRange)); // the trace's positions
    // The old rule's figure is inside the range: the bounding radius plays no part now.
    CHECK(5.67f - 0.97f <= kRange);

    // Flat ground: a bot 4.5 yd away is there (no walk, the trigger quiet: no jitter).
    CHECK(Reached(4.5f, 0.0f));
    CHECK(Reached(0.0f, 0.0f));
    // The edge of the range counts; just past it does not.
    CHECK(StayPointReached(kRange, 0.0f, 0.0f, kRange));
    CHECK(StayPointReached(0.0f, 0.0f, kRange, kRange));
    CHECK(!StayPointReached(kRange + 0.01f, 0.0f, 0.0f, kRange));
    CHECK(!Reached(5.67f, 0.0f));
    // No rounding: 5.04 yd is beyond 5.0 (the old rule rounded it to 5.0).
    CHECK(!StayPointReached(5.04f, 0.0f, 0.0f, kRange));

    // Height counts: within range on the ground but far above or below is not there.
    CHECK(!Reached(0.0f, 5.5f));
    CHECK(!Reached(4.84f, 4.6f));  // Zul'Farrak r16 step 37 (6.7 yd in 3D): the walk moves it now
    CHECK(Reached(3.0f, 3.8f));    // 4.84 yd in 3D: there
    CHECK(Reached(3.0f, -3.8f));

    // The range is the caller's: outside a dungeon the follow range is wider.
    CHECK(Reached(5.67f, 0.0f, 6.0f));
    CHECK(!Reached(5.67f, 6.6f, 8.0f));
    CHECK(Reached(5.67f, 6.6f, 9.0f));

    std::printf("test_stay_arrival_policy: %d checks passed\n", checks);
    return 0;
}
