#include "../ai/playerbot/MagmaHazardPolicy.h"
#include "../ai/playerbot/SwimPathPolicy.h"

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
    // The flags must say water or ocean, and a magma or slime bit beside them refuses.
    CHECK(!SwimSampleInWater(kLiquidUnderWater, 0));
    CHECK(!SwimSampleInWater(kLiquidUnderWater, 0x10));               // dark water bit alone
    CHECK(SwimSampleInWater(kLiquidUnderWater, kLiquidTypeWater | 0x10));
    CHECK(!SwimSampleInWater(kLiquidUnderWater, kLiquidTypeWater | kLiquidTypeMagma));
    CHECK(!SwimSampleInWater(kLiquidUnderWater, kLiquidTypeOcean | kLiquidTypeSlime));
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
    // The request a swim line makes now (0: no raw-id filter) finds it, and the step is kept.
    bool const inWater = SwimLineInWater(-357.5f, 283.0f, -76.5f, -362.7f, 277.3f, -77.2f, BfdPool);
    CHECK(inWater);
    CHECK(KeepPathStep(0x81, false, inWater, true));
    // The land rule stands: the same line leaving the pool is refused.
    CHECK(!SwimLineInWater(-357.5f, 283.0f, -76.5f, -330.0f, 283.0f, -76.5f, BfdPool));
}

// Codex review 2026-10-23: even all liquids (0x0F) as the request would miss a raw
// WMO liquid id with none of the low four bits set (16, 32, ...), since the vmap ANDs
// the request with the raw id. The request is 0 (no filter, in both the vmap and the
// ADT test) and the answer's type flags (LiquidType.dbc) decide: water or ocean yes,
// magma or slime no; a raw id the DBC does not know reads type flag 0x01 (magma).
struct RawPool
{
    std::uint32_t rawId = 1;
    // The middle stretch x in [midFrom, midTo] (on the line y = 283): its own raw id,
    // or dry (midDry).
    float midFrom = 0.0f, midTo = -1.0f;
    std::uint32_t midRawId = 1;
    bool midDry = false;

    static std::uint32_t Flags(std::uint32_t id)
    {
        switch (id)
        {
            case 1: case 16: case 32: return kLiquidTypeWater; // water (16, 32: ids with no low bits)
            case 2: return kLiquidTypeOcean;
            case 3: return kLiquidTypeMagma;
            case 4: return kLiquidTypeSlime;
            default: return kLiquidTypeMagma;                  // unknown to LiquidType.dbc: 1 << 0
        }
    }

    std::uint32_t operator()(float x, float y, float z, std::uint32_t request, std::uint32_t& typeFlags) const
    {
        typeFlags = 0;
        bool const mid = x >= midFrom && x <= midTo;
        if (mid && midDry)
            return kLiquidNoWater;
        std::uint32_t const id = mid ? midRawId : rawId;
        if (request && !(id & request))
            return kLiquidNoWater;                             // VMapManager2::GetLiquidLevel
        if (x > -340.0f || x < -380.0f || y < 260.0f || y > 300.0f || z < -80.5f)
            return kLiquidNoWater;
        typeFlags = Flags(id);
        return z < -58.0f ? kLiquidUnderWater : kLiquidInWater;
    }
};

