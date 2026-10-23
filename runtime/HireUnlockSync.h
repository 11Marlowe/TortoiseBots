// Issue #490 PR 2 (kills): hire-unlock counter sync math. Pure, core-free:
// given a counter, the encounters a character has recorded and the
// optional-map set, computes the quest-log counter value and the missing
// boss list shown on the "Mercenary contracts" gossip page. Unit-tested
// standalone (tools/test_hire_unlock_sync.cpp). The host sync code reads
// this header, so the gossip page and the quest counters can never disagree.

#pragma once

#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "policy/hire/HireGearPolicy.h"
#include "HireUnlockPolicy.h"

namespace TortoiseBots
{

// Quest-log counter value for one counter: recorded encounters belonging to
// this counter plus catalog bosses on optional maps (counted automatically,
// never stored), capped at the counter size. Never lowers what the client
inline uint8_t HireUnlockCounterTarget(HireUnlockCounter const& counter,
    std::set<uint32_t> const& recordedEncounters, std::set<uint32_t> const& optionalMaps,
    uint8_t current)
{
    uint8_t target = 0;
    for (uint8_t i = 0; i < counter.bossCount; ++i)
    {
        bool done = recordedEncounters.count(counter.bosses[i].entry) != 0;
        if (!done)
            for (uint32_t map : optionalMaps)
                if (map == counter.bosses[i].mapId)
                {
                    done = true;
                    break;
                }
        if (done)
            ++target;
    }
    if (target < current)
        target = current;
    return target;
}

// Bosses of one counter still missing a kill: neither recorded nor covered
// by an optional map. Free-text duty rosters only; bound is the catalog, so
// the list stays short.
inline void MissingHireUnlockBosses(HireUnlockCounter const& counter,
    std::set<uint32_t> const& recordedEncounters, std::set<uint32_t> const& optionalMaps,
    std::vector<HireUnlockBoss const*>& missing)
{
    missing.clear();
    for (uint8_t i = 0; i < counter.bossCount; ++i)
    {
        if (recordedEncounters.count(counter.bosses[i].entry) != 0)
            continue;
        bool optional = false;
        for (uint32_t map : optionalMaps)
            if (map == counter.bosses[i].mapId)
            {
                optional = true;
                break;
            }
        if (!optional)
            missing.push_back(&counter.bosses[i]);
    }
}

// Gossip-safe label length for one missing-boss line (1.12 gossip truncates
// long lines; the recruiter splits longer rosters into sub-pages).
constexpr size_t kHireUnlockGossipLineMax = 120;

// "Label: x/N" counter summary with the missing roster appended. Returns
// false when the roster does not fit one line: the caller then opens a
// sub-page per counter instead of truncating names.
inline bool HireUnlockCounterLine(HireUnlockCounter const& counter, uint8_t have,
    std::set<uint32_t> const& recordedEncounters, std::set<uint32_t> const& optionalMaps,
    std::string& line)
{
    std::vector<HireUnlockBoss const*> missing;
    MissingHireUnlockBosses(counter, recordedEncounters, optionalMaps, missing);
    line = std::string(counter.label) + ": " +
        std::to_string(have) + "/" + std::to_string(counter.bossCount);
    if (missing.empty())
        return true;
    line += ", missing: ";
    for (size_t i = 0; i < missing.size(); ++i)
    {
        if (i)
            line += ", ";
        line += missing[i]->name;
    }
    return line.size() <= kHireUnlockGossipLineMax;
}

// True when the contract branch is visible to this viewer: capital
// recruiter, tiers enabled, real player, level 60. Pool bots only ever see
// the hire wizard.
inline bool ShouldShowHireContracts(bool tiersEnabled, bool isCapitalRecruiter,
    bool isBot, uint32_t level)
{
    return tiersEnabled && isCapitalRecruiter && !isBot && level == kHireGearHireLevel;
}

} // namespace TortoiseBots
