#pragma once
#include <cmath>
#include <cstdint>

// Pure policy: which core PathFinder answers a bot's own route keeps
// (WorldPosition::getPathStepFrom), and the water test a straight swim line
// must pass before it is kept.
//
// The core answers a swimming unit whose start and end lie in deep water with
// a straight two-point path instead of a navmesh path (PathFinder.cpp
// BuildPolyPath: BuildUnderwaterPath, type NORMAL|UNDERWATER = 0x81; a start or
// end with no polygon in swimmable water: NORMAL|NOT_USING_PATH = 0x11). The
// core's own movement (follow, MovePoint with MOVE_PATHFINDING) walks that
// path. getPathStepFrom kept only type == NORMAL (or INCOMPLETE), so every such
// answer came back empty and the bot's move failed (dispatch-short) every 2 s,
// while `cmd move`'s own check (RemoteCommander::CheckMove, bit tests) had
// acked it: Blackfathom Deeps 2026-10-22/23, 6 of 6 "lead bot is stuck" pauses
// began with the lead bot in a deep pool (BFD-STUCK-POINTS-2026-10-23).
//
// The same NOT_USING_PATH bit also marks the core's straight line on land (a
// map or tile with no mmaps, UNIT_STAT_IGNORE_PATHFINDING, the NoMovementAreas
// list): that line goes through walls. So a straight answer is kept only when
// the whole line lies in water: every sample along it (start and end included,
// at most kSwimSampleYd apart) is in water or ocean at or below the surface
// (getLiquidStatus IN_WATER, UNDER_WATER or WATER_WALK; it answers NO_WATER for
// a point more than 2 yd under the floor, or where the floor rises above the
// water), and the line is in sight (vmaps and dynamic objects). A line that
// crosses land, a wall, a closed door, magma or slime is refused, as before.
// A bot in water whose own route goes nowhere hands the move to the core instead (ChooseSwimHandoff, below).
namespace ai
{
    // The core's PathType bits (src/game/Maps/PathFinder.h), mirrored so this
    // header builds on its own; WorldPosition.cpp static_asserts them.
    constexpr std::uint32_t kPathTypeNormal = 0x0001;
    constexpr std::uint32_t kPathTypeShortcut = 0x0002;
    constexpr std::uint32_t kPathTypeIncomplete = 0x0004;
    constexpr std::uint32_t kPathTypeNoPath = 0x0008;
    constexpr std::uint32_t kPathTypeNotUsingPath = 0x0010;
    constexpr std::uint32_t kPathTypeDestForced = 0x0020;
    constexpr std::uint32_t kPathTypeFlyPath = 0x0040;
    constexpr std::uint32_t kPathTypeUnderwater = 0x0080;

    // The core's GridMapLiquidStatus values (GridMapDefines.h), mirrored too.
    constexpr std::uint32_t kLiquidNoWater = 0x0;
    constexpr std::uint32_t kLiquidAboveWater = 0x1;
    constexpr std::uint32_t kLiquidWaterWalk = 0x2;
    constexpr std::uint32_t kLiquidInWater = 0x4;
    constexpr std::uint32_t kLiquidUnderWater = 0x8;

    // The core's liquid type flags (GridMapDefines.h MAP_LIQUID_TYPE_*), mirrored.
    constexpr std::uint32_t kLiquidTypeMagma = 0x01;
    constexpr std::uint32_t kLiquidTypeOcean = 0x02;
    constexpr std::uint32_t kLiquidTypeSlime = 0x04;
    constexpr std::uint32_t kLiquidTypeWater = 0x08;
    // A swim line asks the core for any liquid with no type filter (request 0)
    // and decides water against magma and slime by the answer's type flags
    // (GridMapLiquidData::type_flags, from LiquidType.dbc), as
    // Player::UpdateTerainEnvironmentFlags reads them. A filtered request is
    // unusable: the vmap test VMapManager2::GetLiquidLevel ANDs the request with
    // the WMO's raw liquid type id, not a flag (`if (ReqLiquidType && !(type &
    // ReqLiquidType)) return false;`; the ADT test, GridMap::getLiquidStatus,
    // skips its filter for 0 too). Every Blackfathom Deeps WMO liquid has raw id
    // 1, so water | ocean (0x0A) found no water there at all (r26-19z44e, step
    // 82), and even all liquids (0x0F) would miss a raw id with none of the low
    // four bits set (16, 32, ...). Codex review 2026-10-23.
    constexpr std::uint32_t kSwimLiquidRequest = 0;

