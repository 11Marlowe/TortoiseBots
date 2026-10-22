// pi-lens-ignore-file: all
#include "BotCommandHandlers.h"
#include "CommandShared.h"
#include "BotCommands.h"
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
// Lens fallback stubs — core headers absent in static analysis. Real build uses
// the mangosd headers via -I src/game. Guards use the same macros as the
// real headers so the stubs are never active in a real build.
#ifndef MANGOSSERVER_CHAT_H
class WorldSession;
class ChatHandler {
public:
    void PSendSysMessage(const char* fmt, ...) {}
    void SendSysMessage(const char*) {}
    WorldSession* GetSession() const { return nullptr; }
};
#endif
#ifndef _OBJECTMGR_H
struct PlayerCacheData { uint32_t uiGuid; uint32_t uiAccount; };
struct LensOM { PlayerCacheData* GetPlayerDataByName(const std::string&) { return nullptr; } PlayerCacheData* GetPlayerDataByGUID(uint32_t) { return nullptr; } Player* GetPlayer(const char*) { return nullptr; } };
static LensOM sObjectMgr;
inline bool normalizePlayerName(std::string&, size_t = 32, bool = true) { return true; }
#endif
#ifndef MANGOS_OBJECTACCESSOR_H
class Player;
struct LensOA { Player* FindPlayerByName(const char*) { return nullptr; } Player* FindPlayer(ObjectGuid) { return nullptr; } };
static LensOA sObjectAccessor;
#endif
#ifndef __UNIT_H
using uint32 = uint32_t;
constexpr int SEC_GAMEMASTER = 3;
#ifndef OBJECT_GUID_H
struct ObjectGuid { uint32 GetCounter() const { return 0; } };
#endif
#ifndef HIGHGUID_PLAYER
constexpr int HIGHGUID_PLAYER = 0;
#endif
constexpr int CHAT_MSG_WHISPER = 1;
class WorldPacket { public: WorldPacket() {} template<typename T> WorldPacket& operator<<(T const&) { return *this; } };
class Player {
public:
    ObjectGuid GetObjectGuid() const { return ObjectGuid(); }
    const char* GetName() const { return ""; }
    WorldSession* GetSession() const { return nullptr; }
    void* GetGroupInvite() const { return nullptr; }
    bool IsInSameGroupWith(Player*) const { return false; }
};
class WorldSession {
public:
    Player* GetPlayer() const { return nullptr; }
    int GetSecurity() const { return 0; }
    uint32 GetAccountId() const { return 0; }
    bool IsHeadless() const { return false; }
    void HandleGroupInviteOpcode(WorldPacket&) {}
    void HandleGroupUninviteOpcode(WorldPacket&) {}
};
#endif
#ifndef MANGOSSERVER_LOG_H
struct Log { void outString(const char*, ...) {} void outError(const char*, ...) {} };
static Log sLog;
#endif

