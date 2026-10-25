// pi-lens-ignore-file: all
#include "BotCommandHandlers.h"
#include "CommandShared.h"
#include "BotCommandContext.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "../runtime/BotManager.h"
#include "../runtime/BotActivityLease.h"
#include "../runtime/HireLifecycle.h"
#include "../runtime/ClaimLifecycle.h"
#include "../runtime/AltBotPolicy.h"
#include "../runtime/AltBotService.h"
#include "../runtime/HireProvisionService.h"
#include "../runtime/HireRaidPolicy.h"
#include "../runtime/HireSpecPolicy.h"
#include "../runtime/HireLevelPolicy.h"
#include "../runtime/HireTierRequestPolicy.h"
#include "BotCommandContext.h"
#include "../behavior/PlayerConvenience.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "../runtime/AhMarketService.h"
#include "../host/ModuleVersion.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "../runtime/PlayerbotAIStorage.h"
#include "../runtime/RandomBotAccountRegistry.h"
#include "../runtime/RandomBotPoolReset.h"
#include "../runtime/RandomBotService.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "../ai/playerbot/PlayerbotAI.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "../ai/playerbot/PlayerbotDbStore.h"
#include "../ai/playerbot/ChatHelper.h"
#include "../ai/playerbot/BotState.h"
#include "../ai/playerbot/PullOpeningPolicy.h"
#include "../ai/playerbot/PullStartPolicy.h"
#include "../ai/playerbot/PullRefusalPolicy.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "../ai/playerbot/strategy/generic/PullStrategy.h"
#include "../ai/playerbot/strategy/values/RtiTargetValue.h"
#include "../ai/playerbot/strategy/values/AttackersValue.h"
#include "../ai/playerbot/strategy/values/PossibleAttackTargetsValue.h"
#include "../ai/playerbot/strategy/values/PositionValue.h"
#include "../ai/playerbot/strategy/values/ItemUsageValue.h"
#include "../ai/playerbot/strategy/actions/EquipAction.h"
#include "../ai/playerbot/strategy/actions/UnequipAction.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "playerbot/PlayerbotFactory.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "playerbot/PlayerbotAIConfig.h"
#include "playerbot/SurvivePolicy.h"
#include "runtime/OwnedBotQolPolicy.h"
#include "../runtime/RemoteCommanderPolicy.h"
#include "../ai/playerbot/AiFactory.h"
#include "../runtime/RemoteCommanderView.h"
#include "../runtime/RemoteCommanderTasks.h"
#include "../runtime/RemoteCommanderEnter.h"
#include "../runtime/CommanderTelemetry.h"
#include "../ai/playerbot/LootObjectStack.h"
#include "../ai/playerbot/strategy/Engine.h"
#include "../ai/playerbot/strategy/Value.h"
#include "../ai/playerbot/GuidPosition.h"
#include "../host/ModuleLog.h"
#include "../ai/playerbot/strategy/shaman/ShamanTotemCommandPolicy.h"
#include "../ai/playerbot/strategy/shaman/ShamanTotemGroupPolicy.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "Chat.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "ObjectMgr.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "ObjectAccessor.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "Player.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "World.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "Group.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "WorldSession.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "WorldPacket.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "Log.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "Group/Group.h"
#include "Database/DatabaseEnv.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "Maps/PathFinder.h"
#include "Movement/spline/MoveSplineInit.h"
#include "Maps/Cell.h"
#include "Maps/Map.h"
#include "Database/DBCStores.h"
#include "Database/SQLStorages.h"
#include "Maps/CellImpl.h"
#include "Maps/GridNotifiers.h"
#include "Maps/GridNotifiersImpl.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "Creature.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "Spell.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "Threat/ThreatManager.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <initializer_list>
#include <memory>
#include <cstring>
#include <sstream>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace TortoiseBots {
namespace BotCommands {

bool HandleAhBot(ChatHandler* handler, char const* args)
{
    if (!handler)
        return false;

    Player* requester = Requester(handler);
    if (requester && !IsBotAdministrator(requester))
    {
        handler->PSendSysMessage("You do not have permission to manage the AH bot.");
        return true;
    }

    std::string argStr = args ? args : "";
    argStr = Trim(argStr);
    if (argStr.empty() || argStr == "help")
    {
        handler->PSendSysMessage("AHBot commands:");
        handler->PSendSysMessage("  .bot ah status - Display synthetic AH engine status and telemetry");
        handler->PSendSysMessage("  .bot ah reload - Reload price overrides and bans from ahbot_items");
        handler->PSendSysMessage("  .bot ah rebuild [all] - Restart market pass (all = expire active unbid synthetic items)");
        handler->PSendSysMessage("  .bot ah item <id> - View override for item");
        handler->PSendSysMessage("  .bot ah item <id> reset - Remove override for item");
        handler->PSendSysMessage("  .bot ah item <id> <value> [chance] [min] [max] - Set override (0 0 = blacklist)");
        return true;
    }

    std::stringstream ss(argStr);
    std::string subCmd;
    ss >> subCmd;
    for (char& c : subCmd) c = tolower(c);

    if (subCmd == "status")
    {
        std::string status = AhMarketService::Instance().GetStatus();
        handler->PSendSysMessage("%s", status.c_str());
        return true;
    }

    if (subCmd == "reload")
    {
        AhMarketService::Instance().ReloadOverrides();
        handler->PSendSysMessage("AHBot overrides reloaded from database.");
        return true;
    }

    if (subCmd == "rebuild")
    {
        std::string mode;
        ss >> mode;
        for (char& c : mode) c = tolower(c);
        bool all = (mode == "all");
        AhMarketService::Instance().RebuildMarket(all);
        if (all)
            handler->PSendSysMessage("AHBot market rebuild scheduled (expiring active unbid synthetic listings).");
        else
            handler->PSendSysMessage("AHBot market rebuild scheduled.");
        return true;
    }

    if (subCmd == "item")
    {
        uint32_t itemId = 0;
        if (!(ss >> itemId) || itemId == 0)
        {
            handler->PSendSysMessage("Usage: .bot ah item <id> [value [chance [min [max]]]] / reset");
            return true;
        }

        std::string nextToken;
        if (!(ss >> nextToken))
        {
            if (AhMarketService::Instance().IsItemBlacklisted(itemId))
            {
                handler->PSendSysMessage("AHBot item %u is blacklisted/banned.", itemId);
            }
            else
            {
                handler->PSendSysMessage("AHBot item %u has no blacklist or has an active price override.", itemId);
            }
            return true;
        }

        for (char& c : nextToken) c = tolower(c);
        if (nextToken == "reset")
        {
            AhMarketService::Instance().ResetItemOverride(itemId);
            handler->PSendSysMessage("AHBot item %u override reset.", itemId);
            return true;
        }

        uint32_t value = 0;
        try {
            value = std::stoul(nextToken);
        } catch (...) {
            handler->PSendSysMessage("Invalid value '%s' for item override.", nextToken.c_str());
            return true;
        }

        uint32_t chance = 100;
        uint32_t minAmount = 1;
        uint32_t maxAmount = 1;

        if (ss >> chance)
        {
            if (ss >> minAmount)
            {
                if (!(ss >> maxAmount))
                    maxAmount = minAmount;
            }
        }

        AhMarketService::Instance().SetItemOverride(itemId, value, chance, minAmount, maxAmount);
        if (value == 0 && chance == 0)
        {
            handler->PSendSysMessage("AHBot item %u blacklisted (value=0, chance=0).", itemId);
        }
        else
        {
            handler->PSendSysMessage("AHBot item %u override set: value=%u copper, chance=%u%%, amount=%u..%u.",
                itemId, value, chance, minAmount, maxAmount);
        }
        return true;
    }

    handler->PSendSysMessage("Unknown AHBot command '%s'. Try .bot ah help", subCmd.c_str());
    return true;
}


} // namespace BotCommands
} // namespace TortoiseBots
