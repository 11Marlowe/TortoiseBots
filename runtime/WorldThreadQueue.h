#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <thread>
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
// Issue #846: one sequence shared by every world-thread queue (this one and
// BotCommandQueue). Each queue stamps an item under its own lock, so a queue's
// items are in sequence order, and one producer's items are in the order it
// queued them across queues. The world-thread drain runs the queues merged by
// this number, so a master's `/p stay` followed by `/p .bot action follow` in
// one tick runs in that order, although the two land in different queues.
inline std::atomic<uint64_t>& WorldQueueSequenceCounter()
{
    static std::atomic<uint64_t> sequence{ 0 };
    return sequence;
}

inline uint64_t NextWorldQueueSequence()
{
    return WorldQueueSequenceCounter().fetch_add(1, std::memory_order_relaxed) + 1;
}

// One past every number issued so far. An item stamped below it is already in
// its queue for whoever locks that queue after reading this: the stamp and the
// push happen under the queue's lock.
inline uint64_t WorldQueueSequenceEnd()
{
    return WorldQueueSequenceCounter().load(std::memory_order_relaxed) + 1;
}

template <class Item>
class WorldThreadQueue
{
public:
    struct Entry
    {
        uint64_t seq;
        uint64_t key;
        Item item;
    };

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
            ++m_droppedPerKey[key];
            return false;
        }
        ++pending;
        m_pending.push_back(Entry{ NextWorldQueueSequence(), key, std::move(item) });
        return true;
    }

    // World thread. Takes every item stamped before `limit`, in arrival
    // order; later items stay queued. The cap counts what is still queued, so
    // a key regains room as soon as its items are taken.
    std::vector<Entry> TakeBefore(uint64_t limit)
    {
        std::vector<Entry> batch;
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t count = 0;
        while (count < m_pending.size() && m_pending[count].seq < limit)
            ++count;
        if (count == m_pending.size())
        {
            batch.swap(m_pending);
            m_pendingPerKey.clear();
            return batch;
        }
        batch.reserve(count);
        for (size_t i = 0; i < count; ++i)
        {
            auto it = m_pendingPerKey.find(m_pending[i].key);
            if (it != m_pendingPerKey.end() && --it->second == 0)
                m_pendingPerKey.erase(it);
            batch.push_back(std::move(m_pending[i]));
        }
        m_pending.erase(m_pending.begin(), m_pending.begin() + static_cast<std::ptrdiff_t>(count));
        return batch;
    }

    // World thread. Calls run(key, item) for every item queued so far, in
    // arrival order. The lock is released first, so run may enqueue (that item
    // waits for the next drain). Returns the number of items handed to run.
    template <class Run>
    size_t Drain(Run run)
    {
        std::vector<Entry> batch = TakeBefore(UINT64_MAX);
        for (Entry& entry : batch)
            run(entry.key, entry.item);
        return batch.size();
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pending.clear();
        m_pendingPerKey.clear();
        m_droppedPerKey.clear();
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

    // Issue #846: per key, the items dropped since the last call.
    std::unordered_map<uint64_t, uint64_t> TakeDroppedPerKey()
    {
        std::unordered_map<uint64_t, uint64_t> dropped;
        std::lock_guard<std::mutex> lock(m_mutex);
        dropped.swap(m_droppedPerKey);
        return dropped;
    }

private:
    size_t const m_maxPendingPerKey;
    mutable std::mutex m_mutex;
    std::vector<Entry> m_pending;
    std::unordered_map<uint64_t, size_t> m_pendingPerKey;
    std::unordered_map<uint64_t, uint64_t> m_droppedPerKey;
    uint64_t m_dropped = 0;
};

// Issue #846: runs two batches (each in sequence order, as TakeBefore returns
// them) as one, by sequence: runA(entry) / runB(entry).
template <class A, class RunA, class B, class RunB>
void RunInSequence(std::vector<A>& a, RunA runA, std::vector<B>& b, RunB runB)
{
    size_t i = 0, j = 0;
    while (i < a.size() || j < b.size())
    {
        if (j == b.size() || (i < a.size() && a[i].seq < b[j].seq))
            runA(a[i++]);
        else
            runB(b[j++]);
    }
}

// Issue #846: a queue's cap drops items silently; this turns the per-key drop
// counts into at most one report per key per interval, carrying everything
// dropped for that key since its last report. World thread only.
class DropReportLimiter
{
public:
    explicit DropReportLimiter(uint64_t intervalMs) : m_intervalMs(intervalMs) {}

    void Add(uint64_t key, uint64_t count)
    {
        if (count)
            m_keys[key].pending += count;
    }

    // Calls report(key, count) for every key with unreported drops whose last
    // report is at least one interval old (or that never reported), and
    // forgets keys that stayed quiet for an interval.
    template <class Report>
    void Flush(uint64_t nowMs, Report report)
    {
        for (auto it = m_keys.begin(); it != m_keys.end();)
        {
            State& state = it->second;
            bool const due = !state.reported || nowMs - state.lastMs >= m_intervalMs;
            if (state.pending && due)
            {
                report(it->first, state.pending);
                state.pending = 0;
                state.lastMs = nowMs;
                state.reported = true;
            }
            else if (!state.pending && due)
            {
                it = m_keys.erase(it);
                continue;
            }
            ++it;
        }
    }

    size_t TrackedKeys() const { return m_keys.size(); }

private:
    struct State
    {
        uint64_t pending = 0;
        uint64_t lastMs = 0;
        bool reported = false;
    };
    uint64_t const m_intervalMs;
    std::unordered_map<uint64_t, State> m_keys;
};

// Issue #844: which thread is the world thread. The world thread is the one
// that deletes bot AIs, so code running on it may use an AI directly; any other
// thread queues instead. Unmarked (before the first world tick), every thread
// counts as "not the world thread", the safe answer.
class WorldThreadMark
{
public:
    // World thread, every tick.
    void MarkCurrent() { m_id.store(std::this_thread::get_id(), std::memory_order_relaxed); }

    // Any thread. Only the marking thread can read back its own id.
    bool IsCurrent() const { return m_id.load(std::memory_order_relaxed) == std::this_thread::get_id(); }

private:
    std::atomic<std::thread::id> m_id{};
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
