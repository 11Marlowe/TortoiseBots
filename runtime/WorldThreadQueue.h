#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace TortoiseBots {

// Issue #839: hand work from a core thread that may overlap the world-thread
// bot update (AsyncPackets, ChannelBroadcaster, another map's thread) to the
// world thread. The producer only copies its data in and returns; the world
// hook drains everything queued so far, in arrival order, so each key (a bot
// guid) keeps the order its items arrived in.
//
// Items are keyed by a raw guid, never by a Player or PlayerbotAI pointer: the
// drain resolves the key again and the consumer drops items whose owner is
// gone. A per-key cap bounds memory when a key floods (for example a bot in a
// busy chat channel); the overflow is dropped and counted.
//
// Plain standard library, so tools/test_world_thread_queue.cpp exercises it
// standalone (including several producer threads against a draining thread).
template <class Item>
class WorldThreadQueue
{
public:
    explicit WorldThreadQueue(size_t maxPendingPerKey) : m_maxPendingPerKey(maxPendingPerKey) {}

    // Any thread. False (item dropped) when the key already has the maximum
    // pending.
    bool Enqueue(uint64_t key, Item item)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t& pending = m_pendingPerKey[key];
        if (pending >= m_maxPendingPerKey)
        {
            ++m_dropped;
            return false;
        }
        ++pending;
        m_pending.emplace_back(key, std::move(item));
        return true;
    }

    // World thread. Calls run(key, item) for every item queued so far, in
    // arrival order. The lock is released first, so run may enqueue (that item
    // waits for the next drain). Returns the number of items handed to run.
    template <class Run>
    size_t Drain(Run run)
    {
        std::vector<std::pair<uint64_t, Item>> batch;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            batch.swap(m_pending);
            m_pendingPerKey.clear();
        }
        for (auto& entry : batch)
            run(entry.first, entry.second);
        return batch.size();
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pending.clear();
        m_pendingPerKey.clear();
    }

    size_t PendingCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pending.size();
    }

    // Items dropped by the cap since start (diagnostics).
    uint64_t DroppedCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_dropped;
    }

private:
    size_t const m_maxPendingPerKey;
    mutable std::mutex m_mutex;
    std::vector<std::pair<uint64_t, Item>> m_pending;
    std::unordered_map<uint64_t, size_t> m_pendingPerKey;
    uint64_t m_dropped = 0;
};

// Issue #839: master guid -> bot guids, built on the world thread from
// BotManager's records and published under a lock, so packet hooks on other
// threads read a consistent copy instead of walking BotManager::m_bots while
// the world thread inserts and erases. The world thread pays one lock per
// publish (once per world tick), never per lookup.
class MasterBotView
{
public:
    using Map = std::unordered_map<uint64_t, std::vector<uint64_t>>;

    // World thread: replace the published view.
    void Publish(Map view)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_view.swap(view);
        // `view` now holds the old map and is destroyed outside the lock.
    }

    // Any thread: a copy of the bot guids published for that master.
    std::vector<uint64_t> BotsFor(uint64_t masterGuid) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_view.find(masterGuid);
        return it == m_view.end() ? std::vector<uint64_t>() : it->second;
    }

private:
    mutable std::mutex m_mutex;
    Map m_view;
};

} // namespace TortoiseBots