static void TestRawLiquidIdsAreNotFiltered()
{
    CHECK(kSwimLiquidRequest == 0);
    // Water under raw ids 1, 16 and 32: a swim line.
    for (std::uint32_t id : {1u, 16u, 32u, 2u})
    {
        RawPool pool;
        pool.rawId = id;
        CHECK(SwimLineInWater(-375.0f, 283.0f, -76.0f, -345.0f, 283.0f, -76.0f, pool));
        // 16 and 32 had read dry under an all-liquids request (0x0F).
        std::uint32_t flags = 0;
        if (id >= 16)
            CHECK(pool(-360.0f, 283.0f, -76.0f, 0x0F, flags) == kLiquidNoWater);
    }
    // Magma or slime, by raw id, or a raw id the DBC does not know: refused.
    for (std::uint32_t id : {3u, 4u, 99u})
    {
        RawPool pool;
        pool.rawId = id;
        CHECK(!SwimLineInWater(-375.0f, 283.0f, -76.0f, -345.0f, 283.0f, -76.0f, pool));
    }
    // A water line with a magma, slime or dry middle: refused (the INCOMPLETE swim
    // step in getPathStepFrom asks this same full-line test now, not its two ends).
    for (int c = 0; c < 3; ++c)
    {
        RawPool pool;
        pool.midFrom = -361.0f;
        pool.midTo = -359.0f;
        if (c == 0)
            pool.midRawId = 3;
        else if (c == 1)
            pool.midRawId = 4;
        else
            pool.midDry = true;
        CHECK(!SwimLineInWater(-375.0f, 283.0f, -76.0f, -345.0f, 283.0f, -76.0f, pool));
        // Both ends are in water all the same (what the old ends-only check asked).
        std::uint32_t flags = 0;
        std::uint32_t const first = pool(-375.0f, 283.0f, -76.0f, 0, flags);
        CHECK(SwimSampleInWater(first, flags));
        std::uint32_t const last = pool(-345.0f, 283.0f, -76.0f, 0, flags);
        CHECK(SwimSampleInWater(last, flags));
    }
}

// Blackfathom Deeps r26 step 82: the lead bot on the pool bed, the core's swim line to a pool point
// 15.9 yd off, the bot's own route only its own spot.
static SwimHandoffFacts PoolStall()
{
    SwimHandoffFacts f;
    f.inWater = true;
    f.routeEndFromBot = 0.0f;
    f.endFromBot = 15.9f;
    f.coreType = 0x81;
    f.coreLineHitsModel = false;
    f.swimmableHere = true;
    f.endSwimmable = true;
    f.endHasGround = false;        // round 1: only the swim-out reads it (a swimmable end is the core's)
    f.endFromSurfaceYd = -20.9f;
    f.surfaceLineClear = true;
    return f;
}

static void TestPoolStallHandsTheCoreLine()
{
    SwimHandoffFacts f = PoolStall();
    CHECK(ChooseSwimHandoff(f) == SwimHandoff::CoreLine);   // fails on the stub
    f.endFromBot = 41.8f;                                    // r29 step 580
    CHECK(ChooseSwimHandoff(f) == SwimHandoff::CoreLine);
    f.endFromBot = 62.4f;                                   // Karazhan Crypt link 2 across the shaft
    CHECK(ChooseSwimHandoff(f) == SwimHandoff::CoreLine);
    f.routeEndFromBot = 0.4f;                                // a route of a few inches is no progress either
    CHECK(ChooseSwimHandoff(f) == SwimHandoff::CoreLine);
}

static void TestHandoffGuards()
{
    SwimHandoffFacts f = PoolStall();
    f.routeEndFromBot = 10.0f;  CHECK(ChooseSwimHandoff(f) == SwimHandoff::None);   // (a) the route walks
    f = PoolStall(); f.endFromBot = 1.5f;  CHECK(ChooseSwimHandoff(f) == SwimHandoff::None);   // (b) there already
    f = PoolStall(); f.inWater = false;    CHECK(ChooseSwimHandoff(f) == SwimHandoff::None);   // (c) on land
    f = PoolStall(); f.excluded = true;    CHECK(ChooseSwimHandoff(f) == SwimHandoff::None);   // (d) a real player, transport, flight
    for (std::uint32_t type : {0x91u, 0x83u, 0x85u, 0x89u, 0x08u, 0x04u, 0xC1u, 0xA1u})
    {
        f = PoolStall();
        f.coreType = type;
        f.endSwimmable = true;  // the swim-out never takes a swimmable end
        CHECK(ChooseSwimHandoff(f) == SwimHandoff::None);                              // (e) never a shortcut, partial, failed, fly, forced
    }
    // Karazhan Crypt refast slot7 step 204: mid-water the core answers 0x11
    // (no polygon in reach, swimmable start). On land it stays refused: the
    // old "never 0x11" guard is now "0x11 only when both ends are in water".
    f = PoolStall();
    f.coreType = 0x11;
    f.endSwimmable = false;
    f.endInWater = true;  // the swim-out never takes a water end
    CHECK(ChooseSwimHandoff(f) == SwimHandoff::CoreLine);   // the 0x11 shaft shortcut is a swim line
    f = PoolStall(); f.coreLineHitsModel = true;  CHECK(ChooseSwimHandoff(f) == SwimHandoff::None);   // (f) a wall on the line
    f = PoolStall(); f.endFromBot = 120.0f;       CHECK(ChooseSwimHandoff(f) == SwimHandoff::None);   // (g) a swim line over 100 yd
    f.coreType = 0x01;                            CHECK(ChooseSwimHandoff(f) == SwimHandoff::CoreLine); // a mesh path has no cap
}

