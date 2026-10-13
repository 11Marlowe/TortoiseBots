#pragma once

#include <cstdint>
#include <vector>

// Pure decision rule for Retaliation (warrior, 30 min cooldown). Retaliation
// counters melee swings only, so one melee mob on the warrior, or a caster
// that hits from range, is a poor use of the cooldown. The action sends one
// snapshot per live attacker; this rule counts the melee ones.
namespace ai
{
    struct RetaliationAttacker
    {
        bool alive;         // the attacker is alive
        bool targetsBot;    // its victim is the warrior
        bool inMeleeReach;  // it is within melee reach of the warrior right now
    };

    inline uint32_t CountRetaliationMeleeAttackers(const std::vector<RetaliationAttacker>& attackers)
    {
        uint32_t count = 0;
        for (const RetaliationAttacker& attacker : attackers)
        {
            if (attacker.alive && attacker.targetsBot && attacker.inMeleeReach)
                ++count;
        }
        return count;
    }

    // Mod-playerbots threshold (CastRetaliationAction::isUseful): two melee attackers.
    inline bool RetaliationWorthCooldown(const std::vector<RetaliationAttacker>& attackers)
    {
        return CountRetaliationMeleeAttackers(attackers) >= 2;
    }
}
