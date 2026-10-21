// Standalone check for ai/playerbot/MagmaHazardPolicy.h (the magma guard of
// MovementAction::MoveTo2 and the formation station trigger):
//   g++ -std=c++17 -Wall -Wextra -Werror tools/test_magma_hazard_policy.cpp -o /tmp/test_magma_hazard_policy && /tmp/test_magma_hazard_policy
#include "../ai/playerbot/MagmaHazardPolicy.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using ai::CutPathBeforeMagma;
using ai::IsMagmaHazard;
using ai::MagmaGuardApplies;
using ai::MagmaInteriorSamples;
using ai::MagmaPathCut;
using ai::MagmaPoint;
using ai::MagmaRefusal;
using ai::MagmaRefusalActive;
using ai::MagmaRefusalFor;
using ai::kMagmaMaxInteriorSamples;
using ai::kMagmaRefusalSeconds;

// GridMapDefines.h, mirrored for the test (the static_asserts in
// MovementActions.cpp pin the header's copies to the core's).
constexpr std::uint32_t kMagma = 0x01, kOcean = 0x02, kSlime = 0x04, kWater = 0x08;
constexpr std::uint32_t kNoWater = 0x00, kAbove = 0x01, kWalk = 0x02, kIn = 0x04, kUnder = 0x08;

static MagmaPoint P(float x, float y = 0.0f, float z = 0.0f)
{
    MagmaPoint p;
    p.x = x;
    p.y = y;
    p.z = z;
    return p;
}

// A basin between x = 10 and x = 20 (the RFC trogg-cave floor, flattened).
static bool Basin(MagmaPoint const& p)
{
    return p.x >= 10.0f && p.x <= 20.0f;
}