static void TestKarazhanFloorToDryRampHandsTheCorePath()
{
    SwimHandoffFacts f = PoolStall();
    f.coreType = 0x01;
    f.endFromBot = 56.7f;          // -11221.3 -1560.0 -45.9 -> -11220.0 -1586.8 4.1
    f.endSwimmable = false;
    f.endFromSurfaceYd = 1.1f;     // surface 3.0
    CHECK(ChooseSwimHandoff(f) == SwimHandoff::CoreLine);
    f.coreLineHitsModel = true;    // the model test is for the straight line only
    CHECK(ChooseSwimHandoff(f) == SwimHandoff::CoreLine);
}

// Karazhan Crypt refast slot7 step 204: the lead floats at -24.1, 22 yd over
// the shaft floor. The core answers 0x11 for the floor point (no polygon in
// reach, swimmable start); the hand-off takes it as a core line only when
// both ends are in water, no model is on the line, and it is <= 100 yd.
// Otherwise it falls through to the swim-out (the Stormwrought pool-bed stall
// still needs that), and inWater alone (the old route test) is not enough.
static void TestShaftMidWaterHandsTheSwimShortcut()
{
    SwimHandoffFacts f = PoolStall();
    f.coreType = 0x11;
    f.endSwimmable = false;
    f.endInWater = true;
    f.endFromBot = 22.4f;
    f.endFromSurfaceYd = -46.0f;
    CHECK(ChooseSwimHandoff(f) == SwimHandoff::CoreLine);   // fails on the stub
    SwimHandoffFacts g = f; g.swimmableHere = false;
    CHECK(ChooseSwimHandoff(g) == SwimHandoff::None);       // on land: no shortcut
    g = f; g.endInWater = false;
    CHECK(ChooseSwimHandoff(g) == SwimHandoff::None);       // the end dry behind terrain: refused
    g = f; g.coreLineHitsModel = true;
    CHECK(ChooseSwimHandoff(g) == SwimHandoff::None);       // a model on the line
    g = f; g.endFromBot = 120.0f;
    CHECK(ChooseSwimHandoff(g) == SwimHandoff::None);       // over 100 yd
    g = f; g.inWater = false;
    CHECK(ChooseSwimHandoff(g) == SwimHandoff::None);       // the bot itself dry
    // Fall-through: 0x11 for a dry shore point with ground is the swim-out.
    g = f;
    g.endInWater = false;
    g.endSwimmable = false;
    g.endHasGround = true;
    g.endGroundYd = 10.0f;
    g.endFromSurfaceYd = 0.5f;
    g.surfaceLineClear = true;
    CHECK(ChooseSwimHandoff(g) == SwimHandoff::SwimOut);
}

