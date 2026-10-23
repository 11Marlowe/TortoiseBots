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

static constexpr float kFar = 8.7f; // the Crescent Grove distance: short of the point

// A refused dispatch (MoveTo false) out of combat: a failed walk at once.
static void Refused(StayWalkBackoff& b, std::uint32_t now)
{
    StayWalkDispatched(b, false, kFar, false, now);
}

// A dispatch MoveTo accepted that leaves the bot standing short of the point: the attempt
// fails once the grace is over.
static void StopsShort(StayWalkBackoff& b, std::uint32_t& now)
{
    StayWalkDispatched(b, true, kFar, false, now);
    CHECK(b.attemptOn);
    now += kStayAttemptGraceMs;
    StayWalkDue(b, false, kFar, false, false, now);
    CHECK(!b.attemptOn);
}

// Refused walks: three in a row rest the walk 10 s, a failure right after rests it 20 s,
// then up to 60 s; the WorldTimer wrap is safe.
static void TestBackoffOnRefusedWalks()
{
    StayWalkBackoff b;
    StayWalkTrack(b, 1.0f, 2.0f, 3.0f, 33);
    std::uint32_t now = 1000;
    CHECK(StayWalkDue(b, false, kFar, false, false, now));
    Refused(b, now);
    Refused(b, now);
    CHECK(StayWalkDue(b, false, kFar, false, false, now) && !StayWalkBackingOff(b, now)); // two: still walks
    Refused(b, now);                                                                         // the third: rests
    CHECK(StayWalkBackingOff(b, now) && !StayWalkDue(b, false, kFar, false, false, now));
    CHECK(StayWalkBackingOff(b, now + kStayWalkBackoffFirstMs - 2));
    CHECK(!StayWalkBackingOff(b, now + kStayWalkBackoffFirstMs + 1));
    now += kStayWalkBackoffFirstMs + 1;
    CHECK(StayWalkDue(b, false, kFar, false, false, now));
    Refused(b, now);
    CHECK(StayWalkBackingOff(b, now + 2 * kStayWalkBackoffFirstMs - 2));
    CHECK(!StayWalkBackingOff(b, now + 2 * kStayWalkBackoffFirstMs + 1));
    for (int i = 0; i < 6; ++i)
    {
        now += kStayWalkBackoffMaxMs + 1;
        Refused(b, now);
    }
    CHECK(b.lengthMs == kStayWalkBackoffMaxMs);
    CHECK(StayWalkBackingOff(b, now + kStayWalkBackoffMaxMs - 2) && !StayWalkBackingOff(b, now + kStayWalkBackoffMaxMs + 1));

    StayWalkBackoff w;
    std::uint32_t const late = 0xFFFFFFFFu - 1000u;
    for (int i = 0; i < 3; ++i)
        Refused(w, late);
    CHECK(StayWalkBackingOff(w, late) && StayWalkBackingOff(w, late + 5000u));
    CHECK(!StayWalkBackingOff(w, late + kStayWalkBackoffFirstMs + 2));
}

// Round 2 of the review: a dispatch MoveTo accepted is not a success. Three that leave the
// bot short (a path makeShortCut emptied, a wait for a transport, a one-point MovePoint)
// rest the walk; a walk that stalls while "moving" fails the same way.
static void TestAcceptedDispatchThatStopsShortFails()
{
    StayWalkBackoff b;
    StayWalkTrack(b, 1.0f, 2.0f, 3.0f, 33);
    std::uint32_t now = 1000;
    // Inside the grace a bot not yet moving is not judged.
    StayWalkDispatched(b, true, kFar, false, now);
    CHECK(StayWalkDue(b, false, kFar, false, false, now + kStayAttemptGraceMs - 1) && b.attemptOn && b.fails == 0);
    now += kStayAttemptGraceMs;
    CHECK(StayWalkDue(b, false, kFar, false, false, now) && !b.attemptOn && b.fails == 1);
    StopsShort(b, now);
    CHECK(StayWalkDue(b, false, kFar, false, false, now) && b.fails == 2);
    StopsShort(b, now);
    CHECK(b.fails == 3 && StayWalkBackingOff(b, now) && !StayWalkDue(b, false, kFar, false, false, now));

    // A walk that moves but gains less than 1 yd in 3 s (re-sent every tick: the attempt
    // keeps its start) fails; one that gains 1 yd every 2 s goes on.
    StayWalkBackoff m;
    StayWalkTrack(m, 1.0f, 2.0f, 3.0f, 33);
    now = 1000;
    StayWalkDispatched(m, true, kFar, false, now);
    float d = kFar;
    for (int i = 0; i < 2; ++i)
    {
        now += 2000;
        d -= 1.1f;
        StayWalkDue(m, false, d, true, false, now);
        StayWalkDispatched(m, true, d, false, now); // the action re-sends: no new attempt
        CHECK(m.attemptOn && m.fails == 0 && m.attemptStartYd == kFar);
    }
    now += kStayAttemptStallMs - 1;
    StayWalkDue(m, false, d - 0.5f, true, false, now);
    CHECK(m.attemptOn && m.fails == 0);
    now += 1;
    StayWalkDue(m, false, d - 0.5f, true, false, now);
    CHECK(!m.attemptOn && m.fails == 1);
}

