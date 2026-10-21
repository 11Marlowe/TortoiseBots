#pragma once

// Magma guard for bot movement (RFC r3-19z20, step 22: the lead tank's idle
// "formation station" walk ended in the trogg-cave lava basin and the party
// followed it in; 5 deaths). The core hurts a player standing in magma with a
// flat 605-610 fire tick (Player::UpdateTerainEnvironmentFlags sets
// ENVIRONMENT_FLAG_IN_MAGMA, the mirror timer calls
// EnvironmentalDamage(DAMAGE_LAVA, EnvironmentalDamage.Min..Max)), which kills
// a level-16 bot on its first or second tick. The navmesh calls a shallow
// basin floor plain ground, so the path finder walks straight in.
//
// This header owns the pure rules, MovementAction::MoveTo2 and the formation
// station trigger own the terrain query, so the rules stay testable in
// tools/test_magma_hazard_policy.cpp. No core includes: the core's liquid
// values are mirrored below and pinned with static_asserts where the core
// headers are in scope (MovementActions.cpp).
//
// Magma only. Slime does no damage in this core (Player.cpp, the DAMAGE_SLIME
// tick is commented out), and refusing it would strand bots in Undercity.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ai
{
    // GridMapDefines.h: MAP_LIQUID_TYPE_MAGMA (a type_flags bit).
    constexpr std::uint32_t kMagmaLiquidTypeFlag = 0x01;
    // GridMapDefines.h GridMapLiquidStatus: LIQUID_MAP_WATER_WALK,
    // LIQUID_MAP_IN_WATER, LIQUID_MAP_UNDER_WATER.
    constexpr std::uint32_t kLiquidStatusWaterWalk = 0x02;
    constexpr std::uint32_t kLiquidStatusInWater = 0x04;
    constexpr std::uint32_t kLiquidStatusUnderWater = 0x08;

    // Spacing (yards) of the samples taken between two path points: the
    // magma lip in Ragefire Chasm is a few yards wide, a path corner can sit
    // either side of it.
    constexpr float kMagmaSampleStepYd = 2.0f;

    // The core's rule for "in magma" (Player::UpdateTerainEnvironmentFlags):
    // the liquid there is magma and the point is under it, in it, or just on
    // its surface. `typeFlags` is GridMapLiquidData::type_flags, `status` the
    // GridMapLiquidStatus that getLiquidStatus returned.
    inline bool IsMagmaHazard(std::uint32_t typeFlags, std::uint32_t status)
    {
        if (!(typeFlags & kMagmaLiquidTypeFlag))
            return false;
        return (status & (kLiquidStatusUnderWater | kLiquidStatusInWater | kLiquidStatusWaterWalk)) != 0;
    }

    // Where the guard applies: bots on an instance map (dungeon or raid,
    // Map::IsDungeon). The movement layer runs for every bot on the server;
    // the open world keeps its old movement (no added terrain queries for the
    // random population, no refused grind or quest targets).
    inline bool MagmaGuardApplies(bool mapIsInstance)
    {
        return mapIsInstance;
    }

    struct MagmaPoint
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
    };

    // The verdict on a route: keep the first `keepPoints` path points, then
    // (when `addPoint`) the extra point `point`, the last safe sample between
    // two path points. `cut` false: the route stays as it is.
    struct MagmaPathCut
    {
        bool cut = false;
        std::size_t keepPoints = 0;
        bool addPoint = false;
        MagmaPoint point;
    };

    // How many samples lie strictly between two path points `length` yards
    // apart, at most kMagmaSampleStepYd apart.
    inline std::size_t MagmaInteriorSamples(float length)
    {
        if (!(length > kMagmaSampleStepYd))
            return 0;
        return static_cast<std::size_t>(std::ceil(length / kMagmaSampleStepYd)) - 1;
    }

    // The route from `start` (the bot) along `path`, sampled at each path
    // point and every kMagmaSampleStepYd between them, ends before its first
    // magma sample. A bot already standing in magma walks out first: the
    // leading run of magma samples is not a cut (only magma after dry ground
    // is). A route that never leaves magma is left alone (nothing safe to stop
    // at; the destination check refuses a magma end point before this runs).
    template <typename InMagma>
    MagmaPathCut CutPathBeforeMagma(MagmaPoint const& start, std::vector<MagmaPoint> const& path, InMagma inMagma)
    {
        MagmaPathCut result;
        bool dry = !inMagma(start);
        MagmaPoint lastSafe = start;
        MagmaPoint prev = start;
        for (std::size_t i = 0; i < path.size(); ++i)
        {
            MagmaPoint const& next = path[i];
            float const dx = next.x - prev.x, dy = next.y - prev.y, dz = next.z - prev.z;
            float const length = std::sqrt(dx * dx + dy * dy + dz * dz);
            std::size_t const interior = MagmaInteriorSamples(length);
            for (std::size_t k = 0; k <= interior; ++k)
            {
                bool const atPoint = k == interior;
                MagmaPoint sample = next;
                if (!atPoint)
                {
                    float const t = static_cast<float>(k + 1) / static_cast<float>(interior + 1);
                    sample.x = prev.x + dx * t;
                    sample.y = prev.y + dy * t;
                    sample.z = prev.z + dz * t;
                }
                if (inMagma(sample))
                {
                    if (dry)
                    {
                        result.cut = true;
                        result.keepPoints = i;
                        // The last safe sample is a path point already kept
                        // (or the bot's own spot) unless it lies inside this
                        // segment.
                        result.addPoint = k > 0;
                        result.point = lastSafe;
                        return result;
                    }
                    continue;
                }
                dry = true;
                lastSafe = sample;
            }
            prev = next;
        }
        return result;
    }
}
