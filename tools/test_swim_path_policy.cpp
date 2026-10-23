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

    bool Land(float, float, float) { return false; }
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
    CHECK(SwimLineInWater(2.0f, 0.0f, -9.5f, 15.9f, 0.0f, -9.6f, OpenPool));
    // 6b step 75: up from the pool floor to the surface over the rim (the core
    // clamps the end to the surface: WATER_WALK).
    CHECK(SwimLineInWater(2.0f, 0.0f, -9.5f, 32.0f, 5.0f, 0.0f, OpenPool));
}

// The same bits on land (no mmaps, a no-movement area: the core's straight
// line through walls) are refused: the line is not in water.
static void TestStraightLineOnLandIsRefused()
{
    CHECK(!SwimLineInWater(50.0f, 0.0f, 1.0f, 70.0f, 0.0f, 1.0f, Pool));
    CHECK(!KeepPathStep(0x11, false, false, true));
    CHECK(!KeepPathStep(0x81, false, false, true));
    // In water but out of sight (a wall, a closed door): refused too.
    CHECK(!KeepPathStep(0x81, false, true, false));
    // From the pool out onto dry land: the land part fails.
    CHECK(!SwimLineInWater(30.0f, 0.0f, -5.0f, 50.0f, 0.0f, 1.0f, Pool));
    // Across a ridge that rises out of the water: refused.
    CHECK(!SwimLineInWater(5.0f, 0.0f, -5.0f, 30.0f, 0.0f, -5.0f, Pool));
    // A line that dives under the pool floor: refused.
    CHECK(!SwimLineInWater(2.0f, 0.0f, -9.0f, 16.0f, 0.0f, -20.0f, OpenPool));
    // Every sample is asked: nothing passes on land.
    CHECK(!SwimLineInWater(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, Land));
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
    CHECK(SwimSampleCount(1000.0f) == kSwimMaxSamples);
    CHECK(SwimSampleInWater(kLiquidInWater));
    CHECK(SwimSampleInWater(kLiquidUnderWater));
    CHECK(SwimSampleInWater(kLiquidWaterWalk));
    CHECK(!SwimSampleInWater(kLiquidAboveWater));
    CHECK(!SwimSampleInWater(kLiquidNoWater));
    // The ridge is 2 yd wide: samples 2 yd apart still find it on a 25 yd line.
    CHECK(!SwimLineInWater(6.0f, 0.0f, -5.0f, 31.0f, 0.0f, -5.0f, Pool));
}

int main()
{
    TestSwimAnswersInWaterAreKept();
    TestStraightLineOnLandIsRefused();
    TestNoPathAndShortcutsAreRefused();
    TestNavmeshPathsAsBefore();
    TestSampling();
    std::cout << "swim path policy: all tests passed\n";
    return 0;
}