static void TestPoolBedSwimsOutToTheApron()
{
    SwimHandoffFacts f = PoolStall();
    f.coreType = 0x04;
    f.routeEndFromBot = 0.4f;
    f.endFromBot = 13.2f;
    f.endSwimmable = false;
    f.endHasGround = true;             // the apron: floor under the end
    f.endGroundYd = 8.2f;
    f.endFromSurfaceYd = 0.1f;
    f.surfaceLineClear = true;
    CHECK(ChooseSwimHandoff(f) == SwimHandoff::SwimOut);       // fails on the stub
    f.coreType = 0x08;                                          // no polygon at all: the same
    CHECK(ChooseSwimHandoff(f) == SwimHandoff::SwimOut);
    SwimHandoffFacts g = f; g.endFromSurfaceYd = 10.0f;    CHECK(ChooseSwimHandoff(g) == SwimHandoff::None);   // a ledge above the water
    g = f; g.endFromSurfaceYd = -8.0f;                     CHECK(ChooseSwimHandoff(g) == SwimHandoff::None);   // a point down on the bed
    g = f; g.endGroundYd = 45.0f;                          CHECK(ChooseSwimHandoff(g) == SwimHandoff::None);   // too far out
    g = f; g.surfaceLineClear = false;                     CHECK(ChooseSwimHandoff(g) == SwimHandoff::None);   // a wall in the way
    g = f; g.endSwimmable = true;                          CHECK(ChooseSwimHandoff(g) == SwimHandoff::None);   // a swimmable end is the core's
    g = f; g.swimmableHere = false;                        CHECK(ChooseSwimHandoff(g) == SwimHandoff::None);   // shallow: wade, not swim
    g = f; g.endHasGround = false;                         CHECK(ChooseSwimHandoff(g) == SwimHandoff::None);   // round 1: no floor under the end
}

// Round 1 (Codex + Opus 2026-10-25): a curved 0x1 path through magma whose chord is clear is still
// refused. The glue guards path.getPath(), not the bot->end chord; this pins the guard's input: the
// walked path's own points, not the two ends. Pure shape: the route bends through a magma basin the
// straight bot->end chord misses.
static void TestCurvedPathThroughMagmaIsRefused()
{
    MagmaPoint const start{ 0.0f, 0.0f, 0.0f };
    std::vector<MagmaPoint> const chord{ MagmaPoint{ 30.0f, 10.0f, 0.0f } };
    std::vector<MagmaPoint> const walked{ MagmaPoint{ 15.0f, -5.0f, 0.0f }, MagmaPoint{ 30.0f, 10.0f, 0.0f } };
    auto const inMagma = [](MagmaPoint const& m) { return m.x >= 14.0f && m.x <= 16.0f && m.y < 0.0f; };
    CHECK(!CutPathBeforeMagma(start, chord, inMagma).cut);   // the chord misses the basin
    CHECK(CutPathBeforeMagma(start, walked, inMagma).cut);   // the walked path crosses it
}

// Round 1: a 1-point route far away is not "no progress" (the old `route.size() < 2 ? 0` read it as
// a stall). The glue now measures progress as the last point's distance; a 2.5 yd leg walks.
static void TestSinglePointRouteFarAwayIsProgress()
{
    SwimHandoffFacts f = PoolStall();
    f.routeEndFromBot = 2.5f;
    CHECK(ChooseSwimHandoff(f) == SwimHandoff::None);
}

// Round 1: the swim-out needs a floor within 1 yd below its end (Eldermaw's apron has one; a point
// over the drop does not).
static void TestSwimOutNeedsGroundUnderTheEnd()
{
    SwimHandoffFacts f = PoolStall();
    f.coreType = 0x04;
    f.routeEndFromBot = 0.4f;
    f.endFromBot = 13.2f;
    f.endSwimmable = false;
    f.endHasGround = true;
    f.endGroundYd = 8.2f;
    f.endFromSurfaceYd = 0.1f;
    f.surfaceLineClear = true;
    CHECK(ChooseSwimHandoff(f) == SwimHandoff::SwimOut);
    SwimHandoffFacts g = f; g.endHasGround = false;
    CHECK(ChooseSwimHandoff(g) == SwimHandoff::None);
}

