#pragma once

#include "playerbot/PlayerbotAI.h"

class Player;
class Unit;

namespace ai
{
    // Optional pvp_seek.csv rows: one per seek-role grant, one per new
    // engagement. Written only when the file is listed in AllowedLogFiles.
    class WorldPvpSeekLog
    {
    public:
        static void LogGrant(PlayerbotAI* ai, Player* bot);
        static void LogEngagement(PlayerbotAI* ai, Player* bot, Unit* target);
    };
}
