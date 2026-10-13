#pragma once

#include "ScriptObjects.h"

namespace TortoiseBots {

// Native AllCommandScript adapter. It keeps the public `.bot` surface in the
// module without adding a bot-specific branch to core ChatHandler code.
class BotChatAdapter final : public AllCommandScript
{
public:
    BotChatAdapter();

    bool CanExecuteCommand(ChatHandler* handler, char const* command, char const* args) override;

    // World thread only (BotHostAdapter::OnUpdate, before the bot update):
    // runs the players' `.bot` and TBM addon commands queued since the last
    // tick (issue #833).
    static void DrainQueuedCommands();
};

} // namespace TortoiseBots
