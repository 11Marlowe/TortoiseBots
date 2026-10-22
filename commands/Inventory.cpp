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

// Mirrors WorldSession::HandleSetTradeItemOpcode, which cancels the whole
// trade on a refused item: only offer items the core will accept.
static bool CanTradeItemTo(Player* bot, Item* item, Player* requester)
{
    if (!bot || !item || !requester || !item->CanBeTraded())
        return false;
    if (item->IsSoulBound())
        return bot->GetMapId() == item->GetOriginMapId() &&
            bot->GetGroup() == requester->GetGroup() &&
            item->CanTradeSoulBoundToPlayer(requester->GetObjectGuid());
    return true;
}

// Flags: e = the bot can equip it, u = the mature item-usage value calls it
// an upgrade, b = it cannot be traded to the requester right now.
static std::string InventoryItemFlags(PlayerbotAI* ai, Player* bot, Player* requester, Item* item)
{
    std::string flags;
    ItemPrototype const* proto = item->GetProto();
    if (proto && proto->InventoryType != INVTYPE_NON_EQUIP && bot->CanUseItem(proto) == EQUIP_ERR_OK)
    {
        flags += 'e';
        ai::ItemUsage usage = ai->GetAiObjectContext()->GetValue<ai::ItemUsage>("item usage",
            ai::ItemQualifier(item).GetQualifier())->Get();
        if (usage == ai::ItemUsage::ITEM_USAGE_EQUIP)
            flags += 'u';
    }
    if (!CanTradeItemTo(bot, item, requester))
        flags += 'b';
    return flags.empty() ? "-" : flags;
}

// TBM:INV_BEGIN|<bot>|<copper>
// TBM:INV_EQ|<slot>|<itemId>|<durability>|<maxDurability>
// TBM:INV_ITEM|<bag>|<slot>|<itemId>|<count>|<flags>
// TBM:INV_END|<bot>|<freeSlots>|<totalSlots>
static void SendInventorySnapshot(ChatHandler* handler, Player* bot, Player* requester)
{
    PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
    if (!handler || !bot || !ai)
        return;
    std::string const name = ProtocolSafe(bot->GetName());
    handler->PSendSysMessage("TBM:INV_BEGIN|%s|%u", name.c_str(), bot->GetMoney());
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;
        handler->PSendSysMessage("TBM:INV_EQ|%u|%u|%u|%u", static_cast<uint32>(slot), item->GetEntry(),
            item->GetUInt32Value(ITEM_FIELD_DURABILITY), item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY));
    }

    uint32 freeSlots = 0;
    uint32 totalSlots = 0;
    auto emit = [&](uint8 bag, uint8 slot, Item* item)
    {
        ++totalSlots;
        if (!item)
        {
            ++freeSlots;
            return;
        }
        handler->PSendSysMessage("TBM:INV_ITEM|%u|%u|%u|%u|%s", static_cast<uint32>(bag),
            static_cast<uint32>(slot), item->GetEntry(), item->GetCount(),
            InventoryItemFlags(ai, bot, requester, item).c_str());
    };
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        emit(INVENTORY_SLOT_BAG_0, slot, bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));
    for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END; ++bagSlot)
    {
        Bag* bag = (Bag*)bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bagSlot);
        if (!bag)
            continue;
        for (uint32 slot = 0; slot < bag->GetBagSize(); ++slot)
            emit(bagSlot, static_cast<uint8>(slot), bag->GetItemByPos(static_cast<uint8>(slot)));
    }
    handler->PSendSysMessage("TBM:INV_END|%s|%u|%u", name.c_str(), freeSlots, totalSlots);
}

bool HandleInventory(ChatHandler* handler, char const* args)
{
    Player* requester = Requester(handler);
    Player* bot = nullptr;
    BotRecord* record = nullptr;
    std::string name;
    if (!requester || !ResolveOwnedBot(handler, args, bot, record, name) ||
        !PlayerbotAIStorage::Instance().GetAI(bot))
    {
        SendActionError(handler, "inv", "no-bot", "Usage: .bot inv <online bot you control>");
        return true;
    }
    SendInventorySnapshot(handler, bot, requester);
    return true;
}

