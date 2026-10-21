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

    std::cout << "All magma hazard policy tests passed.\n";
    return 0;
}
