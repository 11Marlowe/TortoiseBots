#pragma once

#include <cstdint>
#include <string>

// Pure policy for the Issue #218 instance guard's leave-instance restore
// (PR #748). No core includes: AiFactory translates game state into these
// plain inputs, so the rule stays testable in
// tools/test_instance_guard_policy.cpp without the server.

namespace ai
{
    // The open-world non-combat strategies the guard holds back in a dungeon
    // or raid. Bit i of the withheld mask stands for entry i. Keep identical
    // to AiFactory's kInstanceGuardStrategies order.
    inline std::uint8_t InstanceGuardBit(unsigned index) { return std::uint8_t(1u << index); }

    inline char const* InstanceGuardStrategy(unsigned index)
    {
        static char const* const strategies[] = { "grind", "travel", "tfish", "rpg", "rpg craft", "wander" };
        return strategies[index];
    }

    inline unsigned InstanceGuardStrategyCount() { return 6; }

    inline unsigned InstanceGuardWanderIndex() { return 5; }

    // Leaving the instance: which withheld entries come back as "+name" adds.
    // A bot with a real-player master keeps following and gets nothing back
    // (#741's rule); only masterless pool bots get the restore. "wander"
    // never comes back next to "follow" (siblings: +wander replaces follow).
    // Entries the player never had are never in the mask, so nothing the
    // player turned off is re-armed.
    inline std::string InstanceGuardRestore(std::uint8_t withheld, bool hasRealPlayerMaster, bool hasFollow)
    {
        if (withheld == 0 || hasRealPlayerMaster)
            return "";

        std::string restore;
        for (unsigned i = 0; i < InstanceGuardStrategyCount(); ++i)
        {
            if (!(withheld & InstanceGuardBit(i)))
                continue;
            if (i == InstanceGuardWanderIndex() && hasFollow)
                continue;
            restore += restore.empty() ? "+" : ",+";
            restore += InstanceGuardStrategy(i);
        }
        return restore;
    }
}
