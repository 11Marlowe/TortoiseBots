#pragma once

#include <atomic>
#include <ctime>

// When a CalculatedValue recalculates. Pure policy, no game types, so
// tools/test_value_refresh_policy.cpp can compile it on its own.
//
// Today's rule (AiPlayerbot.ValueRefreshAtInterval = 0) refreshes once
// now - lastCheckTime >= checkInterval / 2 in whole seconds: interval 0 and 1
// recalculate on every read (1 / 2 == 0), 2 every second, 5 every 2 s, N every
// floor(N / 2) s.
//
// With the toggle on, a value with checkInterval >= 2 refreshes once its full
// interval has passed. Two groups keep today's rule:
//   - every value with checkInterval 0 or 1 (still recalculated on every
//     read): health, mana, auras, targets, inventory reads and the rest of the
//     default-interval values were all written against per-read results;
//   - values that sense the fight around the bot and mark themselves with
//     keepLegacyRefresh (attackers and their counts, aggro, possible targets,
//     hazards, boss target, AoE heal, enemy players) and the free-move
//     values plus grind target (follow restart after the master moves, and
//     the heal/grind/possible-target filters that read the free-move centre).
namespace ai
{
    // Set once from PlayerbotAIConfig::Initialize; read on every value read.
    inline std::atomic<bool> gValueRefreshAtInterval{false};

    // Seconds a cached result is served before it is recalculated.
    inline int ValueRefreshSeconds(int checkInterval, bool atInterval, bool keepLegacy)
    {
        if (atInterval && !keepLegacy && checkInterval >= 2)
            return checkInterval;
        return checkInterval / 2;
    }

    inline bool ValueNeedsRefresh(std::time_t now, std::time_t lastCheckTime, int checkInterval,
        bool atInterval, bool keepLegacy)
    {
        if (!lastCheckTime)
            return true;
        if (checkInterval < 2 && (now - lastCheckTime > 0.1))
            return true;
        return now - lastCheckTime >= ValueRefreshSeconds(checkInterval, atInterval, keepLegacy);
    }

    inline bool ValueRefreshAtIntervalEnabled()
    {
        return gValueRefreshAtInterval.load(std::memory_order_relaxed);
    }
}