    // Samples along a swim line at most this far apart, always: a line longer
    // than kSwimMaxLineYd is refused rather than sampled more sparsely (a capped
    // count let a dry strip narrower than the stretched spacing hide between two
    // samples; Codex review 2026-10-23). The core's next try then gives a shorter
    // step, or the move keeps the navmesh path.
    constexpr float kSwimSampleYd = 2.0f;
    constexpr float kSwimMaxLineYd = 80.0f;

    enum class PathStepVerdict
    {
        Keep,           // a navmesh path: NORMAL, or INCOMPLETE when a partial path is allowed
        KeepIfSwimLine, // the core's straight swim answer: kept only when SwimLineInWater passes
        Refuse,         // NOPATH, a shortcut, a fly path, or a partial path where a complete one is wanted
    };

    inline PathStepVerdict ClassifyPathStepType(std::uint32_t type, bool forceNormalPath)
    {
        if (type == kPathTypeNormal)
            return PathStepVerdict::Keep;
        if (type == kPathTypeIncomplete)
            return forceNormalPath ? PathStepVerdict::Refuse : PathStepVerdict::Keep;
        if (!(type & kPathTypeNormal))
            return PathStepVerdict::Refuse;
        // Only NORMAL with UNDERWATER and/or NOT_USING_PATH is a swim line. Any
        // other bit is refused: a shortcut through terrain (BuildUnderwaterPath's
        // NORMAL|SHORTCUT when the end is dry), a partial or failed path, a
        // forced end, a flying unit's line, or a bit this rule does not know.
        if (type & ~(kPathTypeNormal | kPathTypeUnderwater | kPathTypeNotUsingPath))
            return PathStepVerdict::Refuse;
        return PathStepVerdict::KeepIfSwimLine;
    }

    // One sample of a swim line: in water at or below the surface.
    // One sample of a swim line: in water at or below the surface, and the
    // liquid there neither magma nor slime (typeFlags: the answer's type flags,
    // read only when the status is not NO_WATER).
    inline bool SwimSampleInWater(std::uint32_t liquidStatus, std::uint32_t typeFlags)
    {
        if (liquidStatus != kLiquidInWater && liquidStatus != kLiquidUnderWater && liquidStatus != kLiquidWaterWalk)
            return false;
        // Water or ocean, and neither magma nor slime (a raw id LiquidType.dbc does
        // not know reads type flags 0x01, magma, and is refused).
        return (typeFlags & (kLiquidTypeWater | kLiquidTypeOcean)) && !(typeFlags & (kLiquidTypeMagma | kLiquidTypeSlime));
    }

    // How many samples a line of `length` yd takes (both ends included), at most
    // kSwimSampleYd apart.
    inline std::uint32_t SwimSampleCount(float length)
    {
        if (!(length > 0.0f))
            return 1;
        return static_cast<std::uint32_t>(std::ceil(length / kSwimSampleYd)) + 1;
    }

    // The first sample of the straight line a -> b that is not in water: its index (0 at a), and its
    // liquid status, type flags and spot in the out-parameters; -1 when every sample is in water, -2 for
    // a line longer than kSwimMaxLineYd. liquidAt(x, y, z, request, typeFlags) answers the core's liquid
    // status at a point for the liquid types `request` (always kSwimLiquidRequest) and sets the
    // answer's type flags.
    template <class LiquidAt>
    inline int SwimLineFirstFailing(float ax, float ay, float az, float bx, float by, float bz, LiquidAt&& liquidAt,
        std::uint32_t& status, std::uint32_t& typeFlags, float& sx, float& sy, float& sz)
    {
        float const dx = bx - ax, dy = by - ay, dz = bz - az;
        float const length = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (!(length <= kSwimMaxLineYd))
            return -2;
        std::uint32_t const samples = SwimSampleCount(length);
        for (std::uint32_t i = 0; i < samples; ++i)
        {
            float const t = samples == 1 ? 0.0f : static_cast<float>(i) / static_cast<float>(samples - 1);
            sx = ax + dx * t;
            sy = ay + dy * t;
            sz = az + dz * t;
            typeFlags = 0;
            status = liquidAt(sx, sy, sz, kSwimLiquidRequest, typeFlags);
            if (!SwimSampleInWater(status, typeFlags))
                return static_cast<int>(i);
        }
        return -1;
    }

    // Whether the straight line a -> b lies in water: every sample in water, at most kSwimMaxLineYd long.
    template <class LiquidAt>
    inline bool SwimLineInWater(float ax, float ay, float az, float bx, float by, float bz, LiquidAt&& liquidAt)
    {
        std::uint32_t status = 0, typeFlags = 0;
        float sx = 0.0f, sy = 0.0f, sz = 0.0f;
        return SwimLineFirstFailing(ax, ay, az, bx, by, bz, liquidAt, status, typeFlags, sx, sy, sz) == -1;
    }

