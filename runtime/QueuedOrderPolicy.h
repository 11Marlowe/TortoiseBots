#pragma once

#include <cstddef>
#include <queue>
#include <string>
#include <utility>

namespace TortoiseBots {

// Live test of issue #846 (2026-10-13): `/p stay` then `/p .bot action follow`
// in one world tick still ended in stay. The drain hands the bot its `stay`
// chat line before the command, as typed, but the AI only queues a chat
// command (PlayerbotAI::chatCommands); it turns into a trigger and then the
// "stay chat shortcut" action on the bot's own later AI turns. A `.bot` order
// runs its action at once, so the older chat order, executed afterwards, won.
//
// The rule: an explicit `.bot` movement order supersedes the bot's movement
// orders that are still queued from chat. The command drain runs a command
// only after every chat line queued before it reached the AI (issue #846), so
// anything still waiting in the AI at that moment is older than the order.
//
// The chat words that set a bot's movement mode: the ChatCommandTrigger names
// whose actions are the "<word> chat shortcut" movement actions
// (ChatCommandHandlerStrategy). A queued chat command matches only when it
// is exactly one of them, which is the only form that reaches that trigger
// ("follow target" is a different trigger).
inline char const* const* MovementChatOrders(size_t& count)
{
    static char const* const kOrders[] = { "follow", "stay", "guard", "free", "flee", "wander" };
    count = sizeof(kOrders) / sizeof(kOrders[0]);
    return kOrders;
}

inline bool IsMovementChatOrder(std::string const& command)
{
    size_t count = 0;
    char const* const* orders = MovementChatOrders(count);
    for (size_t i = 0; i < count; ++i)
        if (command == orders[i])
            return true;
    return false;
}

// Removes the movement orders from a bot's queued chat commands, keeping the
// rest in their order. `commandOf(holder)` returns a holder's command text.
// Returns how many were removed.
template <class Holder, class CommandOf>
size_t DropQueuedMovementOrders(std::queue<Holder>& commands, CommandOf commandOf)
{
    std::queue<Holder> kept;
    size_t dropped = 0;
    while (!commands.empty())
    {
        Holder& holder = commands.front();
        if (IsMovementChatOrder(commandOf(holder)))
            ++dropped;
        else
            kept.push(std::move(holder));
        commands.pop();
    }
    commands.swap(kept);
    return dropped;
}

} // namespace TortoiseBots
