#pragma once

// Pure raid quest chain rule. A quest is raid when it is a direct raid quest
// (raid type, raid-scale group, raid map, or a raid-exclusive start item), or
// when an earlier quest in its chain is a chain source. Chain sources are
// quests on a raid map, quests started by a raid-exclusive item, and raid-type
// quests filed under a quest sort (negative ZoneOrSort, e.g. Legendary). Raid-scale
// quests are raid but never taint their successors. The predicates are supplied
// by the caller, so this header stays free of core and database types. Exercised
// standalone in tools/test_raid_quest_chain_policy.cpp.

#include <cstdint>
#include <map>
#include <unordered_set>
#include <vector>

namespace TortoiseBots
{

// Whether a quest's raid status spreads to the quests after it. A raid-type
// quest taints only when filed under a quest sort (zoneOrSort < 0, e.g. the
// Legendary chains of Thunderfury and Atiesh); one filed under an ordinary zone
// (the UBRS and Tier 0.5 chains under Blackrock Spire / Blackrock Mountain)
// does not, its follow-ups being 5-man dungeon rewards.
inline bool TaintsChain(bool onRaidMap, bool raidExclusiveStart, bool raidType, int32_t zoneOrSort)
{
    return onRaidMap || raidExclusiveStart || (raidType && zoneOrSort < 0);
}

// quest id -> ids of the quests that come before it in a chain.
using QuestPredecessors = std::map<uint32_t, std::vector<uint32_t>>;

// Chain ids are stored signed in quest_template (PrevQuestId, NextQuestId);
// the magnitude names the linked quest. Zero means no link.
inline uint32_t ChainQuestId(int32_t questId)
{
    return questId < 0 ? uint32_t(-int64_t(questId)) : uint32_t(questId);
}

// Records the predecessor edges one quest row declares. PrevQuestId names a
// predecessor; NextQuestId and NextQuestInChain name a successor of this quest.
inline void RecordQuestChainLinks(QuestPredecessors& predecessors, uint32_t questId, int32_t prevQuestId,
    int32_t nextQuestId, uint32_t nextQuestInChain)
{
    if (uint32_t prev = ChainQuestId(prevQuestId))
        predecessors[questId].push_back(prev);
    if (uint32_t next = ChainQuestId(nextQuestId))
        predecessors[next].push_back(questId);
    if (nextQuestInChain)
        predecessors[nextQuestInChain].push_back(questId);
}

// True when questId is a direct raid quest, or when a quest reachable through
// its predecessors is a chain source. Each quest is visited once, so cyclic
// chain data terminates. Successors never make a predecessor raid.
template <typename IsDirectRaid, typename IsChainSource>
inline bool ReachesRaidQuest(uint32_t questId, IsDirectRaid isDirectRaid, IsChainSource isChainSource,
    QuestPredecessors const& predecessors)
{
    if (isDirectRaid(questId))
        return true;
    std::vector<uint32_t> pending;
    std::unordered_set<uint32_t> visited;
    visited.insert(questId);
    auto pushPredecessors = [&](uint32_t id)
    {
        auto found = predecessors.find(id);
        if (found != predecessors.end())
            for (uint32_t previous : found->second)
                pending.push_back(previous);
    };
    pushPredecessors(questId);
    while (!pending.empty())
    {
        uint32_t current = pending.back();
        pending.pop_back();
        if (!current || !visited.insert(current).second)
            continue;
        if (isChainSource(current))
            return true;
        pushPredecessors(current);
    }
    return false;
}

}