namespace TortoiseBots {
namespace BotCommands {

// ── TBM bot panel: inventory snapshot, item orders, behaviour toggles ──────
// The panel addresses items by the server's own bag/slot pair, so an order
// always lands on the exact item the player clicked - never on "the first
// item with this name" like the whisper grammar.

bool HandleAutogear(ChatHandler* handler, char const* args)
{
    std::string text = Trim(args ? args : "");
    size_t split = text.find_first_of(" \t");
    std::string botToken = split == std::string::npos ? text : text.substr(0, split);
    std::string param = split == std::string::npos ? "" : text.substr(split + 1);
    Player* requester = Requester(handler);
    Player* bot = nullptr;
    BotRecord* record = nullptr;
    std::string name;
    if (!requester || botToken.empty() || !ResolveOwnedBot(handler, botToken.c_str(), bot, record, name))
    {
        handler->PSendSysMessage("Usage: .bot autogear <online bot name> [quality|ilvl] (same account only)");
        return true;
    }
    if (!IsOwnedBotQolEligible(bot, record))
    {
        handler->PSendSysMessage("'%s' is a pool bot and keeps progressing organically.", name.c_str());
        return true;
    }
    if (!sPlayerbotAIConfig.ownedBotAutogearEnabled)
    {
        handler->PSendSysMessage("Owned-bot autogear is disabled (AiPlayerbot.OwnedBotAutogearEnabled = 0).");
        return true;
    }
    if (!bot->IsAlive())
    {
        handler->PSendSysMessage("Bot '%s' must be alive.", name.c_str());
        return true;
    }
    OwnedBotAutogearRequest parsed = ParseOwnedBotAutogearArg(param,
        sPlayerbotAIConfig.ownedBotAutogearQualityCap, sPlayerbotAIConfig.ownedBotAutogearIlvlCap);
    if (!parsed.ok)
    {
        handler->PSendSysMessage("Unknown autogear option '%s'. Use a quality (white, green, blue, purple) or an item level number.", Trim(param).c_str());
        return true;
    }
    PlayerbotFactory factory(bot, bot->GetLevel());
    factory.AutogearOwned(parsed.quality, parsed.ilvlCap);
    handler->PSendSysMessage("Re-geared %s within the configured caps.", name.c_str());

    // Autogear hands out no ammo: say so when the ranged weapon it left
    // equipped has none in the bags (auto shot cannot fire), like the tank's
    // "no shield equipped" warning.
    Item const* ranged = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED);
    ItemPrototype const* rangedProto = ranged ? ranged->GetProto() : nullptr;
    if (rangedProto && rangedProto->Class == ITEM_CLASS_WEAPON && !factory.HasAmmoInBagsFor(rangedProto->SubClass))
    {
        bool const gun = rangedProto->SubClass == ITEM_SUBCLASS_WEAPON_GUN;
        handler->PSendSysMessage("Warning: %s has %s equipped but no %s in the bags - auto shot will not fire until %s gets some (owned-bot maintenance or hand them over).",
            name.c_str(), rangedProto->Name1, gun ? "bullets" : "arrows", name.c_str());
    }
    return true;
}

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

// ---- Managed random-bot pool (issue #265) ---------------------------------
// The module's AllCommandScript intercepts `.bot` before the core command
// table can protect a subcommand, so every permission check below is local.
// Adoption and account listing are server-console-only: no player, addon
// message, hired bot or headless session can reach them.
static bool IsServerConsole(ChatHandler* handler)
{
    if (!handler || handler->GetSession())
        return false;
#ifdef MANGOSSERVER_CHAT_H
    // Remote-access and Discord commands also use null-session CliHandler
    // instances, carrying the calling account's security level. ChatHandler
    // keeps GetAccessLevel() protected, so the concrete handler type is the only
    // way to read it: only the stdin console (and operator-written
    // `pending_commands` rows, which the core also queues as SEC_CONSOLE) carries
    // SEC_CONSOLE, so a remote user of any account rank is rejected here.
    CliHandler* cli = dynamic_cast<CliHandler*>(handler);
    return cli && cli->GetAccessLevel() == SEC_CONSOLE;
#else
    return true; // lens stub: the real build always takes the branch above
#endif
}

static bool RequirePoolAdmin(ChatHandler* handler)
{
    if (IsServerConsole(handler))
        return true;
    WorldSession* session = handler ? handler->GetSession() : nullptr;
    if (!session || !session->GetPlayer())
    {
        handler->PSendSysMessage("You must be in-game or at the server console.");
        return false;
    }
    if (session->GetSecurity() < SEC_GAMEMASTER)
    {
        handler->PSendSysMessage("Your account may not inspect the bot pool.");
        return false;
    }
    return true;
}

static bool RequireServerConsole(ChatHandler* handler)
{
    if (IsServerConsole(handler))
        return true;
    handler->PSendSysMessage("This command is only available at the server console.");
    return false;
}

static bool HandlePoolStatus(ChatHandler* handler)
{
    if (!RequirePoolAdmin(handler))
        return true;

    RandomBotAccountRegistry& registry = RandomBotAccountRegistry::Instance();
    RandomBotPoolReset& reset = RandomBotPoolReset::Instance();

    // Parsed from the module's startup configuration (aiplayerbot.conf is read
    // once at startup, so `.reload config` cannot change what a reset does).
    // This reports the configured mode even when the service never planned one.
    PoolResetSetting setting = ParsePoolResetSetting(sPlayerbotAIConfig.randomBotPoolReset);
    std::string mode = PoolResetModeName(setting.mode);
    if (setting.mode == PoolResetMode::Once)
        mode += ":" + setting.token;
    if (setting.mode == PoolResetMode::Invalid)
        mode += " (" + setting.error + ")";

    // Live count of the characters on managed pool accounts. A failed count is
    // reported as unavailable rather than as zero.
    uint32 managedCharacters = 0;
    std::string countError;
    bool haveCharacterCount = registry.CountManagedCharacters(managedCharacters, countError);

    handler->PSendSysMessage("TortoiseBots pool status:");
    handler->PSendSysMessage("  reset setting      : %s", mode.c_str());
    if (setting.mode == PoolResetMode::Once)
        handler->PSendSysMessage("  requested generation: %s", setting.token.c_str());
    handler->PSendSysMessage("  applied generation : %s",
        reset.AppliedGeneration().empty() ? "(none)" : reset.AppliedGeneration().c_str());
    handler->PSendSysMessage("  managed accounts   : %u%s",
        static_cast<uint32>(registry.AccountCount()),
        registry.IsValidated() ? "" : " (registry NOT validated)");
    if (haveCharacterCount)
        handler->PSendSysMessage("  managed characters : %u", managedCharacters);
    else
        handler->PSendSysMessage("  managed characters : (count unavailable: %s)", countError.c_str());
    handler->PSendSysMessage("  reset phase        : %s", reset.PhaseName());
    if (reset.TargetCount() && reset.Phase() == PoolResetPhase::SettlingAuctions)
        handler->PSendSysMessage("  reset progress     : %u/%u characters' auctions settled",
            reset.SettledAuctionOwnerCount(), reset.TargetCount());
    else if (reset.TargetCount())
        handler->PSendSysMessage("  reset progress     : %u/%u characters deleted",
            reset.ProcessedCount(), reset.TargetCount());
    // The service is the single authority for availability: it combines the
    // registry state with the reset state.
    handler->PSendSysMessage("  pool available     : %s",
        RandomBotService::Instance().IsPoolAvailable() ? "yes" : "no");
    if (!reset.LastFailure().empty())
        handler->PSendSysMessage("  last failure       : %s", reset.LastFailure().c_str());
    return true;
}

static bool HandlePoolAccounts(ChatHandler* handler)
{
    if (!RequireServerConsole(handler))
        return true;

    RandomBotAccountRegistry& registry = RandomBotAccountRegistry::Instance();
    if (!registry.IsValidated())
    {
        handler->PSendSysMessage("The managed-account registry could not be read: %s",
            registry.LastError().empty() ? "unknown error" : registry.LastError().c_str());
        return true;
    }

    uint32 total = 0;
    std::string countError;
    bool haveTotal = registry.CountManagedCharacters(total, countError);
    handler->PSendSysMessage("Managed pool accounts: %u, characters: %s",
        static_cast<uint32>(registry.AccountCount()),
        haveTotal ? std::to_string(total).c_str() : "(count unavailable)");
    for (PoolAccount const& account : registry.Accounts())
    {
        uint32 characters = 0;
        std::unique_ptr<QueryResult> count(CharacterDatabase.PQuery(
            "SELECT COUNT(*) FROM `characters` WHERE `deleteDate` IS NULL AND `account` = '%u'", account.accountId));
        if (count)
            characters = count->Fetch()[0].GetUInt32();
        handler->PSendSysMessage("  #%u %s [%s] - %u character(s)%s",
            account.accountId, account.username.c_str(), RegistrationSourceName(account.source), characters,
            count ? "" : " (character count unavailable)");
    }
    if (!registry.AccountCount())
        handler->PSendSysMessage("  (none; the pool is empty)");
    return true;
}

static bool HandlePoolAdopt(ChatHandler* handler, char const* args)
{
    if (!RequireServerConsole(handler))
        return true;

    std::string sub = Trim(args ? args : "");
    std::string subLower = sub;
    for (char& c : subLower)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    RandomBotAccountRegistry& registry = RandomBotAccountRegistry::Instance();

    if (subLower == "preview")
    {
        RandomBotAccountRegistry::AdoptionPreview preview = registry.PreviewLegacyAdoption();
        if (!preview.ok)
        {
            handler->PSendSysMessage("Adoption preview failed: %s", preview.error.c_str());
            return true;
        }

        handler->PSendSysMessage("Accounts matching prefix '%s': %u", preview.prefix.c_str(),
            static_cast<uint32>(preview.accounts.size()));
        for (RandomBotAccountRegistry::LegacyAccount const& account : preview.accounts)
            handler->PSendSysMessage("  #%u %s - %u character(s) %s",
                account.accountId, account.username.c_str(), account.characterCount,
                account.alreadyRegistered ? "[already managed]" : "[NOT managed]");

        if (preview.accounts.empty())
        {
            handler->PSendSysMessage("Nothing to adopt.");
            return true;
        }

        handler->PSendSysMessage("Total: %u account(s), %u character(s); %u account(s) still need adoption.",
            static_cast<uint32>(preview.accounts.size()), preview.totalCharacters, preview.pendingAccounts);
        handler->PSendSysMessage("Adoption registers accounts only; no character is changed or deleted.");
        if (preview.pendingAccounts)
            handler->PSendSysMessage("To enroll them, run exactly: bot pool adopt confirm %s (valid for 5 minutes)",
                preview.challenge.c_str());
        else
            handler->PSendSysMessage("Every matching account is already managed.");
        return true;
    }

    if (subLower.rfind("confirm", 0) == 0)
    {
        std::string challenge = Trim(subLower.substr(7));
        if (challenge.empty())
        {
            handler->PSendSysMessage("Usage: bot pool adopt confirm <challenge> (from 'bot pool adopt preview')");
            return true;
        }

        RandomBotAccountRegistry::AdoptionConfirm confirm = registry.ConfirmLegacyAdoption(challenge, time(nullptr));
        if (!confirm.ok)
        {
            handler->PSendSysMessage("Adoption aborted: %s", confirm.error.c_str());
            return true;
        }
        handler->PSendSysMessage("Adoption complete: %u account(s) registered, %u already managed. No character was changed.",
            confirm.adopted, confirm.alreadyRegistered);
        return true;
    }

    handler->PSendSysMessage("Usage: bot pool adopt preview / bot pool adopt confirm <challenge>");
    return true;
}

bool HandlePool(ChatHandler* handler, char const* args)
{
    std::string sub = Trim(args ? args : "");
    std::string subLower = sub;
    for (char& c : subLower)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (subLower == "status")
        return HandlePoolStatus(handler);
    if (subLower == "accounts")
        return HandlePoolAccounts(handler);
    if (subLower.rfind("adopt", 0) == 0)
        return HandlePoolAdopt(handler, subLower.substr(5).c_str());

    handler->PSendSysMessage("Usage: bot pool status / accounts / adopt preview / adopt confirm <challenge>");
    return true;
}

// `.bot help` answers with the addon protocol version line; the TBM addon
// parses it, so the output must stay exactly as it is.
bool HandleHelp(ChatHandler* handler, char const* /*args*/)
{
    handler->PSendSysMessage("TBM:VERSION|%s", BuildVersion().c_str());
    return true;
}

// One row per `.bot` subcommand: primary name, handler, short usage, aliases.
// Dispatch, the top-level usage line and the docs check
// (tools/verify_command_docs.py) all read this table.
struct BotCommandEntry
{
    char const* name;
    bool (*handler)(ChatHandler*, char const*);
    char const* usage;
    std::initializer_list<char const*> aliases;
};

static BotCommandEntry const botCommands[] = {
    {"ah", HandleAhBot, "ah <status|reload|rebuild|item ...>", {"ahbot"}},
    {"action", HandleAction, "action <intent>", {}},
    {"commander", HandleCommander, "commander <bind|unbind|status|view ...>", {}},
    {"cmd", HandleCommanderOrder, "cmd <order>", {}},
    {"add", HandleAdd, "add <characterName>", {}},
    {"remove", HandleRemove, "remove <characterName>", {}},
    {"logout", HandleLogout, "logout <online bot name>", {}},
    {"roster", HandleRoster, "roster", {}},
    {"follow", HandleFollow, "follow <botName>", {}},
    {"invite", HandleInvite, "invite <online bot name>", {}},
    {"uninvite", HandleUninvite, "uninvite <online bot name>", {"kick"}},
    {"stay", HandleStay, "stay <online bot name>", {}},
    {"guard", HandleGuard, "guard <online bot name>", {}},
    {"free", HandleFree, "free <online bot name>", {}},
    {"ready", HandleReady, "ready <online bot name>", {}},
    {"attack", HandleAttack, "attack <online bot name>", {}},
    {"interrupt", HandleInterrupt, "interrupt <online bot name>", {}},
    {"formation", HandleFormation, "formation [botName] <formation>", {}},
    {"list", HandleList, "list", {}},
    {"stats", HandleStats, "stats", {}},
    {"status", HandleStatus, "status <online bot name>", {}},
    {"lease", HandleLease, "lease [status]", {}},
    {"version", HandleVersion, "version", {"v", "about", "credits"}},
    {"pullback", HandlePullback, "pullback", {"pull-back"}},
    {"role", HandleRole, "role <online bot name/self/your name> <tank/healer/dps/clear>", {}},
    {"summon", HandleSummon, "summon <online bot name>", {}},
    {"maintenance", HandleMaintenance, "maintenance <online bot name>", {}},
    {"autogear", HandleAutogear, "autogear <online bot name> [quality|ilvl]", {}},
    {"hire", HandleHire, "hire <class> [dps/tank/healer] ... | hire raid <preset> [tier]", {}},
    {"pool", HandlePool, "pool <status|accounts|adopt ...>", {}},
    {"loot", HandleLoot, "loot [on/off]", {}},
    {"repair", HandleRepair, "repair", {}},
    {"sell", HandleSell, "sell", {}},
    {"rest", HandleRest, "rest", {"drink", "eat"}},
    {"release", HandleRelease, "release", {}},
    {"corpse", HandleCorpseShortcut, "corpse run", {}},
    {"corpserun", HandleCorpseRun, "corpserun", {}},
    {"learn", HandleLearn, "learn", {}},
    {"trade", HandleTrade, "trade", {}},
    {"command", HandleMatureCommand, "command <botName> <Playerbot command>", {}},
    {"inv", HandleInventory, "inv <online bot you control>", {"inventory"}},
    {"item", HandleItem, "item <bot> <trade|equip|unequip|give ...>", {}},
    {"behavior", HandleBehavior, "behavior <bot|party|raid> <behavior> <on|off>", {"behaviour"}},
    {"totem", HandleTotem, "totem <bot|party|raid> ...", {}},
    {"strategy", HandleStrategy, "strategy <+/-/~><strategy name> [role or botName]", {}},
    {"help", HandleHelp, "help", {"h"}},
};

static BotCommandEntry const* FindBotCommand(std::string const& cmd)
{
    for (BotCommandEntry const& entry : botCommands)
    {
        if (cmd == entry.name)
            return &entry;
        for (char const* alias : entry.aliases)
            if (cmd == alias)
                return &entry;
    }
    return nullptr;
}

static std::string BuildBotUsageLine()
{
    std::string usage = "Usage: .bot ";
    for (size_t i = 0; i < sizeof(botCommands) / sizeof(botCommands[0]); ++i)
    {
        if (i)
            usage += '/';
        usage += botCommands[i].name;
    }
    return usage;
}

// pi-lens-ignore: clang:incomplete_member_access,clang:unknown_typename
bool HandleChatCommand(ChatHandler* handler, char const* args)
{
    if (!handler || !args)
        return false;

    // Trim leading spaces
    while (*args == ' ' || *args == '\t') ++args;
    if (!*args)
    {
        handler->PSendSysMessage("%s", BuildBotUsageLine().c_str());
        return true;
    }

    // Extract subcommand
    std::string cmd;
    const char* p = args;
    while (*p && *p != ' ' && *p != '\t')
    {
        cmd += *p;
        ++p;
    }
    while (*p == ' ' || *p == '\t') ++p;
    const char* subArgs = p;

    // Normalize cmd to lowercase
    for (char& c : cmd) c = tolower(c);

    BotCommandEntry const* entry = FindBotCommand(cmd);
    if (!entry)
    {
        handler->PSendSysMessage("Unknown bot command '%s'. Try .bot help", cmd.c_str());
        return true;
    }
    return entry->handler(handler, subArgs);
}

// pi-lens-ignore: clang:incomplete_member_access
bool TryHandleBotCommand(ChatHandler* handler, char const* text)
{
    if (!handler || !text)
        return false;
    // Check for ".ahbot" alias
    if (strncmp(text, "ahbot", 5) == 0 && (text[5] == '\0' || text[5] == ' ' || text[5] == '\t'))
    {
        const char* args = text + 5;
        while (*args == ' ' || *args == '\t') ++args;
        return HandleAhBot(handler, args);
    }
    // text is the full command after '.' — e.g., "bot add Dudette"
    // We only handle "bot ..." here; return false to let normal handler continue.
    if (strncmp(text, "bot", 3) != 0)
        return false;
    // Ensure "bot" is followed by space or end
    if (text[3] != '\0' && text[3] != ' ' && text[3] != '\t')
        return false;
    const char* args = text + 3;
    while (*args == ' ' || *args == '\t') ++args;
    return HandleChatCommand(handler, args);
}

} // namespace BotCommands
} // namespace TortoiseBots
