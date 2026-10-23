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

    // Samples along a swim line at most this far apart, and never more than
    // kSwimMaxSamples of them (a longer line samples more sparsely).
    constexpr float kSwimSampleYd = 2.0f;
    constexpr std::uint32_t kSwimMaxSamples = 40;

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
    inline bool SwimSampleInWater(std::uint32_t liquidStatus)
    {
        return liquidStatus == kLiquidInWater || liquidStatus == kLiquidUnderWater || liquidStatus == kLiquidWaterWalk;
    }

    // How many samples a line of `length` yd takes (both ends included).
    inline std::uint32_t SwimSampleCount(float length)
    {
        if (!(length > 0.0f))
            return 1;
        std::uint32_t const steps = static_cast<std::uint32_t>(std::ceil(length / kSwimSampleYd));
        std::uint32_t const capped = steps < kSwimMaxSamples - 1 ? steps : kSwimMaxSamples - 1;
        return capped + 1;
    }

    // Whether the straight line a -> b lies in water: liquidAt(x, y, z) answers
    // the core's liquid status at a point; every sample must be in water.
    template <class LiquidAt>
    inline bool SwimLineInWater(float ax, float ay, float az, float bx, float by, float bz, LiquidAt&& liquidAt)
    {
        float const dx = bx - ax, dy = by - ay, dz = bz - az;
        std::uint32_t const samples = SwimSampleCount(std::sqrt(dx * dx + dy * dy + dz * dz));
        for (std::uint32_t i = 0; i < samples; ++i)
        {
            float const t = samples == 1 ? 0.0f : static_cast<float>(i) / static_cast<float>(samples - 1);
            if (!SwimSampleInWater(liquidAt(ax + dx * t, ay + dy * t, az + dz * t)))
                return false;
        }
        return true;
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
}
