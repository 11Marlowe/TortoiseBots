#pragma once

#include <cstdint>

namespace ai
{
    // Taxi paths whose nodes in the shipped 1.18.1 TaxiPathNode.dbc are
    // incomplete. Flying one crashes the world server: the core's
    // FlightPathMovementGenerator reads the missing node slot.
    //   274  Filming -> zeppelin target: no nodes at all (dev path, unused).
    //   355  Shimmerstar Lake -> Valormok: node slots 0-2 missing.
    // A core with the taxi path repair drops 274 and rebuilds 355 from its
    // reverse path 356 at startup; only then may bots fly them
    // (AiPlayerbot.AllowRepairedTaxiPaths = 1).
    inline bool TaxiPathNeedsCoreRepair(std::uint32_t pathId)
    {
        return pathId == 274 || pathId == 355;
    }

    inline bool TaxiPathAllowed(std::uint32_t pathId, bool allowRepairedPaths)
    {
        return allowRepairedPaths || !TaxiPathNeedsCoreRepair(pathId);
    }
}
