#include "BotChatAdapter.h"

#include "BotAddonAdapter.h"
#include "../commands/BotCommandQueue.h"
#include "../commands/BotCommands.h"
#include "../ai/playerbot/PlayerbotAIConfig.h"
#include "Chat.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "WorldSession.h"

#include <cctype>
#include <string>
#include <typeinfo>

namespace TortoiseBots {

BotChatAdapter::BotChatAdapter()
    : AllCommandScript("tortoisebots_commands")
{
}

bool BotChatAdapter::CanExecuteCommand(ChatHandler* handler, char const* command, char const* args)
{
    if (!command)
        return true;

    std::string commandName(command);
    for (char& character : commandName)
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

    if (commandName != "bot")
        return true;

    // Returning false tells the generic AllCommandScript registry that this
    // command has been consumed; normal core command lookup must not see it.
    // Issue #833: a player's command is queued for the world thread (see
    // BotCommandQueue.h). The server console, remote access and
    // `pending_commands` (null-session CliHandlers) already run on the world
    // thread and keep running inline. A handler subclass with its own reply
    // sink also stays inline: the drain can only rebuild a plain ChatHandler.
    WorldSession* session = handler ? handler->GetSession() : nullptr;
    Player* requester = session ? session->GetPlayer() : nullptr;
    if (!sPlayerbotAIConfig.enabled || !requester || typeid(*handler) != typeid(ChatHandler))
    {
        BotCommands::HandleChatCommand(handler, args ? args : "");
        return false;
    }

    BotCommandQueue::Instance().Enqueue(requester->GetObjectGuid().GetRawValue(), BotCommandReply::Chat,
        args ? args : "");
    return false;
}

void BotChatAdapter::DrainQueuedCommands()
{
    BotCommandQueue::Instance().Drain(
        [](uint64 guid) -> Player*
        {
            // Not-in-world lookup: a player between maps still owns the
            // command, exactly as when it ran inline; a logged-out player is
            // gone from the accessor and the command is dropped.
            Player* player = ObjectAccessor::FindPlayerNotInWorld(ObjectGuid(guid));
            return player && player->GetSession() ? player : nullptr;
        },
        [](Player* requester, PendingBotCommand const& command)
        {
            if (command.reply == BotCommandReply::Addon)
            {
                BotAddonAdapter::ExecuteCommand(requester, command.args);
                return;
            }
            ChatHandler handler(requester->GetSession());
            BotCommands::HandleChatCommand(&handler, command.args.c_str());
        },
        [](Player* requester, DroppedBotCommands const& dropped)
        {
            std::string const text = "Too many bot commands at once: " + std::to_string(dropped.count) +
                " ignored.";
            if (dropped.reply == BotCommandReply::Addon)
            {
                BotAddonAdapter::SendReply(requester, text);
                return;
            }
            ChatHandler(requester->GetSession()).SendSysMessage(text.c_str());
        });
}

} // namespace TortoiseBots
