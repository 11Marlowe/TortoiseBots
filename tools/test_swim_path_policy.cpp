#include "../ai/playerbot/SwimPathPolicy.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using namespace ai;

namespace
{
    // A pool: water from x = 0 to x = 40 (surface at z = 0, floor at z = -10),
    // dry land (the floor above the water) beyond x = 40, and a dry ridge
    // across the pool from x = 18 to x = 20.
    std::uint32_t Pool(float x, float /*y*/, float z)
    {
        if (x < 0.0f || x > 40.0f || (x >= 18.0f && x <= 20.0f))
            return kLiquidNoWater;
        if (z < -12.0f)
            return kLiquidNoWater; // more than 2 yd under the floor
        if (z > 0.0f)
            return kLiquidAboveWater;
        if (z > -0.1f)
            return kLiquidWaterWalk;
        if (z > -2.0f)
            return kLiquidInWater;
        return kLiquidUnderWater;
    }

    // The same pool with no ridge.
    std::uint32_t OpenPool(float x, float y, float z)
    {
        if (x >= 18.0f && x <= 20.0f && z <= 0.0f && z >= -12.0f)
            return z > -0.1f ? kLiquidWaterWalk : (z > -2.0f ? kLiquidInWater : kLiquidUnderWater);
        return Pool(x, y, z);
    }

    std::uint32_t Land(float, float, float) { return kLiquidNoWater; }

    // A fake terrain answers water (type flag WATER) wherever `f` finds liquid.
    template <class F>
    auto Water(F f)
    {
        return [f](float x, float y, float z, std::uint32_t, std::uint32_t& typeFlags)
        {
            typeFlags = kLiquidTypeWater;
            return f(x, y, z);
        };
    }
}

// The Blackfathom Deeps answers: the core's swim path (0x81) and its
// no-polygon swim shortcut (0x11) are kept when the line is in water.
static void TestSwimAnswersInWaterAreKept()
{
    CHECK(ClassifyPathStepType(0x81, false) == PathStepVerdict::KeepIfSwimLine);
    CHECK(ClassifyPathStepType(0x11, false) == PathStepVerdict::KeepIfSwimLine);
    CHECK(ClassifyPathStepType(0x91, false) == PathStepVerdict::KeepIfSwimLine);
    // forceNormalPath asks for a complete path; a swim line is one.
    CHECK(ClassifyPathStepType(0x81, true) == PathStepVerdict::KeepIfSwimLine);
    CHECK(KeepPathStep(0x81, false, true, true));
    CHECK(KeepPathStep(0x11, false, true, true));
    CHECK(KeepPathStep(0x81, true, true, true));
    // 6c step 82: pool floor to pool floor, the whole line under water.
    CHECK(SwimLineInWater(2.0f, 0.0f, -9.5f, 15.9f, 0.0f, -9.6f, Water(OpenPool)));
    // 6b step 75: up from the pool floor to the surface over the rim (the core
    // clamps the end to the surface: WATER_WALK).
    CHECK(SwimLineInWater(2.0f, 0.0f, -9.5f, 32.0f, 5.0f, 0.0f, Water(OpenPool)));
}

// The same bits on land (no mmaps, a no-movement area: the core's straight
// line through walls) are refused: the line is not in water.
static void TestStraightLineOnLandIsRefused()
{
    CHECK(!SwimLineInWater(50.0f, 0.0f, 1.0f, 70.0f, 0.0f, 1.0f, Water(Pool)));
    CHECK(!KeepPathStep(0x11, false, false, true));
    CHECK(!KeepPathStep(0x81, false, false, true));
    // In water but out of sight (a wall, a closed door): refused too.
    CHECK(!KeepPathStep(0x81, false, true, false));
    // From the pool out onto dry land: the land part fails.
    CHECK(!SwimLineInWater(30.0f, 0.0f, -5.0f, 50.0f, 0.0f, 1.0f, Water(Pool)));
    // Across a ridge that rises out of the water: refused.
    CHECK(!SwimLineInWater(5.0f, 0.0f, -5.0f, 30.0f, 0.0f, -5.0f, Water(Pool)));
    // A line that dives under the pool floor: refused.
    CHECK(!SwimLineInWater(2.0f, 0.0f, -9.0f, 16.0f, 0.0f, -20.0f, Water(OpenPool)));
    // Every sample is asked: nothing passes on land.
    CHECK(!SwimLineInWater(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, Water(Land)));
}

