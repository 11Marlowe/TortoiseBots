#include "playerbot/playerbot.h"
#include "DebugAction.h"
#include "playerbot/PlayerbotAIConfig.h"
#include <playerbot/TravelNode.h>
#include "ChooseTravelTargetAction.h"
#include "playerbot/strategy/values/SharedValueContext.h"
#include "playerbot/strategy/values/FreeMoveValues.h"
#include "playerbot/strategy/actions/RpgSubActions.h"
#include "playerbot/LootObjectStack.h"
#include "GameEventMgr.h"
#include "playerbot/TravelMgr.h"
#include "playerbot/PlayerbotHelpMgr.h"
#include "Transports/Transport.h"
#include "Maps/PathFinder.h"
#include "playerbot/PlayerbotLLMInterface.h"

#include "vmap/VMapFactory.h"

#include "Maps/GridNotifiers.h"
#include "Maps/GridNotifiersImpl.h"
#include "Maps/CellImpl.h"

#include <iomanip>
#include <typeindex>
#include "SayAction.h"
#include "Maps/MoveMap.h"

using namespace ai;
using namespace MaNGOS;

bool DebugAction::HandleLLM(Event& event, Player* requester, const std::string& text)
{
    Player* player = bot;
    std::map<std::string, std::string> jsonFill;
    jsonFill["<prompt>"] = text.substr(4);
    jsonFill["<context>"] = "";
    jsonFill["<pre prompt>"] = "";
    jsonFill["<post prompt>"] = "";
    std::string json = BOT_TEXT2(sPlayerbotAIConfig.llmApiJson, jsonFill);
    std::vector<std::string> debugLines = { json };
    std::string response = PlayerbotLLMInterface::Generate(json, sPlayerbotAIConfig.llmGenerationTimeout, sPlayerbotAIConfig.llmMaxSimultaniousGenerations, debugLines);
    for(auto line : debugLines)
        ai->TellPlayerNoFacing(requester, line, PlayerbotSecurityLevel::PLAYERBOT_SECURITY_ALLOW_ALL, true, false);
    ai->TellPlayerNoFacing(requester, response, PlayerbotSecurityLevel::PLAYERBOT_SECURITY_ALLOW_ALL, true, false);
    return true;
}

bool DebugAction::HandleChatReplyDo(Event& event, Player* requester, const std::string& text)
{
    ChatReplyAction::ChatReplyDo(bot, CHAT_MSG_WHISPER, requester->GetGUIDLow(), 0, text.substr(12), "", requester->GetName());
    return true;
}

bool DebugAction::HandleMonsterTalk(Event& event, Player* requester, const std::string& text)
{
    GuidPosition guidP = GuidPosition(requester->GetSelectionGuid(), requester);
    Player* player = requester;
    if (!guidP.IsUnit())
        return false;
    Action* action = context->GetAction("rpg ai chat");
    if (!action)
        return false;
    RpgAIChatAction* rpgAction = (RpgAIChatAction*)action;
    if (!rpgAction)
        return false;
    std::string line = text.substr(12);
    rpgAction->ManualChat(guidP, line);
    return true;
}