int main()
{
    std::cout << "Starting TortoiseBots magma hazard policy tests...\n";

    // The core's in-magma rule: magma and under, in, or on its surface.
    {
        CHECK(IsMagmaHazard(kMagma, kIn));
        CHECK(IsMagmaHazard(kMagma, kUnder));
        CHECK(IsMagmaHazard(kMagma, kWalk));
        CHECK(!IsMagmaHazard(kMagma, kAbove));
        CHECK(!IsMagmaHazard(kMagma, kNoWater));
        std::cout << "  [PASS] magma under, in or on the surface is a hazard, above it is not\n";
    }

    // Other liquids are no hazard: slime does no damage in this core
    // (Undercity), water and ocean are swum.
    {
        for (std::uint32_t status : { kWalk, kIn, kUnder })
        {
            CHECK(!IsMagmaHazard(kSlime, status));
            CHECK(!IsMagmaHazard(kWater, status));
            CHECK(!IsMagmaHazard(kOcean, status));
            CHECK(!IsMagmaHazard(0, status));
        }
        std::cout << "  [PASS] slime, water, ocean and no liquid are no hazard\n";
    }

    // The guard runs on instance maps only; the open world keeps its old
    // movement.
    {
        CHECK(MagmaGuardApplies(true));
        CHECK(!MagmaGuardApplies(false));
        std::cout << "  [PASS] the guard applies on instance maps only\n";
    }

    // Sample spacing between two path points.
    {
        CHECK(MagmaInteriorSamples(0.0f) == 0);
        CHECK(MagmaInteriorSamples(2.0f) == 0);
        CHECK(MagmaInteriorSamples(3.0f) == 1);
        CHECK(MagmaInteriorSamples(10.0f) == 4);
        CHECK(MagmaInteriorSamples(std::nanf("")) == 0);
        std::cout << "  [PASS] at most 2 yd between samples\n";
    }

    // A dry route is left alone.
    {
        MagmaPathCut c = CutPathBeforeMagma(P(0), { P(3), P(6), P(9) }, Basin);
        CHECK(!c.cut);
        std::cout << "  [PASS] a dry route is unchanged\n";
    }

    // A path point in the basin: the route ends at the point before it.
    {
        MagmaPathCut c = CutPathBeforeMagma(P(0), { P(2), P(4), P(6), P(8), P(10), P(12) }, Basin);
        CHECK(c.cut);
        CHECK(c.keepPoints == 4);
        CHECK(!c.addPoint);
        std::cout << "  [PASS] a route ends before its first magma point\n";
    }

    // The first path point is already in magma: nothing is kept (MoveTo2
    // then refuses the move).
    {
        MagmaPathCut c = CutPathBeforeMagma(P(9), { P(15), P(30) }, Basin);
        CHECK(c.cut);
        CHECK(c.keepPoints == 0);
        CHECK(!c.addPoint);  // 9 -> 15: the first sample (11) is magma, the last dry one is the bot's own spot
        std::cout << "  [PASS] magma right ahead leaves the bot where it stands\n";
    }

    // Two dry corners either side of the basin: the straight leg between
    // them crosses it, the samples find it and the route ends at the last dry
    // sample on that leg.
    {
        MagmaPathCut c = CutPathBeforeMagma(P(0), { P(4), P(30) }, Basin);
        CHECK(c.cut);
        CHECK(c.keepPoints == 1);
        CHECK(c.addPoint);
        CHECK(c.point.x < 10.0f && c.point.x >= 8.0f);
        std::cout << "  [PASS] a leg crossing magma between two dry points ends before it\n";
    }

    // Magma only at the last point (a destination the caller did not check).
    {
        MagmaPathCut c = CutPathBeforeMagma(P(0), { P(2), P(4), P(6), P(8), P(10) }, Basin);
        CHECK(c.cut);
        CHECK(c.keepPoints == 4);
        std::cout << "  [PASS] magma at the last point cuts the last point\n";
    }

    // A bot already standing in magma walks out: the leading magma run is no
    // cut, magma after dry ground is.
    {
        MagmaPathCut out = CutPathBeforeMagma(P(15), { P(18), P(22), P(26) }, Basin);
        CHECK(!out.cut);
        MagmaPathCut back = CutPathBeforeMagma(P(15), { P(22), P(24), P(16) }, Basin);
        CHECK(back.cut);
        CHECK(back.keepPoints == 2);
        CHECK(back.addPoint);
        CHECK(back.point.x > 20.0f);
        std::cout << "  [PASS] a bot in magma walks out, and stops before magma again\n";
    }

    // A route that never leaves magma is left alone (no dry spot to stop at).
    {
        MagmaPathCut c = CutPathBeforeMagma(P(12), { P(14), P(18) }, Basin);
        CHECK(!c.cut);
        std::cout << "  [PASS] a route that never leaves magma is unchanged\n";
    }

    // Outside a basin (no magma anywhere) nothing changes, however long.
    {
        std::vector<MagmaPoint> path;
        for (int i = 1; i <= 40; ++i)
            path.push_back(P(-5.0f * i, 1.0f, 2.0f));
        MagmaPathCut c = CutPathBeforeMagma(P(0), path, Basin);
        CHECK(!c.cut);
        std::cout << "  [PASS] a route away from magma is unchanged\n";
    }

    // The sample cap: a long dry route makes at most 1 (start) + 48
    // (between points) + 1 per path point lookups.
    {
        std::vector<MagmaPoint> path;
        for (int i = 1; i <= 10; ++i)
            path.push_back(P(-30.0f * i));
        std::size_t calls = 0;
        MagmaPathCut c = CutPathBeforeMagma(P(0), path, [&calls](MagmaPoint const&) { ++calls; return false; });
        CHECK(!c.cut);
        CHECK(kMagmaMaxInteriorSamples == 48);
        CHECK(calls == 1 + 48 + path.size());
        std::cout << "  [PASS] at most 48 samples between path points per route\n";
    }

    // Past the cap only path points are looked at: the samples nearest the
    // bot keep their 2 yd spacing and still find a crossing there; a crossing
    // between two dry points beyond the cap is not seen (a magma path point
    // there still is).
    {
        // 0 -> 120 is 59 samples; the basin at 10..20 is inside the first 48.
        MagmaPathCut near = CutPathBeforeMagma(P(0), { P(120) }, Basin);
        CHECK(near.cut && near.addPoint && near.point.x >= 8.0f && near.point.x < 10.0f);
        // A basin far out: 0 -> 100 uses 48 samples (to 96 yd)... then 100 -> 140 has none.
        auto farBasin = [](MagmaPoint const& p) { return p.x >= 110.0f && p.x <= 120.0f; };
        MagmaPathCut far = CutPathBeforeMagma(P(0), { P(100), P(140) }, farBasin);
        CHECK(!far.cut);
        MagmaPathCut farPoint = CutPathBeforeMagma(P(0), { P(100), P(115), P(140) }, farBasin);
        CHECK(farPoint.cut && farPoint.keepPoints == 1 && !farPoint.addPoint);
        std::cout << "  [PASS] past the cap only path points are checked\n";
    }

    // The refusal memory: the same destination (same map, same 2 yd cell) is
    // refused again from memory until the time runs out; another cell,
    // another map or a later time looks again.
    {
        MagmaRefusal none;
        CHECK(!MagmaRefusalActive(none, 389, -205.5f, -16.4f, -65.7f, 100));
        MagmaRefusal r = MagmaRefusalFor(389, -205.5f, -16.4f, -65.7f, 100);
        CHECK(MagmaRefusalActive(r, 389, -205.5f, -16.4f, -65.7f, 100));
        CHECK(MagmaRefusalActive(r, 389, -205.1f, -16.1f, -65.2f, 100 + kMagmaRefusalSeconds - 1));
        CHECK(!MagmaRefusalActive(r, 389, -205.5f, -16.4f, -65.7f, 100 + kMagmaRefusalSeconds));
        CHECK(!MagmaRefusalActive(r, 389, -201.0f, -16.4f, -65.7f, 101));
        CHECK(!MagmaRefusalActive(r, 389, -205.5f, -12.0f, -65.7f, 101));
        CHECK(!MagmaRefusalActive(r, 389, -205.5f, -16.4f, -61.7f, 101));
        CHECK(!MagmaRefusalActive(r, 1, -205.5f, -16.4f, -65.7f, 101));
        MagmaRefusal cleared = r;
        cleared.set = false;
        CHECK(!MagmaRefusalActive(cleared, 389, -205.5f, -16.4f, -65.7f, 101));
        std::cout << "  [PASS] a refused destination is refused from memory for 10 s\n";
    }

    std::cout << "All magma hazard policy tests passed.\n";
    return 0;
}