    // The whole verdict for one path step: Keep, or KeepIfSwimLine with the
    // line in water and in sight.
    inline bool KeepPathStep(std::uint32_t type, bool forceNormalPath, bool swimLineInWater, bool swimLineInSight)
    {
        switch (ClassifyPathStepType(type, forceNormalPath))
        {
            case PathStepVerdict::Keep: return true;
            case PathStepVerdict::KeepIfSwimLine: return swimLineInWater && swimLineInSight;
            default: return false;
        }
    }

    // MoveTo2's swim hand-off (MovementActions.cpp MovementAction::SwimHandoff). Blackfathom Deeps r26
    // step 82 and r29 step 580 (2026-10-23/24): a lead bot in a deep pool got the core's straight swim
    // line (0x81) for a pool point, the route's own swim-line test (SwimPathFits) refused it, the route
    // was the bot's own spot and the move failed (dispatch-short) every 2 s although the core's own
    // movement walks that line (its follow did, BFD 6b). Karazhan Crypt r40 step 113 (2026-10-25): the
    // lead on the flooded shaft's floor stood at the dive link's lower end while the core had a complete
    // path up it. Stormwrought Ruins r41 steps 717/719: the lead on Eldermaw's pool bed, cut off from the
    // shore, had no path at all to the apron at the waterline. Now a bot in water whose own route makes no
    // progress (fewer than 2 points, or ending within kSwimNoProgressYd of it) hands the move to the
    // core when the core answers a complete path (0x1) or its swim line (0x81, no static model on it, at
    // most kSwimHandoffMaxLineYd), or swims up and out to a shore point at the waterline within
    // kSwimOutMaxGroundYd. Never a real player's character, a transport, a flight; never 0x11 (the
    // core's straight line on land), a shortcut, a partial or failed path.
    constexpr float kSwimNoProgressYd = 2.0f;
    constexpr float kSwimHandoffMaxLineYd = 100.0f;
    constexpr float kSwimOutMaxGroundYd = 30.0f;
    constexpr float kSwimOutSurfaceBandYd = 3.0f;
    constexpr float kSwimOutSurfaceDepthYd = 0.5f;

    enum class SwimHandoff
    {
        None,     // the bot's own move as before
        CoreLine, // MovePoint with pathfinding: the core plans and walks it
        SwimOut,  // up to just under the surface, then straight to the end
    };

    struct SwimHandoffFacts
    {
        bool inWater = false;            // IsInWater || IsUnderwater || swimming
        bool excluded = false;           // a real player's character, a transport, flying, a taxi, another map
        float routeEndFromBot = 0.0f;    // the bot's own route: its last point to the bot (0: fewer than 2 points)
        float endFromBot = 0.0f;         // the move's end to the bot (3D)
        std::uint32_t coreType = 0;      // PathFinder(bot) to the end: getPathType
        bool coreLineHitsModel = false;  // a static model on the line bot -> the core's actual end
        bool swimmableHere = false;      // the core's IsSwimmable at the bot's spot
        bool endSwimmable = false;       // the core's IsSwimmable at the end
        float endGroundYd = 0.0f;        // bot -> end on the ground plane
        float endFromSurfaceYd = 0.0f;   // end z minus the water level over the bot
        bool surfaceLineClear = false;   // bot -> the rise point and the rise point -> the end, both in sight
    };

    inline SwimHandoff ChooseSwimHandoff(SwimHandoffFacts const& f)
    {
        if (!f.inWater || f.excluded)
            return SwimHandoff::None;
        if (!(f.endFromBot > kSwimNoProgressYd) || f.routeEndFromBot > kSwimNoProgressYd)
            return SwimHandoff::None;
        if (f.coreType == kPathTypeNormal)
            return SwimHandoff::CoreLine;
        if (f.coreType == (kPathTypeNormal | kPathTypeUnderwater))
            return !f.coreLineHitsModel && f.endFromBot <= kSwimHandoffMaxLineYd ? SwimHandoff::CoreLine
                                                                                  : SwimHandoff::None;
        if (f.swimmableHere && !f.endSwimmable && f.endGroundYd <= kSwimOutMaxGroundYd &&
            std::fabs(f.endFromSurfaceYd) <= kSwimOutSurfaceBandYd && f.surfaceLineClear)
            return SwimHandoff::SwimOut;
        return SwimHandoff::None;
    }

    // Whether the swim-out rises first: the bot more than 1 yd below kSwimOutSurfaceDepthYd under the level.
    inline bool SwimOutNeedsRise(float botZ, float waterLevel)
    {
        return (waterLevel - kSwimOutSurfaceDepthYd) - botZ > 1.0f;
    }
}
