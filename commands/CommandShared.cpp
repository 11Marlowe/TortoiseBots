#include "CommandShared.h"

#include "../runtime/BotManager.h"
#include "BotCommandContext.h"

// pi-lens-ignore: clang:pp_file_not_found
#include "Chat.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "ObjectAccessor.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "ObjectMgr.h"

#include <string>

namespace TortoiseBots {
namespace BotCommands {

std::string Trim(std::string value)
{
    size_t first = value.find_first_not_of(" \t");
    if (first == std::string::npos)
        return {};
    size_t last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

Player* Requester(ChatHandler* handler)
{
    return handler && handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
}

bool ResolveOwnedBot(ChatHandler* handler, char const* args, Player*& bot, BotRecord*& record, std::string& name)
{
    bot = nullptr;
    record = nullptr;
    name = Trim(args ? args : "");
    if (name.empty() || !normalizePlayerName(name))
        return false;

    bot = sObjectMgr.GetPlayer(name.c_str());
    if (!bot)
        bot = sObjectAccessor.FindPlayerByName(name.c_str());
    if (!bot)
        return false;

    record = BotManager::Instance().FindBot(bot->GetObjectGuid());
    if (!CanControlBot(Requester(handler), record))
        return false;

    return IsLiveHeadlessBot(bot, record);
}

std::string ProtocolSafe(std::string value)
{
    for (char& character : value)
    {
        if (character == '|' || character == '\r' || character == '\n')
            character = ' ';
    }
    return value;
}

void SendActionError(ChatHandler* handler, std::string const& intent,
    char const* code, char const* message)
{
    if (!handler)
        return;
    std::string safeMessage = ProtocolSafe(message ? message : "request rejected");
    handler->PSendSysMessage("TBM:ACTION_ERR|%s|%s|%s", ProtocolSafe(intent).c_str(),
        code ? code : "rejected", safeMessage.c_str());
    NoteCommanderOrderResult(handler, intent, false,
        std::string(code ? code : "rejected") + ": " + safeMessage);
}

void SendActionAck(ChatHandler* handler, std::string const& intent,
    std::string const& scope, uint32 count, std::string const& executor)
{
    if (!handler)
        return;
    std::string safeScope = ProtocolSafe(scope);
    std::string safeExecutor = executor.empty() ? "-" : ProtocolSafe(executor);
    handler->PSendSysMessage("TBM:ACTION_ACK|%s|%s|%u|%s",
        ProtocolSafe(intent).c_str(), safeScope.c_str(), count, safeExecutor.c_str());
    NoteCommanderOrderResult(handler, intent, true, safeScope + " " + std::to_string(count));
}

} // namespace BotCommands
} // namespace TortoiseBots