// A dispatch that brings the bot 3 yd nearer, or to the point, starts it all over.
static void TestProgressOrArrivalResets()
{
    StayWalkBackoff b;
    StayWalkTrack(b, 1.0f, 2.0f, 3.0f, 33);
    std::uint32_t now = 1000;
    StopsShort(b, now);
    StopsShort(b, now);
    CHECK(b.fails == 2);
    StayWalkDispatched(b, true, kFar, false, now);
    now += 500;
    StayWalkDue(b, false, kFar - kStayAttemptGainYd + 0.1f, true, false, now);
    CHECK(b.attemptOn && b.fails == 2); // not yet 3 yd
    StayWalkDue(b, false, kFar - kStayAttemptGainYd, true, false, now);
    CHECK(!b.attemptOn && b.fails == 0);

    StopsShort(b, now);
    StopsShort(b, now);
    StayWalkDispatched(b, true, kFar, false, now);
    CHECK(!StayWalkDue(b, true, 4.9f, true, false, now + 300)); // reached: not due, all clear
    CHECK(!b.attemptOn && b.fails == 0 && b.lengthMs == 0 && !StayWalkBackingOff(b, now));

    // Reaching the point ends a rest too.
    for (int i = 0; i < 3; ++i)
        Refused(b, now);
    CHECK(StayWalkBackingOff(b, now));
    CHECK(!StayWalkDue(b, true, 1.0f, false, false, now) && !StayWalkBackingOff(b, now));
}

// Round 2: failures in combat do not count (a path clipped at a hostile in reach).
static void TestCombatFailuresDoNotCount()
{
    StayWalkBackoff b;
    StayWalkTrack(b, 1.0f, 2.0f, 3.0f, 33);
    std::uint32_t now = 1000;
    for (int i = 0; i < 5; ++i)
        StayWalkDispatched(b, false, kFar, true, now);
    CHECK(b.fails == 0 && !StayWalkBackingOff(b, now));
    StayWalkDispatched(b, true, kFar, true, now);
    now += kStayAttemptGraceMs;
    StayWalkDue(b, false, kFar, false, true, now);
    CHECK(!b.attemptOn && b.fails == 0);
    StayWalkDue(b, false, kFar, false, false, now);
    CHECK(b.fails == 0); // no attempt was on: nothing to judge
}

// A new stay point or an explicit order (even to the same spot) starts it over; the same
// point by itself keeps it.
static void TestNewPointOrOrderResets()
{
    StayWalkBackoff b;
    StayWalkTrack(b, 1.0f, 2.0f, 3.0f, 33);
    std::uint32_t now = 1000;
    for (int i = 0; i < 3; ++i)
        Refused(b, now);
    StayWalkTrack(b, 1.0f, 2.0f, 3.0f, 33);
    CHECK(StayWalkBackingOff(b, now));
    StayWalkTrack(b, 1.0f, 2.0f, 3.5f, 33);
    CHECK(!StayWalkBackingOff(b, now) && b.fails == 0 && StayWalkDue(b, false, kFar, false, false, now));
    for (int i = 0; i < 3; ++i)
        Refused(b, now);
    StayWalkTrack(b, 1.0f, 2.0f, 3.5f, 34); // another map
    CHECK(!StayWalkBackingOff(b, now) && b.fails == 0);
    for (int i = 0; i < 3; ++i)
        Refused(b, now);
    StayWalkDispatched(b, true, kFar, false, now);
    StayWalkOrdered(b);                      // `cmd move` to the same spot
    CHECK(!StayWalkBackingOff(b, now) && b.fails == 0 && !b.attemptOn && !b.keyed);
    StayWalkTrack(b, 1.0f, 2.0f, 3.5f, 34);
    CHECK(StayWalkDue(b, false, kFar, false, false, now));
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
    // On flat ground the same 5.67 yd is not reached either: no bounding radius comes off.
    CHECK(!Reached(5.67f, 0.0f));

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

    TestBackoffOnRefusedWalks();
    TestAcceptedDispatchThatStopsShortFails();
    TestProgressOrArrivalResets();
    TestCombatFailuresDoNotCount();
    TestNewPointOrOrderResets();

    std::printf("test_stay_arrival_policy: %d checks passed\n", checks);
    return 0;
}