// Round 2 (Codex HIGH 2026-10-25): the slime guard looked only at the route's
// WAYPOINTS, so a leg between two safe points could cross a slime pool. Now
// one sampled liquid-hazard guard treats slime like magma on the hand-off
// routes (0x81, 0x1, swim-out): the same CutPathBeforeMagma sampling with the
// combined predicate. Pure shape on a fake slime strip (x = 10..20).
static void TestSlimeHazardNeedsSlimeAndLiquid()
{
    CHECK(IsSlimeHazard(kLiquidTypeSlime, kLiquidInWater));
    CHECK(IsSlimeHazard(kLiquidTypeSlime, kLiquidUnderWater));
    CHECK(IsSlimeHazard(kLiquidTypeSlime, kLiquidWaterWalk));
    CHECK(!IsSlimeHazard(kLiquidTypeSlime, kLiquidAboveWater));
    CHECK(!IsSlimeHazard(kLiquidTypeSlime, kLiquidNoWater));
    CHECK(!IsSlimeHazard(kLiquidTypeWater, kLiquidInWater));
    CHECK(!IsSlimeHazard(kLiquidTypeMagma, kLiquidInWater));
    CHECK(ClassifyLiquidHazard(kLiquidTypeSlime, kLiquidInWater, true) == LiquidHazard::Slime);
    CHECK(ClassifyLiquidHazard(kLiquidTypeSlime, kLiquidInWater, false) == LiquidHazard::None);
    CHECK(ClassifyLiquidHazard(kLiquidTypeMagma, kLiquidInWater, true) == LiquidHazard::Magma);
    CHECK(ClassifyLiquidHazard(kLiquidTypeMagma, kLiquidInWater, false) == LiquidHazard::Magma);
    CHECK(ClassifyLiquidHazard(kLiquidTypeWater, kLiquidInWater, true) == LiquidHazard::None);
}

static void TestSlimeCrossingIsRefused()
{
    // A slime pool from x = 10 to x = 20; the hand-off's combined predicate.
    auto const inHazard = [](MagmaPoint const& m)
    {
        std::uint32_t const flags = (m.x >= 10.0f && m.x <= 20.0f) ? kLiquidTypeSlime : 0;
        std::uint32_t const status = flags ? kLiquidInWater : kLiquidNoWater;
        return ClassifyLiquidHazard(flags, status, true) != LiquidHazard::None;
    };
    MagmaPoint const start{ 0.0f, 0.0f, 0.0f };
    // The 0x81 line: start and end safe, the single leg across the pool.
    CHECK(CutPathBeforeMagma(start, { MagmaPoint{ 30.0f, 0.0f, 0.0f } }, inHazard).cut);
    // The 0x1 path: two safe waypoints, the leg between them across the pool.
    CHECK(CutPathBeforeMagma(start, { MagmaPoint{ 4.0f, 0.0f, 0.0f }, MagmaPoint{ 30.0f, 0.0f, 0.0f } },
        inHazard).cut);
    // The swim-out legs: up to just under the surface, then straight to the end.
    MagmaPoint const bed{ 0.0f, 0.0f, -5.0f };
    CHECK(CutPathBeforeMagma(bed, { MagmaPoint{ 0.0f, 0.0f, -0.5f }, MagmaPoint{ 30.0f, 0.0f, 0.0f } },
        inHazard).cut);
    // A clean leg stays allowed.
    CHECK(!CutPathBeforeMagma(start, { MagmaPoint{ 8.0f, 0.0f, 0.0f } }, inHazard).cut);
    // Ordinary moves are unchanged: with slime off the same crossings pass.
    auto const inMagmaOnly = [](MagmaPoint const& m)
    {
        std::uint32_t const flags = (m.x >= 10.0f && m.x <= 20.0f) ? kLiquidTypeSlime : 0;
        std::uint32_t const status = flags ? kLiquidInWater : kLiquidNoWater;
        return ClassifyLiquidHazard(flags, status, false) != LiquidHazard::None;
    };
    CHECK(!CutPathBeforeMagma(start, { MagmaPoint{ 30.0f, 0.0f, 0.0f } }, inMagmaOnly).cut);
}

