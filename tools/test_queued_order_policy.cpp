// Live test of issue #846 (2026-10-13): `/p stay` then `/p .bot action follow`
// 0 ms or 50 ms apart ended in stay; 100 ms and more ended in follow. The
// drain hands the bot its chat line before the command, but the AI only queues
// it and runs it on its own later turn, after the command's action. Exercises
// runtime/QueuedOrderPolicy.h standalone, and replays the world tick with the
// real queues (BotCommandQueue, WorldThreadQueue) and a model of the bot's
// queued chat commands, at 0, 50, 100 and 250 ms gaps.
#include "../commands/BotCommandQueue.h"
#include "../runtime/QueuedOrderPolicy.h"
#include "../runtime/WorldThreadQueue.h"

#include <cassert>
#include <cstdint>
#include <queue>
#include <string>
#include <vector>

using namespace TortoiseBots;

namespace {

struct Holder
{
    std::string command;
};

std::string CommandOf(Holder& holder)
{
    return holder.command;
}

void TestMovementWords()
{
    assert(IsMovementChatOrder("stay"));
    assert(IsMovementChatOrder("follow"));
    assert(IsMovementChatOrder("guard"));
    assert(IsMovementChatOrder("free"));
    assert(IsMovementChatOrder("flee"));
    assert(IsMovementChatOrder("wander"));
    // Other triggers, and forms that never reach a movement trigger.
    assert(!IsMovementChatOrder("follow target"));
    assert(!IsMovementChatOrder("wander far"));
    assert(!IsMovementChatOrder("attack"));
    assert(!IsMovementChatOrder("summon"));
    assert(!IsMovementChatOrder("loot"));
    assert(!IsMovementChatOrder(""));
    assert(!IsMovementChatOrder("Stay"));
}

void TestDropKeepsOthersInOrder()
{
    std::queue<Holder> commands;
    for (char const* text : { "stay", "loot", "follow", "follow target", "summon", "flee" })
        commands.push(Holder{ text });
    assert(DropQueuedMovementOrders(commands, CommandOf) == 3);
    std::vector<std::string> left;
    while (!commands.empty())
    {
        left.push_back(commands.front().command);
        commands.pop();
    }
    assert((left == std::vector<std::string>{ "loot", "follow target", "summon" }));

    std::queue<Holder> empty;
    assert(DropQueuedMovementOrders(empty, CommandOf) == 0);
    assert(empty.empty());
}

// One bot as the world thread sees it: chat commands the AI queued
// (PlayerbotAI::chatCommands), and its movement mode. The AI turns its queue
// into actions only on its own turns, every `turnEvery` world ticks.
struct BotModel
{
    std::queue<Holder> chatCommands;
    std::string movement = "follow";
    int turnEvery = 1;

    void HandleChat(std::string const& text) { chatCommands.push(Holder{ text }); }

    void Turn(int tick)
    {
        if (tick % turnEvery != 0)
            return;
        while (!chatCommands.empty())
        {
            std::string const command = chatCommands.front().command;
            chatCommands.pop();
            if (IsMovementChatOrder(command))
                movement = command;
        }
    }
};

constexpr uint64_t kMaster = 1;
constexpr uint64_t kBot = 100;
constexpr int kTickMs = 100;

struct Input
{
    int atMs;
    bool chat;
    std::string text;
};

// Replays one burst: each input is queued on its producer thread at its time
// (chat as the bot's copy in the deferred chat queue, `.bot action <x>` in
// the command queue), and every world tick runs BotHostAdapter::OnUpdate's
// order: commands (each after the chat queued before it), the rest of the
// chat, then the bot's own turn.
std::string Replay(std::vector<Input> const& inputs, bool supersede, int turnEvery)
{
    BotCommandQueue commands;
    WorldThreadQueue<std::string> chat(64);
    BotModel bot;
    bot.turnEvery = turnEvery;

    size_t next = 0;
    for (int tick = 1; tick <= 40; ++tick)
    {
        int const tickStartMs = tick * kTickMs;
        while (next < inputs.size() && inputs[next].atMs < tickStartMs)
        {
            Input const& input = inputs[next++];
            if (input.chat)
                chat.Enqueue(kBot, input.text);
            else
                commands.Enqueue(kMaster, BotCommandReply::Chat, input.text);
        }

        auto deliver = [&](uint64_t before)
        {
            for (auto& entry : chat.TakeBefore(before))
                bot.HandleChat(entry.item);
        };
        commands.Drain(
            [](uint64_t guid) -> bool { return guid == kMaster; },
            [&](bool, PendingBotCommand const& command)
            {
                deliver(command.seq);
                // `.bot action <movement>`: runs its action at once.
                if (supersede)
                    DropQueuedMovementOrders(bot.chatCommands, CommandOf);
                bot.movement = command.args;
            },
            [](bool, DroppedBotCommands const&) {});
        deliver(commands.DeferredRunLimit());
        bot.Turn(tick);
    }
    return bot.movement;
}

std::vector<Input> StayThenFollow(int gapMs)
{
    return { Input{ 10, true, "stay" }, Input{ 10 + gapMs, false, "follow" } };
}

void TestLiveBurstGaps()
{
    // Without the rule: the live failure, at the gaps it failed at.
    assert(Replay(StayThenFollow(0), false, 1) == "stay");
    assert(Replay(StayThenFollow(50), false, 1) == "stay");
    // ... and the gaps that passed (next tick, the bot already ran its stay).
    assert(Replay(StayThenFollow(100), false, 1) == "follow");
    // A bot whose AI thinks every few ticks keeps the chat order queued
    // longer, so the window is wider than one tick.
    assert(Replay(StayThenFollow(100), false, 3) == "stay");

    // With the rule the newer order wins at every gap and turn interval.
    for (int gap : { 0, 1, 50, 99, 100, 200, 250, 2500 })
        for (int turnEvery : { 1, 2, 3, 5 })
            assert(Replay(StayThenFollow(gap), true, turnEvery) == "follow");
}

void TestNewerChatStillWins()
{
    // `.bot action stay` then `/p follow`: the chat line is the newer order;
    // it reaches the AI after the command and is not dropped.
    for (int gap : { 0, 50, 100, 250 })
        for (int turnEvery : { 1, 3 })
        {
            std::vector<Input> inputs{ Input{ 10, false, "stay" }, Input{ 10 + gap, true, "follow" } };
            assert(Replay(inputs, true, turnEvery) == "follow");
        }
}

void TestOnlyOlderChatIsDropped()
{
    // stay (chat), follow (.bot), stay (chat) in one tick: the last chat line
    // reaches the AI after the command, so it stands.
    std::vector<Input> inputs{ Input{ 10, true, "stay" }, Input{ 11, false, "follow" }, Input{ 12, true, "stay" } };
    for (int turnEvery : { 1, 3 })
        assert(Replay(inputs, true, turnEvery) == "stay");
}

} // namespace

int main()
{
    TestMovementWords();
    TestDropKeepsOthersInOrder();
    TestLiveBurstGaps();
    TestNewerChatStillWins();
    TestOnlyOlderChatIsDropped();
    return 0;
}
