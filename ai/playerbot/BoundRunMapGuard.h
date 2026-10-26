#pragma once
// A bot of a bound party whose run is running or paused never leaves the run plan's map by its own move
// (pip S2 Deadmines 2026-10-26: a player's ghost on map 0 made the bots' follow ping-pong through the portal).
#include <cstdint>
namespace ai {
// runPlanMap: the plan's map of the bot's running or paused run, 0 = no such run.
inline bool BoundRunRefusesMapChange(uint32_t botMap, uint32_t destMap, uint32_t runPlanMap)
{
    return runPlanMap != 0 && botMap == runPlanMap && destMap != botMap;
}
}
