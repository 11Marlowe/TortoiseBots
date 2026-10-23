#pragma once

#include "playerbot/PlayerbotAI.h"

class Player;
class Unit;

namespace ai
{
    // Shared open-world PvP seek checks for enemy-player targeting. Both the
    // proactive target list and the attacker validation use these so a seek
    // fight the non-combat trigger starts stays valid once combat begins.
    class WorldPvpSeekChecks
    {
    public:
        // True when this bot seeks open-world PvP targets right now: the mode
        // is on, the bot holds the role, and it is not in a battleground.
        static bool IsSeeking(PlayerbotAI* ai, Player* bot);

        // Target rules for a seek fight: enemy faction (checked by the
        // caller via IsValid), PvP flagged, not in a prohibited zone, in
        // engage range with line of sight, level-gap odds (stable per pair),
        // no challenge mode except War Mode, never an invisible GM.
        static bool IsSeekTarget(PlayerbotAI* ai, Player* bot, Player* target);

        // Assist: a same-faction player or bot in a PvP fight within ~30 yd
        // whose attacker is a valid seek target. Returns the attacker.
        static Unit* FindAssistTarget(PlayerbotAI* ai, Player* bot);
    };
}