static void TestNoPathAndShortcutsAreRefused()
{
    CHECK(ClassifyPathStepType(0x08, false) == PathStepVerdict::Refuse);  // NOPATH
    CHECK(ClassifyPathStepType(0x09, false) == PathStepVerdict::Refuse);  // NORMAL|NOPATH
    CHECK(ClassifyPathStepType(0x89, false) == PathStepVerdict::Refuse);
    CHECK(ClassifyPathStepType(0x02, false) == PathStepVerdict::Refuse);  // SHORTCUT
    CHECK(ClassifyPathStepType(0x03, false) == PathStepVerdict::Refuse);  // NORMAL|SHORTCUT (dry end of an underwater path)
    CHECK(ClassifyPathStepType(0x51, false) == PathStepVerdict::Refuse);  // NORMAL|NOT_USING_PATH|FLYPATH
    CHECK(ClassifyPathStepType(0x43, false) == PathStepVerdict::Refuse);  // a flyer's shortcut
    CHECK(ClassifyPathStepType(0x21, false) == PathStepVerdict::Refuse);  // DEST_FORCED
    CHECK(ClassifyPathStepType(0x101, false) == PathStepVerdict::Refuse); // an unknown bit
    CHECK(ClassifyPathStepType(0x00, false) == PathStepVerdict::Refuse);  // BLANK
    CHECK(ClassifyPathStepType(0x10, false) == PathStepVerdict::Refuse);  // NOT_USING_PATH alone
    CHECK(ClassifyPathStepType(0x80, false) == PathStepVerdict::Refuse);  // UNDERWATER alone
    CHECK(!KeepPathStep(0x08, false, true, true));
    CHECK(!KeepPathStep(0x03, false, true, true));
}

// A navmesh path keeps the old rule: NORMAL always, INCOMPLETE unless a
// complete path is wanted, INCOMPLETE|UNDERWATER (a swim end too far above the
// surface) never.
static void TestNavmeshPathsAsBefore()
{
    CHECK(ClassifyPathStepType(0x01, false) == PathStepVerdict::Keep);
    CHECK(ClassifyPathStepType(0x01, true) == PathStepVerdict::Keep);
    CHECK(ClassifyPathStepType(0x04, false) == PathStepVerdict::Keep);
    CHECK(ClassifyPathStepType(0x04, true) == PathStepVerdict::Refuse);
    CHECK(ClassifyPathStepType(0x84, false) == PathStepVerdict::Refuse);
    CHECK(ClassifyPathStepType(0x05, false) == PathStepVerdict::Refuse);
    // Kept without asking the water.
    CHECK(KeepPathStep(0x01, false, false, false));
    CHECK(KeepPathStep(0x04, false, false, false));
    CHECK(!KeepPathStep(0x04, true, true, true));
}

static void TestSampling()
{
    CHECK(SwimSampleCount(0.0f) == 1);
    CHECK(SwimSampleCount(1.0f) == 2);
    CHECK(SwimSampleCount(2.0f) == 2);
    CHECK(SwimSampleCount(2.1f) == 3);
    CHECK(SwimSampleCount(10.0f) == 6);
    CHECK(SwimSampleCount(80.0f) == 41);
    CHECK(SwimSampleCount(1000.0f) == 501); // never stretched past 2 yd
    CHECK(SwimSampleInWater(kLiquidInWater, kLiquidTypeWater));
    CHECK(SwimSampleInWater(kLiquidUnderWater, kLiquidTypeWater));
    CHECK(SwimSampleInWater(kLiquidWaterWalk, kLiquidTypeOcean));
    CHECK(!SwimSampleInWater(kLiquidAboveWater, kLiquidTypeWater));
    CHECK(!SwimSampleInWater(kLiquidNoWater, kLiquidTypeWater));
    // Magma and slime are no swim water (the type flags decide, as for a player).
    CHECK(!SwimSampleInWater(kLiquidUnderWater, kLiquidTypeMagma));
    CHECK(!SwimSampleInWater(kLiquidInWater, kLiquidTypeSlime));
    // The ridge is 2 yd wide: samples 2 yd apart still find it on a 25 yd line.
    CHECK(!SwimLineInWater(6.0f, 0.0f, -5.0f, 31.0f, 0.0f, -5.0f, Water(Pool)));
}