// Issue #473: opt-in owned/hired-bot quality of life (all parts off by
// default, pool bots never eligible). Maintenance refreshes one owned bot's
// kit through the same bounded helpers as the hire/refresh paths
// (consumables, reagents, ammo, thrown, bandages, then a free repair);
// autogear re-gears incrementally within the quality/ilvl caps. With the
// flags off the commands reply that the feature is disabled.
bool IsOwnedBotQolEligible(Player* bot, BotRecord const* record)
{
    if (!bot || !record)
        return false;
    if (!BotManager::Instance().IsBot(bot->GetObjectGuid()))
        return false;
    if (BotManager::Instance().IsRandomBot(bot->GetObjectGuid()))
        return HireLifecycle::Instance().IsHired(bot->GetObjectGuid());
    return true;
}

bool HandleMaintenance(ChatHandler* handler, char const* args)
{
    Player* requester = Requester(handler);
    Player* bot = nullptr;
    BotRecord* record = nullptr;
    std::string name;
    if (!requester || !ResolveOwnedBot(handler, args, bot, record, name))
    {
        handler->PSendSysMessage("Usage: .bot maintenance <online bot name> (same account only)");
        return true;
    }
    if (!IsOwnedBotQolEligible(bot, record))
    {
        handler->PSendSysMessage("'%s' is a pool bot and keeps progressing organically.", name.c_str());
        return true;
    }
    if (!sPlayerbotAIConfig.ownedBotMaintenanceEnabled)
    {
        handler->PSendSysMessage("Owned-bot maintenance is disabled (AiPlayerbot.OwnedBotMaintenanceEnabled = 0).");
        return true;
    }
    if (!bot->IsAlive())
    {
        handler->PSendSysMessage("Bot '%s' must be alive.", name.c_str());
        return true;
    }
    PlayerbotFactory factory(bot, bot->GetLevel());
    factory.AddReagents();
    factory.AddPotions();
    factory.AddFood();
    factory.AddConsumes();
    factory.AddBandages();
    factory.InitAmmo();
    factory.InitThrown();
    bot->DurabilityRepairAll(false, 0.0f);
    bot->SaveToDB();
    handler->PSendSysMessage("Maintained %s (consumables, reagents, ammo, repair).", name.c_str());
    return true;
}

// Opens a trade window between the bot and the requester. The core creates
// both trade records immediately; the requester's client opens its window
// when TRADE_STATUS_BEGIN_TRADE arrives.
static bool OpenTradeWith(Player* bot, Player* requester)
{
    if (bot->GetTrader() == requester)
        return true;
    if (bot->GetTrader() || requester->GetTrader())
        return false;
    WorldPacket packet(CMSG_INITIATE_TRADE, 8);
    packet << requester->GetObjectGuid();
    bot->GetSession()->HandleInitiateTradeOpcode(packet);
    return bot->GetTrader() == requester;
}

