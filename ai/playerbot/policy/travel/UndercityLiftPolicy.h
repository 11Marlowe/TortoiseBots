#pragma once

#include <cstdint>

// Pure policy for the Undercity lifts. The city lies under the Ruins of
// Lordaeron and the only ways in are the throne-room elevators (and the
// sewer canals). Pool bots routed into or out of the city stood in the
// throne room 115 yd above the auctioneers: the walk "succeeded" every tick
// without the bot moving, so the trip timed out after ~35 s and the bot
// re-picked it (live 2026-10-12: 14-19 timed-out trips per 10 min, one bot
// eight in a row). mod-playerbots has no lift handling either; it teleports a
// bot that makes no progress toward a far destination (NewRpgBaseAction
// MoveFarTo). Here the lift ride is replaced by a teleport between the two
// levels; the world-facing guards (pool bot, nobody watching) live in
// MoveToTravelTargetAction.

namespace ai
{
    inline constexpr uint32_t kUndercityMapId = 0;

    // 2D footprint of the underground city (creature spawns x 1246-1814,
    // y 20-479, z -85..-23) and the two levels inside it: the city floors
    // below, the ruins and throne room (z 55-69) above.
    inline bool UndercityFootprint(float x, float y)
    {
        return x > 1230.0f && x < 1830.0f && y > 0.0f && y < 500.0f;
    }

    inline bool InUndercity(uint32_t mapId, float x, float y, float z)
    {
        return mapId == kUndercityMapId && UndercityFootprint(x, y) && z < -15.0f;
    }

    inline bool AboveUndercity(uint32_t mapId, float x, float y, float z)
    {
        return mapId == kUndercityMapId && UndercityFootprint(x, y) && z > 20.0f;
    }

    enum class UndercityLift
    {
        None,
        Down, // bot in the ruins, destination in the city below
        Up,   // bot in the city, destination outside it
    };

    inline UndercityLift UndercityLiftFor(uint32_t botMap, float bx, float by, float bz,
        uint32_t goalMap, float gx, float gy, float gz)
    {
        bool const goalInCity = InUndercity(goalMap, gx, gy, gz);
        if (goalInCity && AboveUndercity(botMap, bx, by, bz))
            return UndercityLift::Down;
        if (!goalInCity && InUndercity(botMap, bx, by, bz))
            return UndercityLift::Up;
        return UndercityLift::None;
    }

    // Landing for the way up: a spot in the throne room where bots stood on
    // the mesh (live 2026-10-12), from where every route out is a plain walk.
    inline constexpr float kUndercityUpX = 1567.2f;
    inline constexpr float kUndercityUpY = 240.6f;
    inline constexpr float kUndercityUpZ = 57.6f;
}