// Codex review 2026-10-23: a capped sample count stretched the spacing past 2 yd
// on a long line, so a dry strip could hide between two samples. The spacing stays
// 2 yd and a line longer than kSwimMaxLineYd is refused.
static void TestLongLinesAreNeverSampledSparsely()
{
    // A long pool (x 0 to 200) with a dry strip of `width` yd starting at `from`.
    auto strip = [](float from, float width)
    {
        return [from, width](float x, float, float z) -> std::uint32_t
        {
            if (x < 0.0f || x > 200.0f || (x >= from && x <= from + width) || z > 0.0f || z < -12.0f)
                return kLiquidNoWater;
            return kLiquidUnderWater;
        };
    };
    // An 80 yd all-water line: kept.
    CHECK(SwimLineInWater(0.0f, 0.0f, -5.0f, kSwimMaxLineYd, 0.0f, -5.0f, Water(strip(300.0f, 1.0f))));
    // A narrow strip at 79-81 yd on that 80 yd line: refused.
    CHECK(!SwimLineInWater(0.0f, 0.0f, -5.0f, kSwimMaxLineYd, 0.0f, -5.0f, Water(strip(79.0f, 2.0f))));
    // A 2 yd strip anywhere on it is found (the spacing is 2 yd).
    for (float from = 1.0f; from < 78.0f; from += 0.7f)
        CHECK(!SwimLineInWater(0.0f, 0.0f, -5.0f, kSwimMaxLineYd, 0.0f, -5.0f, Water(strip(from, 2.0f))));
    // 100 yd with a 2 yd strip at 31-33, between the old capped samples (2.56 yd
    // apart: 30.77 and 33.33): refused, and an all-water 100 yd line too (too long).
    CHECK(!SwimLineInWater(0.0f, 0.0f, -5.0f, 100.0f, 0.0f, -5.0f, Water(strip(31.0f, 2.0f))));
    CHECK(!SwimLineInWater(0.0f, 0.0f, -5.0f, 100.0f, 0.0f, -5.0f, Water(strip(300.0f, 1.0f))));
    CHECK(!SwimLineInWater(0.0f, 0.0f, -5.0f, kSwimMaxLineYd + 0.5f, 0.0f, -5.0f, Water(strip(300.0f, 1.0f))));
}

// Blackfathom Deeps r26-19z44e slot 6, step 82: the lead bot at -357.5 283 -76.5
// on the floor of the big pool, the point -362.7 277.3 -77.2, 7.7 yd; the core
// answered 0x81, and the bot's move failed (dispatch-short) 163 times. The pool is
// the BFD WMO's group 5 liquid (Blackfathom_Instance.wmo.vmo, read 2026-10-23): raw
// liquid type id 1, surface at -55.97; no WMO triangle on the line at +0 to +2 yd.
// The core's vmap liquid test ANDs the request with that raw id
// (VMapManager2::GetLiquidLevel: `ReqLiquidType && !(type & ReqLiquidType)` refuses),
// so a request for water and ocean (0x0A) found no liquid there at all.
static std::uint32_t BfdPool(float x, float y, float z, std::uint32_t request, std::uint32_t& typeFlags)
{
    std::uint32_t const rawId = 1;          // every BFD WMO liquid
    float const surface = -55.97f, floor = -78.5f;
    typeFlags = 0;
    if (request && !(rawId & request))
        return kLiquidNoWater;              // the vmap refuses; BFD has no ADT liquid to fall back to
    if (x > -340.0f || x < -380.0f || y < 260.0f || y > 300.0f || z < floor - 2.0f)
        return kLiquidNoWater;
    typeFlags = kLiquidTypeWater;           // LiquidType.dbc 1: water
    int const delta = static_cast<int>((surface - z) * 10.0f);
    return delta > 20 ? kLiquidUnderWater : delta > 0 ? kLiquidInWater : delta > -1 ? kLiquidWaterWalk : kLiquidAboveWater;
}

static void TestBlackfathomPoolLineIsKept()
{
    // The old request (water | ocean) found no water: the line was refused.
    std::uint32_t flags = 0;
    CHECK(BfdPool(-357.5f, 283.0f, -76.5f, kLiquidTypeWater | kLiquidTypeOcean, flags) == kLiquidNoWater);
    // The request a swim line makes now (all liquids) finds it, and the step is kept.
    CHECK(kSwimLiquidRequest == (kLiquidTypeMagma | kLiquidTypeOcean | kLiquidTypeSlime | kLiquidTypeWater));
    bool const inWater = SwimLineInWater(-357.5f, 283.0f, -76.5f, -362.7f, 277.3f, -77.2f, BfdPool);
    CHECK(inWater);
    CHECK(KeepPathStep(0x81, false, inWater, true));
    // The land rule stands: the same line leaving the pool is refused.
    CHECK(!SwimLineInWater(-357.5f, 283.0f, -76.5f, -330.0f, 283.0f, -76.5f, BfdPool));
}

int main()
{
    TestSwimAnswersInWaterAreKept();
    TestStraightLineOnLandIsRefused();
    TestNoPathAndShortcutsAreRefused();
    TestNavmeshPathsAsBefore();
    TestSampling();
    TestLongLinesAreNeverSampledSparsely();
    TestBlackfathomPoolLineIsKept();
    std::cout << "swim path policy: all tests passed\n";
    return 0;
}