// .bot item <bot> trade
// .bot item <bot> equip|unequip|give <bag> <slot>
// Every reply is one TBM:ACTION_ACK / TBM:ACTION_ERR with intent "item <op>";
// equip and unequip follow it with a fresh inventory snapshot.
bool HandleItem(ChatHandler* handler, char const* args)
{
    Player* requester = Requester(handler);
    std::istringstream input(Trim(args ? args : ""));
    std::string botName;
    std::string op;
    input >> botName >> op;
    for (char& c : op)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    std::string const intent = "item " + (op.empty() ? std::string("?") : op);
    if (op != "trade" && op != "equip" && op != "unequip" && op != "give")
    {
        SendActionError(handler, intent, "invalid", "Usage: .bot item <bot> trade | equip/unequip/give <bag> <slot>");
        return true;
    }

    Player* bot = nullptr;
    BotRecord* record = nullptr;
    std::string name;
    PlayerbotAI* ai = nullptr;
    if (!requester || !ResolveOwnedBot(handler, botName.c_str(), bot, record, name) ||
        !(ai = PlayerbotAIStorage::Instance().GetAI(bot)))
    {
        SendActionError(handler, intent, "no-bot", "Unknown or uncontrollable bot name.");
        return true;
    }
    std::string const scope = "bot:" + std::string(bot->GetName());
    if (!bot->IsAlive())
    {
        SendActionError(handler, intent, "dead", "The bot must be alive.");
        return true;
    }

    if (op == "trade")
    {
        if (!OpenTradeWith(bot, requester))
        {
            SendActionError(handler, intent, "busy", "Trade could not be opened (busy, too far, or in combat).");
            return true;
        }
        SendActionAck(handler, intent, scope, 1, "open");
        return true;
    }

    uint32 bag = 0;
    uint32 slot = 0;
    if (!(input >> bag >> slot) || bag > 255 || slot > 255)
    {
        SendActionError(handler, intent, "invalid", "Missing bag and slot.");
        return true;
    }
    // Carried items only: equipment, backpack and equipped bags. Bank slots
    // share bag 255 with the backpack and must never be reachable from here.
    bool const equipped = Player::IsEquipmentPos(static_cast<uint8>(bag), static_cast<uint8>(slot));
    if (!equipped && !Player::IsInventoryPos(static_cast<uint8>(bag), static_cast<uint8>(slot)))
    {
        SendActionError(handler, intent, "invalid", "Only carried items can be ordered.");
        return true;
    }
    Item* item = bot->GetItemByPos(static_cast<uint8>(bag), static_cast<uint8>(slot));
    if (!item || !item->GetProto())
    {
        SendActionError(handler, intent, "no-item", "That item is no longer there; refresh the bag view.");
        SendInventorySnapshot(handler, bot, requester);
        return true;
    }
    uint32 const itemId = item->GetEntry();

    if (op == "equip")
    {
        ItemPrototype const* proto = item->GetProto();
        if (equipped || proto->InventoryType == INVTYPE_NON_EQUIP || bot->CanUseItem(proto) != EQUIP_ERR_OK)
        {
            SendActionError(handler, intent, "cannot-equip", "The bot cannot equip this item.");
            return true;
        }
        ai::EquipAction::EquipItem(ai, requester, item, true);
        // Equip and bag slots all sit below the backpack range; ammo stays in
        // the bag and is equipped through the ammo field instead.
        bool const done = proto->InventoryType == INVTYPE_AMMO
            ? bot->GetUInt32Value(PLAYER_AMMO_ID) == itemId
            : item->GetBagSlot() == INVENTORY_SLOT_BAG_0 && item->GetSlot() < INVENTORY_SLOT_ITEM_START;
        if (!done)
            SendActionError(handler, intent, "failed", "The equip was refused (combat, level, or skill).");
        else
            SendActionAck(handler, intent, scope, 1, std::to_string(itemId));
        SendInventorySnapshot(handler, bot, requester);
        return true;
    }

    if (op == "unequip")
    {
        if (!equipped)
        {
            SendActionError(handler, intent, "not-equipped", "That item is not equipped.");
            return true;
        }
        ai::UnequipAction::UnequipItem(ai, requester, item, true);
        if (bot->GetItemByPos(static_cast<uint8>(bag), static_cast<uint8>(slot)) == item)
            SendActionError(handler, intent, "failed", "No free bag space, or the item cannot be removed now.");
        else
            SendActionAck(handler, intent, scope, 1, std::to_string(itemId));
        SendInventorySnapshot(handler, bot, requester);
        return true;
    }

    // give: put the item into a trade window with the requester. Without an
    // open trade the first request opens one and answers "pending"; the addon
    // repeats the request once its trade window is shown.
    if (equipped)
    {
        SendActionError(handler, intent, "equipped", "Unequip the item first.");
        return true;
    }
    if (!CanTradeItemTo(bot, item, requester))
    {
        SendActionError(handler, intent, "no-trade", "This item cannot be traded (soulbound or quest item).");
        return true;
    }
    if (bot->GetTrader() != requester)
    {
        if (!OpenTradeWith(bot, requester))
        {
            SendActionError(handler, intent, "busy", "Trade could not be opened (busy, too far, or in combat).");
            return true;
        }
        SendActionAck(handler, intent, scope, 1, std::to_string(itemId) + " pending");
        return true;
    }
    if (item->IsInTrade())
    {
        SendActionAck(handler, intent, scope, 1, std::to_string(itemId));
        return true;
    }
    TradeData* trade = bot->GetTradeData();
    int tradeSlot = -1;
    for (uint8 i = 0; trade && i < TRADE_SLOT_TRADED_COUNT && tradeSlot < 0; ++i)
        if (!trade->GetItem(TradeSlots(i)))
            tradeSlot = i;
    if (tradeSlot < 0)
    {
        SendActionError(handler, intent, "trade-full", "The trade window is full.");
        return true;
    }
    WorldPacket packet(CMSG_SET_TRADE_ITEM, 3);
    packet << static_cast<uint8>(tradeSlot) << static_cast<uint8>(bag) << static_cast<uint8>(slot);
    bot->GetSession()->HandleSetTradeItemOpcode(packet);
    if (!item->IsInTrade())
    {
        SendActionError(handler, intent, "failed", "The trade refused this item.");
        return true;
    }
    SendActionAck(handler, intent, scope, 1, std::to_string(itemId));
    return true;
}


} // namespace BotCommands
} // namespace TortoiseBots
