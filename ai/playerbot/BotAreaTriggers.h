#pragma once

#include "BotAreaTriggerPolicy.h"

class Player;

namespace ai
{
    // Fires the allowlisted area triggers a grouped bot in a dungeon (not a raid) walks into
    // (Sagiroth/TortoiseBots#984; see BotAreaTriggerPolicy.h). Called from
    // PlayerbotAI::UpdateAI on the world thread, at most every
    // kBotAreaTriggerCheckMs per bot. The first call builds a per-map index of
    // the qualifying triggers in one pass over the core's area-trigger store;
    // each check then tests the bot's position against its map's few entries,
    // with no grid or cell visit.
    // runTrigger: the trigger the current move step of the bot's active run names (0 = none),
    // armed on top of the listed ones.
    void UpdateBotAreaTriggers(Player* bot, BotAreaTriggerState& state, uint32_t runTrigger);
}
