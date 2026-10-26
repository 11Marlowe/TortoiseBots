#pragma once

// The run of a bot, in one lookup (fix/plan-gossip-allow review round 1): the
// bot AI asks for its run's armed area triggers twice a second per bot, and a
// scan over every run per bot was O(bots x runs). The run leader
// (commands/BotCommands.cpp RebuildRunGroups, the one owner) rebuilds this
// index from the runs and their bindings whenever it updates the runs and when
// a run starts; a reader checks what it finds against the live run and binding,
// so a run erased since is never returned. Keyed by the bound group's id: a
// member that joins or leaves is found through its own group, with no update.
// Plain data and the standard library only; tested in tools/test_run_group_index.cpp.

#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace TortoiseBots {
namespace DungeonRun {

class RunGroupIndex
{
public:
    void Clear() { byGroup_.clear(); }
    // groupId 0 is no group and is not indexed.
    void Add(std::uint32_t groupId, std::uint64_t commanderRaw)
    {
        if (groupId)
            byGroup_[groupId] = commanderRaw;
    }
    // The commander key of the run bound to the group, 0 = none. One hash lookup.
    std::uint64_t Find(std::uint32_t groupId) const
    {
        ++lookups_;
        auto const it = byGroup_.find(groupId);
        return it == byGroup_.end() ? 0 : it->second;
    }
    std::size_t Size() const { return byGroup_.size(); }
    std::uint64_t Lookups() const { return lookups_; }

private:
    std::unordered_map<std::uint32_t, std::uint64_t> byGroup_;
    mutable std::uint64_t lookups_ = 0;
};

// A bot's run by its group's id (0 = the bot is in no group: nothing is looked up).
inline std::uint64_t RunOfGroup(RunGroupIndex const& index, std::uint32_t groupId)
{
    return groupId ? index.Find(groupId) : 0;
}

} // namespace DungeonRun
} // namespace TortoiseBots
