// Issue #833: players' `.bot` / TBM commands are queued off the chat threads
// and drained on the world thread. Exercises the queue policy standalone.
#include "../commands/BotCommandQueue.h"

#include <cassert>
#include <set>
#include <string>
#include <thread>
#include <vector>

using namespace TortoiseBots;

namespace {

struct FakePlayer
{
    uint64_t guid;
};

struct Run
{
    uint64_t guid;
    BotCommandReply reply;
    std::string args;
};

} // namespace

int main()
{
    // Arrival order is kept across requesters and transports; nothing runs
    // until the drain, and a drained queue is empty.
    {
        BotCommandQueue queue;
        FakePlayer alice{ 1 }, bob{ 2 };
        assert(queue.Enqueue(1, BotCommandReply::Chat, "stay"));
        assert(queue.Enqueue(2, BotCommandReply::Addon, "list"));
        assert(queue.Enqueue(1, BotCommandReply::Addon, "action follow"));
        assert(queue.PendingCount() == 3);

        std::vector<Run> runs;
        size_t executed = queue.Drain(
            [&](uint64_t guid) -> FakePlayer* { return guid == 1 ? &alice : guid == 2 ? &bob : nullptr; },
            [&](FakePlayer* p, PendingBotCommand const& c) { runs.push_back(Run{ p->guid, c.reply, c.args }); },
            [&](FakePlayer*, DroppedBotCommands const&) { assert(false); });
        assert(executed == 3);
        assert(runs.size() == 3);
        assert(runs[0].guid == 1 && runs[0].reply == BotCommandReply::Chat && runs[0].args == "stay");
        assert(runs[1].guid == 2 && runs[1].reply == BotCommandReply::Addon && runs[1].args == "list");
        assert(runs[2].guid == 1 && runs[2].reply == BotCommandReply::Addon && runs[2].args == "action follow");
        assert(queue.PendingCount() == 0);
    }

    // Logout before the drain: the requester is no longer found, so the command
    // is dropped (no reply, nothing executed); other players' commands still run.
    {
        BotCommandQueue queue;
        FakePlayer bob{ 2 };
        queue.Enqueue(1, BotCommandReply::Chat, "summon");
        queue.Enqueue(2, BotCommandReply::Chat, "summon");
        std::vector<uint64_t> ran;
        size_t executed = queue.Drain(
            [&](uint64_t guid) -> FakePlayer* { return guid == 2 ? &bob : nullptr; },
            [&](FakePlayer* p, PendingBotCommand const&) { ran.push_back(p->guid); },
            [&](FakePlayer*, DroppedBotCommands const&) { assert(false); });
        assert(executed == 1);
        assert((ran == std::vector<uint64_t>{ 2 }));
        assert(queue.PendingCount() == 0);
    }

    // Per-requester cap: the overflow is dropped, counted and reported once to
    // that requester at the drain; another requester is unaffected. The count
    // resets with the drain.
    {
        BotCommandQueue queue;
        FakePlayer alice{ 1 }, bob{ 2 };
        for (size_t i = 0; i < BotCommandQueue::kMaxPendingPerRequester; ++i)
            assert(queue.Enqueue(1, BotCommandReply::Addon, "status"));
        assert(!queue.Enqueue(1, BotCommandReply::Addon, "status"));
        assert(!queue.Enqueue(1, BotCommandReply::Addon, "status"));
        assert(queue.Enqueue(2, BotCommandReply::Chat, "list"));

        size_t reports = 0;
        size_t executed = queue.Drain(
            [&](uint64_t guid) -> FakePlayer* { return guid == 1 ? &alice : guid == 2 ? &bob : nullptr; },
            [&](FakePlayer*, PendingBotCommand const&) {},
            [&](FakePlayer* p, DroppedBotCommands const& d)
            {
                ++reports;
                assert(p->guid == 1 && d.count == 2 && d.reply == BotCommandReply::Addon);
            });
        assert(executed == BotCommandQueue::kMaxPendingPerRequester + 1);
        assert(reports == 1);
        assert(queue.Enqueue(1, BotCommandReply::Chat, "list"));
    }

    // Dropped-count report for a requester who logged out is discarded too.
    {
        BotCommandQueue queue;
        for (size_t i = 0; i <= BotCommandQueue::kMaxPendingPerRequester; ++i)
            queue.Enqueue(1, BotCommandReply::Chat, "x");
        size_t executed = queue.Drain(
            [&](uint64_t) -> FakePlayer* { return nullptr; },
            [&](FakePlayer*, PendingBotCommand const&) { assert(false); },
            [&](FakePlayer*, DroppedBotCommands const&) { assert(false); });
        assert(executed == 0);
        assert(queue.PendingCount() == 0);
    }

    // A command that queues another command during the drain does not deadlock;
    // the new one waits for the next drain.
    {
        BotCommandQueue queue;
        FakePlayer alice{ 1 };
        queue.Enqueue(1, BotCommandReply::Chat, "first");
        std::vector<std::string> ran;
        auto find = [&](uint64_t) -> FakePlayer* { return &alice; };
        auto none = [&](FakePlayer*, DroppedBotCommands const&) { assert(false); };
        queue.Drain(find,
            [&](FakePlayer*, PendingBotCommand const& c)
            {
                ran.push_back(c.args);
                queue.Enqueue(1, BotCommandReply::Chat, "second");
            },
            none);
        assert((ran == std::vector<std::string>{ "first" }));
        assert(queue.PendingCount() == 1);
        queue.Drain(find, [&](FakePlayer*, PendingBotCommand const& c) { ran.push_back(c.args); }, none);
        assert((ran == std::vector<std::string>{ "first", "second" }));
    }

    // Producers on other threads (map / AsyncPackets) while the world thread
    // drains: every accepted command runs exactly once.
    {
        BotCommandQueue queue;
        constexpr int kThreads = 4;
        constexpr int kPerThread = 2000;
        std::set<std::string> seen;
        size_t executed = 0;
        auto find = [](uint64_t guid) -> uint64_t { return guid; };
        auto run = [&](uint64_t, PendingBotCommand const& c)
        {
            bool inserted = seen.insert(c.args).second;
            assert(inserted);
            (void)inserted;
        };
        auto none = [](uint64_t, DroppedBotCommands const&) { assert(false); };
        std::vector<std::thread> producers;
        for (int t = 0; t < kThreads; ++t)
        {
            producers.emplace_back([&queue, t]
            {
                for (int i = 0; i < kPerThread; ++i)
                {
                    // Distinct requesters per message keep the cap out of the way.
                    uint64_t guid = 1 + static_cast<uint64_t>(t) * kPerThread + i;
                    while (!queue.Enqueue(guid, BotCommandReply::Chat, std::to_string(guid)))
                        std::this_thread::yield();
                }
            });
        }
        while (executed < static_cast<size_t>(kThreads * kPerThread))
            executed += queue.Drain(find, run, none);
        for (std::thread& producer : producers)
            producer.join();
        executed += queue.Drain(find, run, none);
        assert(executed == static_cast<size_t>(kThreads * kPerThread));
        assert(seen.size() == static_cast<size_t>(kThreads * kPerThread));
    }

    // Shutdown forgets anything still queued.
    {
        BotCommandQueue queue;
        queue.Enqueue(1, BotCommandReply::Chat, "list");
        queue.Clear();
        assert(queue.PendingCount() == 0);
    }

    return 0;
}
