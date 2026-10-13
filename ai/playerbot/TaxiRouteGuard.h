#pragma once

#include "playerbot/TaxiPathPolicy.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "ObjectMgr.h"

#include <vector>

namespace ai
{
    // True when every leg of a bot's flight (consecutive taxi nodes) uses a
    // path the bot may fly (TaxiPathPolicy.h). Checked before each
    // Player::ActivateTaxiPathTo a bot starts.
    inline bool BotTaxiRouteAllowed(std::vector<uint32> const& nodes)
    {
        for (size_t i = 1; i < nodes.size(); ++i)
        {
            uint32 path = 0;
            uint32 cost = 0;
            sObjectMgr.GetTaxiPath(nodes[i - 1], nodes[i], path, cost);
            if (!TaxiPathAllowed(path, sPlayerbotAIConfig.allowRepairedTaxiPaths))
                return false;
        }
        return true;
    }
}
