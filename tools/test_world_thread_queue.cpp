// Issue #839: chat to bots and master->bot lookups cross from packet threads
// to the world thread. Exercises the queue and the published master view
// standalone, including producer threads against a draining / publishing
// world thread. Issue #844 adds the AI-lifetime model: packet threads only
// queue copies keyed by guid while the world thread deletes and recreates the
// owners and resolves every key again when it drains (run it under
// -fsanitize=thread to catch any producer that touches an owner).
#include "../runtime/WorldThreadQueue.h"

#include <atomic>
#include <cassert>
#include <map>
#include <mutex>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

using namespace TortoiseBots;

int main()
{
    // Arrival order is kept, per key and overall; items are move-only like
    // the copied WorldPacket; a drained queue is empty.
    {
        WorldThreadQueue<std::unique_ptr<std::string>> queue(8);
        assert(queue.Enqueue(1, std::make_unique<std::string>("stay")));
        assert(queue.Enqueue(2, std::make_unique<std::string>("hello")));
        assert(queue.Enqueue(1, std::make_unique<std::string>("follow")));
        std::vector<std::pair<uint64_t, std::string>> seen;
        size_t n = queue.Drain([&](uint64_t key, std::unique_ptr<std::string>& item) { seen.emplace_back(key, *item); });
        assert(n == 3);
        assert(seen.size() == 3);
        assert(seen[0].first == 1 && seen[0].second == "stay");
        assert(seen[1].first == 2 && seen[1].second == "hello");
        assert(seen[2].first == 1 && seen[2].second == "follow");
        assert(queue.PendingCount() == 0);
    }

    // Owner gone before the drain (bot logged out): the consumer resolves the
    // key again and skips it; other keys still run.
    {
        WorldThreadQueue<std::string> queue(8);
        queue.Enqueue(1, "for a bot that logged out");
        queue.Enqueue(2, "for a live bot");
        std::set<uint64_t> live = { 2 };
        std::vector<std::string> handled;
        queue.Drain([&](uint64_t key, std::string& item)
        {
            if (live.count(key))
                handled.push_back(item);
        });
        assert((handled == std::vector<std::string>{ "for a live bot" }));
    }

    // Per-key cap: overflow dropped and counted; other keys unaffected; the
    // count resets with the drain.
    {
        WorldThreadQueue<int> queue(3);
        for (int i = 0; i < 3; ++i)
            assert(queue.Enqueue(7, i));
        assert(!queue.Enqueue(7, 3));
        assert(queue.Enqueue(8, 0));
        assert(queue.DroppedCount() == 1);
        std::vector<int> forSeven;
        queue.Drain([&](uint64_t key, int& v) { if (key == 7) forSeven.push_back(v); });
        assert((forSeven == std::vector<int>{ 0, 1, 2 }));
        assert(queue.Enqueue(7, 4));
    }

    // Enqueue from inside the drain does not deadlock and waits for the next drain.
    {
        WorldThreadQueue<int> queue(8);
        queue.Enqueue(1, 1);
        std::vector<int> ran;
        queue.Drain([&](uint64_t, int& v) { ran.push_back(v); queue.Enqueue(1, 2); });
        assert((ran == std::vector<int>{ 1 }));
        queue.Drain([&](uint64_t, int& v) { ran.push_back(v); });
        assert((ran == std::vector<int>{ 1, 2 }));
        queue.Clear();
        assert(queue.PendingCount() == 0);
    }

    // Producer threads (AsyncPackets, ChannelBroadcaster, map threads) against
    // the draining world thread: every accepted item runs exactly once and each
    // key's items stay in order.
    {
        constexpr int kThreads = 4;
        constexpr int kPerThread = 3000;
        WorldThreadQueue<int> queue(1000000);
        std::atomic<int> done{ 0 };
        std::vector<std::thread> producers;
        for (int t = 0; t < kThreads; ++t)
        {
            producers.emplace_back([&queue, &done, t]
            {
                for (int i = 0; i < kPerThread; ++i)
                    queue.Enqueue(static_cast<uint64_t>(t), i);
                ++done;
            });
        }
        std::map<uint64_t, int> next;
        size_t total = 0;
        auto run = [&](uint64_t key, int& v)
        {
            assert(v == next[key]);
            next[key] = v + 1;
        };
        while (done.load() < kThreads)
            total += queue.Drain(run);
        for (std::thread& producer : producers)
            producer.join();
        total += queue.Drain(run);
        assert(total == static_cast<size_t>(kThreads * kPerThread));
        for (int t = 0; t < kThreads; ++t)
            assert(next[static_cast<uint64_t>(t)] == kPerThread);
    }

    // Master view: lookups copy the published list; unknown masters get none;
    // a republish replaces the view (removed bots disappear).
    {
        MasterBotView view;
        assert(view.BotsFor(100).empty());
        view.Publish(MasterBotView::Map{ { 100, { 1, 2 } }, { 200, { 3 } } });
        assert((view.BotsFor(100) == std::vector<uint64_t>{ 1, 2 }));
        assert((view.BotsFor(200) == std::vector<uint64_t>{ 3 }));
        view.Publish(MasterBotView::Map{ { 100, { 2 } } });
        assert((view.BotsFor(100) == std::vector<uint64_t>{ 2 }));
        assert(view.BotsFor(200).empty());
    }

    // Readers on packet threads while the world thread republishes every
    // tick: each read sees one whole published list, never a torn one.
    {
        MasterBotView view;
        std::atomic<bool> stop{ false };
        std::vector<std::thread> readers;
        for (int r = 0; r < 3; ++r)
        {
            readers.emplace_back([&]
            {
                while (!stop.load())
                {
                    std::vector<uint64_t> bots = view.BotsFor(42);
                    // Published lists are always 1..n for some n.
                    for (size_t i = 0; i < bots.size(); ++i)
                        assert(bots[i] == i + 1);
                }
            });
        }
        for (uint64_t tick = 0; tick < 5000; ++tick)
        {
            MasterBotView::Map map;
            for (uint64_t b = 1; b <= tick % 40; ++b)
                map[42].push_back(b);
            view.Publish(std::move(map));
        }
        stop = true;
        for (std::thread& reader : readers)
            reader.join();
    }

    // World-thread mark: unmarked, no thread is the world thread; once
    // marked, only the marking thread is.
    {
        WorldThreadMark mark;
        assert(!mark.IsCurrent());
        bool otherSaw = true;
        std::thread([&] { otherSaw = mark.IsCurrent(); }).join();
        assert(!otherSaw);
        mark.MarkCurrent();
        assert(mark.IsCurrent());
        std::thread([&] { otherSaw = mark.IsCurrent(); }).join();
        assert(!otherSaw);
    }

    // Issue #844: AI lifetime. The "world thread" owns the AIs (a map guid ->
    // owner, like PlayerbotAIStorage + PlayerbotAIAdapter) and deletes and
    // recreates them (logout / login) while packet threads keep sending; a
    // packet is shared by several bots (a master packet). Producers never see
    // an owner; the drain resolves the guid again and skips a bot that is gone.
    {
        struct Owner
        {
            uint64_t guid;
            std::mutex packetMutex; // PacketHandlingHelper::m_botPacketMutex
            std::vector<int> packets;
        };
        struct Deferred
        {
            int route;
            std::shared_ptr<int const> packet;
        };
        constexpr uint64_t kBots = 8;
        constexpr int kThreads = 3;
        constexpr int kPerThread = 4000;
        WorldThreadQueue<Deferred> queue(1000000);
        std::map<uint64_t, std::unique_ptr<Owner>> owners;
        for (uint64_t g = 1; g <= kBots; ++g)
            owners[g] = std::unique_ptr<Owner>(new Owner{ g, {}, {} });

        std::atomic<int> done{ 0 };
        std::vector<std::thread> producers;
        for (int t = 0; t < kThreads; ++t)
        {
            producers.emplace_back([&queue, &done, t]
            {
                for (int i = 0; i < kPerThread; ++i)
                {
                    auto packet = std::make_shared<int const>(t * kPerThread + i);
                    // One copy queued for every bot of the master.
                    for (uint64_t g = 1; g <= kBots; ++g)
                        queue.Enqueue(g, Deferred{ t, packet });
                }
                ++done;
            });
        }

        size_t delivered = 0;
        size_t skipped = 0;
        auto run = [&](uint64_t guid, Deferred& item)
        {
            auto it = owners.find(guid);
            if (it == owners.end())
            {
                ++skipped;
                return;
            }
            Owner& owner = *it->second;
            assert(owner.guid == guid);
            std::lock_guard<std::mutex> lock(owner.packetMutex);
            owner.packets.push_back(*item.packet);
            ++delivered;
        };
        uint64_t tick = 0;
        while (done.load() < kThreads)
        {
            // Logout of one bot and login of another, as BotManager does it.
            uint64_t const out = 1 + tick % kBots;
            owners.erase(out);
            queue.Drain(run);
            owners[out] = std::unique_ptr<Owner>(new Owner{ out, {}, {} });
            ++tick;
        }
        for (std::thread& producer : producers)
            producer.join();
        queue.Drain(run);
        assert(delivered + skipped == static_cast<size_t>(kThreads * kPerThread) * kBots);
        assert(queue.PendingCount() == 0);
    }

    return 0;
}