// Round 3 (Codex HIGH 2026-10-25): CutPathBeforeMagma spends one route-wide
// budget of kMagmaMaxInteriorSamples interior samples (~96 yd); past it later
// legs are checked only at their endpoints, so slime between two safe late
// waypoints is missed. A swim hand-off route longer than the budget covers at
// full density is refused instead (the bot keeps the old behaviour); a short
// clean route stays allowed. Pure shape on a fake slime strip (x = 120..130).
static void TestLongHandoffRouteIsRefused()
{
    CHECK(kMagmaFullSampleCoveredYd == kMagmaMaxInteriorSamples * kMagmaSampleStepYd);
    CHECK(kMagmaFullSampleCoveredYd == 96.0f);
    auto const inHazard = [](MagmaPoint const& m)
    {
        std::uint32_t const flags = (m.x >= 120.0f && m.x <= 130.0f) ? kLiquidTypeSlime : 0;
        std::uint32_t const status = flags ? kLiquidInWater : kLiquidNoWater;
        return ClassifyLiquidHazard(flags, status, true) != LiquidHazard::None;
    };
    MagmaPoint const start{ 0.0f, 0.0f, 0.0f };
    // A multi-leg 0x1 path: three 50 yd legs (150 yd total); the first two
    // spend all 48 samples (to x = 98), so the last leg sees only its
    // endpoint (150) and the slime between the safe 100 and 150 is missed.
    std::vector<MagmaPoint> const legs{ MagmaPoint{ 50.0f, 0.0f, 0.0f }, MagmaPoint{ 100.0f, 0.0f, 0.0f },
        MagmaPoint{ 150.0f, 0.0f, 0.0f } };
    CHECK(!CutPathBeforeMagma(start, legs, inHazard).cut); // the hole: past the budget only endpoints are seen
}

static void TestFirstFailingSampleIsNamed()
{
    std::uint32_t status = 99, flags = 99;
    float sx = 0.0f, sy = 0.0f, sz = 0.0f;
    // The ridge at x 18-20 (Pool): 15 samples 2 yd apart from x 2, the first dry one is x 18, index 8.
    CHECK(SwimLineFirstFailing(2.0f, 0.0f, -5.0f, 30.0f, 0.0f, -5.0f, Water(Pool), status, flags, sx, sy, sz) == 8);
    CHECK(status == kLiquidNoWater && std::fabs(sx - 18.0f) < 0.01f);
    // A dry start is sample 0.
    CHECK(SwimLineFirstFailing(-5.0f, 0.0f, -5.0f, 10.0f, 0.0f, -5.0f, Water(Pool), status, flags, sx, sy, sz) == 0);
    // All in water, and too long.
    CHECK(SwimLineFirstFailing(2.0f, 0.0f, -9.5f, 15.9f, 0.0f, -9.6f, Water(OpenPool), status, flags, sx, sy, sz) == -1);
    CHECK(SwimLineFirstFailing(0.0f, 0.0f, -5.0f, 90.0f, 0.0f, -5.0f, Water(OpenPool), status, flags, sx, sy, sz) == -2);
    // SwimLineInWater is the same verdict.
    CHECK(!SwimLineInWater(2.0f, 0.0f, -5.0f, 30.0f, 0.0f, -5.0f, Water(Pool)));
    CHECK(SwimLineInWater(2.0f, 0.0f, -9.5f, 15.9f, 0.0f, -9.6f, Water(OpenPool)));
}

static void TestSwimOutRises()
{
    CHECK(SwimOutNeedsRise(191.9f, 202.2f));      // the bed: up first
    CHECK(!SwimOutNeedsRise(201.5f, 202.2f));     // already at 0.7 yd under the surface
    CHECK(!SwimOutNeedsRise(200.8f, 202.2f));     // 0.9 yd below the rise point: no rise
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
    TestRawLiquidIdsAreNotFiltered();
    TestPoolStallHandsTheCoreLine();
    TestHandoffGuards();
    TestKarazhanFloorToDryRampHandsTheCorePath();
    TestShaftMidWaterHandsTheSwimShortcut();
    TestCurvedPathThroughMagmaIsRefused();
    TestSinglePointRouteFarAwayIsProgress();
    TestSwimOutNeedsGroundUnderTheEnd();
    TestSlimeHazardNeedsSlimeAndLiquid();
    TestSlimeCrossingIsRefused();
    TestLongHandoffRouteIsRefused();
    TestFirstFailingSampleIsNamed();
    TestSwimOutRises();
    std::cout << "swim path policy: all tests passed\n";
    return 0;
}
