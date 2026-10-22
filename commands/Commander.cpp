// pi-lens-ignore-file: all
#include "BotCommandHandlers.h"
#include "BotCommands.h"
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

// ---------------------------------------------------------------------------
// Remote commander (issue #884, phase 1)
//
// `.bot commander bind|unbind|status` and `.bot cmd <order>`, behind
// AiPlayerbot.RemoteCommander (default 0). A commander (a GM or the owner of
// every bot) binds a bot-only party of up to five, led by one of its bots, and
// leads it from anywhere: orders name their target by GUID or a 3D point and
// reach the group through the binding, not through the commander's selection
// or group membership. The decisions are in runtime/RemoteCommanderPolicy.h;
// this is the world glue. World thread only (the command queue drains here),
// like every other `.bot` command.
//
// Nothing here touches a bot's durable master: the followers follow the lead
// bot through their per-bot "manual follow target" value (the AI's own
// FollowTargetValue override, the value behind the whispered `follow target`),
// which the binding sets and puts back on release.
// ---------------------------------------------------------------------------

namespace {

RemoteCommander::BindingTable& CommanderBindings()
{
    static RemoteCommander::BindingTable table;
    return table;
}

// Each follower's own "manual follow target" from before the binding pointed
// it at the lead bot, put back on release.
std::unordered_map<uint64, ai::GuidPosition>& SavedFollowTargets()
{
    static std::unordered_map<uint64, ai::GuidPosition> saved;
    return saved;
}

// The bots a binding gave the "threat" combat strategy (they did not have it
// before): never stored with the bot, and taken off when they leave.
RemoteCommander::CommanderThreatLedger& CommanderThreatBots()
{
    static RemoteCommander::CommanderThreatLedger ledger;
    return ledger;
}

// What each binding took off its bots' non-combat strategies (the open-world
// kit), given back on release.
RemoteCommander::CommanderAutonomyLedger& CommanderAutonomyBots()
{
    static RemoteCommander::CommanderAutonomyLedger ledger;
    return ledger;
}

uint32 s_commanderSweepMs = 0;

// Issue #884, phase 2: one perception feed per binding (`.bot commander view
// on|off`). Made on bind, off until the commander turns it on; dropped on
// release; nothing here runs while none exists. The frame's lines are reused
// frame to frame.
struct CommanderView
{
    bool enabled = false;
    uint32 seq = 0;
    RemoteCommander::View::Pacer pacer;
    RemoteCommander::View::Frame frame;
};

std::unordered_map<uint64, CommanderView>& CommanderViews()
{
    static std::unordered_map<uint64, CommanderView> views;
    return views;
}

// The commanders' last orders with the module's answer, for the observability
// dashboard (runtime/CommanderTelemetry.h): bounded, noted when a reply is
// sent, cleared with the bindings. s_commanderOrderDetail is the arguments of
// the order being handled now (set only while HandleCommanderOrder runs, so a
// later reply of a bot task carries none).
TortoiseBots::CommanderTelemetry::OrderLog& CommanderOrderLog()
{
    static TortoiseBots::CommanderTelemetry::OrderLog log;
    return log;
}

std::string s_commanderOrderDetail;

struct CommanderOrderDetailScope
{
    explicit CommanderOrderDetailScope(std::string const& detail) { s_commanderOrderDetail = detail; }
    ~CommanderOrderDetailScope() { s_commanderOrderDetail.clear(); }
};

// The character with this raw guid, also while it is between maps (a zoning
// commander or lead bot must not lose the binding).
Player* FindCharacter(uint64 raw)
{
    return raw ? ObjectAccessor::FindPlayerNotInWorld(ObjectGuid(raw)) : nullptr;
}

Player* FindOnlineByName(std::string name)
{
    name = Trim(name);
    if (name.empty() || !normalizePlayerName(name))
        return nullptr;
    Player* player = sObjectMgr.GetPlayer(name.c_str());
    if (!player)
        player = sObjectAccessor.FindPlayerByName(name.c_str());
    return player;
}

// Every live bot of the group, the lead bot first, resolved from the group's
// member slots through ObjectAccessor (never the GroupReference list, #225).
// Fills a caller-kept vector (the view feed reuses one).
void FillCommandedBots(Group* group, Player* lead, std::vector<Player*>& bots)
{
    bots.clear();
    if (!group || !lead)
        return;
    if (IsLiveHeadlessBot(lead))
        bots.push_back(lead);
    for (Group::MemberSlot const& slot : group->GetMemberSlots())
    {
        if (slot.guid == lead->GetObjectGuid())
            continue;
        Player* member = ObjectAccessor::FindPlayer(slot.guid);
        if (member && IsLiveHeadlessBot(member))
            bots.push_back(member);
    }
}

std::vector<Player*> CommandedBots(Group* group, Player* lead)
{
    std::vector<Player*> bots;
    FillCommandedBots(group, lead, bots);
    return bots;
}

RemoteCommander::BindingState ReadBindingState(RemoteCommander::Binding const& binding)
{
    RemoteCommander::BindingState state;
    state.enabled = sPlayerbotAIConfig.remoteCommander;
    Player* commander = FindCharacter(binding.commanderGuid);
    state.commanderOnline = commander && commander->GetSession() &&
        commander->GetSession()->HasNetworkTransport();
    Player* lead = FindCharacter(binding.leadGuid);
    state.leadOnline = lead != nullptr;
    if (Group* group = lead ? lead->GetGroup() : nullptr)
    {
        state.leadGroupId = group->GetId();
        state.leadIsLeader = group->IsLeader(lead->GetObjectGuid());
        state.leadGroupIsRaidOrBg = group->isRaidGroup() || group->isBGGroup();
        state.memberCount = group->GetMembersCount();
        for (Group::MemberSlot const& slot : group->GetMemberSlots())
            if (!BotManager::Instance().IsBot(slot.guid))
                state.realPlayerInGroup = true;
    }
    return state;
}

ai::Value<ai::GuidPosition>* ManualFollowTarget(PlayerbotAI* ai)
{
    if (!ai || !ai->GetAiObjectContext())
        return nullptr;
    return ai->GetAiObjectContext()->GetValue<ai::GuidPosition>("manual follow target");
}

void ResetFollowTargetCache(PlayerbotAI* ai)
{
    if (ai::Value<Unit*>* follow = ai->GetAiObjectContext()->GetValue<Unit*>("follow target"))
        follow->Reset();
}

// Point one follower's follow target at the lead bot, keeping its own value
// for the release.
void PointFollowerAtLead(uint64 commanderRaw, Player* bot, Player* lead)
{
    if (!bot || !lead || bot == lead)
        return;
    PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
    ai::Value<ai::GuidPosition>* value = ManualFollowTarget(ai);
    if (!value)
        return;
    uint64 const botRaw = bot->GetObjectGuid().GetRawValue();
    auto& saved = SavedFollowTargets();
    if (saved.find(botRaw) == saved.end())
    {
        ai::GuidPosition previous = value->Get();
        saved[botRaw] = previous.GetRawValue() == lead->GetObjectGuid().GetRawValue()
            ? ai::GuidPosition() : previous;
    }
    value->Set(ai::GuidPosition(lead));
    ResetFollowTargetCache(ai);
    CommanderBindings().AddFollower(commanderRaw, botRaw);
}

// Put a follower's own follow target back, unless something else changed it
// while it was bound.
void RestoreFollower(uint64 botRaw, uint64 leadRaw)
{
    auto& saved = SavedFollowTargets();
    auto it = saved.find(botRaw);
    ai::GuidPosition previous = it != saved.end() ? it->second : ai::GuidPosition();
    if (it != saved.end())
        saved.erase(it);
    Player* bot = FindCharacter(botRaw);
    PlayerbotAI* ai = bot ? PlayerbotAIStorage::Instance().GetAI(bot) : nullptr;
    ai::Value<ai::GuidPosition>* value = ManualFollowTarget(ai);
    if (!value || value->Get().GetRawValue() != leadRaw)
        return;
    value->Set(previous);
    ResetFollowTargetCache(ai);
}

// A commanded bot backs off near the tank's threat like a bot in a player's
// party (RemoteCommander::KeepsCommandedThreatBackoff). AiFactory gives the
// strategy on every strategy reset while the bot is bound; this gives it at
// bind time, and remembers the bots that did not have it before.
void GiveCommanderThreatBackoff(Player* bot)
{
    PlayerbotAI* ai = bot ? PlayerbotAIStorage::Instance().GetAI(bot) : nullptr;
    if (!ai)
        return;
    bool const eligible = !bot->InBattleGround() &&
        RemoteCommander::KeepsCommandedThreatBackoff(true, PlayerbotAI::IsTank(bot));
    if (CommanderThreatBots().Give(bot->GetObjectGuid().GetRawValue(), eligible,
            ai->HasStrategy("threat", BotState::BOT_STATE_COMBAT)))
        ai->ChangeStrategy("+threat", BotState::BOT_STATE_COMBAT);
}

// Takes the binding's "threat" strategy off a bot that left the binding,
// unless the general rule still gives it (a real-player master).
void RestoreCommanderThreatBackoff(uint64 botRaw)
{
    if (!CommanderThreatBots().Holds(botRaw))
        return;
    Player* bot = FindCharacter(botRaw);
    PlayerbotAI* ai = bot ? PlayerbotAIStorage::Instance().GetAI(bot) : nullptr;
    bool const stillDue = ai && AiFactory::KeepsPartyThreatBackoff(bot, ai);
    if (CommanderThreatBots().Release(botRaw, stillDue) && ai)
        ai->ChangeStrategy("-threat", BotState::BOT_STATE_COMBAT);
}

bool OnInstanceMap(Player* bot)
{
    return bot && bot->GetMap() && (bot->GetMap()->IsDungeon() || bot->GetMap()->IsRaid());
}

// Gives a bot that left the binding the open-world kit the binding took off
// it, as it had it before the bind. Inside a dungeon or raid the kit waits
// until the bot leaves the instance (CommanderAutonomyMapChange). A bot that
// now follows a real player gets nothing back (#741: it keeps following its
// player). A bot still in a bound group keeps holding.
void RestoreCommanderAutonomy(uint64 botRaw)
{
    Player* bot = FindCharacter(botRaw);
    if (bot && IsInCommandedGroup(bot))
        return;
    uint32 const mask = CommanderAutonomyBots().Release(botRaw);
    PlayerbotAI* ai = bot ? PlayerbotAIStorage::Instance().GetAI(bot) : nullptr;
    if (!mask || !ai || ai->HasRealPlayerMaster())
        return;
    if (!RemoteCommander::RestoresAutonomyNow(OnInstanceMap(bot)))
    {
        CommanderAutonomyBots().Defer(botRaw, mask);
        return;
    }
    ai->ChangeStrategy(RemoteCommander::CommanderAutonomyChange(mask, '+'), BotState::BOT_STATE_NON_COMBAT);
}

// A bot the binding held the kit off that is no longer in a bound group (one
// that joined after the bind and left again; followers and the release are
// handled where they leave) gets it back. Once a second, from the sweep.
void SweepCommanderAutonomy()
{
    if (CommanderAutonomyBots().Empty())
        return;
    static std::vector<uint64> held;
    CommanderAutonomyBots().HeldBots(held);
    for (uint64 botRaw : held)
    {
        Player* bot = FindCharacter(botRaw);
        // Offline: its snapshot kept the kit (PlayerbotDbStore::Save).
        if (!bot)
        {
            CommanderAutonomyBots().Release(botRaw);
            continue;
        }
        if (!IsInCommandedGroup(bot))
            RestoreCommanderAutonomy(botRaw);
    }
}

// The commander's running bot tasks (loot, open, use) end with the binding;
// defined with the task code below.
void CancelCommanderTasks(uint64 commanderRaw);
// A later order takes the bot(s) over (onlyBotRaw 0 = every bot of the commander): the running
// tasks end with ERR cancelled. An order that sets movement itself (move, stay, follow, summon)
// keeps the walk's stance; after attack, focus or pull the bot re-forms as after a finished walk.
void SupersedeCommanderTasks(uint64 commanderRaw, uint64 onlyBotRaw, RemoteCommander::LaterOrder later);

// Ends a binding: followers get their own follow target back. The lead bot
// keeps holding where it stands. notify: tell the commander (automatic
// release); an explicit unbind answers through its own ACK.
uint32 ReleaseBinding(uint64 commanderRaw, RemoteCommander::ReleaseReason reason, bool notify)
{
    if (CommanderBindings().FindByCommander(commanderRaw))
        CancelCommanderTasks(commanderRaw);
    RemoteCommander::Binding removed = CommanderBindings().Unbind(commanderRaw);
    CommanderViews().erase(commanderRaw);
    if (!removed.commanderGuid)
        return 0;
    for (uint64 follower : removed.followers)
    {
        RestoreFollower(follower, removed.leadGuid);
        RestoreCommanderThreatBackoff(follower);
        RestoreCommanderAutonomy(follower);
    }
    RestoreCommanderThreatBackoff(removed.leadGuid);
    RestoreCommanderAutonomy(removed.leadGuid);
    Player* commander = FindCharacter(commanderRaw);
    Player* lead = FindCharacter(removed.leadGuid);
    // A bot that joined after the bind is no follower but may hold the
    // binding's "threat" (noted by AiFactory): release it too.
    if (Group* group = lead ? lead->GetGroup() : nullptr)
        if (group->GetId() == removed.groupId)
            for (Group::MemberSlot const& slot : group->GetMemberSlots())
            {
                RestoreCommanderThreatBackoff(slot.guid.GetRawValue());
                RestoreCommanderAutonomy(slot.guid.GetRawValue());
            }
    // The commander's own hires go back to its party (or wait for it to come
    // back online inside the logout grace): a hire lives in its hirer's party.
    // After the follow targets and the threat back-off above were restored,
    // while the hires are still in the bound party.
    HireLifecycle::Instance().EndCommand(::ObjectGuid(commanderRaw));
    TB_LOG_DETAIL("TortoiseBots: remote commander %s released group of %s: %s",
        commander ? commander->GetName() : "(offline)", lead ? lead->GetName() : "(offline)",
        RemoteCommander::ReleaseReasonText(reason));
    if (notify && commander && commander->IsInWorld() && commander->GetSession())
    {
        ChatHandler(commander).PSendSysMessage("TBM:ACTION_ACK|commander unbind|party|%u|%s",
            static_cast<uint32>(removed.followers.size()),
            ProtocolSafe(RemoteCommander::ReleaseReasonText(reason)).c_str());
    }
    return static_cast<uint32>(removed.followers.size());
}

// Followers that left the bound group (kicked, invited by their owner into
// another party, left on their own) get their own follow target back at once;
// otherwise they would keep walking after the lead bot and ignore their
// owner's orders. The rest of the binding stands.
void ReleaseDepartedFollowers(uint64 commanderRaw, RemoteCommander::Binding const& binding)
{
    if (binding.followers.empty())
        return;
    std::vector<uint64> members;
    Player* lead = FindCharacter(binding.leadGuid);
    if (Group* group = lead ? lead->GetGroup() : nullptr)
        for (Group::MemberSlot const& slot : group->GetMemberSlots())
            members.push_back(slot.guid.GetRawValue());
    std::vector<uint64> const departed = RemoteCommander::DepartedFollowers(binding, members);
    uint64 const leadRaw = binding.leadGuid;
    for (uint64 follower : departed)
    {
        RestoreFollower(follower, leadRaw);
        RestoreCommanderThreatBackoff(follower);
        RestoreCommanderAutonomy(follower);
        CommanderBindings().RemoveFollower(commanderRaw, follower);
    }
}

// The requester's binding if it still stands (a stale one is released here,
// a follower that left the group is let go).
RemoteCommander::Binding const* StandingBinding(uint64 commanderRaw)
{
    RemoteCommander::Binding const* binding = CommanderBindings().FindByCommander(commanderRaw);
    if (!binding)
        return nullptr;
    RemoteCommander::ReleaseReason reason = RemoteCommander::CheckBinding(*binding, ReadBindingState(*binding));
    if (reason == RemoteCommander::ReleaseReason::None)
    {
        ReleaseDepartedFollowers(commanderRaw, *binding);
        return CommanderBindings().FindByCommander(commanderRaw);
    }
    ReleaseBinding(commanderRaw, reason, true);
    return nullptr;
}

RemoteCommander::BindMember MemberFacts(Player* commander, Player* bot)
{
    RemoteCommander::BindMember facts;
    if (!bot)
        return facts;
    BotRecord* record = BotManager::Instance().FindBot(bot->GetObjectGuid());
    facts.controllable = record && CanControlBot(commander, record);
    facts.liveBot = record && IsLiveHeadlessBot(bot, record);
    // Hired by somebody else: refused. The commander's own hires may be bound
    // (the hire lifecycle suspends their "left the hirer's party" end).
    HireLifecycle& hires = HireLifecycle::Instance();
    facts.hired = RemoteCommander::HireBlocksBind(hires.IsHired(bot->GetObjectGuid()),
        hires.GetMaster(bot->GetObjectGuid()).GetRawValue(), commander ? commander->GetObjectGuid().GetRawValue() : 0);
    // One binding per bot: its saved follow target belongs to that binding.
    facts.inOtherBinding = CommanderBindings().FindByBot(bot->GetObjectGuid().GetRawValue()) != nullptr;
    return facts;
}

// The commander leaves the party the bind keeps (the party and its instance
// bind outlive a member who leaves; the core disbands one left with fewer than
// two members, so this refuses unless two bots stay). Server side, so no
// leave packet reaches the bots' "master left" handler. True when the
// commander is out of the group.
bool DropCommanderFromParty(Group* group, Player* commander)
{
    if (!group || !commander || !group->IsMember(commander->GetObjectGuid()))
        return group != nullptr;
    if (!RemoteCommander::CanDropCommanderFromParty(group->GetMembersCount()))
        return false;
    group->RemoveMember(commander->GetObjectGuid(), 0);
    return !group->IsMember(commander->GetObjectGuid());
}

// The lead bot invites, the member accepts through the native handlers - the
// same opcodes the AI's own invite/accept actions use, without the accept
// action's master rebind: the member keeps its durable master.
bool JoinLeadGroup(Player* lead, Player* member)
{
    if (lead->GetGroup() && member->GetGroup() == lead->GetGroup())
        return true;
    if (Group* group = lead->GetGroup())
        if (group->IsFull())
            return false;
    if (member->GetGroupInvite())
        member->UninviteFromGroup();
    WorldPacket invite;
    invite << member->GetName() << uint32(0);
    lead->GetSession()->HandleGroupInviteOpcode(invite);
    if (!member->GetGroupInvite())
        return false;
    WorldPacket accept;
    accept << uint32(0);
    member->GetSession()->HandleGroupAcceptOpcode(accept);
    return lead->GetGroup() && member->GetGroup() == lead->GetGroup();
}

// Followers follow the lead bot; the lead bot holds until a move order.
// reanchorLead: the lead bot holds where it stands now (bind); otherwise a
// lead bot already holding keeps its spot, so a `follow` during a move does
// not stop the lead bot short. Returns the bots that took the order.
uint32 FormUpOnLead(Player* commander, Player* lead, std::vector<Player*> const& bots,
    bool reanchorLead)
{
    uint32 accepted = 0;
    uint64 const commanderRaw = commander->GetObjectGuid().GetRawValue();
    for (Player* bot : bots)
    {
        if (!bot || !bot->IsInWorld())
            continue;
        PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
        if (!ai)
            continue;
        ai->DropQueuedMovementOrders();
        if (bot == lead)
        {
            if (!reanchorLead && ai->HasStrategy("stay", BotState::BOT_STATE_NON_COMBAT))
                ++accepted;
            else if (ExecuteQuietAction(ai, "stay chat shortcut", ai::Event("stay", "", commander)))
                ++accepted;
            continue;
        }
        PointFollowerAtLead(commanderRaw, bot, lead);
        bot->GetMotionMaster()->Clear();
        if (ExecuteQuietAction(ai, "follow chat shortcut", ai::Event("follow", "", commander)))
            ++accepted;
    }
    return accepted;
}

// ---------------------------------------------------------------------------
// Perception feed (issue #884, phase 2)
//
// Every AiPlayerbot.RemoteCommanderViewMs a bound commander whose feed is on
// gets one TBM:VIEW frame (grammar: runtime/RemoteCommanderView.h). The
// snapshot is read from what the bound bots already hold - their unit fields,
// their own cached "attackers" and "possible targets" lists (read only when
// already calculated: HasCachedValue, never a fresh calculation), the units
// attacking them, their victim and selection - and, for each hostile found
// that way, its own threat list. Hostiles are resolved by guid on the seeing
// bot's map (a hash lookup): no grid or world scan happens here.
// ---------------------------------------------------------------------------

RemoteCommander::View::Snapshot& ViewSnapshot()
{
    static RemoteCommander::View::Snapshot snapshot;
    return snapshot;
}

uint32 CurrentSpellId(Unit* unit)
{
    if (Spell* spell = unit->GetCurrentSpell(CURRENT_GENERIC_SPELL))
        if (spell->m_spellInfo)
            return spell->m_spellInfo->Id;
    if (Spell* spell = unit->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
        if (spell->m_spellInfo)
            return spell->m_spellInfo->Id;
    return 0;
}

char ViewStateLetter(PlayerbotAI* ai, Player* bot)
{
    if (!bot->IsAlive())
        return 'd';
    if (!ai)
        return 'n';
    switch (ai->GetState())
    {
        case BotState::BOT_STATE_COMBAT: return 'c';
        case BotState::BOT_STATE_DEAD: return 'd';
        case BotState::BOT_STATE_REACTION: return 'r';
        default: return 'n';
    }
}

// The short failure code of the view feed's party record (grammar:
// runtime/RemoteCommanderView.h) for the SpellCastResult the engine kept from
// the failed action's last cast check; nullptr when it has no code.
char const* ViewFailReason(uint32 castFail)
{
    switch (static_cast<SpellCastResult>(castFail))
    {
        case SPELL_FAILED_NO_POWER: return "oom";
        case SPELL_FAILED_OUT_OF_RANGE:
        case SPELL_FAILED_TOO_CLOSE: return "range";
        case SPELL_FAILED_LINE_OF_SIGHT: return "los";
        case SPELL_FAILED_NOT_READY:
        case SPELL_FAILED_ITEM_NOT_READY: return "cd";
        case SPELL_FAILED_MOVING: return "moving";
        case SPELL_FAILED_SPELL_IN_PROGRESS: return "casting";
        case SPELL_FAILED_ITEM_GONE:
        case SPELL_FAILED_REAGENTS:
        case SPELL_FAILED_EQUIPPED_ITEM:
        case SPELL_FAILED_NEED_AMMO: return "no-item";
        case SPELL_FAILED_BAD_TARGETS:
        case SPELL_FAILED_BAD_IMPLICIT_TARGETS:
        case SPELL_FAILED_TARGETS_DEAD:
        case SPELL_FAILED_TARGET_ENEMY:
        case SPELL_FAILED_TARGET_FRIENDLY: return "no-target";
        default: return nullptr;
    }
}

// One of the bot's guid-list values, only when it already holds a calculated
// list; otherwise nothing (the bot calculates it on its own schedule).
void ReadCachedGuidList(PlayerbotAI* ai, char const* name, std::list<ObjectGuid>& out)
{
    out.clear();
    ai::AiObjectContext* context = ai ? ai->GetAiObjectContext() : nullptr;
    if (!context || !context->HasValue(name))
        return;
    auto* value = dynamic_cast<ai::CalculatedValue<std::list<ObjectGuid>>*>(
        context->GetValue<std::list<ObjectGuid>>(name));
    if (value && value->HasCachedValue())
        out = value->LazyGet();
}

// A hostile the bot sees: in world on its map, alive, not friendly, and seen
// (stealth and invisibility detection, as the order targets check it).
Unit* SeenHostile(Player* bot, ObjectGuid guid)
{
    if (guid.IsEmpty() || guid == bot->GetObjectGuid())
        return nullptr;
    Map* map = bot->GetMap();
    Unit* unit = map ? map->GetUnit(guid) : nullptr;
    if (!unit || !unit->IsInWorld() || !unit->IsAlive())
        return nullptr;
    if (BotManager::Instance().IsBot(unit->GetObjectGuid()) || sServerFacade.IsFriendlyTo(unit, bot))
        return nullptr;
    if (!unit->IsVisibleForOrDetect(bot, bot->GetCamera().GetBody(), true))
        return nullptr;
    return unit;
}

// hostileOnly: leave out a unit the bot does not react to as hostile (the
// first pass over the sight lists, so neutrals never take a slot from an enemy).
void AddViewHostile(RemoteCommander::View::Snapshot& snapshot, RemoteCommander::View::HostileSet& taken,
    Player* bot, ObjectGuid guid, bool hostileOnly = false)
{
    if (taken.Full() || guid.IsEmpty() || taken.Contains(guid.GetRawValue()))
        return;
    Unit* unit = SeenHostile(bot, guid);
    if (!unit)
        return;
    // How the seeing bot reacts to it: a field read on the unit's faction.
    ReputationRank const reaction = bot->GetReactionTo(unit);
    if (hostileOnly && reaction > REP_HOSTILE)
        return;
    if (!taken.Add(guid.GetRawValue()))
        return;
    RemoteCommander::View::Hostile* hostile = snapshot.AddHostile();
    if (!hostile)
        return;
    hostile->guid = guid.GetRawValue();
    hostile->entry = unit->GetEntry();
    hostile->x = unit->GetPositionX();
    hostile->y = unit->GetPositionY();
    hostile->z = unit->GetPositionZ();
    hostile->hpPct = unit->GetMaxHealth()
        ? static_cast<uint32>(unit->GetHealth() * 100ull / unit->GetMaxHealth()) : 0;
    hostile->target = unit->GetTargetGuid().GetRawValue();
    hostile->casting = CurrentSpellId(unit);
    hostile->stealthed = unit->HasStealthAura();
    Creature* creature = unit->ToCreature();
    hostile->evading = creature && creature->IsInEvadeMode();
    hostile->crowdControlled = unit->HasUnitState(UNIT_STAT_STUNNED | UNIT_STAT_CONFUSED |
        UNIT_STAT_FLEEING | UNIT_STAT_ROOT);
    hostile->ccKind = CommanderTelemetry::CcKind(unit->HasUnitState(UNIT_STAT_STUNNED),
        unit->HasUnitState(UNIT_STAT_CONFUSED), unit->HasUnitState(UNIT_STAT_FLEEING), unit->HasUnitState(UNIT_STAT_ROOT));
    hostile->level = unit->GetLevel();
    if (creature)
        hostile->rank = CommanderTelemetry::RankKind(creature->GetCreatureInfo()->rank,
            creature->HasExtraFlag(CREATURE_FLAG_EXTRA_INSTANCE_BIND));
    // What kind of unit it is: fields the unit and its template already hold
    // (no lookup, no scan).
    hostile->reaction = reaction <= REP_HOSTILE ? 'h' : (reaction >= REP_FRIENDLY ? 'f' : 'n');
    hostile->critter = unit->GetCreatureType() == CREATURE_TYPE_CRITTER;
    hostile->civilian = creature && creature->IsCivilian();
    hostile->guard = creature && creature->IsGuard();
    hostile->unattackable = unit->HasFlag(UNIT_FIELD_FLAGS,
        UNIT_FLAG_SPAWNING | UNIT_FLAG_IMMUNE_TO_PLAYER | UNIT_FLAG_NON_ATTACKABLE_2);

    // Engaged: the top of its own threat list, in % of the top entry.
    if (!unit->IsInCombat() || !unit->CanHaveThreatList())
        return;
    ThreatList const& list = unit->GetThreatManager().getThreatList();
    if (list.empty())
        return;
    RemoteCommander::View::ThreatTop* top = snapshot.AddThreat(hostile->guid);
    if (!top)
        return;
    for (HostileReference const* ref : list)
        if (ref)
            top->Push(ref->getUnitGuid().GetRawValue(), ref->getThreat());
    if (!top->count)
        snapshot.DropLastThreat();
}

void CollectViewSnapshot(RemoteCommander::Binding const& binding, RemoteCommander::View::Snapshot& snapshot)
{
    static std::vector<Player*> bots;
    static RemoteCommander::View::HostileSet taken;
    static std::list<ObjectGuid> cached;

    snapshot.Clear();
    snapshot.lead = binding.leadGuid;
    Player* lead = FindCharacter(binding.leadGuid);
    Group* group = lead ? lead->GetGroup() : nullptr;
    if (!group || group->GetId() != binding.groupId)
        return;
    FillCommandedBots(group, lead, bots);

    for (Player* bot : bots)
    {
        RemoteCommander::View::PartyMember& member = snapshot.AddPartyMember();
        member.guid = bot->GetObjectGuid().GetRawValue();
        member.name = bot->GetName();
        member.hp = bot->GetHealth();
        member.maxHp = bot->GetMaxHealth();
        Powers const powerType = bot->GetPowerType();
        member.powerType = static_cast<uint8>(powerType);
        member.power = bot->GetPower(powerType);
        member.maxPower = bot->GetMaxPower(powerType);
        member.x = bot->GetPositionX();
        member.y = bot->GetPositionY();
        member.z = bot->GetPositionZ();
        member.o = bot->GetOrientation();
        member.map = bot->GetMapId();
        member.instance = bot->GetInstanceId();
        ObjectGuid target = bot->GetTargetGuid();
        if (target.IsEmpty())
            if (Unit* victim = bot->GetVictim())
                target = victim->GetObjectGuid();
        member.target = target.GetRawValue();
        member.casting = CurrentSpellId(bot);
        member.dead = !bot->IsAlive();
        PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
        member.state = ViewStateLetter(ai, bot);
        if (ai)
            if (ai::Engine* engine = ai->GetCurrentEngine())
            {
                // The newest verdict as parts (name, result letter, reason code).
                member.action = engine->GetLastOutcomeName();
                member.result = engine->GetLastOutcomeResult();
                if (char const* reason = ViewFailReason(engine->GetLastOutcomeCastFail()))
                    member.reason = reason;
            }
    }

    taken.Clear();
    for (Player* bot : bots)
    {
        if (taken.Full())
            break;
        if (!bot->IsInWorld() || !bot->IsAlive())
            continue;
        // What it fights first, then what it merely sees.
        if (Unit* victim = bot->GetVictim())
            AddViewHostile(snapshot, taken, bot, victim->GetObjectGuid());
        AddViewHostile(snapshot, taken, bot, bot->GetTargetGuid());
        for (Unit* attacker : bot->GetAttackers())
            if (attacker)
                AddViewHostile(snapshot, taken, bot, attacker->GetObjectGuid());
        PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
        ReadCachedGuidList(ai, "attackers", cached);
        for (ObjectGuid const& guid : cached)
            AddViewHostile(snapshot, taken, bot, guid);
    }
    // Then everything the bots' own sight lists hold, up to the cap: the
    // hostile ones first, the neutral ones (marked n) with the room left.
    for (bool hostileOnly : { true, false })
        for (Player* bot : bots)
        {
            if (taken.Full())
                break;
            if (!bot->IsInWorld() || !bot->IsAlive())
                continue;
            ReadCachedGuidList(PlayerbotAIStorage::Instance().GetAI(bot), "possible targets", cached);
            for (ObjectGuid const& guid : cached)
                AddViewHostile(snapshot, taken, bot, guid, hostileOnly);
        }
}

// Each tick while a binding exists: build the frames that fall due, send at
// most kLinesPerTick lines per commander.
void UpdateCommanderViews(uint32 diff)
{
    auto& views = CommanderViews();
    if (views.empty())
        return;
    static std::string scratch;
    uint32 const interval = sPlayerbotAIConfig.remoteCommanderViewMs;
    for (auto& entry : views)
    {
        CommanderView& view = entry.second;
        if (!view.enabled)
            continue;
        RemoteCommander::Binding const* binding = CommanderBindings().FindByCommander(entry.first);
        if (!binding)
            continue;
        Player* commander = FindCharacter(entry.first);
        bool const reachable = commander && commander->IsInWorld() && commander->GetSession();
        if (view.pacer.Due(diff, interval) && reachable)
        {
            CollectViewSnapshot(*binding, ViewSnapshot());
            view.pacer.Start(RemoteCommander::View::BuildFrame(ViewSnapshot(), ++view.seq, view.frame, scratch));
        }
        if (!view.pacer.Pending())
            continue;
        if (!reachable)
        {
            view.pacer.Abandon();
            continue;
        }
        size_t first = 0;
        size_t const count = view.pacer.NextBatch(first);
        ChatHandler handler(commander);
        for (size_t i = 0; i < count; ++i)
            handler.SendSysMessage(view.frame.lines[first + i].c_str());
    }
}

} // namespace

static bool HandleCommanderBind(ChatHandler* handler, Player* commander, std::string const& args)
{
    std::string const intent = "commander bind";
    std::vector<std::string> names = RemoteCommander::SplitOrderArgs(args);
    if (names.empty() || names.size() > RemoteCommander::kMaxGroupSize)
    {
        SendActionError(handler, intent, "invalid", "Usage: .bot commander bind <LeadBot> [<Bot> ...] (a party of up to five bots)");
        return true;
    }

    uint64 const commanderRaw = commander->GetObjectGuid().GetRawValue();
    RemoteCommander::BindFacts facts;
    facts.enabled = sPlayerbotAIConfig.remoteCommander;
    facts.commanderIsBot = BotManager::Instance().IsBot(commander->GetObjectGuid());
    // Settle every binding first (a stale one, or a follower that already
    // left its group, must not block this bind).
    for (uint64 other : CommanderBindings().Commanders())
        StandingBinding(other);
    facts.commanderAlreadyBound = CommanderBindings().FindByCommander(commanderRaw) != nullptr;

    Player* lead = FindOnlineByName(names[0]);
    facts.lead = MemberFacts(commander, lead);
    Group* commanderParty = commander->GetGroup();
    Group* leadGroup = lead ? lead->GetGroup() : nullptr;
    // A lead bot in the commander's own party keeps that party (the instance
    // bind lives on the group): the lead bot leads it and the commander leaves.
    // Only a raid or a party with another player is rebuilt, as before.
    RemoteCommander::BindPlanFacts planFacts;
    planFacts.leadInCommanderParty = leadGroup && leadGroup == commanderParty;
    planFacts.leadInOwnGroup = leadGroup && !planFacts.leadInCommanderParty;
    if (planFacts.leadInCommanderParty)
    {
        planFacts.partyIsRaidOrBg = leadGroup->isRaidGroup() || leadGroup->isBGGroup();
        for (Group::MemberSlot const& slot : leadGroup->GetMemberSlots())
            if (slot.guid != commander->GetObjectGuid() && !BotManager::Instance().IsBot(slot.guid))
                planFacts.partyHasOtherPlayer = true;
    }
    RemoteCommander::BindGroupPlan const plan = RemoteCommander::DecideBindGroup(planFacts);
    bool const keepParty = plan == RemoteCommander::BindGroupPlan::KeepCommanderParty;
    bool const leadLeavesParty = plan == RemoteCommander::BindGroupPlan::ReformFromCommanderParty;
    if (leadGroup && (keepParty || plan == RemoteCommander::BindGroupPlan::AdoptLeadGroup))
    {
        facts.leadGrouped = true;
        facts.leadGroupIsRaidOrBg = leadGroup->isRaidGroup() || leadGroup->isBGGroup();
        for (Group::MemberSlot const& slot : leadGroup->GetMemberSlots())
            if (slot.guid != commander->GetObjectGuid() && !BotManager::Instance().IsBot(slot.guid))
                facts.leadGroupHasRealPlayer = true;
        facts.leadGroupBound = CommanderBindings().FindByGroup(leadGroup->GetId()) != nullptr;

        // A kept party is commanded as a whole: every bot in it must have been
        // named (a bot is never taken under command without being asked).
        if (keepParty)
        {
            std::vector<ObjectGuid> named;
            for (size_t i = 1; i < names.size(); ++i)
                if (Player* bot = FindOnlineByName(names[i]))
                    named.push_back(bot->GetObjectGuid());
            for (Group::MemberSlot const& slot : leadGroup->GetMemberSlots())
                if (slot.guid != lead->GetObjectGuid() && slot.guid != commander->GetObjectGuid() &&
                    std::find(named.begin(), named.end(), slot.guid) == named.end())
                    facts.keptPartyHasUnnamedBot = true;
        }

        // Making the lead bot leader copies the lead's own instance binds onto
        // the group (Player::ConvertInstancesToGroup), replacing the group's
        // bind for the same map: the bots would be ejected all the same.
        if (!leadGroup->IsLeader(lead->GetObjectGuid()))
            for (auto const& entry : lead->GetBoundInstances())
            {
                InstanceGroupBind* groupBind = leadGroup->GetBoundInstance(entry.first);
                if (groupBind && groupBind->state != entry.second.state)
                    facts.leadBindWouldReplaceGroupBind = true;
            }
    }

    // The group to be: the lead's current bot-only group (adopted as it is)
    // plus the named bots.
    std::vector<Player*> joiners;
    std::vector<ObjectGuid> counted;
    if (lead)
        counted.push_back(lead->GetObjectGuid());
    if (facts.leadGrouped)
    {
        for (Group::MemberSlot const& slot : leadGroup->GetMemberSlots())
        {
            if (slot.guid == lead->GetObjectGuid() || slot.guid == commander->GetObjectGuid())
                continue;
            counted.push_back(slot.guid);
            facts.members.push_back(MemberFacts(commander, ObjectAccessor::FindPlayer(slot.guid)));
        }
    }
    for (size_t i = 1; i < names.size(); ++i)
    {
        Player* bot = FindOnlineByName(names[i]);
        if (bot && std::find(counted.begin(), counted.end(), bot->GetObjectGuid()) != counted.end())
            continue;
        RemoteCommander::BindMember member = MemberFacts(commander, bot);
        if (bot)
        {
            Group* botGroup = bot->GetGroup();
            member.inOtherGroup = botGroup && botGroup != commanderParty;
            counted.push_back(bot->GetObjectGuid());
            joiners.push_back(bot);
        }
        facts.members.push_back(member);
    }

    RemoteCommander::BindRefusal refusal = RemoteCommander::CheckBind(facts);
    if (refusal != RemoteCommander::BindRefusal::None)
    {
        SendActionError(handler, intent, RemoteCommander::BindRefusalCode(refusal),
            RemoteCommander::BindRefusalText(refusal));
        return true;
    }

    // The commander's own hires may be bound: from here until the binding
    // stands they are in transition, so moving them between parties does not
    // end their hire (the commander leaves their party, or they leave it to
    // join the lead bot's). A failed bind returns them to the commander.
    bool hiresInTransition = false;
    {
        ::ObjectGuid const commanderGuid = commander->GetObjectGuid();
        HireLifecycle& hires = HireLifecycle::Instance();
        auto beginCommand = [&](Player* bot)
        {
            if (bot && hires.BeginCommand(bot->GetObjectGuid(), commanderGuid))
                hiresInTransition = true;
        };
        beginCommand(lead);
        for (Player* bot : joiners)
            beginCommand(bot);
        if (facts.leadGrouped)
            for (Group::MemberSlot const& slot : leadGroup->GetMemberSlots())
                beginCommand(ObjectAccessor::FindPlayer(slot.guid));
    }
    auto failBind = [&](char const* code, char const* message)
    {
        if (hiresInTransition)
            HireLifecycle::Instance().EndCommand(commander->GetObjectGuid());
        SendActionError(handler, intent, code, message);
        return true;
    };

    // Form or adopt the party. A kept commander party stays the same group
    // (and so keeps its instance bind): the lead bot takes the lead, the named
    // bots outside it join, and the commander leaves it last (earlier only
    // when the party is full and a bot is still to join). Otherwise: out of the
    // commander's party, the lead bot leads, every named bot joins it.
    // Re-forming a party of only the commander and named bots (a raid: the
    // commander and five hires), the commander leaves it first: the party the
    // dungeon is bound to then never disbands with the commander as its last
    // member outside, which resets that dungeon under the bots
    // (Group::Disband keeps the bind only for a last member standing in it).
    Group* const formerParty = commanderParty;
    bool everyBotNamed = leadLeavesParty && formerParty && !planFacts.partyHasOtherPlayer;
    if (everyBotNamed)
        for (Group::MemberSlot const& slot : formerParty->GetMemberSlots())
            if (slot.guid != commander->GetObjectGuid() && slot.guid != lead->GetObjectGuid() &&
                std::none_of(joiners.begin(), joiners.end(),
                    [&](Player* bot) { return bot->GetObjectGuid() == slot.guid; }))
                everyBotNamed = false;
    if (everyBotNamed)
        DropCommanderFromParty(formerParty, commander);
    // The group pointer stays valid after a disband here (the module's
    // RemoveMember never deletes it; a disbanded group has no members).
    auto leaveFormerParty = [&](Player* bot)
    {
        if (formerParty && formerParty->IsMember(bot->GetObjectGuid()))
            formerParty->RemoveMember(bot->GetObjectGuid(), 0);
    };
    if (leadLeavesParty)
        leaveFormerParty(lead);
    if (!keepParty)
        for (Player* bot : joiners)
            leaveFormerParty(bot);
    if (Group* group = lead->GetGroup())
        if (!group->IsLeader(lead->GetObjectGuid()))
            group->ChangeLeader(lead->GetObjectGuid());
    for (Player* bot : joiners)
    {
        if (keepParty && lead->GetGroup() && lead->GetGroup()->IsFull() &&
            !DropCommanderFromParty(lead->GetGroup(), commander))
        {
            return failBind("invite-failed", "The party is full and the commander cannot leave it.");
        }
        if (!JoinLeadGroup(lead, bot))
        {
            std::string message = std::string("The group invitation for ") + bot->GetName() +
                " was refused (another pending invite, a faction rule, or a full group). Bind again to adopt the group as it is now.";
            return failBind("invite-failed", message.c_str());
        }
    }
    if (keepParty && lead->GetGroup() && !DropCommanderFromParty(lead->GetGroup(), commander))
    {
        return failBind("invite-failed", "The commander cannot leave the party (it would be left with fewer than two bots).");
    }

    Group* group = lead->GetGroup();
    if (!group || !group->IsLeader(lead->GetObjectGuid()) || group->GetMembersCount() < 2)
    {
        return failBind("invite-failed", "The lead bot could not form a group.");
    }

    RemoteCommander::Binding binding;
    binding.commanderGuid = commanderRaw;
    binding.leadGuid = lead->GetObjectGuid().GetRawValue();
    binding.groupId = group->GetId();
    RemoteCommander::BindResult result = CommanderBindings().Bind(binding);
    if (result != RemoteCommander::BindResult::Bound)
    {
        return failBind(result == RemoteCommander::BindResult::GroupAlreadyBound
            ? "group-bound" : "already-bound", "The group or you are already bound.");
    }

    // The commander's hires stay hires while bound (the "left the hirer's
    // party" end is suspended for them until the binding ends).
    HireLifecycle::Instance().ConfirmCommand(commander->GetObjectGuid());

    // The perception feed starts off with every new binding; `.bot commander
    // view on` turns it on.
    CommanderViews()[commanderRaw] = CommanderView();

    std::vector<Player*> bots = CommandedBots(group, lead);
    // Bots moved between parties inside a dungeon stay in it (no hearth 60 s later).
    for (Player* bot : bots)
        HireLifecycle::KeepInstanceAfterPartyMove(bot);
    FormUpOnLead(commander, lead, bots, true);
    for (Player* bot : bots)
    {
        GiveCommanderThreatBackoff(bot);
        // No own walks while bound: the lead bot (a group leader's kit) and any
        // follower lose grind/travel/rpg until the release.
        HoldCommanderAutonomy(bot, PlayerbotAIStorage::Instance().GetAI(bot), false);
    }
    std::string roster;
    for (Player* bot : bots)
        roster += (roster.empty() ? "" : ", ") + std::string(bot->GetName());
    handler->PSendSysMessage("You command %s's group: %s. Orders: .bot cmd <order>.", lead->GetName(), roster.c_str());
    SendActionAck(handler, intent, "party", static_cast<uint32>(bots.size()), lead->GetName());
    return true;
}

bool HandleCommander(ChatHandler* handler, char const* args)
{
    Player* commander = Requester(handler);
    std::string text = Trim(args ? args : "");
    size_t split = text.find_first_of(" \t");
    std::string verb = text.substr(0, split);
    for (char& c : verb)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    std::string rest = split == std::string::npos ? std::string() : Trim(text.substr(split + 1));
    std::string const intent = "commander " + (verb.empty() ? std::string("unknown") : ProtocolSafe(verb));

    if (verb != "bind" && verb != "unbind" && verb != "status" && verb != "view")
    {
        SendActionError(handler, intent, "invalid", "Usage: .bot commander bind <LeadBot> [<Bot> ...] / unbind / status / view on|off");
        return true;
    }
    if (!commander || !commander->GetSession())
    {
        SendActionError(handler, intent, "requester", "You must be in-game.");
        return true;
    }
    if (!sPlayerbotAIConfig.remoteCommander)
    {
        SendActionError(handler, intent, "disabled",
            RemoteCommander::BindRefusalText(RemoteCommander::BindRefusal::Disabled));
        return true;
    }

    if (verb == "bind")
        return HandleCommanderBind(handler, commander, rest);

    uint64 const commanderRaw = commander->GetObjectGuid().GetRawValue();
    RemoteCommander::Binding const* binding = StandingBinding(commanderRaw);
    if (!binding)
    {
        SendActionError(handler, intent, "not-bound", "You command no group. Use .bot commander bind <LeadBot> first.");
        return true;
    }
    Player* lead = FindCharacter(binding->leadGuid);
    std::string const leadName = lead ? lead->GetName() : "-";

    if (verb == "unbind")
    {
        uint32 const hiresBound = HireLifecycle::Instance().CommandedHireCount(commander->GetObjectGuid());
        uint32 restored = ReleaseBinding(commanderRaw, RemoteCommander::ReleaseReason::Unbound, false);
        if (hiresBound)
            handler->PSendSysMessage("You no longer command %s's group. Your %u hired companions rejoin your party; "
                "any other bots stay grouped.", leadName.c_str(), hiresBound);
        else
            handler->PSendSysMessage("You no longer command %s's group. The bots stay grouped; %s holds where it stands.",
                leadName.c_str(), leadName.c_str());
        SendActionAck(handler, intent, "party", restored, leadName);
        return true;
    }

    if (verb == "view")
    {
        std::string mode = rest;
        for (char& c : mode)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (mode != "on" && mode != "off")
        {
            SendActionError(handler, intent, "invalid", "Usage: .bot commander view on|off");
            return true;
        }
        CommanderView& view = CommanderViews()[commanderRaw];
        view.enabled = mode == "on";
        if (!view.enabled)
            view.pacer.Abandon();
        handler->PSendSysMessage("The view of %s's group is %s.", leadName.c_str(), view.enabled ? "on" : "off");
        std::vector<Player*> viewBots = CommandedBots(lead ? lead->GetGroup() : nullptr, lead);
        SendActionAck(handler, intent, "party", static_cast<uint32>(viewBots.size()), mode);
        return true;
    }

    // status
    std::vector<Player*> bots = CommandedBots(lead ? lead->GetGroup() : nullptr, lead);
    std::string roster;
    for (Player* bot : bots)
    {
        roster += (roster.empty() ? "" : ", ") + std::string(bot->GetName());
        if (!bot->IsAlive())
            roster += " (dead)";
    }
    handler->PSendSysMessage("You command %s's group: %s.", leadName.c_str(), roster.c_str());
    SendActionAck(handler, intent, "party", static_cast<uint32>(bots.size()), leadName);
    return true;
}

namespace {

// One standing order context: the commander, the lead bot and its group.
struct CommanderOrder
{
    Player* commander = nullptr;
    Player* lead = nullptr;
    Group* group = nullptr;
    std::vector<Player*> bots;
    // Whose attack rules judge a target: the lead bot, or while it is dead
    // the first live bot of the group on its map (a dead lead bot must not
    // stop the rest of the group from fighting).
    Player* judge = nullptr;
};

// The bound group as a BotCommandContext for the shared pull/CC executors.
// The lead bot stands in for the requester: its position is the pull anchor
// and the DPS hold spot, and its map judges the target. partyBots is the
// whole group, lead included (it may be the tank).
BotCommandContext CommanderContext(CommanderOrder const& order, Unit* target)
{
    BotCommandContext context;
    context.requester = order.lead;
    context.requesterTarget = target;
    context.enemyTarget = target;
    context.group = order.group;
    context.partyBots = order.bots;
    return context;
}

Player* FindCommandedBot(CommanderOrder const& order, std::string const& name)
{
    Player* bot = FindOnlineByName(name);
    if (!bot)
        return nullptr;
    for (Player* member : order.bots)
        if (member == bot)
            return bot;
    return nullptr;
}

} // namespace

// GUID -> a live, hostile, attackable unit on the lead bot's map and instance.
static Unit* ResolveOrderTarget(ChatHandler* handler, std::string const& intent,
    CommanderOrder const& order, std::string const& token)
{
    uint64 raw = 0;
    if (!RemoteCommander::ParseGuid(token, raw))
    {
        SendActionError(handler, intent, "invalid", "Name the target by GUID: 0x<hex> or a decimal number.");
        return nullptr;
    }
    // Map::GetUnit only finds units on the lead bot's own map and instance.
    Map* map = order.lead->GetMap();
    Unit* unit = map ? map->GetUnit(ObjectGuid(raw)) : nullptr;
    RemoteCommander::TargetFacts facts;
    facts.found = unit != nullptr;
    if (unit)
    {
        facts.inWorld = unit->IsInWorld();
        facts.groupMemberOrBot = BotManager::Instance().IsBot(unit->GetObjectGuid()) ||
            (order.group && order.group->IsMember(unit->GetObjectGuid()));
        facts.alive = unit->IsAlive();
        facts.friendly = sServerFacade.IsFriendlyTo(unit, order.judge);
        facts.attackable = order.judge->IsValidAttackTarget(unit);
        // Seen (stealth and invisibility detection, as the bots' own target
        // values check it) by a live bot of the group, and targetable.
        if (unit->IsTargetable(true, true))
            for (Player* bot : order.bots)
                if (bot->IsAlive() && bot->IsInWorld() && bot->GetMap() == map &&
                    unit->IsVisibleForOrDetect(bot, bot->GetCamera().GetBody(), true))
                {
                    facts.visible = true;
                    break;
                }
    }
    RemoteCommander::TargetVerdict verdict = RemoteCommander::CheckOrderTarget(facts);
    if (verdict != RemoteCommander::TargetVerdict::Ok)
    {
        SendActionError(handler, intent, RemoteCommander::TargetVerdictCode(verdict),
            RemoteCommander::TargetVerdictText(verdict));
        return nullptr;
    }
    return unit;
}

namespace {

// A `cmd attack` held for a bot that had lost control of itself (stun, Hex,
// fear: RemoteCommander::HoldsAttackUntilControl), keyed by the bot's GUID.
// UpdateHeldAttacks carries it out when the bot can act again. A later order
// of the commander for that bot (SupersedeCommanderTasks) or the end of the
// binding (CancelCommanderTasks) drops it. World thread only.
struct HeldAttack
{
    uint64 commanderRaw = 0;
    uint64 targetRaw = 0;
    uint32 heldMs = 0;
};

std::unordered_map<uint64, HeldAttack>& HeldAttacks()
{
    static std::unordered_map<uint64, HeldAttack> held;
    return held;
}

void DropHeldAttacks(uint64 commanderRaw, uint64 onlyBotRaw)
{
    auto& held = HeldAttacks();
    for (auto it = held.begin(); it != held.end();)
    {
        if (it->second.commanderRaw == commanderRaw && (!onlyBotRaw || it->first == onlyBotRaw))
            it = held.erase(it);
        else
            ++it;
    }
}

// The per-bot steps of an attack order. selectionOwner carries the target:
// AttackMyTargetAction reads its event owner's selection.
// A party-wide order leaves dedicated healers healing; an order that names
// the healer (`cmd attack <guid> <Healer>`) makes it attack (player control
// first): its explicit attack target then also lets its wand through the
// healer wand gate (HealerMayWand).
bool RunBotAttackOrder(PlayerbotAI* ai, Player* bot, Player* selectionOwner, ObjectGuid targetGuid, bool named = false)
{
    RelaxTacticalMovement(ai);
    ai->ChangeStrategy("-wait for attack", BotState::BOT_STATE_ALL);
    bool accepted = true;
    if (named || !PlayerbotAI::IsHeal(bot))
    {
        selectionOwner->SetSelectionGuid(targetGuid);
        accepted = ExecuteQuietAction(ai, "attack my target", ai::Event("action attack", "", selectionOwner));
    }
    ExecuteQuietNextAction(ai, false);
    return accepted;
}

} // namespace

// Same per-bot steps as `.bot action attack`, with the lead bot's selection
// carrying the GUID-named target: AttackMyTargetAction reads its event
// owner's selection. The commander's own selection is never touched.
static bool CommanderAttack(ChatHandler* handler, CommanderOrder const& order, Unit* target,
    std::string const& intent, Player* onlyBot = nullptr)
{
    ObjectGuid const targetGuid = target->GetObjectGuid();
    uint32 succeeded = 0;
    std::string stayingTank;
    std::vector<std::string> heldBots;
    uint64 const commanderRaw = order.commander->GetObjectGuid().GetRawValue();
    SupersedeCommanderTasks(commanderRaw,
        onlyBot ? onlyBot->GetObjectGuid().GetRawValue() : 0, RemoteCommander::LaterOrder::Attack);
    for (Player* bot : order.bots)
    {
        if (onlyBot && bot != onlyBot)
            continue;
        if (!bot || !bot->IsInWorld() || bot->GetMap() != order.lead->GetMap())
            continue;
        PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
        if (!ai)
            continue;
        // A bot still pulling this target keeps its pull (RemoteCommander::
        // AttackLeavesPullRunning): the pull end hands it the fight, so it
        // counts as having taken the order.
        PullStrategy const* pull = PullStrategy::Get(ai);
        Unit* const pullTarget = pull ? pull->GetTarget() : nullptr;
        if (RemoteCommander::AttackLeavesPullRunning(pullTarget != nullptr, pullTarget == target))
        {
            ++succeeded;
            continue;
        }
        // A tank holding another mob keeps it (RemoteCommander::
        // PartyAttackLeavesTankOnItsMob); it is fighting, so it counts as
        // having taken the order.
        Unit* victim = bot->GetVictim();
        bool const holdsAnotherMob = victim && victim != target && victim->IsAlive() &&
            victim->GetVictim() == bot;
        if (RemoteCommander::PartyAttackLeavesTankOnItsMob(onlyBot == nullptr,
                PlayerbotAI::IsTank(bot), holdsAnotherMob))
        {
            ++succeeded;
            stayingTank = bot->GetName();
            continue;
        }
        // A bot that has lost control of itself (stunned, hexed, feared)
        // cannot take the attack now: the order waits for it
        // (RemoteCommander::HoldsAttackUntilControl) and counts as taken.
        if (RemoteCommander::HoldsAttackUntilControl(PlayerbotAI::IsHeal(bot),
                bot->HasUnitState(UNIT_STAT_CAN_NOT_REACT_OR_LOST_CONTROL)))
        {
            HeldAttack& held = HeldAttacks()[bot->GetObjectGuid().GetRawValue()];
            held.commanderRaw = commanderRaw;
            held.targetRaw = targetGuid.GetRawValue();
            held.heldMs = 0;
            heldBots.push_back(bot->GetName());
            ++succeeded;
            continue;
        }
        // The lead bot's selection is set again for every bot: an earlier
        // bot's first AI step may be the lead bot's own, which can change it.
        if (RunBotAttackOrder(ai, bot, order.lead, targetGuid, onlyBot != nullptr))
            ++succeeded;
    }
    if (!succeeded)
    {
        // A named bot farther from the target than it can see refuses it
        // (AttackAction::IsTargetValid, SightDistance): say so, so the caller
        // can tell it from a refusal it can do nothing about. Uldaman
        // 2026-10-14: the tank, revived at the instance entrance after a
        // wipe, was ordered at a mob 330 yd away and answered only "failed".
        if (onlyBot && onlyBot->IsInWorld() && onlyBot->GetMap() == target->GetMap() &&
            sServerFacade.getDistance2d(onlyBot, target) > sPlayerbotAIConfig.sightDistance)
        {
            std::string const message = "That bot is too far from the target (" +
                std::to_string(int(sServerFacade.getDistance2d(onlyBot, target))) + " yd).";
            SendActionError(handler, intent, "too-far", message.c_str());
            return true;
        }
        SendActionError(handler, intent, "failed", onlyBot ? "That bot did not accept the attack."
            : "No bot of the group accepted the attack.");
        return true;
    }
    if (onlyBot)
        SendActionAck(handler, intent, "bot:" + std::string(onlyBot->GetName()), succeeded,
            RemoteCommander::AttackAckDetail("", heldBots));
    else
        SendActionAck(handler, intent, "party", succeeded, RemoteCommander::AttackAckDetail(stayingTank, heldBots));
    return true;
}

// Each tick while an attack is held: run it for a bot that can act again,
// drop it when the bot or the target is gone or the wait ran out.
static void UpdateHeldAttacks(uint32 diff)
{
    auto& held = HeldAttacks();
    if (held.empty())
        return;
    struct Due
    {
        uint64 botRaw;
        uint64 targetRaw;
    };
    std::vector<Due> due;
    for (auto it = held.begin(); it != held.end();)
    {
        it->second.heldMs += diff;
        Player* bot = FindCharacter(it->first);
        bool const botReady = bot && bot->IsInWorld() && !bot->IsBeingTeleported() && bot->IsAlive() &&
            PlayerbotAIStorage::Instance().GetAI(bot);
        Unit* target = botReady && bot->GetMap() ? bot->GetMap()->GetUnit(ObjectGuid(it->second.targetRaw)) : nullptr;
        bool const targetAlive = target && target->IsInWorld() && target->IsAlive();
        bool const lostControl = botReady && bot->HasUnitState(UNIT_STAT_CAN_NOT_REACT_OR_LOST_CONTROL);
        // The party crowd-controlled the target during the hold (a mage's sheep): hitting it would break that.
        bool const targetCced = targetAlive &&
            ai::PossibleAttackTargetsValue::HoldsBreakableCc(PlayerbotAIStorage::Instance().GetAI(bot), target, bot);
        switch (RemoteCommander::StepHeldAttack(botReady, targetAlive, lostControl, it->second.heldMs, targetCced))
        {
            case RemoteCommander::HeldAttackStep::Wait:
                ++it;
                break;
            case RemoteCommander::HeldAttackStep::Run:
                due.push_back({it->first, it->second.targetRaw});
                it = held.erase(it);
                break;
            case RemoteCommander::HeldAttackStep::Drop:
                it = held.erase(it);
                break;
        }
    }
    // Run after the sweep, so nothing the AI step does can touch the map being iterated.
    for (Due const& order : due)
    {
        Player* bot = FindCharacter(order.botRaw);
        PlayerbotAI* ai = bot ? PlayerbotAIStorage::Instance().GetAI(bot) : nullptr;
        if (ai)
            RunBotAttackOrder(ai, bot, bot, ObjectGuid(order.targetRaw));
    }
}

// `.bot action focus skull` on a GUID: Skull goes on the target in the bound
// group, every bot attacks its Skull target.
static bool CommanderFocus(ChatHandler* handler, CommanderOrder const& order, Unit* target,
    std::string const& intent)
{
    order.group->SetTargetIcon(static_cast<uint8>(RtiTargetValue::GetRtiIndex("skull")), target->GetObjectGuid());
    SupersedeCommanderTasks(order.commander->GetObjectGuid().GetRawValue(), 0, RemoteCommander::LaterOrder::Focus);
    uint32 succeeded = 0;
    for (Player* bot : order.bots)
    {
        if (!bot || !bot->IsInWorld())
            continue;
        PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
        if (!ai)
            continue;
        RelaxTacticalMovement(ai);
        bool set = ExecuteQuietAction(ai, "rti", ai::Event("focus skull", "skull", order.lead));
        bool accepted = set && ExecuteQuietAction(ai, "attack rti target", ai::Event("focus skull", "", order.lead));
        ExecuteQuietNextAction(ai, true);
        if (accepted)
            ++succeeded;
    }
    if (!succeeded)
    {
        SendActionError(handler, intent, "failed", "No bot of the group accepted the focus order.");
        return true;
    }
    SendActionAck(handler, intent, "party", succeeded);
    return true;
}

// The lead bot walks to the point on its navmesh path (MoveTo through the
// mature stay/return-to-stay pair, inside instances too) and holds there; the
// rest of the group follows it.
static bool CommanderMoveDirect(ChatHandler* handler, CommanderOrder const& order,
    RemoteCommander::MovePoint const& point, std::string const& intent);

static bool CommanderMove(ChatHandler* handler, CommanderOrder const& order, std::string const& args,
    std::string const& intent)
{
    RemoteCommander::MoveOrder request;
    if (!RemoteCommander::ParseMoveOrder(args, request))
    {
        SendActionError(handler, intent, "invalid",
            "Usage: .bot cmd move <x> <y> <z> [direct] (map coordinates on the lead bot's map)");
        return true;
    }
    if (request.direct)
        return CommanderMoveDirect(handler, order, request.point, intent);
    RemoteCommander::MovePoint const point = request.point;
    Player* lead = order.lead;
    if (!lead->IsAlive() || lead->IsTaxiFlying() || lead->IsBeingTeleported())
    {
        SendActionError(handler, intent, "lead-unavailable", "The lead bot is dead, on a taxi or teleporting.");
        return true;
    }
    PlayerbotAI* leadAi = PlayerbotAIStorage::Instance().GetAI(lead);
    if (!leadAi)
    {
        SendActionError(handler, intent, "no-ai", "The lead bot has no AI.");
        return true;
    }

    float const dx = point.x - lead->GetPositionX();
    float const dy = point.y - lead->GetPositionY();
    float const dz = point.z - lead->GetPositionZ();
    RemoteCommander::MoveFacts facts;
    facts.distanceFromLead = std::sqrt(dx * dx + dy * dy + dz * dz);
    facts.maxDistance = sPlayerbotAIConfig.reactDistance;
    PathFinder path(lead);
    path.calculate(point.x, point.y, point.z, false);
    facts.pathType = static_cast<uint32>(path.getPathType());
    Vector3 const end = path.getActualEndPosition();
    float const ex = end.x - point.x;
    float const ey = end.y - point.y;
    float const ez = end.z - point.z;
    facts.endOffset = std::sqrt(ex * ex + ey * ey + ez * ez);
    RemoteCommander::MoveVerdict verdict = RemoteCommander::CheckMove(facts);
    if (verdict != RemoteCommander::MoveVerdict::Ok)
    {
        std::string message;
        if (verdict == RemoteCommander::MoveVerdict::TooFar)
            message = "The point is more than " + std::to_string(static_cast<uint32>(facts.maxDistance)) +
                " yd (AiPlayerbot.ReactDistance) from the lead bot. Move in shorter legs.";
        else if (verdict == RemoteCommander::MoveVerdict::NoPath)
            message = "The lead bot has no walkable path to that point.";
        else
            message = "The point is not on walkable ground (the path ends more than 5 yd from it).";
        SendActionError(handler, intent, RemoteCommander::MoveVerdictCode(verdict), message.c_str());
        return true;
    }

    SupersedeCommanderTasks(order.commander->GetObjectGuid().GetRawValue(), 0, RemoteCommander::LaterOrder::Move);

    // Same transition as `.bot action come`, with the point as the stay spot.
    leadAi->DropQueuedMovementOrders();
    PlayerConvenience::Instance().NoteHoldOrder(lead->GetObjectGuid());
    leadAi->Reset();
    leadAi->SetMovementStrategy("stay");
    SetStayAnchor(leadAi, point.x, point.y, point.z, lead->GetMapId());
    ExecuteQuietAction(leadAi, "return to stay position",
        ai::Event("return to stay position", "stay", order.commander));

    // The group follows the lead bot.
    std::vector<Player*> followers;
    for (Player* bot : order.bots)
        if (bot != lead)
            followers.push_back(bot);
    uint32 following = FormUpOnLead(order.commander, lead, followers, false);
    SendActionAck(handler, intent, "party", following + 1, lead->GetName());
    return true;
}

// `cmd move <x> <y> <z> direct`: for a gap the navmesh does not bridge. The lead bot walks a straight
// line to the point (a MovePoint without MOVE_PATHFINDING: the core's own straight walk), the way a
// player walks it. Only a short step: the lead bot within kDirectMaxDistance horizontally and
// kDirectMaxVertical vertically of the point, the point on the ground (the map height at most
// kDirectGroundTolerance below it) and in the lead bot's sight (no-los). The bots walk to that floor.
// The others take the lead bot's line: each walks to the lead bot's spot first (on its complete navmesh
// path of at most kDirectMaxGatherPath yd, else straight when it stands near it on its floor and sees it)
// and on from there to the point, so a follower below a ledge or behind a corner never walks its own
// line through it. Nothing moves unless the lead bot passes and every follower has a way to its spot
// (too-far). Afterwards the lead bot holds at the point and the others follow it, as after a plain
// `cmd move`.
static bool CommanderMoveDirect(ChatHandler* handler, CommanderOrder const& order,
    RemoteCommander::MovePoint const& point, std::string const& intent)
{
    Player* lead = order.lead;
    if (!lead->IsAlive() || lead->IsTaxiFlying() || lead->IsBeingTeleported())
    {
        SendActionError(handler, intent, "lead-unavailable", "The lead bot is dead, on a taxi or teleporting.");
        return true;
    }
    if (!PlayerbotAIStorage::Instance().GetAI(lead))
    {
        SendActionError(handler, intent, "no-ai", "The lead bot has no AI.");
        return true;
    }

    float const leadX = lead->GetPositionX();
    float const leadY = lead->GetPositionY();
    float const leadZ = lead->GetPositionZ();
    {
        float const dx = point.x - leadX;
        float const dy = point.y - leadY;
        RemoteCommander::DirectVerdict const step =
            RemoteCommander::CheckDirectStep(std::sqrt(dx * dx + dy * dy), point.z - leadZ);
        if (step != RemoteCommander::DirectVerdict::Ok)
        {
            std::string const message = "The point is more than " +
                std::to_string(static_cast<uint32>(RemoteCommander::kDirectMaxDistance)) + " yd (or " +
                std::to_string(static_cast<uint32>(RemoteCommander::kDirectMaxVertical)) +
                " yd up or down) from " + lead->GetName() + ". A direct step is short: walk closer first.";
            SendActionError(handler, intent, RemoteCommander::DirectVerdictCode(step), message.c_str());
            return true;
        }
    }

    // The floor under the point (the vmap/terrain height searched down from just above it). The bots walk to it,
    // not to the point's own z: a floor above the point would be inside the geometry, one far below leaves them hanging.
    Map* const map = lead->GetMap();
    float ground = INVALID_HEIGHT;
    if (MaNGOS::IsValidMapCoord(point.x, point.y, point.z))
        ground = map->GetHeight(point.x, point.y, point.z, true, RemoteCommander::kDirectGroundTolerance + 2.0f);
    if (RemoteCommander::CheckDirectGround(ground, point.z) != RemoteCommander::DirectVerdict::Ok)
    {
        SendActionError(handler, intent, RemoteCommander::DirectVerdictCode(RemoteCommander::DirectVerdict::NoGround),
            "The point has no ground within 3 yd below it (or 0.5 yd above it).");
        return true;
    }

    // Nothing is checked on the line itself (no navmesh), so the lead bot must see the point: a lead on the pit floor
    // below the ramp would otherwise cut through the wall.
    float const h = RemoteCommander::kDirectLosHeight;
    bool const leadSees = MaNGOS::IsValidMapCoord(leadX, leadY, leadZ) &&
        map->isInLineOfSight(leadX, leadY, leadZ + h, point.x, point.y, ground + h);
    if (RemoteCommander::CheckDirectLos(leadSees) != RemoteCommander::DirectVerdict::Ok)
    {
        std::string const message = std::string(lead->GetName()) + " has no line of sight to the point. A direct step "
            "does not cut through walls: walk the lead bot to where it sees it first.";
        SendActionError(handler, intent, RemoteCommander::DirectVerdictCode(RemoteCommander::DirectVerdict::NoLos),
            message.c_str());
        return true;
    }

    // Each follower's route: to the lead bot's spot, then the lead bot's line to the point.
    Vector3 const leadSpot(leadX, leadY, leadZ);
    Vector3 const target(point.x, point.y, ground);
    struct DirectWalker
    {
        Player* bot;
        Movement::PointsArray route; // empty for the lead bot (a MovePoint)
    };
    std::vector<DirectWalker> walkers;
    walkers.push_back({ lead, {} });
    for (Player* bot : order.bots)
    {
        if (!bot || bot == lead || !bot->IsInWorld() || !bot->IsAlive() || bot->GetMap() != lead->GetMap() ||
            bot->IsTaxiFlying() || bot->IsBeingTeleported() || !PlayerbotAIStorage::Instance().GetAI(bot))
            continue;
        float const bx = bot->GetPositionX();
        float const by = bot->GetPositionY();
        float const bz = bot->GetPositionZ();
        float const horizontal = std::sqrt((bx - leadX) * (bx - leadX) + (by - leadY) * (by - leadY));
        bool const valid = MaNGOS::IsValidMapCoord(bx, by, bz);
        bool const nearLead = horizontal <= RemoteCommander::kDirectNearLead &&
            std::fabs(bz - leadZ) <= RemoteCommander::kDirectNearLeadVertical;
        bool const seesLead = valid && nearLead && map->isInLineOfSight(bx, by, bz + h, leadX, leadY, leadZ + h);
        bool pathComplete = false;
        float pathLength = -1.0f;
        Movement::PointsArray route;
        if (valid)
        {
            PathFinder path(bot);
            path.calculate(leadX, leadY, leadZ, false);
            Vector3 const end = path.getActualEndPosition();
            uint32 const type = static_cast<uint32>(path.getPathType());
            pathComplete = (type & PATHFIND_NORMAL) &&
                !(type & (PATHFIND_INCOMPLETE | PATHFIND_NOPATH | PATHFIND_SHORTCUT)) &&
                (end - leadSpot).length() <= 2.0f && path.getPath().size() >= 2;
            pathLength = path.Length();
            if (pathComplete)
                route = path.getPath();
        }
        RemoteCommander::DirectJoin const join =
            RemoteCommander::ChooseDirectJoin(horizontal, bz - leadZ, seesLead, pathComplete, pathLength);
        if (join == RemoteCommander::DirectJoin::None)
        {
            std::string const message = std::string(bot->GetName()) + " is not with " + lead->GetName() +
                " (no navmesh path of at most " +
                std::to_string(static_cast<uint32>(RemoteCommander::kDirectMaxGatherPath)) +
                " yd to its spot, and not within " + std::to_string(static_cast<uint32>(RemoteCommander::kDirectNearLead)) +
                " yd of it on its floor in its sight). A direct step takes the lead bot's line: gather the party on it first.";
            SendActionError(handler, intent, RemoteCommander::DirectVerdictCode(RemoteCommander::DirectVerdict::TooFar),
                message.c_str());
            return true;
        }
        if (join == RemoteCommander::DirectJoin::Straight)
            route = { Vector3(bx, by, bz) };
        // Up to the lead bot's spot, then on along its line; a point on top of the previous one is dropped (a spline
        // needs distinct points).
        for (Vector3 const& next : { leadSpot, target })
            if (route.empty() || (next - route.back()).length() > 0.3f)
                route.push_back(next);
        if (route.size() < 2)
            route.push_back(target);
        walkers.push_back({ bot, std::move(route) });
    }

    SupersedeCommanderTasks(order.commander->GetObjectGuid().GetRawValue(), 0, RemoteCommander::LaterOrder::Move);

    uint32 walking = 0;
    for (DirectWalker const& walker : walkers)
    {
        Player* bot = walker.bot;
        PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
        float length = 0.0f;
        if (bot == lead)
        {
            ai->DropQueuedMovementOrders();
            PlayerConvenience::Instance().NoteHoldOrder(lead->GetObjectGuid());
            ai->Reset();
            ai->SetMovementStrategy("stay");
            SetStayAnchor(ai, point.x, point.y, ground, lead->GetMapId());
            // The straight walk. The bot's own AI would re-plan it on the navmesh at its next step
            // (the stay spot is ahead of it), so the AI sleeps for the walk, as it does after a path move.
            bot->GetMotionMaster()->Clear();
            bot->GetMotionMaster()->MovePoint(0, point.x, point.y, ground, MOVE_RUN_MODE);
            length = (target - leadSpot).length();
        }
        else
        {
            FormUpOnLead(order.commander, lead, std::vector<Player*>{ bot }, false);
            bot->GetMotionMaster()->Clear();
            Movement::MoveSplineInit init(*bot, "CommanderMoveDirect");
            init.MovebyPath(walker.route);
            init.SetWalk(false);
            init.Launch();
            for (size_t i = 1; i < walker.route.size(); ++i)
                length += (walker.route[i] - walker.route[i - 1]).length();
        }
        // A bot in a fight keeps its AI (a healer asleep for 4 s would stop healing); it may re-plan the step.
        if (!bot->IsInCombat())
        {
            float const speed = bot->GetSpeed(MOVE_RUN) > 1.0f ? bot->GetSpeed(MOVE_RUN) : 7.0f;
            // Capped: on a long route the bot wakes before the end, so one attacked on the way still reacts.
            ai->SetActionDuration(std::min<uint32>(static_cast<uint32>(length / speed * 1000.0f) +
                sPlayerbotAIConfig.reactDelay, RemoteCommander::kDirectMaxSleepMs));
        }
        ++walking;
    }
    if (!walking)
    {
        SendActionError(handler, intent, "failed", "No bot of the group could take the step.");
        return true;
    }
    SendActionAck(handler, intent, "party", walking, lead->GetName());
    return true;
}

static bool CommanderStayFollowRest(ChatHandler* handler, CommanderOrder const& order,
    std::string const& verb, std::string const& intent)
{
    uint32 succeeded = 0;
    // Any of these orders replaces an attack still held for a bot under crowd control.
    DropHeldAttacks(order.commander->GetObjectGuid().GetRawValue(), 0);
    if (verb == "stay" || verb == "follow")
        SupersedeCommanderTasks(order.commander->GetObjectGuid().GetRawValue(), 0,
            verb == "stay" ? RemoteCommander::LaterOrder::Stay : RemoteCommander::LaterOrder::Follow);
    if (verb == "follow")
        succeeded = FormUpOnLead(order.commander, order.lead, order.bots, false);
    else
    {
        for (Player* bot : order.bots)
        {
            if (!bot || !bot->IsInWorld())
                continue;
            PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
            if (!ai)
                continue;
            bool accepted = false;
            if (verb == "stay")
            {
                ai->DropQueuedMovementOrders();
                accepted = ExecuteQuietAction(ai, "stay chat shortcut", ai::Event("stay", "", order.commander));
            }
            else // rest: same as `.bot action rest`
            {
                bool rested = false;
                if (bot->GetPowerType() == POWER_MANA && bot->GetPower(POWER_MANA) < bot->GetMaxPower(POWER_MANA))
                    rested = ExecuteQuietAction(ai, "drink", ai::Event("rest", "drink", order.commander)) || rested;
                if (bot->GetHealth() < bot->GetMaxHealth())
                    rested = ExecuteQuietAction(ai, "food", ai::Event("rest", "food", order.commander)) || rested;
                accepted = rested || (bot->GetHealth() >= bot->GetMaxHealth() &&
                    (bot->GetPowerType() != POWER_MANA || bot->GetPower(POWER_MANA) >= bot->GetMaxPower(POWER_MANA)));
            }
            if (accepted)
                ++succeeded;
        }
    }
    if (!succeeded)
    {
        SendActionError(handler, intent, "failed", "No bot of the group accepted the order.");
        return true;
    }
    SendActionAck(handler, intent, "party", succeeded, verb == "rest" ? "" : order.lead->GetName());
    return true;
}

// revive: speeds up the dead bot's own way back (release now, run to the
// corpse); the dead strategy then revives it at the corpse. A healer in the
// group still resurrects a body that has not released on its own.
static bool CommanderRevive(ChatHandler* handler, CommanderOrder const& order, Player* bot,
    std::string const& intent)
{
    if (bot->IsAlive())
    {
        SendActionError(handler, intent, "not-dead", "That bot is alive.");
        return true;
    }
    PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
    if (!ai)
    {
        SendActionError(handler, intent, "no-ai", "That bot has no AI.");
        return true;
    }
    bool released = ExecuteQuietAction(ai, "release", ai::Event("revive", "release", order.commander));
    bool running = ExecuteQuietAction(ai, "corpse run", ai::Event("revive", "corpse run", order.commander));
    if (!released && !running)
    {
        SendActionError(handler, intent, "failed", "The bot could neither release nor run back to its corpse.");
        return true;
    }
    SendActionAck(handler, intent, "bot:" + std::string(bot->GetName()), 1, released ? "release" : "corpse run");
    return true;
}

// summon: the `.bot summon` flow (3 s, portal, follow restored on arrival),
// with the lead bot as the summon point instead of the commander.
static bool CommanderSummon(ChatHandler* handler, CommanderOrder const& order, Player* bot,
    std::string const& intent)
{
    Player* lead = order.lead;
    if (bot == lead)
    {
        SendActionError(handler, intent, "invalid", "The lead bot is the summon point; summon another bot to it.");
        return true;
    }
    if (!lead->IsAlive() || lead->IsTaxiFlying() || lead->IsBeingTeleported())
    {
        SendActionError(handler, intent, "lead-unavailable", "The lead bot is dead, on a taxi or teleporting.");
        return true;
    }
    if (!bot->IsAlive() || bot->IsInCombat() || bot->IsTaxiFlying() || bot->IsBeingTeleported())
    {
        SendActionError(handler, intent, "unavailable", "That bot is dead, in combat, on a taxi or teleporting.");
        return true;
    }
    if (PlayerConvenience::Instance().IsBusy(bot->GetObjectGuid()))
    {
        SendActionError(handler, intent, "busy", "That bot already has a summon pending.");
        return true;
    }
    // As `.bot summon`: the summoner becomes the bot's master if it is not.
    BotRecord* record = BotManager::Instance().FindBot(bot->GetObjectGuid());
    if (!record || (record->masterGuid != order.commander->GetObjectGuid() &&
        !BotManager::Instance().BindBotMaster(bot->GetObjectGuid(), order.commander->GetObjectGuid())))
    {
        SendActionError(handler, intent, "failed", "The bot could not be assigned to you for the summon.");
        return true;
    }
    SupersedeCommanderTasks(order.commander->GetObjectGuid().GetRawValue(), bot->GetObjectGuid().GetRawValue(),
        RemoteCommander::LaterOrder::Summon);
    if (!PlayerConvenience::Instance().RequestSummon(order.commander, bot, lead))
    {
        SendActionError(handler, intent, "failed", "The summon was refused.");
        return true;
    }
    SendActionAck(handler, intent, "bot:" + std::string(bot->GetName()), 1, lead->GetName());
    return true;
}

// ---------------------------------------------------------------------------
// Per-bot player orders: loot, open, use (and lootmethod)
//
// What a player does at a corpse, a chest, a lever or in the bags, done by one
// bound bot on the commander's word. `cmd loot <guid> <bot> [itemId]`, `cmd
// open <guid> <bot>`, `cmd use <itemId> <bot> [guid]` name a bot of the
// commanded group that the commander owns (or a GM). Synchronous refusals answer
// with TBM:ACTION_ERR at once. An accepted order runs as a short task in
// this file: walk (the "stay" anchor and a direct path step, bounded by the
// react distance and a time budget), do the one action through the bot's own
// loot or use code, and answer later with the final TBM:ACTION_ACK or
// TBM:ACTION_ERR of the same verb. TBM:ACTION_PENDING marks an accepted
// order that is still running. Decisions: runtime/RemoteCommanderTasks.h.
// World thread only (UpdateRemoteCommanders drives the tasks); nothing runs
// while no task exists.
// ---------------------------------------------------------------------------

namespace {

struct CommanderTask
{
    RemoteCommander::TaskState state;
    uint64 commanderRaw = 0;
    uint64 botRaw = 0;
    uint64 targetRaw = 0;     // the corpse or game object to walk to; 0 = none
    uint64 useTargetRaw = 0;  // use: the unit or game object the item is used on
    uint32 itemId = 0;        // loot: only this item (0 = all); use: the item
    uint32 spellId = 0;       // use: the item's on-use spell
    uint32 actSpellId = 0;    // the spell the started act is casting (0 = none): the one the hold protects
    uint32 actCastMs = 0;     // that spell's cast time at the start
    uint32 actRunMs = 0;      // how long it has run since
    RemoteCommander::Point3 actFrom; // where the bot stood when it started
    bool lootLike = false;    // ends with a loot window (corpse, chest)
    bool noWalk = false;      // nothing to walk to
    bool walking = false;     // the movement stance was changed for the walk
    bool wasStaying = false;
    bool actStarted = false;
    bool superseded = false;  // a later order set the bot's movement: it is not ours to put back
    bool departed = false;    // the bot left the commanded party
    uint32 ritualId = 0;      // ritual: the order's group (RitualGroups); its clock runs the act, not this task
    bool ready = false;       // ritual: the bot stands in range of the object, holding
    uint32 actTries = 0;
    uint32 nudgeMs = 0;
    uint32 baseline = 0;      // open: the game object's state when the action began
    RemoteCommander::Point3 stand;
    std::string lastCode;     // the newest refusal while the action is retried
    std::string lastMessage;
    std::string intent;
    std::string botName;
    // cmd enter: this task walks to an area trigger and sends it (StepEnterTask).
    // Its result goes to the order's tally, not to the commander on its own.
    bool enter = false;
    bool enterLead = false;   // the lead bot of the order: the others follow it in
    uint32 enterOrder = 0;    // the order's serial (EnterOrders())
    uint32 triggerId = 0;
    RemoteCommander::EnterState enterState;
};

std::unordered_map<uint64, CommanderTask>& CommanderTasks()
{
    static std::unordered_map<uint64, CommanderTask> tasks;
    return tasks;
}

uint32 s_commanderTaskMs = 0;

// A `cmd ritual` order: the bots' tasks (one per bot, kind Ritual, linked by ritualId) and the clock
// that decides for all of them (RemoteCommander::AdvanceRitual).
struct RitualGroup
{
    uint32 id = 0;
    uint64 commanderRaw = 0;
    uint64 targetRaw = 0;
    std::vector<uint64> members;  // bot GUIDs, in the order typed; the first uses first
    RemoteCommander::RitualState state;
    std::string intent;
};

std::unordered_map<uint32, RitualGroup>& RitualGroups()
{
    static std::unordered_map<uint32, RitualGroup> groups;
    return groups;
}

uint32 s_ritualNextId = 0;

// A ritual order ended for a reason of one member (or of the whole group): ends the other members, answers once.
void FailRitualGroup(uint32 id, uint64 exceptBot, std::string const& code, std::string const& message);

// cmd enter: one record per running order, keyed by a serial. The bots' tasks report
// their result here; when all have reported the order answers once (ACK entered, or the
// ERR that names the cause). Decisions: runtime/RemoteCommanderEnter.h.
struct EnterOrderState
{
    uint64 commanderRaw = 0;
    uint32 triggerId = 0;
    std::string intent;
    std::string scope; // party, or bot:<Name> for the one bot named
    RemoteCommander::EnterTally tally;
};

std::unordered_map<uint32, EnterOrderState>& EnterOrders()
{
    static std::unordered_map<uint32, EnterOrderState> orders;
    return orders;
}

uint32 s_enterSerial = 0;

void NoteEnterResult(CommanderTask const& task, bool ok, std::string const& code, std::string const& message)
{
    auto& orders = EnterOrders();
    auto it = orders.find(task.enterOrder);
    if (it == orders.end())
        return;
    EnterOrderState& order = it->second;
    if (ok)
        RemoteCommander::NoteEntered(order.tally, task.enterLead);
    else
        RemoteCommander::NoteFailed(order.tally, task.enterLead, task.botName, code, message);
    if (!RemoteCommander::EnterComplete(order.tally))
        return;
    RemoteCommander::EnterAnswer const answer = RemoteCommander::FinalEnterAnswer(order.tally);
    Player* commander = FindCharacter(order.commanderRaw);
    if (commander && commander->IsInWorld() && commander->GetSession())
    {
        ChatHandler handler(commander);
        if (answer.ok)
            SendActionAck(&handler, order.intent, order.scope, answer.count, answer.detail);
        else
            SendActionError(&handler, order.intent, answer.code.c_str(), answer.message.c_str());
    }
    orders.erase(it);
}

struct TaskTarget
{
    Creature* creature = nullptr;
    GameObject* go = nullptr;
    WorldObject* object() const
    {
        if (creature)
            return creature;
        return go;
    }
};

TaskTarget ResolveTaskTarget(PlayerbotAI* ai, uint64 raw)
{
    TaskTarget target;
    ObjectGuid const guid(raw);
    if (guid.IsGameObject())
        target.go = ai->GetGameObject(guid);
    else if (guid.IsAnyTypeCreature())
        target.creature = ai->GetCreature(guid);
    return target;
}

// Still there to walk to: a corpse, or a spawned game object.
bool TaskTargetPresent(TaskTarget const& target)
{
    if (target.creature)
        return sServerFacade.GetDeathState(target.creature) == CORPSE;
    if (target.go)
        return sServerFacade.isSpawned(target.go);
    return false;
}

// The state of a game object that a use changes: its loot state, its go state
// and whether it is still spawned.
uint32 PackGoState(GameObject* go)
{
    if (!go)
        return 0;
    return (static_cast<uint32>(go->getLootState()) << 8) | static_cast<uint32>(go->GetGoState());
}

// The lock of a game object, as the core's lock rows say it: a key item the
// bot carries, or skill it has.
RemoteCommander::LockFacts GoLockFacts(Player* bot, GameObject* go)
{
    RemoteCommander::LockFacts facts;
    if (!go || !go->GetGOInfo())
        return facts;
    LockEntry const* lock = sLockStore.LookupEntry(go->GetGOInfo()->GetLockId());
    if (!lock)
        return facts;
    for (int i = 0; i < 8; ++i)
    {
        if (lock->Type[i] == LOCK_KEY_ITEM && lock->Index[i] > 0)
        {
            uint32 const key = lock->Index[i];
            if (!facts.hasKey)
            {
                facts.keyItem = key;
                facts.hasKey = bot->HasItemCount(key, 1, false);
            }
        }
        else if (lock->Type[i] == LOCK_KEY_SKILL && !facts.skillId)
        {
            uint32 const skillId = SkillByLockType(LockType(lock->Index[i]));
            if (skillId > 0)
            {
                facts.skillId = skillId;
                facts.reqSkill = std::max<uint32>(1, lock->Skill[i]);
                facts.skillValue = bot->GetSkillValue(skillId);
            }
        }
    }
    return facts;
}

// ---------------------------------------------------------------------------
// Targets by entry (cmd open|loot|use entry:<id>[@x,y,z], cmd state)
// ---------------------------------------------------------------------------

float DistanceToPoint(RemoteCommander::MovePoint const& point, WorldObject const* object)
{
    float const dx = object->GetPositionX() - point.x;
    float const dy = object->GetPositionY() - point.y;
    float const dz = object->GetPositionZ() - point.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// Grid-search predicate: a spawned game object of the entry within range of the point.
class EntryGameObjectCheck
{
public:
    EntryGameObjectCheck(uint32 entry, RemoteCommander::MovePoint const& reference, float range)
        : m_entry(entry), m_reference(reference), m_range(range) {}
    bool operator()(GameObject* go)
    {
        return go && go->GetEntry() == m_entry && sServerFacade.isSpawned(go) &&
            DistanceToPoint(m_reference, go) <= m_range;
    }

private:
    uint32 m_entry;
    RemoteCommander::MovePoint m_reference;
    float m_range;
};

// Grid-search predicate: a corpse of the creature entry within range of the point.
class EntryCorpseCheck
{
public:
    EntryCorpseCheck(uint32 entry, RemoteCommander::MovePoint const& reference, float range)
        : m_entry(entry), m_reference(reference), m_range(range) {}
    bool operator()(Creature* creature)
    {
        return creature && creature->GetEntry() == m_entry &&
            sServerFacade.GetDeathState(creature) == CORPSE && DistanceToPoint(m_reference, creature) <= m_range;
    }

private:
    uint32 m_entry;
    RemoteCommander::MovePoint m_reference;
    float m_range;
};

// A by-entry target's point must be within ReactDistance of the bot, checked
// before anything is searched. True without a point (the search then runs
// around the bot).
bool EntryPointReachable(Player* bot, RemoteCommander::TargetSpec const& spec)
{
    if (!spec.hasPoint)
        return true;
    RemoteCommander::MovePoint botPoint;
    botPoint.x = bot->GetPositionX();
    botPoint.y = bot->GetPositionY();
    botPoint.z = bot->GetPositionZ();
    float const dx = spec.point.x - botPoint.x;
    float const dy = spec.point.y - botPoint.y;
    float const dz = spec.point.z - botPoint.z;
    return RemoteCommander::EntryPointInReach(std::sqrt(dx * dx + dy * dy + dz * dz), sPlayerbotAIConfig.reactDistance);
}

// The GUID of the nearest spawned game object of spec.entry (and, with
// corpses, the nearest corpse of that creature entry, a lootable one first)
// around the order's point, or around the bot when it gave none. One bounded
// grid search of the bot's map (the cells that radius touches, loaded ones
// only), never a world scan. 0 when nothing is there.
uint64 FindEntryTarget(Player* bot, RemoteCommander::TargetSpec const& spec, bool corpses)
{
    Map* map = bot->GetMap();
    if (!map || !spec.ByEntry())
        return 0;
    RemoteCommander::MovePoint reference = spec.point;
    if (!spec.hasPoint)
    {
        reference.x = bot->GetPositionX();
        reference.y = bot->GetPositionY();
        reference.z = bot->GetPositionZ();
    }
    float const radius = RemoteCommander::EntrySearchRadius(spec.hasPoint, sPlayerbotAIConfig.reactDistance);
    if (radius <= 0.0f)
        return 0;
    // Only what the bot can reach is ever returned (within ReactDistance of
    // the bot), so a not-found does not tell what lies farther away.
    RemoteCommander::MovePoint botPoint;
    botPoint.x = bot->GetPositionX();
    botPoint.y = bot->GetPositionY();
    botPoint.z = bot->GetPositionZ();

    std::vector<RemoteCommander::EntryCandidate> objects;
    {
        std::list<GameObject*> found;
        EntryGameObjectCheck check(spec.entry, reference, radius);
        MaNGOS::GameObjectListSearcher<EntryGameObjectCheck> searcher(found, check);
        Cell::VisitGridObjects(reference.x, reference.y, map, searcher, radius);
        for (GameObject* go : found)
            if (RemoteCommander::EntryPointInReach(DistanceToPoint(botPoint, go), sPlayerbotAIConfig.reactDistance))
                objects.push_back({go->GetObjectGuid().GetRawValue(), DistanceToPoint(reference, go)});
    }
    std::vector<RemoteCommander::EntryCandidate> lootable;
    std::vector<RemoteCommander::EntryCandidate> corpseAny;
    if (corpses)
    {
        std::list<Creature*> found;
        EntryCorpseCheck check(spec.entry, reference, radius);
        MaNGOS::CreatureListSearcher<EntryCorpseCheck> searcher(found, check);
        Cell::VisitGridObjects(reference.x, reference.y, map, searcher, radius);
        for (Creature* creature : found)
        {
            if (!RemoteCommander::EntryPointInReach(DistanceToPoint(botPoint, creature), sPlayerbotAIConfig.reactDistance))
                continue;
            RemoteCommander::EntryCandidate const candidate{creature->GetObjectGuid().GetRawValue(),
                DistanceToPoint(reference, creature)};
            corpseAny.push_back(candidate);
            if (creature->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE))
                lootable.push_back(candidate);
        }
    }
    // Objects and lootable corpses compete on distance; a corpse that has
    // nothing left is chosen only when nothing else is there (the order then
    // answers that there is nothing to loot).
    std::vector<RemoteCommander::EntryCandidate> pool = objects;
    pool.insert(pool.end(), lootable.begin(), lootable.end());
    if (pool.empty())
        pool = corpseAny;
    uint64 guid = 0;
    return RemoteCommander::PickNearestEntry(pool, radius, guid) ? guid : 0;
}

// The GUID an order's target names: the GUID as typed, or the nearest object
// of the entry. 0 (with the ERR already sent) when the point is out of the bot's reach (`too-far`,
// before any search) or the search finds nothing. The ACK of a by-entry order is preceded by one
// `TBM:ACTION_TARGET` line, printed only after both checks passed, that names the GUID chosen, so a client can log it and use it later (cmd state, cmd attack).
uint64 ResolveOrderGuid(ChatHandler* handler, std::string const& intent, Player* bot,
    RemoteCommander::TargetSpec const& spec, bool corpses)
{
    if (!spec.ByEntry())
        return spec.guid;
    if (!EntryPointReachable(bot, spec))
    {
        SendActionError(handler, intent, RemoteCommander::kCodeTooFar,
            "That point is farther from the bot than AiPlayerbot.ReactDistance. Move the group closer first.");
        return 0;
    }
    uint64 const guid = FindEntryTarget(bot, spec, corpses);
    if (!guid)
    {
        SendActionError(handler, intent, RemoteCommander::kCodeNotFound,
            corpses
                ? "No spawned game object or corpse of that entry is near the point (or the bot)."
                : "No spawned game object of that entry is near the point (or the bot).");
        return 0;
    }
    handler->PSendSysMessage("TBM:ACTION_TARGET|%s|bot:%s|%s|%u", ProtocolSafe(intent).c_str(),
        ProtocolSafe(bot->GetName()).c_str(), std::to_string(guid).c_str(), spec.entry);
    return guid;
}

bool TaskInRange(CommanderTask const& task, Player* bot, TaskTarget const& target)
{
    if (task.noWalk)
        return true;
    if (target.creature)
        return target.creature->IsWithinDistInMap(bot, bot->GetMaxLootDistance(target.creature), true,
            SizeFactor::None);
    if (target.go)
        return RemoteCommander::InUseRange(bot->GetDistance(target.go), INTERACTION_DISTANCE);
    return false;
}

void TaskReplyAck(CommanderTask const& task, uint32 count, std::string const& payload)
{
    Player* commander = FindCharacter(task.commanderRaw);
    if (!commander || !commander->IsInWorld() || !commander->GetSession())
        return;
    ChatHandler handler(commander);
    SendActionAck(&handler, task.intent, "bot:" + task.botName, count, payload);
}

void TaskReplyErr(CommanderTask const& task, std::string const& code, std::string const& message)
{
    Player* commander = FindCharacter(task.commanderRaw);
    if (!commander || !commander->IsInWorld() || !commander->GetSession())
        return;
    ChatHandler handler(commander);
    SendActionError(&handler, task.intent, code.c_str(), message.c_str());
}

// The walk is over: a bot that followed goes back to following the lead bot;
// one that was holding a spot, or is the lead bot, holds where it stands.
void EndTaskWalk(CommanderTask const& task, Player* bot, PlayerbotAI* ai)
{
    if (!task.walking || task.superseded || !bot || !ai || !bot->IsInWorld())
        return;
    if (task.departed)
    {
        // Left the party: its own follow target is back (ReleaseDepartedFollowers); the lead bot is not its lead.
        if (task.wasStaying)
            SetStayAnchor(ai, bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), bot->GetMapId());
        else
            ai->SetMovementStrategy("follow");
        return;
    }
    if (task.wasStaying)
    {
        SetStayAnchor(ai, bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), bot->GetMapId());
        return;
    }
    Player* commander = FindCharacter(task.commanderRaw);
    RemoteCommander::Binding const* binding = CommanderBindings().FindByCommander(task.commanderRaw);
    Player* lead = binding ? FindCharacter(binding->leadGuid) : nullptr;
    if (commander && lead && lead->IsInWorld())
    {
        std::vector<Player*> one(1, bot);
        FormUpOnLead(commander, lead, one, false);
    }
    else
        ai->SetMovementStrategy("follow");
}

// Tasks that ended while their bot was out of the world or between maps (a
// map change, a teleport): their stance and the act hold are put back once the
// bot stands in a world again; a bot that logged out is dropped.
std::vector<CommanderTask>& PendingStanceRestores()
{
    static std::vector<CommanderTask> pending;
    return pending;
}

void EndTaskStance(CommanderTask const& task, Player* bot, PlayerbotAI* ai)
{
    if (task.lootLike && ai->GetAiObjectContext())
        ai->GetAiObjectContext()->GetValue<ai::LootObject>("loot target")->Set(ai::LootObject());
    EndTaskWalk(task, bot, ai);
}

// Ends a task: ledger forgotten, loot target cleared, movement stance back,
// then the final answer.
void ReleaseTaskStance(CommanderTask const& task)
{
    RemoteCommander::LootOrders().Forget(task.botRaw);
    if (task.ritualId)
        RemoteCommander::RitualHolds().Release(task.botRaw);
    Player* bot = FindCharacter(task.botRaw);
    PlayerbotAI* ai = bot ? PlayerbotAIStorage::Instance().GetAI(bot) : nullptr;
    if (ai && bot->IsInWorld() && !bot->IsBeingTeleported())
        EndTaskStance(task, bot, ai);
    else if (bot && task.walking && !task.superseded)
        PendingStanceRestores().push_back(task);
}

// A ritual member that is let go before the ritual is over stops its helper channel, so it leaves the
// object's participants and nothing is left half used. Not once the object is active: its cast is
// sent, and a participant leaving then would send the object back to ready.
void StopRitualChannel(CommanderTask const& task)
{
    Player* bot = FindCharacter(task.botRaw);
    if (!bot || !bot->IsInWorld() || !task.actSpellId)
        return;
    Spell* spell = bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
    if (!spell || !spell->m_spellInfo || spell->m_spellInfo->Id != task.actSpellId)
        return;
    GameObject* go = bot->GetMap() ? bot->GetMap()->GetGameObject(ObjectGuid(task.targetRaw)) : nullptr;
    if (go && go->GetGoState() == GO_STATE_ACTIVE)
        return;
    bot->InterruptNonMeleeSpells(false);
}

void FinishCommanderTask(CommanderTask& task, bool ok, uint32 count, std::string const& payload,
    std::string const& code, std::string const& message)
{
    if (task.ritualId)
        StopRitualChannel(task);
    ReleaseTaskStance(task);
    if (task.ritualId)
    {
        // A member ended on its own (a later order, a released binding, a lost bot): the ritual order ends with it.
        FailRitualGroup(task.ritualId, task.botRaw, code, message);
        return;
    }
    if (task.enter)
    {
        NoteEnterResult(task, ok, code, message);
        return;
    }
    if (ok)
        TaskReplyAck(task, count, payload);
    else
        TaskReplyErr(task, code, message);
}

// The walk: the bot holds ("stay") at a point just short of the object and
// walks to it on its own path finding, like `cmd move`.
void StartTaskStayWalk(CommanderTask& task, Player* commander, Player* bot, PlayerbotAI* ai)
{
    ai->DropQueuedMovementOrders();
    PlayerConvenience::Instance().NoteHoldOrder(bot->GetObjectGuid());
    ai->Reset();
    ai->SetMovementStrategy("stay");
    SetStayAnchor(ai, task.stand.x, task.stand.y, task.stand.z, bot->GetMapId());
    ExecuteQuietAction(ai, "return to stay position", ai::Event("return to stay position", "stay", commander));
    task.walking = true;
}

void BeginTaskWalk(CommanderTask& task, Player* commander, Player* bot, PlayerbotAI* ai, WorldObject* object)
{
    task.wasStaying = ai->HasStrategy("stay", BotState::BOT_STATE_NON_COMBAT);
    task.stand = RemoteCommander::StandingPoint(bot->GetPositionX(), bot->GetPositionY(),
        object->GetPositionX(), object->GetPositionY(), object->GetPositionZ());
    StartTaskStayWalk(task, commander, bot, ai);
}

// cmd enter: the walk goes to the middle of the trigger zone. The bot holds there (a follower
// keeps holding while it waits for the lead bot), so it does not trail the lead bot into the dungeon.
void BeginEnterWalk(CommanderTask& task, Player* commander, Player* bot, PlayerbotAI* ai,
    AreaTriggerEntry const* trigger)
{
    task.wasStaying = ai->HasStrategy("stay", BotState::BOT_STATE_NON_COMBAT);
    task.stand = RemoteCommander::Point3{ trigger->x, trigger->y, trigger->z };
    StartTaskStayWalk(task, commander, bot, ai);
}

// A bot that stands still short of the object takes a direct path step to
// the standing point (the stay action stops within its follow range).
void NudgeTaskWalk(CommanderTask& task, Player* bot, uint32 diff)
{
    task.nudgeMs += diff;
    if (task.nudgeMs < RemoteCommander::kNudgeMs)
        return;
    task.nudgeMs = 0;
    if (sServerFacade.isMoving(bot))
        return;
    bot->GetMotionMaster()->MovePoint(0, task.stand.x, task.stand.y, task.stand.z,
        MOVE_RUN_MODE | MOVE_PATHFINDING);
}

// The act hold (RemoteCommander::HoldsForAct): before each attempt to start the
// action, the bot stops where it stands and holds there. A bot that walked
// already holds ("stay") at its standing point, which the act may begin a few
// yards short of; one that stood in range from the start was still following.
// Its stay moves to the spot it stands on, its movement generator is dropped
// and a spell of its own is interrupted (the order beats the automation). The
// stance comes back with the task's end (EndTaskWalk: finish, cancel, timeout).
// Returns true when the bot was still moving (RemoteCommander::ActWaitsForStop):
// it is stopped now, and the caller starts the cast on a later look.
bool HoldForAct(CommanderTask& task, PlayerbotAI* ai, Player* bot)
{
    if (!RemoteCommander::HoldsForAct(task.targetRaw != 0, task.noWalk))
        return false;
    if (!task.walking && !task.superseded)
    {
        task.wasStaying = ai->HasStrategy("stay", BotState::BOT_STATE_NON_COMBAT);
        PlayerConvenience::Instance().NoteHoldOrder(bot->GetObjectGuid());
        ai->SetMovementStrategy("stay");
        task.walking = true;
    }
    ai->DropQueuedMovementOrders();
    SetStayAnchor(ai, bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), bot->GetMapId());
    // Feared, confused, charmed or stunned: the core owns its movement; leave it
    // (the start is refused and retried, or the task ends).
    if (bot->HasUnitState(UNIT_STAT_CAN_NOT_REACT_OR_LOST_CONTROL))
        return false;
    if (bot->IsNonMeleeSpellCasted(false))
        bot->InterruptNonMeleeSpells(false);
    // Read before the clear: the spline keeps running after it either way.
    bool const moving = RemoteCommander::ActWaitsForStop(sServerFacade.isMoving(bot),
        !bot->movespline->Finalized());
    bot->GetMotionMaster()->Clear();
    bot->GetMotionMaster()->MoveIdle();
    if (moving)
        ai->StopMoving();
    return moving;
}

// While the started cast runs: a movement generator something queued since
// (a reaction, a convenience order) is dropped before it takes a step that
// would break the cast. The bot's AI itself sleeps through its own cast.
void KeepStillDuringAct(CommanderTask const& task, Player* bot)
{
    if (!RemoteCommander::HoldsForAct(task.targetRaw != 0, task.noWalk) || !task.actStarted || !task.actSpellId)
        return;
    Spell* spell = bot->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!spell || !spell->m_spellInfo || spell->m_spellInfo->Id != task.actSpellId)
        return;
    if (bot->HasUnitState(UNIT_STAT_CAN_NOT_REACT_OR_LOST_CONTROL))
        return;
    if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
    {
        bot->GetMotionMaster()->Clear();
        bot->GetMotionMaster()->MoveIdle();
    }
}

enum class ActStart : uint8
{
    Started,
    Retry,  // not now (a cast or range check refused); try again on the next look
    Failed, // final: lastCode/lastMessage say why
};

ActStart RefuseAct(CommanderTask& task, ActStart how, char const* code, std::string const& message)
{
    task.lastCode = code;
    task.lastMessage = message;
    return how;
}

// The item's first on-use spell (0 when it has none).
uint32 ItemUseSpell(ItemPrototype const* proto)
{
    if (!proto)
        return 0;
    for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        if (proto->Spells[i].SpellId &&
            (proto->Spells[i].SpellTrigger == ITEM_SPELLTRIGGER_ON_USE ||
             proto->Spells[i].SpellTrigger == ITEM_SPELLTRIGGER_ON_NO_DELAY_USE))
            return proto->Spells[i].SpellId;
    return 0;
}

// The action of a loot-like task, once the bot stands in range.
ActStart BeginLootAct(CommanderTask& task, Player* commander, Player* bot, PlayerbotAI* ai, TaskTarget const& target)
{
    ObjectGuid const guid(task.targetRaw);
    ai::LootObject lootObject(bot, guid);
    if (lootObject.IsEmpty())
        return RefuseAct(task, ActStart::Failed, RemoteCommander::kCodeFailed, target.creature
            ? "This corpse has nothing this bot may loot under the group's loot rules (another member's turn, already looted, or not tapped by the group)."
            : "The object cannot be looted right now (already open, used up, or on the bot's skip list).");

    ai::LootObjectStack* stack = ai->GetAiObjectContext()->GetValue<ai::LootObjectStack*>("available loot")->Get();
    if (target.go)
    {
        RemoteCommander::LockVerdict const lock = RemoteCommander::CheckLock(GoLockFacts(bot, target.go));
        if (lock != RemoteCommander::LockVerdict::Open)
            return RefuseAct(task, ActStart::Failed, RemoteCommander::LockVerdictCode(lock),
                RemoteCommander::LockVerdictText(lock));
        if (!lootObject.IsLootPossible(bot))
            return RefuseAct(task, ActStart::Failed, RemoteCommander::kCodeFailed,
                "Nothing in it for this bot (quest-only contents, bags full, or the object is in use).");
    }
    else if (!target.creature->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE))
        return RefuseAct(task, ActStart::Failed, RemoteCommander::kCodeFailed, "Nothing is left to loot on this corpse.");

    if (bot->IsMounted())
        ai->Unmount();
    RemoteCommander::LootOrders().Watch(task.botRaw, task.targetRaw, task.itemId);
    if (stack)
        stack->NotePlayerOrder(guid);

    if (target.creature)
    {
        // The corpse window straight away: an explicit order, so the strategy's
        // "is it worth the bags" filter (IsLootPossible) does not apply; the
        // loot rights were checked by the loot object above.
        WorldPacket packet(CMSG_LOOT, 8);
        packet << guid;
        bot->GetSession()->HandleLootOpcode(packet);
        return ActStart::Started;
    }

    // A chest or node opens through the bot's own opening code (its lock
    // spell, a key it carries); the window comes back through the loot window
    // handler either way. The order runs "open loot" itself: through the engine
    // a movement action is useless while the bot holds "stay", the stance the
    // walk and the act hold put it in, so every chest order ended before a cast
    // was even tried ("spell check 255", the core's SPELL_CAST_OK).
    ai->ClearLastCastCheckFail();
    ai->GetAiObjectContext()->GetValue<ai::LootObject>("loot target")->Set(lootObject);
    if (ExecuteOrderedAction(ai, "open loot", ai::Event("cmd loot", "", commander)))
        return ActStart::Started;
    RemoteCommander::LootOrders().Forget(task.botRaw);
    // The reason the open did not start: the cast check the bot's CastSpell
    // recorded (its own gates: moving, another cast; or the core's check), else
    // someone else opening or looting it, else the loot range the open loot
    // action keeps.
    static_assert(RemoteCommander::kCastOk == static_cast<uint32>(SPELL_CAST_OK),
        "RemoteCommander::kCastOk must be the core's SPELL_CAST_OK");
    uint32 const castFail = static_cast<uint32>(ai->GetLastCastCheckFail());
    char const* reason = ViewFailReason(castFail);
    if (!reason && castFail == static_cast<uint32>(SPELL_CAST_OK) &&
        (target.go->getLootState() != GO_READY || target.go->HasFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE)))
        reason = "in-use";
    if (!reason && castFail == static_cast<uint32>(SPELL_CAST_OK) && !lootObject.IsInLootRange(bot))
        reason = "range";
    std::string const why = reason ? reason : "";
    return RefuseAct(task, ActStart::Retry, RemoteCommander::CastReasonCode(why),
        RemoteCommander::OpenRefusalText(why, castFail));
}

// A game object that is not a chest: use it like a player would.
ActStart BeginObjectAct(CommanderTask& task, Player* commander, Player* bot, PlayerbotAI* ai, TaskTarget const& target)
{
    RemoteCommander::LockVerdict const lock = RemoteCommander::CheckLock(GoLockFacts(bot, target.go));
    if (lock != RemoteCommander::LockVerdict::Open)
        return RefuseAct(task, ActStart::Failed, RemoteCommander::LockVerdictCode(lock),
            RemoteCommander::LockVerdictText(lock));
    if (bot->IsMounted())
        ai->Unmount();
    task.baseline = PackGoState(target.go);
    ai->ClearLastCastCheckFail();
    std::string const param = "0 go " + std::to_string(task.targetRaw);
    if (ExecuteQuietAction(ai, "commander use", ai::Event("cmd open", param, commander)))
        return ActStart::Started;
    return RefuseAct(task, ActStart::Failed, RemoteCommander::kCodeFailed,
        "The bot could not use it (already used, busy, or out of reach).");
}

ActStart BeginUseAct(CommanderTask& task, Player* commander, Player* bot, PlayerbotAI* ai)
{
    std::string kind = "none";
    if (task.useTargetRaw)
        kind = ObjectGuid(task.useTargetRaw).IsGameObject() ? "go" : "unit";
    ai->ClearLastCastCheckFail();
    std::string const param = std::to_string(task.itemId) + " " + kind + " " + std::to_string(task.useTargetRaw);
    if (ExecuteQuietAction(ai, "commander use", ai::Event("cmd use", param, commander)))
        return ActStart::Started;
    uint32 const castFail = static_cast<uint32>(ai->GetLastCastCheckFail());
    if (castFail != static_cast<uint32>(SPELL_CAST_OK))
    {
        char const* reason = ViewFailReason(castFail);
        return RefuseAct(task, ActStart::Failed, RemoteCommander::CastReasonCode(reason ? reason : ""),
            "The cast failed (spell check " + std::to_string(castFail) + ").");
    }
    return RefuseAct(task, ActStart::Failed, RemoteCommander::kCodeFailed,
        "The bot could not use the item (cooldown, not usable by it, or not usable on that target).");
}

ActStart BeginTaskAct(CommanderTask& task, Player* commander, Player* bot, PlayerbotAI* ai, TaskTarget const& target)
{
    task.actStarted = false;
    ++task.actTries;
    ActStart start;
    // Never start the cast on a bot that is still moving: the hold stopped it,
    // the retry on the next look starts the cast.
    if (HoldForAct(task, ai, bot))
        start = RefuseAct(task, ActStart::Retry, RemoteCommander::CastReasonCode("moving"),
            RemoteCommander::OpenRefusalText("moving", 0));
    else if (task.lootLike)
        start = BeginLootAct(task, commander, bot, ai, target);
    else if (task.state.kind == RemoteCommander::TaskKind::Open)
        start = BeginObjectAct(task, commander, bot, ai, target);
    else
        start = BeginUseAct(task, commander, bot, ai);
    task.actStarted = start == ActStart::Started;
    task.actSpellId = 0;
    // A cast with a cast time (the 5 s Opening of a locked chest): the act stage
    // waits for it to run out before it waits for the result, and the hold keeps
    // the bot still while that spell (only that one) is being cast.
    task.actCastMs = 0;
    task.actRunMs = 0;
    if (task.actStarted)
        if (Spell* spell = bot->GetCurrentSpell(CURRENT_GENERIC_SPELL))
        {
            task.actSpellId = spell->m_spellInfo ? spell->m_spellInfo->Id : 0;
            task.actCastMs = spell->GetCastedTime();
            task.actFrom = RemoteCommander::Point3{ bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ() };
            task.state.actBudgetMs = RemoteCommander::ActBudgetAfterCast(task.state, task.lootLike,
                spell->GetCastedTime());
        }
    return start;
}

// How a used game object stands against the state it had when the bot used it.
RemoteCommander::ObjectEffect ObjectEffectOf(CommanderTask const& task, PlayerbotAI* ai)
{
    GameObject* go = ai->GetGameObject(ObjectGuid(task.targetRaw));
    bool const present = go && sServerFacade.isSpawned(go);
    return RemoteCommander::JudgeObjectEffect(present, task.baseline, present ? PackGoState(go) : 0);
}

// The result of the started action is in.
bool TaskActDone(CommanderTask const& task, Player* bot, PlayerbotAI* ai)
{
    if (!task.actStarted)
        return false;
    if (task.lootLike)
    {
        RemoteCommander::LootWatch const* watch = RemoteCommander::LootOrders().Find(task.botRaw);
        return watch && watch->finished;
    }
    if (task.state.kind == RemoteCommander::TaskKind::Open)
        return ObjectEffectOf(task, ai) == RemoteCommander::ObjectEffect::Reacted;
    return !bot->IsNonMeleeSpellCasted(false);
}

// The success answer of a finished action.
void CompleteCommanderTask(CommanderTask& task)
{
    if (task.lootLike)
    {
        RemoteCommander::LootWatch const* watch = RemoteCommander::LootOrders().Find(task.botRaw);
        if (!watch)
        {
            FinishCommanderTask(task, false, 0, "", RemoteCommander::kCodeFailed, "The loot window was lost.");
            return;
        }
        if (watch->refused)
        {
            FinishCommanderTask(task, false, 0, "", RemoteCommander::kCodeFailed,
                "The server refused the loot window (not this bot's loot under the group rules, or too far).");
            return;
        }
        uint32 const count = RemoteCommander::LootedItemCount(*watch);
        std::string const payload = RemoteCommander::FormatLootedItems(*watch);
        if (task.itemId && !count)
        {
            FinishCommanderTask(task, false, 0, "", RemoteCommander::kCodeFailed,
                "That item was not in the loot for this bot (group rules, taken, or no room in its bags).");
            return;
        }
        FinishCommanderTask(task, true, count, payload, "", "");
        return;
    }
    if (task.state.kind == RemoteCommander::TaskKind::Open)
    {
        FinishCommanderTask(task, true, 1, "opened", "", "");
        return;
    }
    FinishCommanderTask(task, true, 1, task.spellId ? "spell:" + std::to_string(task.spellId) : "-", "", "");
}

// ---------------------------------------------------------------------------
// cmd enter: through a dungeon entrance
//
// A headless bot never sends its own CMSG_AREATRIGGER, so the order does it for the bot:
// the bot walks into the trigger zone (the task walk) and the glue then hands the bot's
// session the same packet a client sends, WorldSession::HandleAreaTriggerOpcode, which
// runs the core's own entry path (the trigger's phase, level and condition, the raid group,
// the per-account instance limit in MapManager::CanPlayerEnter, Player::TeleportTo with
// the instance and group binds, Map::CanEnter). Nothing is skipped or reimplemented
// there; JudgeEntry below only names the reason of a refusal. The lead bot goes first
// (it creates the group's instance bind); the followers hold at the trigger until it
// stands in the dungeon.
// ---------------------------------------------------------------------------

// What the core's entry path asks of this bot at this trigger, for the readable reason of a refusal.
RemoteCommander::EntryFacts GatherEntryFacts(Player* bot, AreaTriggerTeleport const* tele, MapEntry const* targetMap)
{
    RemoteCommander::EntryFacts facts;
    facts.gm = bot->IsGameMaster();
    facts.requiredPhase = tele->requiredPhase;
    facts.phaseOpen = tele->requiredPhase <= sWorld.GetContentPhase();
    facts.level = bot->GetLevel();
    facts.requiredLevel = tele->requiredLevel;
    facts.ignoreLevel = sWorld.getConfig(CONFIG_BOOL_INSTANCE_IGNORE_LEVEL);
    // The maps where a Lunatic challenger enters below the level (the same list as the core's handler).
    static constexpr uint32 kLunaticMaps[] = { 36, 43, 389, 822 };
    for (uint32 mapId : kLunaticMaps)
        if (targetMap->id == mapId && bot->HasChallenge(CHALLENGE_LUNATIC))
            facts.lunaticOverride = true;
    facts.conditionFailed = tele->requiredCondition && bot->GetMap() &&
        !sObjectMgr.IsConditionSatisfied(tele->requiredCondition, bot, bot->GetMap(), bot, CONDITION_FROM_AREATRIGGER);
    facts.conditionMessage = tele->message;
    facts.raidMap = targetMap->IsDungeon() && targetMap->IsRaid();
    facts.ignoreRaid = sWorld.getConfig(CONFIG_BOOL_INSTANCE_IGNORE_RAID);
    Group* group = bot->GetGroup();
    facts.inRaidGroup = group && group->isRaidGroup();
    auto* bound = bot->GetBoundInstanceSaveForSelfOrGroup(targetMap->id);
    facts.instanceCountOk = bot->CheckInstanceCount(bound ? bound->GetInstanceId() : 0);
    return facts;
}

// The entrance a trigger leads through, or null when it is not an instance entrance.
AreaTriggerTeleport const* InstanceEntranceOf(AreaTriggerEntry const* trigger, MapEntry const** targetMap)
{
    if (!trigger)
        return nullptr;
    AreaTriggerTeleport const* tele = sObjectMgr.GetAreaTriggerTeleport(trigger->id);
    MapEntry const* map = tele ? sMapStorage.LookupEntry<MapEntry>(tele->destination.mapId) : nullptr;
    if (!RemoteCommander::IsInstanceEntrance(tele != nullptr, map && map->IsDungeon(), trigger->mapid,
            tele ? tele->destination.mapId : 0))
        return nullptr;
    if (targetMap)
        *targetMap = map;
    return tele;
}

// The walk is over and the lead bot is in (or this is the lead bot): send the core the
// area trigger. Returns true when the task ended on the spot (a refusal).
bool SendEnterTrigger(CommanderTask& task, Player* bot, PlayerbotAI* ai, AreaTriggerTeleport const* tele,
    MapEntry const* targetMap)
{
    RemoteCommander::EntryVerdict const verdict = RemoteCommander::JudgeEntry(GatherEntryFacts(bot, tele, targetMap));
    if (!verdict.ok)
    {
        FinishCommanderTask(task, false, 0, "", verdict.code, verdict.message);
        return true;
    }
    ai->DropQueuedMovementOrders();
    WorldPacket packet(CMSG_AREATRIGGER);
    packet << task.triggerId;
    packet.rpos(0);
    bot->GetSession()->HandleAreaTriggerOpcode(packet);
    if (bot->IsBeingTeleported())
        return false;
    // The core refused: say why when the reason can be read now (the bot's state may have
    // changed with the attempt), else name the checks that only the core sees.
    RemoteCommander::EntryVerdict const again = RemoteCommander::JudgeEntry(GatherEntryFacts(bot, tele, targetMap));
    if (!again.ok)
        FinishCommanderTask(task, false, 0, "", again.code, again.message);
    else
        FinishCommanderTask(task, false, 0, "", RemoteCommander::kCodeFailed,
            "The server refused the entry (the instance is full or locked by an encounter, the bot stands outside the "
            "zone, or a script said no).");
    return true;
}

// One look at one bot's way in. Returns true when the task is over.
bool StepEnterTask(CommanderTask& task, Player* bot, PlayerbotAI* ai, uint32 diff)
{
    auto& orders = EnterOrders();
    auto orderIt = orders.find(task.enterOrder);
    AreaTriggerEntry const* trigger = sObjectMgr.GetAreaTrigger(task.triggerId);
    MapEntry const* targetMap = nullptr;
    AreaTriggerTeleport const* tele = InstanceEntranceOf(trigger, &targetMap);
    if (!bot)
    {
        FinishCommanderTask(task, false, 0, "", RemoteCommander::kCodeUnavailable, "The bot logged out.");
        return true;
    }
    if (orderIt == orders.end() || !tele || !targetMap)
    {
        FinishCommanderTask(task, false, 0, "", RemoteCommander::kCodeFailed, "The entrance or the order is gone.");
        return true;
    }

    RemoteCommander::EnterFacts facts;
    facts.available = ai && bot->IsAlive() && !bot->IsBeingTeleported() && !bot->IsTaxiFlying();
    facts.inTransit = bot->IsAlive() && (!bot->IsInWorld() || bot->IsBeingTeleported());
    facts.arrived = bot->IsInWorld() && !bot->IsBeingTeleported() && bot->GetMapId() == tele->destination.mapId;
    facts.inCombat = bot->IsInCombat();
    facts.inRange = bot->IsInWorld() && IsPointInAreaTriggerZone(trigger, bot->GetMapId(), bot->GetPositionX(),
        bot->GetPositionY(), bot->GetPositionZ(), RemoteCommander::kEnterTriggerMargin);
    facts.distance = bot->GetDistance(trigger->x, trigger->y, trigger->z, SizeFactor::None);
    facts.leadEntered = orderIt->second.tally.leadEntered;
    facts.leadFailed = orderIt->second.tally.leadFailed;

    RemoteCommander::EnterVerdict const verdict = RemoteCommander::AdvanceEnter(task.enterState, facts, diff);
    switch (verdict.step)
    {
        case RemoteCommander::EnterStep::Wait:
            if (facts.available && task.enterState.stage == RemoteCommander::EnterStage::Walk)
                NudgeTaskWalk(task, bot, diff);
            return false;
        case RemoteCommander::EnterStep::SendTrigger:
            return SendEnterTrigger(task, bot, ai, tele, targetMap);
        case RemoteCommander::EnterStep::Succeed:
            FinishCommanderTask(task, true, 1, "entered", "", "");
            return true;
        case RemoteCommander::EnterStep::Fail:
            FinishCommanderTask(task, false, 0, "", verdict.code, verdict.message);
            return true;
    }
    return false;
}

// One look at a running task. Returns true when it is over (answered).
bool StepCommanderTask(CommanderTask& task, uint32 diff)
{
    // A ritual member is stepped by its group (StepRitualGroups), which sees all the bots at once.
    if (task.ritualId)
        return false;
    Player* commander = FindCharacter(task.commanderRaw);
    Player* bot = FindCharacter(task.botRaw);
    PlayerbotAI* ai = bot && bot->IsInWorld() ? PlayerbotAIStorage::Instance().GetAI(bot) : nullptr;
    if (!commander || !commander->IsInWorld())
    {
        FinishCommanderTask(task, false, 0, "", RemoteCommander::kCodeFailed, "The commander left.");
        return true;
    }

    // A follower that left the commanded party is not ours to order any more.
    if (bot && !IsCommandedBy(commander, bot))
    {
        task.departed = true;
        FinishCommanderTask(task, false, 0, "", RemoteCommander::kCodeUnavailable,
            "The bot left the commanded party.");
        return true;
    }

    if (task.enter)
        return StepEnterTask(task, bot, ai, diff);

    RemoteCommander::TaskFacts facts;
    facts.botAvailable = ai && bot->IsAlive() && !bot->IsBeingTeleported() && !bot->IsTaxiFlying();
    facts.teleporting = bot && bot->IsAlive() && (!bot->IsInWorld() || bot->IsBeingTeleported());
    facts.actStarted = task.actStarted;
    TaskTarget target;
    if (facts.botAvailable)
    {
        if (task.targetRaw)
            target = ResolveTaskTarget(ai, task.targetRaw);
        facts.targetPresent = !task.targetRaw || TaskTargetPresent(target);
        facts.inCombat = bot->IsInCombat();
        facts.inRange = TaskInRange(task, bot, target);
        if (WorldObject* object = target.object())
            facts.distance = bot->GetDistance(object);
        facts.actDone = task.state.stage == RemoteCommander::TaskStage::Act && TaskActDone(task, bot, ai);
    }

    if (facts.botAvailable && task.state.stage == RemoteCommander::TaskStage::Act)
        KeepStillDuringAct(task, bot);

    // The held act's cast ended before its time without a result: the core
    // cancelled it. Answer now and say what was seen (a step, a fight).
    if (facts.botAvailable && task.state.stage == RemoteCommander::TaskStage::Act && task.actStarted &&
        task.actSpellId && task.actCastMs && !facts.actDone &&
        RemoteCommander::HoldsForAct(task.targetRaw != 0, task.noWalk))
    {
        task.actRunMs += diff;
        Spell* spell = bot->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        bool const running = spell && spell->m_spellInfo && spell->m_spellInfo->Id == task.actSpellId;
        if (RemoteCommander::ActCastBroken(running, task.actCastMs, task.actRunMs))
        {
            float const moved = bot->GetDistance(task.actFrom.x, task.actFrom.y, task.actFrom.z, SizeFactor::None);
            FinishCommanderTask(task, false, 0, "", RemoteCommander::kCodeFailed,
                RemoteCommander::ActCastBrokenText(task.actRunMs, moved, bot->IsInCombat()));
            return true;
        }
    }

    // A used object that vanished (despawn, map change) is no "opened".
    if (facts.botAvailable && task.state.stage == RemoteCommander::TaskStage::Act && task.actStarted &&
        task.state.kind == RemoteCommander::TaskKind::Open && !task.lootLike &&
        ObjectEffectOf(task, ai) == RemoteCommander::ObjectEffect::Gone)
    {
        FinishCommanderTask(task, false, 0, "", RemoteCommander::kCodeFailed,
            "The object disappeared while the bot used it.");
        return true;
    }

    // A refused attempt that may pass the next look.
    if (RemoteCommander::MayRetryAct(task.state, facts) && !task.actStarted && task.actTries > 0)
    {
        if (task.actTries >= RemoteCommander::kActTries)
        {
            FinishCommanderTask(task, false, 0, "", task.lastCode, task.lastMessage);
            return true;
        }
        if (BeginTaskAct(task, commander, bot, ai, target) == ActStart::Failed)
        {
            FinishCommanderTask(task, false, 0, "", task.lastCode, task.lastMessage);
            return true;
        }
    }

    RemoteCommander::TaskVerdict verdict = RemoteCommander::AdvanceTask(task.state, facts, diff);
    switch (verdict.step)
    {
        case RemoteCommander::TaskStep::Wait:
            if (facts.botAvailable && task.state.stage == RemoteCommander::TaskStage::Walk && !task.noWalk)
                NudgeTaskWalk(task, bot, diff);
            return false;
        case RemoteCommander::TaskStep::BeginAct:
        {
            ActStart const start = BeginTaskAct(task, commander, bot, ai, target);
            if (start == ActStart::Failed)
            {
                FinishCommanderTask(task, false, 0, "", task.lastCode, task.lastMessage);
                return true;
            }
            // An action that is over at once (an instant item) answers on the spot.
            if (start == ActStart::Started && TaskActDone(task, bot, ai))
            {
                CompleteCommanderTask(task);
                return true;
            }
            return false;
        }
        case RemoteCommander::TaskStep::Succeed:
            CompleteCommanderTask(task);
            return true;
        case RemoteCommander::TaskStep::Fail:
        {
            std::string code = verdict.code;
            std::string message = verdict.message;
            if (std::strcmp(verdict.code, RemoteCommander::kCodeTimeout) == 0)
            {
                if (task.state.stage == RemoteCommander::TaskStage::Act && !task.lastCode.empty() && !task.actStarted)
                {
                    code = task.lastCode;
                    message = task.lastMessage;
                }
                else if (task.state.kind == RemoteCommander::TaskKind::Open && !task.lootLike &&
                    task.state.stage == RemoteCommander::TaskStage::Act)
                {
                    code = RemoteCommander::kCodeFailed;
                    message = "The object showed no change after the bot used it.";
                }
            }
            FinishCommanderTask(task, false, 0, "", code, message);
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Summoning rituals (`cmd ritual`): the group's look
// ---------------------------------------------------------------------------

// The ritual object, seen from the map of any member that stands in a world.
GameObject* RitualObject(RitualGroup const& group)
{
    ObjectGuid const guid(group.targetRaw);
    for (uint64 raw : group.members)
    {
        Player* bot = FindCharacter(raw);
        if (bot && bot->IsInWorld() && bot->GetMap())
            return bot->GetMap()->GetGameObject(guid);
    }
    return nullptr;
}

// A bot that already used the object stays where it is: a movement generator something queued since is
// dropped. Its spells are left alone here: the helper channel is the participation itself, and the bot's own AI
// is held for it (RemoteCommander::RitualHolds), so nothing else casts.
void KeepRitualStill(PlayerbotAI* ai, Player* bot)
{
    ai->DropQueuedMovementOrders();
    if (bot->HasUnitState(UNIT_STAT_CAN_NOT_REACT_OR_LOST_CONTROL))
        return;
    if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
    {
        bot->GetMotionMaster()->Clear();
        bot->GetMotionMaster()->MoveIdle();
    }
}

void EndRitualMembers(RitualGroup const& group, uint64 exceptBot, bool stopChannels)
{
    auto& tasks = CommanderTasks();
    for (uint64 raw : group.members)
    {
        if (raw == exceptBot)
            continue;
        auto it = tasks.find(raw);
        if (it == tasks.end() || it->second.ritualId != group.id)
            continue;
        CommanderTask task = it->second;
        tasks.erase(it);
        if (stopChannels)
            StopRitualChannel(task);
        ReleaseTaskStance(task);
    }
}

void RitualReply(RitualGroup const& group, bool ok, uint32 count, std::string const& code, std::string const& message)
{
    Player* commander = FindCharacter(group.commanderRaw);
    if (!commander || !commander->IsInWorld() || !commander->GetSession())
        return;
    ChatHandler handler(commander);
    if (ok)
        SendActionAck(&handler, group.intent, "party", count, "done");
    else
        SendActionError(&handler, group.intent, code.c_str(), message.c_str());
}

void FailRitualGroup(uint32 id, uint64 exceptBot, std::string const& code, std::string const& message)
{
    auto& groups = RitualGroups();
    auto it = groups.find(id);
    if (it == groups.end())
        return;
    RitualGroup const group = it->second;
    groups.erase(it);
    EndRitualMembers(group, exceptBot, true);
    RitualReply(group, false, 0, code, message);
}

// The use of the member whose turn it is: the bot's own "use game object" code, which sends the
// same CMSG_GAMEOBJ_USE a right click does. True when the bot accepted it (it is not yet a participant).
bool SendRitualUse(RitualGroup& group, Player* commander)
{
    auto& tasks = CommanderTasks();
    auto it = tasks.find(group.members[group.state.nextUse]);
    if (it == tasks.end())
        return false;
    CommanderTask& task = it->second;
    Player* bot = FindCharacter(task.botRaw);
    PlayerbotAI* ai = bot ? PlayerbotAIStorage::Instance().GetAI(bot) : nullptr;
    if (!ai)
        return false;
    if (bot->IsMounted())
        ai->Unmount();
    ai->ClearLastCastCheckFail();
    std::string const param = "0 go " + std::to_string(group.targetRaw);
    task.actStarted = ExecuteQuietAction(ai, "commander use", ai::Event("cmd open", param, commander));
    // From its use on the bot does nothing of its own (RitualHolds): the channel is the participation.
    if (task.actStarted)
        RemoteCommander::RitualHolds().Hold(task.botRaw);
    return task.actStarted;
}

// One look at a ritual order. True when it is over (answered, the members let go).
bool StepRitualGroup(RitualGroup& group, uint32 diff)
{
    Player* commander = FindCharacter(group.commanderRaw);
    if (!commander || !commander->IsInWorld())
    {
        FailRitualGroup(group.id, 0, RemoteCommander::kCodeFailed, "The commander left.");
        return true;
    }

    auto& tasks = CommanderTasks();
    GameObject* go = RitualObject(group);
    RemoteCommander::RitualFacts facts;
    facts.targetPresent = go && sServerFacade.isSpawned(go);
    facts.goActive = go && go->GetGoState() == GO_STATE_ACTIVE;
    if (go)
        facts.uniqueUsers = go->GetUniqueUseCount();

    bool allReady = true;
    for (uint64 raw : group.members)
    {
        auto it = tasks.find(raw);
        Player* bot = FindCharacter(raw);
        PlayerbotAI* ai = bot && bot->IsInWorld() ? PlayerbotAIStorage::Instance().GetAI(bot) : nullptr;
        if (it == tasks.end() || it->second.ritualId != group.id || !ai || !bot->IsAlive() ||
            bot->IsBeingTeleported() || bot->IsTaxiFlying() || (go && bot->GetMapId() != go->GetMapId()))
        {
            facts.unavailable = true;
            allReady = false;
            continue;
        }
        CommanderTask& task = it->second;
        if (!IsCommandedBy(commander, bot))
        {
            task.departed = true;
            facts.unavailable = true;
            allReady = false;
            continue;
        }
        if (bot->IsInCombat())
            facts.inCombat = true;

        if (task.state.stage == RemoteCommander::TaskStage::Walk)
        {
            RemoteCommander::TaskFacts walk;
            walk.targetPresent = facts.targetPresent;
            walk.inCombat = bot->IsInCombat();
            walk.distance = go ? bot->GetDistance(go) : 0.0f;
            walk.inRange = go && RemoteCommander::InUseRange(walk.distance, INTERACTION_DISTANCE);
            RemoteCommander::TaskVerdict const verdict = RemoteCommander::AdvanceTask(task.state, walk, diff);
            if (verdict.step == RemoteCommander::TaskStep::Fail && !facts.walkFailCode)
            {
                facts.walkFailCode = verdict.code;
                facts.walkFailMessage = verdict.message;
            }
            else if (verdict.step == RemoteCommander::TaskStep::BeginAct)
                task.ready = true;
            else if (verdict.step == RemoteCommander::TaskStep::Wait)
                NudgeTaskWalk(task, bot, diff);
        }
        if (!task.ready)
        {
            allReady = false;
            continue;
        }
        // Standing in range: hold still. A bot that has not used the object yet is held hard (its own
        // spell and movement dropped); one that has is the participant, its channel is left alone.
        if (task.actStarted)
            KeepRitualStill(ai, bot);
        else
            HoldForAct(task, ai, bot);
    }
    facts.allReady = allReady && !facts.unavailable;

    if (go)
    {
        for (uint64 raw : group.members)
            if (Player* bot = FindCharacter(raw))
                if (go->HasUniqueUser(bot))
                    ++facts.membersJoined;
        if (group.state.nextUse < group.members.size())
            if (Player* next = FindCharacter(group.members[group.state.nextUse]))
            {
                facts.nextInRange = RemoteCommander::InUseRange(next->GetDistance(go), INTERACTION_DISTANCE);
                facts.nextJoined = go->HasUniqueUser(next);
            }
    }

    RemoteCommander::RitualVerdict verdict = RemoteCommander::AdvanceRitual(group.state, facts, diff);
    if (verdict.step == RemoteCommander::RitualStep::UseNext && !SendRitualUse(group, commander))
        verdict = RemoteCommander::RitualUseRefused(group.state);
    switch (verdict.step)
    {
        case RemoteCommander::RitualStep::Wait:
        case RemoteCommander::RitualStep::UseNext:
            return false;
        case RemoteCommander::RitualStep::Succeed:
        {
            RitualGroup const done = group;
            RitualGroups().erase(done.id);
            EndRitualMembers(done, 0, false);
            RitualReply(done, true, done.state.required, "", "");
            return true;
        }
        case RemoteCommander::RitualStep::Fail:
            FailRitualGroup(group.id, 0, verdict.code, verdict.message);
            return true;
    }
    return false;
}

void StepRitualGroups(uint32 diff)
{
    std::vector<uint32> ids;
    for (auto const& entry : RitualGroups())
        ids.push_back(entry.first);
    for (uint32 id : ids)
    {
        auto it = RitualGroups().find(id);
        if (it != RitualGroups().end())
            StepRitualGroup(it->second, diff);
    }
}

// A later order cancels the commander's running tasks (of one bot, or all). Only
// an order that sets the bot's movement itself skips the walk's end; after any
// other the bot re-forms as after a finished walk.
void SupersedeCommanderTasks(uint64 commanderRaw, uint64 onlyBotRaw, RemoteCommander::LaterOrder later)
{
    // A later order replaces an attack still waiting for the bot to get control back.
    DropHeldAttacks(commanderRaw, onlyBotRaw);
    // The keys first: finishing a ritual member ends the other members of its group too.
    auto& tasks = CommanderTasks();
    std::vector<uint64> keys;
    for (auto const& entry : tasks)
        if (entry.second.commanderRaw == commanderRaw && (!onlyBotRaw || entry.second.botRaw == onlyBotRaw))
            keys.push_back(entry.first);
    for (uint64 key : keys)
    {
        auto it = tasks.find(key);
        if (it == tasks.end())
            continue;
        CommanderTask task = it->second;
        tasks.erase(it);
        task.superseded = RemoteCommander::LaterOrderTakesMovement(later);
        FinishCommanderTask(task, false, 0, "", RemoteCommander::kCodeCancelled, "A later order took the bot.");
    }
}

void CancelCommanderTasks(uint64 commanderRaw)
{
    DropHeldAttacks(commanderRaw, 0);
    auto& tasks = CommanderTasks();
    std::vector<uint64> keys;
    for (auto const& entry : tasks)
        if (entry.second.commanderRaw == commanderRaw)
            keys.push_back(entry.first);
    for (uint64 key : keys)
    {
        auto it = tasks.find(key);
        if (it == tasks.end())
            continue;
        CommanderTask task = it->second;
        tasks.erase(it);
        FinishCommanderTask(task, false, 0, "", RemoteCommander::kCodeFailed, "The commanded group was released.");
    }
}

// Each tick while a task runs: every kTaskTickMs a look at each.
// The stances of tasks that ended out of the world (PendingStanceRestores).
void RestorePendingStances()
{
    auto& pending = PendingStanceRestores();
    for (auto it = pending.begin(); it != pending.end();)
    {
        Player* bot = FindCharacter(it->botRaw);
        if (bot && (!bot->IsInWorld() || bot->IsBeingTeleported()))
        {
            ++it;
            continue;
        }
        PlayerbotAI* ai = bot ? PlayerbotAIStorage::Instance().GetAI(bot) : nullptr;
        // A newer order owns the bot's movement now; a bot that logged out is gone.
        if (ai && !CommanderTasks().count(it->botRaw))
            EndTaskStance(*it, bot, ai);
        it = pending.erase(it);
    }
}

void UpdateCommanderTasks(uint32 diff)
{
    if (!PendingStanceRestores().empty())
        RestorePendingStances();
    auto& tasks = CommanderTasks();
    if (tasks.empty())
    {
        s_commanderTaskMs = 0;
        return;
    }
    s_commanderTaskMs += diff;
    if (s_commanderTaskMs < RemoteCommander::kTaskTickMs)
        return;
    uint32 const elapsed = s_commanderTaskMs;
    s_commanderTaskMs = 0;
    if (!RitualGroups().empty())
        StepRitualGroups(elapsed);
    for (auto it = tasks.begin(); it != tasks.end();)
    {
        if (StepCommanderTask(it->second, elapsed))
            it = tasks.erase(it);
        else
            ++it;
    }
}

void ClearCommanderTasks()
{
    CommanderTasks().clear();
    RitualGroups().clear();
    RemoteCommander::RitualHolds().Clear();
    EnterOrders().clear();
    PendingStanceRestores().clear();
    RemoteCommander::LootOrders().Clear();
    s_commanderTaskMs = 0;
}

} // namespace

// The guards every per-bot task order shares: the bot lives and is free to act.
static bool CommanderTaskGuard(ChatHandler* handler, std::string const& intent, Player* bot,
    PlayerbotAI* ai, bool walks)
{
    if (!ai)
    {
        SendActionError(handler, intent, "no-ai", "That bot has no AI.");
        return false;
    }
    if (!bot->IsAlive() || bot->IsTaxiFlying() || bot->IsBeingTeleported())
    {
        SendActionError(handler, intent, RemoteCommander::kCodeUnavailable,
            "That bot is dead, on a taxi or teleporting.");
        return false;
    }
    if (CommanderTasks().count(bot->GetObjectGuid().GetRawValue()) ||
        PlayerConvenience::Instance().IsBusy(bot->GetObjectGuid()))
    {
        SendActionError(handler, intent, RemoteCommander::kCodeBusy,
            "That bot is busy with an earlier order (or a pending summon).");
        return false;
    }
    if (walks && bot->IsInCombat())
    {
        SendActionError(handler, intent, RemoteCommander::kCodeInCombat, "That bot is in combat.");
        return false;
    }
    return true;
}

// Takes the task into the table, starts its walk, looks at it once (it may
// finish on the spot: the answer is then synchronous) and marks it pending.
static bool LaunchCommanderTask(ChatHandler* handler, CommanderOrder const& order, Player* bot,
    PlayerbotAI* ai, CommanderTask task)
{
    task.commanderRaw = order.commander->GetObjectGuid().GetRawValue();
    task.botRaw = bot->GetObjectGuid().GetRawValue();
    task.botName = bot->GetName();
    if (!task.noWalk && task.targetRaw)
    {
        TaskTarget const target = ResolveTaskTarget(ai, task.targetRaw);
        if (!TaskInRange(task, bot, target))
            BeginTaskWalk(task, order.commander, bot, ai, target.object());
    }
    auto& tasks = CommanderTasks();
    CommanderTask& stored = tasks[task.botRaw] = task;
    if (StepCommanderTask(stored, 0))
    {
        tasks.erase(task.botRaw);
        return true;
    }
    uint32 const seconds = (stored.state.walkBudgetMs + stored.state.actBudgetMs) / 1000;
    handler->PSendSysMessage("TBM:ACTION_PENDING|%s|bot:%s|%u|%s", ProtocolSafe(stored.intent).c_str(),
        ProtocolSafe(stored.botName).c_str(), seconds, std::to_string(stored.targetRaw).c_str());
    return true;
}

// cmd loot <corpse-or-go-guid | entry:id[@x,y,z]> <bot> [itemId]
static bool CommanderLoot(ChatHandler* handler, CommanderOrder const& order, Player* bot,
    RemoteCommander::LootOrder const& request, std::string const& intent)
{
    PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
    if (!CommanderTaskGuard(handler, intent, bot, ai, true))
        return true;
    uint64 const guid = ResolveOrderGuid(handler, intent, bot, request.spec, true);
    if (!guid)
        return true;
    TaskTarget const target = ResolveTaskTarget(ai, guid);
    if (!TaskTargetPresent(target))
    {
        SendActionError(handler, intent, RemoteCommander::kCodeInvalidTarget,
            "No corpse or game object with that GUID is on the bot's map.");
        return true;
    }
    float const distance = bot->GetDistance(target.object());
    if (!RemoteCommander::WithinReactDistance(distance, sPlayerbotAIConfig.reactDistance))
    {
        SendActionError(handler, intent, RemoteCommander::kCodeTooFar,
            "The object is farther from the bot than AiPlayerbot.ReactDistance. Move the group closer first.");
        return true;
    }
    if (target.go)
    {
        RemoteCommander::LockVerdict const lock = RemoteCommander::CheckLock(GoLockFacts(bot, target.go));
        if (lock != RemoteCommander::LockVerdict::Open)
        {
            SendActionError(handler, intent, RemoteCommander::LockVerdictCode(lock),
                RemoteCommander::LockVerdictText(lock));
            return true;
        }
    }
    CommanderTask task;
    task.state = RemoteCommander::MakeTask(RemoteCommander::TaskKind::Loot, distance, true);
    task.targetRaw = guid;
    task.itemId = request.itemId;
    task.lootLike = true;
    task.intent = intent;
    return LaunchCommanderTask(handler, order, bot, ai, task);
}

// cmd open <go-guid | entry:id[@x,y,z]> <bot>
static bool CommanderOpen(ChatHandler* handler, CommanderOrder const& order, Player* bot,
    RemoteCommander::OpenOrder const& request, std::string const& intent)
{
    PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
    if (!CommanderTaskGuard(handler, intent, bot, ai, true))
        return true;
    uint64 const guid = ResolveOrderGuid(handler, intent, bot, request.spec, false);
    if (!guid)
        return true;
    TaskTarget const target = ResolveTaskTarget(ai, guid);
    if (!target.go || !TaskTargetPresent(target))
    {
        SendActionError(handler, intent, RemoteCommander::kCodeInvalidTarget,
            "No game object with that GUID is on the bot's map.");
        return true;
    }
    float const distance = bot->GetDistance(target.go);
    if (!RemoteCommander::WithinReactDistance(distance, sPlayerbotAIConfig.reactDistance))
    {
        SendActionError(handler, intent, RemoteCommander::kCodeTooFar,
            "The object is farther from the bot than AiPlayerbot.ReactDistance. Move the group closer first.");
        return true;
    }
    RemoteCommander::LockVerdict const lock = RemoteCommander::CheckLock(GoLockFacts(bot, target.go));
    if (lock != RemoteCommander::LockVerdict::Open)
    {
        SendActionError(handler, intent, RemoteCommander::LockVerdictCode(lock),
            RemoteCommander::LockVerdictText(lock));
        return true;
    }
    CommanderTask task;
    task.lootLike = target.go->GetGoType() == GAMEOBJECT_TYPE_CHEST;
    task.state = RemoteCommander::MakeTask(RemoteCommander::TaskKind::Open, distance, true, task.lootLike);
    task.targetRaw = guid;
    task.intent = intent;
    return LaunchCommanderTask(handler, order, bot, ai, task);
}

// cmd use <itemId> <bot> [target-guid | entry:id[@x,y,z]]
static bool CommanderUse(ChatHandler* handler, CommanderOrder const& order, Player* bot,
    RemoteCommander::UseOrder const& request, std::string const& intent)
{
    PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
    if (!CommanderTaskGuard(handler, intent, bot, ai, false))
        return true;
    ItemPrototype const* proto = sObjectMgr.GetItemPrototype(request.itemId);
    if (!proto)
    {
        SendActionError(handler, intent, RemoteCommander::kCodeInvalid, "No item with that id exists.");
        return true;
    }
    if (!bot->HasItemCount(request.itemId, 1, false))
    {
        SendActionError(handler, intent, RemoteCommander::kCodeNoItem, "The bot carries no such item in its bags.");
        return true;
    }
    if (bot->CanUseItem(proto) != EQUIP_ERR_OK)
    {
        SendActionError(handler, intent, RemoteCommander::kCodeFailed,
            "The bot cannot use that item (level, class or skill).");
        return true;
    }
    uint32 const spellId = ItemUseSpell(proto);
    if (spellId && bot->HasSpellCooldown(spellId))
    {
        SendActionError(handler, intent, RemoteCommander::kCodeBusy, "That item is on cooldown.");
        return true;
    }

    CommanderTask task;
    // Nothing to walk to unless the target is a game object (below): a fight does not stop it.
    task.state = RemoteCommander::MakeInPlaceTask(RemoteCommander::TaskKind::Use);
    task.itemId = request.itemId;
    task.spellId = spellId;
    task.intent = intent;
    task.noWalk = true;
    if (request.target || request.spec.ByEntry())
    {
        // By entry: a game object (the nearest spawned one of that entry).
        uint64 const targetRaw = request.spec.ByEntry()
            ? ResolveOrderGuid(handler, intent, bot, request.spec, false)
            : request.target;
        if (!targetRaw)
            return true;
        ObjectGuid const targetGuid(targetRaw);
        if (targetGuid.IsGameObject())
        {
            GameObject* go = ai->GetGameObject(targetGuid);
            if (!go || !sServerFacade.isSpawned(go))
            {
                SendActionError(handler, intent, RemoteCommander::kCodeInvalidTarget,
                    "No game object with that GUID is on the bot's map.");
                return true;
            }
            float const distance = bot->GetDistance(go);
            if (!RemoteCommander::WithinReactDistance(distance, sPlayerbotAIConfig.reactDistance))
            {
                SendActionError(handler, intent, RemoteCommander::kCodeTooFar,
                    "The object is farther from the bot than AiPlayerbot.ReactDistance. Move the group closer first.");
                return true;
            }
            task.state = RemoteCommander::MakeTask(RemoteCommander::TaskKind::Use, distance, true);
            task.targetRaw = targetRaw;
            task.useTargetRaw = targetRaw;
            task.noWalk = false;
        }
        else if (targetGuid.IsUnit())
        {
            Unit* unit = ai->GetUnit(targetGuid);
            if (!unit || !unit->IsInWorld())
            {
                SendActionError(handler, intent, RemoteCommander::kCodeInvalidTarget,
                    "No unit with that GUID is on the bot's map.");
                return true;
            }
            task.useTargetRaw = targetRaw;
        }
        else
        {
            SendActionError(handler, intent, RemoteCommander::kCodeInvalidTarget,
                "The target GUID is neither a unit nor a game object.");
            return true;
        }
    }
    return LaunchCommanderTask(handler, order, bot, ai, task);
}

// cmd ritual <go-guid | entry:id[@x,y,z]> <bot1> <bot2> [<bot3>...]: the named bots walk to a summoning
// ritual object (Uldaman's altars) and use it together; one PENDING, then one final ACK or ERR for the
// whole party (the grammar and the core's rules are in RemoteCommanderTasks.h).
static bool CommanderRitual(ChatHandler* handler, CommanderOrder const& order,
    RemoteCommander::RitualOrder const& request, std::string const& intent)
{
    std::vector<Player*> bots;
    std::vector<PlayerbotAI*> ais;
    for (std::string const& name : request.bots)
    {
        Player* bot = FindCommandedBot(order, name);
        if (!bot)
        {
            SendActionError(handler, intent, "no-bot", ("That bot is not in your commanded group: " + name).c_str());
            return true;
        }
        BotRecord* record = BotManager::Instance().FindBot(bot->GetObjectGuid());
        if (!RemoteCommander::MayIssueOrder(sPlayerbotAIConfig.remoteCommander, true,
            RemoteCommander::OrderTier::OwnerOrGm, record && CanControlBot(order.commander, record)))
        {
            SendActionError(handler, intent, "denied", "Only each bot's owner (or a GameMaster) may do that.");
            return true;
        }
        PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
        if (!CommanderTaskGuard(handler, intent, bot, ai, true))
            return true;
        bots.push_back(bot);
        ais.push_back(ai);
    }

    uint64 const guid = ResolveOrderGuid(handler, intent, bots[0], request.spec, false);
    if (!guid)
        return true;
    GameObject* go = ObjectGuid(guid).IsGameObject() ? ais[0]->GetGameObject(ObjectGuid(guid)) : nullptr;
    if (!go || !sServerFacade.isSpawned(go))
    {
        SendActionError(handler, intent, RemoteCommander::kCodeInvalidTarget,
            "No game object with that GUID is on the bot's map.");
        return true;
    }
    if (go->GetGoType() != GAMEOBJECT_TYPE_SUMMONING_RITUAL)
    {
        SendActionError(handler, intent, RemoteCommander::kCodeInvalidTarget,
            "That object is not a summoning ritual (use cmd open for a lever, a door or a chest).");
        return true;
    }
    if (go->GetGoState() == GO_STATE_ACTIVE)
    {
        SendActionError(handler, intent, RemoteCommander::kCodeBusy, "That ritual is already in progress.");
        return true;
    }
    GameObjectInfo const* info = go->GetGOInfo();
    uint32 const required = info && info->summoningRitual.reqParticipants ? info->summoningRitual.reqParticipants : 1;
    if (bots.size() < required)
    {
        SendActionError(handler, intent, RemoteCommander::kCodeNotEnough,
            ("That ritual needs " + std::to_string(required) + " different players; " +
             std::to_string(bots.size()) + " bots were named.").c_str());
        return true;
    }
    for (Player* bot : bots)
        if (bot->GetMapId() != go->GetMapId() ||
            !RemoteCommander::WithinReactDistance(bot->GetDistance(go), sPlayerbotAIConfig.reactDistance))
        {
            SendActionError(handler, intent, RemoteCommander::kCodeTooFar,
                "The object is farther from a bot than AiPlayerbot.ReactDistance. Move the group closer first.");
            return true;
        }

    RitualGroup group;
    group.id = ++s_ritualNextId;
    group.commanderRaw = order.commander->GetObjectGuid().GetRawValue();
    group.targetRaw = guid;
    group.intent = intent;
    TaskTarget target;
    target.go = go;
    uint32 slowestWalkMs = 0;
    auto& tasks = CommanderTasks();
    for (size_t i = 0; i < bots.size(); ++i)
    {
        Player* bot = bots[i];
        CommanderTask task;
        task.state = RemoteCommander::MakeTask(RemoteCommander::TaskKind::Ritual, bot->GetDistance(go), true);
        task.commanderRaw = group.commanderRaw;
        task.botRaw = bot->GetObjectGuid().GetRawValue();
        task.botName = bot->GetName();
        task.targetRaw = guid;
        task.intent = intent;
        task.ritualId = group.id;
        task.actSpellId = info ? info->summoningRitual.animSpell : 0; // the helpers' channel (StopRitualChannel)
        slowestWalkMs = std::max(slowestWalkMs, task.state.walkBudgetMs);
        if (!TaskInRange(task, bot, target))
            BeginTaskWalk(task, order.commander, bot, ais[i], go);
        tasks[task.botRaw] = task;
        group.members.push_back(task.botRaw);
    }
    group.state = RemoteCommander::MakeRitual(static_cast<uint32>(bots.size()), required, slowestWalkMs);
    uint32 const id = group.id;
    RitualGroups()[id] = group;
    // Bots that already stand at the object may start at once; a refusal on the spot answers alone.
    if (StepRitualGroup(RitualGroups()[id], 0))
        return true;
    handler->PSendSysMessage("TBM:ACTION_PENDING|%s|party|%u|%s", ProtocolSafe(intent).c_str(),
        (RitualGroups()[id].state.ceilingMs + 999) / 1000, std::to_string(guid).c_str());
    return true;
}

// cmd enter <area trigger id | near> [bot]: the party (or the one bot named) goes through a
// dungeon entrance, the lead bot first. Owner or GM, for every bot it moves.
static bool CommanderEnter(ChatHandler* handler, CommanderOrder const& order,
    RemoteCommander::EnterOrder const& request, std::string const& intent)
{
    bool const single = !request.bot.empty();
    std::vector<Player*> bots;
    if (single)
    {
        Player* named = FindCommandedBot(order, request.bot);
        if (!named)
        {
            SendActionError(handler, intent, "no-bot", "That bot is not in your commanded group.");
            return true;
        }
        bots.push_back(named);
    }
    else
        bots = order.bots;
    if (bots.empty())
    {
        SendActionError(handler, intent, "lead-unavailable", "No bot of the group is in the world.");
        return true;
    }
    for (Player* bot : bots)
    {
        BotRecord* record = BotManager::Instance().FindBot(bot->GetObjectGuid());
        if (!RemoteCommander::MayIssueOrder(sPlayerbotAIConfig.remoteCommander, true,
            RemoteCommander::OrderTier::OwnerOrGm, record && CanControlBot(order.commander, record)))
        {
            std::string const message = std::string("Only the owner of ") + bot->GetName() + " (or a GameMaster) may do that.";
            SendActionError(handler, intent, "denied", message.c_str());
            return true;
        }
    }

    // Which trigger: the id, or the nearest instance entrance to the lead bot (to the bot named).
    uint32 triggerId = request.triggerId;
    if (request.nearest)
    {
        Player* reference = single ? bots[0] : order.lead;
        std::vector<RemoteCommander::TriggerCandidate> candidates;
        if (reference->IsInWorld())
            for (auto const& entry : sObjectMgr.GetAreaTriggersMap())
            {
                AreaTriggerEntry const& at = entry.second;
                if (at.mapid != reference->GetMapId() || !InstanceEntranceOf(&at, nullptr))
                    continue;
                RemoteCommander::TriggerCandidate candidate;
                candidate.id = at.id;
                candidate.distance = reference->GetDistance(at.x, at.y, at.z, SizeFactor::None);
                candidates.push_back(candidate);
            }
        if (!RemoteCommander::PickNearestTrigger(candidates, sPlayerbotAIConfig.reactDistance, triggerId))
        {
            SendActionError(handler, intent, RemoteCommander::kCodeNotFound,
                "No instance entrance within AiPlayerbot.ReactDistance of the bot. Move the group closer first.");
            return true;
        }
    }
    AreaTriggerEntry const* trigger = sObjectMgr.GetAreaTrigger(triggerId);
    MapEntry const* targetMap = nullptr;
    AreaTriggerTeleport const* tele = InstanceEntranceOf(trigger, &targetMap);
    if (!tele || !targetMap)
    {
        SendActionError(handler, intent, RemoteCommander::kCodeNotFound,
            "That area trigger is not an instance entrance (use `near`, or a trigger id that leads into a dungeon).");
        return true;
    }

    // Every bot: free to act, on the entrance's map and within reach, and cleared by the entry's requirements.
    // Nothing moves unless all pass.
    float maxDistance = 0.0f;
    std::vector<float> distances;
    for (Player* bot : bots)
    {
        std::string const name = bot->GetName();
        PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
        if (!CommanderTaskGuard(handler, intent, bot, ai, true))
            return true;
        if (!bot->IsInWorld() || bot->GetMapId() != trigger->mapid)
        {
            SendActionError(handler, intent, RemoteCommander::kCodeTooFar,
                (name + " is not on the entrance's map. Move the group there first.").c_str());
            return true;
        }
        float const distance = bot->GetDistance(trigger->x, trigger->y, trigger->z, SizeFactor::None);
        if (!RemoteCommander::WithinReactDistance(distance, sPlayerbotAIConfig.reactDistance))
        {
            SendActionError(handler, intent, RemoteCommander::kCodeTooFar,
                (name + " is farther from the entrance than AiPlayerbot.ReactDistance. Move the group closer first.").c_str());
            return true;
        }
        RemoteCommander::EntryVerdict const verdict = RemoteCommander::JudgeEntry(GatherEntryFacts(bot, tele, targetMap));
        if (!verdict.ok)
        {
            SendActionError(handler, intent, verdict.code, (name + ": " + verdict.message).c_str());
            return true;
        }
        distances.push_back(distance);
        maxDistance = std::max(maxDistance, distance);
    }

    uint32 const serial = ++s_enterSerial;
    EnterOrderState& state = EnterOrders()[serial];
    state.commanderRaw = order.commander->GetObjectGuid().GetRawValue();
    state.triggerId = triggerId;
    state.intent = intent;
    state.scope = single ? "bot:" + std::string(bots[0]->GetName()) : std::string("party");
    state.tally.total = static_cast<uint32>(bots.size());
    bool const hasFollowers = !single && bots.size() > 1;

    auto& tasks = CommanderTasks();
    std::vector<uint64> raws;
    for (size_t i = 0; i < bots.size(); ++i)
    {
        Player* bot = bots[i];
        CommanderTask task;
        task.commanderRaw = state.commanderRaw;
        task.botRaw = bot->GetObjectGuid().GetRawValue();
        task.botName = bot->GetName();
        task.intent = intent;
        task.enter = true;
        task.enterOrder = serial;
        task.triggerId = triggerId;
        task.enterLead = !single && bot == order.lead;
        task.enterState = RemoteCommander::MakeEnterState(distances[i], maxDistance, hasFollowers && !task.enterLead);
        BeginEnterWalk(task, order.commander, bot, PlayerbotAIStorage::Instance().GetAI(bot), trigger);
        tasks[task.botRaw] = task;
        raws.push_back(task.botRaw);
    }
    // A first look at each, the lead bot first: a bot already at the trigger sends it at once.
    for (uint64 raw : raws)
    {
        auto it = tasks.find(raw);
        if (it != tasks.end() && StepCommanderTask(it->second, 0))
            tasks.erase(raw);
    }
    if (EnterOrders().count(serial))
        handler->PSendSysMessage("TBM:ACTION_PENDING|%s|%s|%u|%u", ProtocolSafe(intent).c_str(),
            ProtocolSafe(EnterOrders()[serial].scope).c_str(),
            RemoteCommander::EnterBudgetMs(maxDistance, hasFollowers) / 1000, triggerId);
    return true;
}

// cmd state <go-guid | entry:id>[@x,y,z]: what a game object is doing now, for a
// client that cannot see it (a door that opens when a fight is won). Read-only,
// one lookup; answers at once, never walks anyone anywhere.
static bool CommanderState(ChatHandler* handler, CommanderOrder const& order,
    RemoteCommander::StateOrder const& request, std::string const& intent)
{
    Player* judge = order.judge;
    if (!judge || !judge->IsInWorld() || !judge->GetMap())
    {
        SendActionError(handler, intent, RemoteCommander::kCodeUnavailable, "The group is between maps. Try again in a moment.");
        return true;
    }
    if (request.spec.ByEntry() && !EntryPointReachable(judge, request.spec))
    {
        SendActionError(handler, intent, RemoteCommander::kCodeTooFar,
            "That point is farther from the bot than AiPlayerbot.ReactDistance. Move the group closer first.");
        return true;
    }
    uint64 const guid = request.spec.ByEntry() ? FindEntryTarget(judge, request.spec, false) : request.spec.guid;
    ObjectGuid const objectGuid(guid);
    GameObject* go = guid && objectGuid.IsGameObject() ? judge->GetMap()->GetGameObject(objectGuid) : nullptr;
    // Only what a bot of the group can reach: within ReactDistance of the judge bot
    // or of any live bot of the party on its map. Anything else answers like a missing object.
    if (go)
    {
        bool reachable = RemoteCommander::EntryPointInReach(judge->GetDistance(go), sPlayerbotAIConfig.reactDistance);
        for (Player* member : order.bots)
            if (!reachable && member && member->IsAlive() && member->IsInWorld() && member->GetMap() == judge->GetMap())
                reachable = RemoteCommander::EntryPointInReach(member->GetDistance(go), sPlayerbotAIConfig.reactDistance);
        if (!reachable)
            go = nullptr;
    }
    if (!go || !sServerFacade.isSpawned(go))
    {
        SendActionError(handler, intent, RemoteCommander::kCodeNotFound,
            request.spec.ByEntry() ? "No spawned game object of that entry is near the point (or the group)."
                                   : "No spawned game object with that GUID is within reach of the group.");
        return true;
    }
    SendActionAck(handler, intent, "party", 1,
        RemoteCommander::FormatGoState(guid, go->GetEntry(), static_cast<uint32>(go->GetGoType()),
            static_cast<uint32>(go->GetGoState()), static_cast<uint32>(go->getLootState()),
            go->GetUInt32Value(GAMEOBJECT_FLAGS)));
    return true;
}

// cmd lootmethod <ffa|group|rr|master|nbg> [threshold]: the lead bot, the
// group's leader, sets it through the same handler a client's loot method
// window uses; the answer reads the method back from the group.
static bool CommanderLootMethod(ChatHandler* handler, CommanderOrder const& order,
    std::vector<std::string> const& parts, std::string const& intent)
{
    int method = 0;
    int threshold = 0;
    bool hasThreshold = false;
    if (parts.empty() || parts.size() > 2 || !RemoteCommander::ParseLootMethod(parts[0], method) ||
        (parts.size() == 2 && !RemoteCommander::ParseLootThreshold(parts[1], threshold)))
    {
        SendActionError(handler, intent, RemoteCommander::kCodeInvalid,
            "Usage: .bot cmd lootmethod <ffa|group|rr|master|nbg> [threshold 1-5 or common/uncommon/rare/epic/legendary]");
        return true;
    }
    hasThreshold = parts.size() == 2;
    Player* lead = order.lead;
    Group* group = order.group;
    if (!lead->GetSession() || !group->IsLeader(lead->GetObjectGuid()))
    {
        SendActionError(handler, intent, "lead-unavailable", "The lead bot does not lead the group.");
        return true;
    }
    if (!hasThreshold)
        threshold = static_cast<int>(group->GetLootThreshold());
    threshold = std::max(RemoteCommander::kThresholdMin, std::min(RemoteCommander::kThresholdMax, threshold));

    WorldPacket packet;
    packet << uint32(method);
    packet << (method == RemoteCommander::kLootMaster ? lead->GetObjectGuid() : ObjectGuid());
    packet << uint32(threshold);
    lead->GetSession()->HandleLootMethodOpcode(packet);

    if (static_cast<int>(group->GetLootMethod()) != method)
    {
        SendActionError(handler, intent, RemoteCommander::kCodeFailed, "The group's loot method did not change.");
        return true;
    }
    std::string state = std::string(RemoteCommander::LootMethodName(static_cast<int>(group->GetLootMethod()))) +
        ":" + std::to_string(static_cast<int>(group->GetLootThreshold()));
    SendActionAck(handler, intent, "party", static_cast<uint32>(order.bots.size()), state);
    return true;
}

bool HandleCommanderOrder(ChatHandler* handler, char const* args)
{
    std::string text = Trim(args ? args : "");
    size_t split = text.find_first_of(" \t");
    std::string verb = text.substr(0, split);
    for (char& c : verb)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    std::string rest = split == std::string::npos ? std::string() : Trim(text.substr(split + 1));
    std::string const intent = "cmd " + (verb.empty() ? std::string("unknown") : ProtocolSafe(verb));
    CommanderOrderDetailScope const detailScope(rest);

    if (!RemoteCommander::IsKnownOrder(verb))
    {
        SendActionError(handler, intent, "invalid",
            "Usage: .bot cmd attack <guid> [bot] / pull|pullback <guid> [seconds] / cc <mark> <guid> [bot] / focus <guid> / move <x> <y> <z> / stay / follow / rest / lootmethod <ffa|group|rr|master|nbg> [threshold] / revive <bot> / summon <bot> / loot <guid|entry:id[@x,y,z]> <bot> [itemId] / open <guid|entry:id[@x,y,z]> <bot> / use <itemId> <bot> [guid|entry:id[@x,y,z]] / ritual <guid|entry:id[@x,y,z]> <bot1> <bot2> [<bot3>...] / state <guid|entry:id[@x,y,z]> / enter <area trigger id|near> [bot]");
        return true;
    }

    CommanderOrder order;
    order.commander = Requester(handler);
    if (!order.commander || !order.commander->GetSession())
    {
        SendActionError(handler, intent, "requester", "You must be in-game.");
        return true;
    }
    if (!sPlayerbotAIConfig.remoteCommander)
    {
        SendActionError(handler, intent, "disabled",
            RemoteCommander::BindRefusalText(RemoteCommander::BindRefusal::Disabled));
        return true;
    }
    RemoteCommander::Binding const* binding = StandingBinding(order.commander->GetObjectGuid().GetRawValue());
    if (!binding)
    {
        SendActionError(handler, intent, "not-bound", "You command no group. Use .bot commander bind <LeadBot> first.");
        return true;
    }
    order.lead = FindCharacter(binding->leadGuid);
    order.group = order.lead ? order.lead->GetGroup() : nullptr;
    if (!order.lead || !order.group || !order.lead->IsInWorld())
    {
        SendActionError(handler, intent, "lead-unavailable", "The lead bot is between maps. Try again in a moment.");
        return true;
    }
    order.bots = CommandedBots(order.group, order.lead);
    order.judge = order.lead;
    if (!order.lead->IsAlive())
        for (Player* bot : order.bots)
            if (bot->IsAlive() && bot->IsInWorld() && bot->GetMap() == order.lead->GetMap())
            {
                order.judge = bot;
                break;
            }

    std::vector<std::string> parts = RemoteCommander::SplitOrderArgs(rest);
    RemoteCommander::OrderTier const tier = RemoteCommander::TierForOrder(verb);

    // enter: owner or GM for every bot it moves, checked in CommanderEnter (the bot is optional).
    if (verb == "enter")
    {
        RemoteCommander::EnterOrder enterRequest;
        if (!RemoteCommander::ParseEnterOrder(parts, enterRequest))
        {
            SendActionError(handler, intent, RemoteCommander::kCodeInvalid,
                "Usage: .bot cmd enter <area trigger id | near> [bot]");
            return true;
        }
        return CommanderEnter(handler, order, enterRequest, intent);
    }

    // Owner-or-GM orders name one bot of the group; the commander must
    // control it now (the bound commander alone is not enough).
    if (tier == RemoteCommander::OrderTier::OwnerOrGm)
    {
        RemoteCommander::LootOrder lootRequest;
        RemoteCommander::OpenOrder openRequest;
        RemoteCommander::UseOrder useRequest;
        RemoteCommander::RitualOrder ritualRequest;
        bool parsed = true;
        char const* usage = nullptr;
        if (verb == "loot")
        {
            parsed = RemoteCommander::ParseLootOrder(parts, lootRequest);
            usage = "Usage: .bot cmd loot <corpse-or-object guid | entry:<id>[@x,y,z]> <bot> [itemId]";
        }
        else if (verb == "open")
        {
            parsed = RemoteCommander::ParseOpenOrder(parts, openRequest);
            usage = "Usage: .bot cmd open <object guid | entry:<id>[@x,y,z]> <bot>";
        }
        else if (verb == "use")
        {
            parsed = RemoteCommander::ParseUseOrder(parts, useRequest);
            usage = "Usage: .bot cmd use <itemId> <bot> [target guid | entry:<id>[@x,y,z]]";
        }
        else if (verb == "ritual")
        {
            parsed = RemoteCommander::ParseRitualOrder(parts, ritualRequest);
            usage = "Usage: .bot cmd ritual <object guid | entry:<id>[@x,y,z]> <bot1> <bot2> [<bot3>...] (2-5 different bots)";
        }
        if (!parsed)
        {
            SendActionError(handler, intent, RemoteCommander::kCodeInvalid, usage);
            return true;
        }
        // A ritual names several bots; each is checked for its owner inside.
        if (verb == "ritual")
            return CommanderRitual(handler, order, ritualRequest, intent);
        std::string const botName = RemoteCommander::OwnerOrderBotName(verb, parts);
        Player* bot = botName.empty() ? nullptr : FindCommandedBot(order, botName);
        if (!bot)
        {
            std::string message = "Usage: .bot cmd " + verb + " <bot of your commanded group>";
            SendActionError(handler, intent, "no-bot", message.c_str());
            return true;
        }
        BotRecord* record = BotManager::Instance().FindBot(bot->GetObjectGuid());
        if (!RemoteCommander::MayIssueOrder(sPlayerbotAIConfig.remoteCommander, true, tier,
            record && CanControlBot(order.commander, record)))
        {
            SendActionError(handler, intent, "denied", "Only this bot's owner (or a GameMaster) may do that.");
            return true;
        }
        if (verb == "loot")
            return CommanderLoot(handler, order, bot, lootRequest, intent);
        if (verb == "open")
            return CommanderOpen(handler, order, bot, openRequest, intent);
        if (verb == "use")
            return CommanderUse(handler, order, bot, useRequest, intent);
        return verb == "revive"
            ? CommanderRevive(handler, order, bot, intent)
            : CommanderSummon(handler, order, bot, intent);
    }

    // Tactical orders: the bound commander counts as a member of its group.
    // Issue #872: an autonomous alt answers a stranger's combat orders only
    // through whispers/party chat, never through a commanded group it never
    // joined. Drop autonomous alts from the order's bot list (a stranger
    // bound the group while the alt roamed in it): an order with no bots
    // left is refused, and a party-wide order never moves the alt.
    order.bots.erase(std::remove_if(order.bots.begin(), order.bots.end(),
        [](Player* bot)
        {
            return bot && AltBotService::Instance().IsAutonomous(bot->GetObjectGuid());
        }), order.bots.end());
    if (!RemoteCommander::MayIssueOrder(sPlayerbotAIConfig.remoteCommander, true, tier, false))
    {
        SendActionError(handler, intent, "denied", "Order refused.");
        return true;
    }
    if (order.bots.empty())
    {
        SendActionError(handler, intent, "denied", "That group holds only autonomous alt-bots; order them with whispers or party chat.");
        return true;
    }
    if (verb == "state")
    {
        RemoteCommander::StateOrder stateRequest;
        if (!RemoteCommander::ParseStateOrder(parts, stateRequest))
        {
            SendActionError(handler, intent, RemoteCommander::kCodeInvalid,
                "Usage: .bot cmd state <object guid | entry:<id>[@x,y,z]>");
            return true;
        }
        return CommanderState(handler, order, stateRequest, intent);
    }
    if (verb == "move")
        return CommanderMove(handler, order, rest, intent);
    if (verb == "lootmethod")
        return CommanderLootMethod(handler, order, parts, intent);
    if (verb == "stay" || verb == "follow" || verb == "rest")
    {
        if (!parts.empty())
        {
            std::string message = "Usage: .bot cmd " + verb;
            SendActionError(handler, intent, "invalid", message.c_str());
            return true;
        }
        return CommanderStayFollowRest(handler, order, verb, intent);
    }

    if (verb == "attack" || verb == "focus" || verb == "pull" || verb == "pullback")
    {
        bool const pull = verb == "pull" || verb == "pullback";
        bool const scoped = verb == "attack";
        std::string option;
        if (parts.empty() || parts.size() > (pull ? 3u : scoped ? 2u : 1u))
        {
            std::string message = "Usage: .bot cmd " + verb + " <guid>" + (pull ? " [seconds] [ranged]" : scoped ? " [bot]" : "");
            SendActionError(handler, intent, "invalid", message.c_str());
            return true;
        }
        Player* attackBot = nullptr;
        if (scoped && parts.size() == 2)
        {
            attackBot = FindCommandedBot(order, parts[1]);
            if (!attackBot)
            {
                SendActionError(handler, intent, "no-bot", "That bot is not in your commanded group.");
                return true;
            }
            // Issue #872: the filter above already dropped autonomous alts
            // from the order; a name that resolves to one is refused, not
            // silently run party-wide.
            if (AltBotService::Instance().IsAutonomous(attackBot->GetObjectGuid()))
            {
                SendActionError(handler, intent, "denied", "That bot is an autonomous alt-bot; order it with whispers or party chat.");
                return true;
            }
        }
        if (pull && parts.size() >= 2)
        {
            // "[seconds] [ranged]": either or both, in either order.
            std::string words = parts[1];
            if (parts.size() == 3)
                words += " " + parts[2];
            ai::PullOrderOption parsed;
            if (!ai::ParsePullOrderOption(words, parsed))
            {
                SendActionError(handler, intent, "invalid",
                    "After the guid: the join delay (0-60 seconds) and/or the word ranged, in either order.");
                return true;
            }
            option = PullOptionText(parsed);
        }
        Unit* target = ResolveOrderTarget(handler, intent, order, parts[0]);
        if (!target)
            return true;
        if (verb == "attack")
            return CommanderAttack(handler, order, target, intent, attackBot);
        if (verb == "focus")
            return CommanderFocus(handler, order, target, intent);
        SupersedeCommanderTasks(order.commander->GetObjectGuid().GetRawValue(), 0, RemoteCommander::LaterOrder::Pull);
        BotCommandContext context = CommanderContext(order, target);
        return RunPullOrder(handler, context, order.lead, verb, option, intent);
    }

    // cc <mark> <guid> [bot]
    if (parts.size() < 2 || parts.size() > 3)
    {
        SendActionError(handler, intent, "invalid", "Usage: .bot cmd cc <mark> <guid> [bot]");
        return true;
    }
    std::string mark = parts[0];
    for (char& c : mark)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (!IsRaidTargetMark(mark))
    {
        SendActionError(handler, intent, "invalid", "Unknown raid mark (star, circle, diamond, triangle, moon, square, cross, skull).");
        return true;
    }
    std::string ccIntent = "cc " + mark;
    if (parts.size() == 3)
    {
        Player* named = FindCommandedBot(order, parts[2]);
        if (!named)
        {
            SendActionError(handler, intent, "no-bot", "That bot is not in your commanded group.");
            return true;
        }
        // Issue #872: same as `attack <guid> <bot>` above.
        if (AltBotService::Instance().IsAutonomous(named->GetObjectGuid()))
        {
            SendActionError(handler, intent, "denied", "That bot is an autonomous alt-bot; order it with whispers or party chat.");
            return true;
        }
        ccIntent += " " + std::string(named->GetName());
    }
    Unit* target = ResolveOrderTarget(handler, intent, order, parts[1]);
    if (!target)
        return true;
    BotCommandContext context = CommanderContext(order, target);
    // A cc order replaces any attack still held for a bot under crowd control.
    DropHeldAttacks(order.commander->GetObjectGuid().GetRawValue(), 0);
    return RunCcOrder(handler, context, order.judge, ccIntent, intent);
}

void UpdateRemoteCommanders(uint32_t diff)
{
    // Nothing runs while no commander is bound.
    if (CommanderBindings().Empty())
    {
        // Every release restored its bots; anything left (a late joiner that
        // left in the last second) goes back now.
        SweepCommanderAutonomy();
        s_commanderSweepMs = 0;
        CommanderOrderLog().Clear();
        if (!CommanderViews().empty())
            CommanderViews().clear();
        if (!HeldAttacks().empty())
            HeldAttacks().clear();
        return;
    }
    s_commanderSweepMs += diff;
    if (s_commanderSweepMs >= RemoteCommander::kSweepIntervalMs)
    {
        s_commanderSweepMs = 0;
        for (uint64 commanderRaw : CommanderBindings().Commanders())
            StandingBinding(commanderRaw);
        SweepCommanderAutonomy();
    }
    // Issue #884, phase 2: the perception feed of the bindings that stand.
    UpdateCommanderViews(diff);
    // Attacks held for bots that had lost control of themselves.
    UpdateHeldAttacks(diff);
    // The bot tasks (loot, open, use) of the commanders.
    UpdateCommanderTasks(diff);
}

void NoteCommanderOrderResult(ChatHandler* handler, std::string const& intent, bool ok,
    std::string const& result)
{
    // Only the answers to a bound commander's `.bot cmd <order>` count.
    if (intent.compare(0, 4, "cmd ") != 0)
        return;
    Player* commander = Requester(handler);
    if (!commander)
        return;
    uint64 const commanderRaw = commander->GetObjectGuid().GetRawValue();
    if (!CommanderBindings().FindByCommander(commanderRaw))
        return;
    CommanderTelemetry::OrderRecord record;
    record.at = static_cast<int64_t>(time(nullptr));
    record.verb = intent.substr(4);
    record.detail = s_commanderOrderDetail;
    record.ok = ok;
    record.result = result;
    CommanderOrderLog().Note(commanderRaw, std::move(record));
}

// The name of a spell (English), empty when unknown.
static std::string SpellNameOf(uint32 spellId)
{
    SpellEntry const* spell = sSpellMgr.GetSpellEntry(spellId);
    return spell ? spell->SpellName[0] : std::string();
}

// The unit the spell a unit is casting is aimed at (raw guid), 0 when it casts nothing or the spell has no unit target.
static uint64 CurrentSpellTargetGuid(Unit* unit)
{
    for (CurrentSpellTypes type : { CURRENT_GENERIC_SPELL, CURRENT_CHANNELED_SPELL })
        if (Spell* spell = unit->GetCurrentSpell(type))
            return spell->m_targets.getUnitTargetGuid().GetRawValue();
    return 0;
}

// What the dashboard calls a game object: a letter for its type, or 0 for the kinds it does not draw (mailboxes, fishing holes ...).
static char DashboardObjectType(GameObject const* go)
{
    switch (go->GetGoType())
    {
        case GAMEOBJECT_TYPE_DOOR: return 'd';
        case GAMEOBJECT_TYPE_CHEST: return 'c';
        case GAMEOBJECT_TYPE_BUTTON: return 'b';
        case GAMEOBJECT_TYPE_GOOBER: return 'g';
        case GAMEOBJECT_TYPE_SUMMONING_RITUAL: return 'r';
        default: return 0;
    }
}

// The doors, chests, levers, goobers (gongs, altars) and rituals the bound bots have seen near them: their own cached "nearest game
// objects" lists (read only when a bot already calculated one, no grid scan), resolved on the lead's map, the ones nearest the lead
// first, capped at kMaxObjectsPerBinding. Once per 2 s snapshot cycle, only while a binding stands.
static void FillCommanderObjects(Player* lead, CommanderTelemetry::BindingRef& ref)
{
    Map* map = lead->GetMap();
    if (!map)
        return;
    ref.objectsReported = true;
    static std::list<ObjectGuid> cached;
    std::vector<std::pair<float, CommanderTelemetry::ObjectInfo>> found;
    std::unordered_set<uint64_t> seen;
    for (Player* bot : ref.bots)
    {
        if (!bot || !bot->IsInWorld() || bot->GetMapId() != lead->GetMapId())
            continue;
        ReadCachedGuidList(PlayerbotAIStorage::Instance().GetAI(bot), "nearest game objects", cached);
        for (ObjectGuid const& guid : cached)
        {
            if (!seen.insert(guid.GetRawValue()).second)
                continue;
            GameObject* go = map->GetGameObject(guid);
            char const type = go && go->IsInWorld() ? DashboardObjectType(go) : 0;
            if (!type)
                continue;
            CommanderTelemetry::ObjectInfo info;
            info.id = guid.GetRawValue();
            info.entry = go->GetEntry();
            info.name = go->GetName() ? go->GetName() : "";
            info.x = go->GetPositionX();
            info.y = go->GetPositionY();
            info.type = type;
            info.state = go->GetGoState() == GO_STATE_ACTIVE ? 'a' : (go->getLootState() != GO_READY ? 'u' : 'r');
            float const dx = info.x - lead->GetPositionX(), dy = info.y - lead->GetPositionY();
            found.emplace_back(dx * dx + dy * dy, std::move(info));
        }
    }
    std::sort(found.begin(), found.end(), [](auto const& a, auto const& b) { return a.first < b.first; });
    size_t const count = std::min(found.size(), CommanderTelemetry::kMaxObjectsPerBinding);
    ref.objects.reserve(count);
    for (size_t i = 0; i < count; ++i)
        ref.objects.push_back(std::move(found[i].second));
}

// The map data of one binding for the dashboard (once per 2 s snapshot cycle, only while a binding stands): the commander's
// position when it is online and in the world, and the hostile units the commander's perception feed would list for this binding
// (the same builder: the bots' own cached lists, resolved by guid on the lead's map, capped at kMaxHostiles; no grid or world
// scan). The bots' targets are read by the emitter from the bots it already walks.
static void FillCommanderMapData(RemoteCommander::Binding const& binding, Player* commander, Player* lead,
    CommanderTelemetry::BindingRef& ref)
{
    if (commander && ref.commanderOnline && commander->IsInWorld())
    {
        ref.commanderPlaced = true;
        ref.commanderMap = commander->GetMapId();
        ref.commanderX = commander->GetPositionX();
        ref.commanderY = commander->GetPositionY();
    }
    if (!lead || !lead->IsInWorld() || ref.bots.empty())
        return;
    static RemoteCommander::View::Snapshot snapshot; // not the feed's own: the feed's frame may still be paced out from it
    CollectViewSnapshot(binding, snapshot);
    Map* map = lead->GetMap();
    ref.mobsReported = true;
    ref.mobsMap = lead->GetMapId();
    size_t const count = std::min(snapshot.hostileCount, CommanderTelemetry::kMaxMobsPerBinding);
    ref.mobs.reserve(count);
    for (size_t i = 0; i < count; ++i)
    {
        RemoteCommander::View::Hostile const& h = snapshot.hostiles[i];
        CommanderTelemetry::MobInfo mob;
        mob.id = h.guid;
        mob.entry = h.entry;
        if (Unit* unit = map ? map->GetUnit(::ObjectGuid(h.guid)) : nullptr)
            mob.name = unit->GetName();
        mob.hpPct = h.hpPct;
        mob.x = h.x;
        mob.y = h.y;
        mob.victim = h.target;
        mob.reaction = h.reaction;
        mob.kind = h.critter ? 'k' : h.civilian ? 'v' : h.guard ? 'g' : h.unattackable ? 'u' : 0;
        mob.level = h.level;
        mob.rank = h.rank;
        mob.cc = h.ccKind;
        if (h.casting)
            mob.cast = SpellNameOf(h.casting);
        ref.mobs.push_back(std::move(mob));
    }
    // What each bound bot is doing: the view snapshot already holds its state, newest action, cast and target; the rest are
    // field and aura reads (crowd control, drink/food), once per bot per 2 s.
    ref.botExtras.reserve(snapshot.partyCount);
    for (size_t i = 0; i < snapshot.partyCount; ++i)
    {
        RemoteCommander::View::PartyMember const& member = snapshot.party[i];
        Player* bot = FindCharacter(member.guid);
        if (!bot)
            continue;
        CommanderTelemetry::BotExtra extra;
        extra.id = member.guid;
        extra.state = member.state;
        extra.action = member.action;
        extra.result = member.result;
        if (member.casting)
            extra.cast = SpellNameOf(member.casting);
        extra.cc = CommanderTelemetry::CcKind(bot->HasUnitState(UNIT_STAT_STUNNED), bot->HasUnitState(UNIT_STAT_CONFUSED),
            bot->HasUnitState(UNIT_STAT_FLEEING), bot->HasUnitState(UNIT_STAT_ROOT));
        extra.moving = bot->IsMoving();
        if (member.casting)
            extra.castTarget = CurrentSpellTargetGuid(bot);
        if (bot->IsAlive() && !bot->IsInCombat())
            if (PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot))
                extra.rest = ai->HasAura("drink", bot) ? 'd' : ai->HasAura("food", bot) ? 'e' : 0;
        if (member.target && map)
            if (Unit* target = map->GetUnit(::ObjectGuid(member.target)))
                extra.targetName = target->GetName();
        ref.botExtras.push_back(std::move(extra));
    }
    FillCommanderObjects(lead, ref);
}

void CollectCommanderTelemetry(std::vector<CommanderTelemetry::BindingRef>& out)
{
    out.clear();
    // Nothing to read while no commander is bound: the common case costs a lookup.
    if (CommanderBindings().Empty())
    {
        CommanderOrderLog().Clear();
        return;
    }
    std::unordered_set<uint64_t> bound;
    for (uint64 commanderRaw : CommanderBindings().Commanders())
    {
        bound.insert(commanderRaw);
        RemoteCommander::Binding const* binding = CommanderBindings().FindByCommander(commanderRaw);
        if (!binding || out.size() >= CommanderTelemetry::kMaxBindings)
            continue;
        CommanderTelemetry::BindingRef ref;
        Player* commander = FindCharacter(commanderRaw);
        ref.commander = commander ? commander->GetName() : "offline #" + std::to_string(::ObjectGuid(commanderRaw).GetCounter());
        ref.commanderGuid = ::ObjectGuid(commanderRaw).GetCounter();
        ref.commanderOnline = commander && commander->GetSession() && commander->GetSession()->HasNetworkTransport();
        ref.groupId = binding->groupId;
        auto const view = CommanderViews().find(commanderRaw);
        ref.viewOn = view != CommanderViews().end() && view->second.enabled;
        Player* lead = FindCharacter(binding->leadGuid);
        Group* group = lead ? lead->GetGroup() : nullptr;
        ref.lead = lead;
        if (group && group->GetId() == binding->groupId)
            FillCommandedBots(group, lead, ref.bots);
        ref.orders = CommanderOrderLog().Recent(commanderRaw);
        FillCommanderMapData(*binding, commander, lead, ref);
        out.push_back(std::move(ref));
    }
    CommanderOrderLog().Retain(bound);
}

// One unit as the Dungeons tab shows it (issue #1101): field reads on the unit and its template, the seeing member's reaction to
// it. Unlike AddViewHostile it does not walk the unit's threat list (the dashboard does not use it).
static void FillDungeonMob(Player* seer, Unit* unit, CommanderTelemetry::MobInfo& mob)
{
    mob.id = unit->GetObjectGuid().GetRawValue();
    mob.entry = unit->GetEntry();
    mob.name = unit->GetName();
    mob.hpPct = unit->GetMaxHealth() ? static_cast<uint32>(unit->GetHealth() * 100ull / unit->GetMaxHealth()) : 0;
    mob.x = unit->GetPositionX();
    mob.y = unit->GetPositionY();
    mob.victim = unit->GetTargetGuid().GetRawValue();
    ReputationRank const reaction = seer->GetReactionTo(unit);
    mob.reaction = reaction <= REP_HOSTILE ? 'h' : (reaction >= REP_FRIENDLY ? 'f' : 'n');
    Creature* creature = unit->ToCreature();
    bool const unattackable = unit->HasFlag(UNIT_FIELD_FLAGS,
        UNIT_FLAG_SPAWNING | UNIT_FLAG_IMMUNE_TO_PLAYER | UNIT_FLAG_NON_ATTACKABLE_2);
    mob.kind = unit->GetCreatureType() == CREATURE_TYPE_CRITTER ? 'k'
        : creature && creature->IsCivilian() ? 'v'
        : creature && creature->IsGuard() ? 'g'
        : unattackable ? 'u' : 0;
    mob.level = unit->GetLevel();
    if (creature)
        mob.rank = CommanderTelemetry::RankKind(creature->GetCreatureInfo()->rank,
            creature->HasExtraFlag(CREATURE_FLAG_EXTRA_INSTANCE_BIND));
    mob.cc = CommanderTelemetry::CcKind(unit->HasUnitState(UNIT_STAT_STUNNED), unit->HasUnitState(UNIT_STAT_CONFUSED),
        unit->HasUnitState(UNIT_STAT_FLEEING), unit->HasUnitState(UNIT_STAT_ROOT));
    if (uint32 const spellId = CurrentSpellId(unit))
        mob.cast = SpellNameOf(spellId);
}

void CollectDungeonMobs(std::vector<DungeonMobTelemetry::RunRef>& runs)
{
    using namespace DungeonMobTelemetry;
    // World thread only (the emitter's snapshot cycle, outside the map update): reused between runs and cycles.
    static std::unordered_set<uint64> tried;
    static std::list<ObjectGuid> cached;
    for (RunRef& run : runs)
    {
        run.candidates.clear();
        run.mobsReported = false;
        bool inWorld = false;
        for (Player* member : run.members)
            inWorld = inWorld || (member && member->IsInWorld() && member->GetMap());
        if (!inWorld)
            continue;
        run.mobsReported = true;
        tried.clear();
        size_t const budget = CandidateBudget(run.cap);
        // A guid is resolved once per run, whoever lists it first (checked before the lookup, so a guid that 40 bots list or
        // that was rejected costs one hash probe after the first).
        auto consider = [&run](Player* seer, ObjectGuid guid, uint8 source)
        {
            if (guid.IsEmpty() || !tried.insert(guid.GetRawValue()).second)
                return;
            Unit* unit = SeenHostile(seer, guid);
            if (!unit)
                return;
            Candidate candidate;
            candidate.source = source;
            FillDungeonMob(seer, unit, candidate.mob);
            run.candidates.push_back(std::move(candidate));
        };
        auto usable = [](Player* member) { return member && member->IsInWorld() && member->IsAlive(); };
        // What the members fight: victims and selections (real players too), then what attacks them.
        for (Player* member : run.members)
        {
            if (!usable(member))
                continue;
            if (Unit* victim = member->GetVictim())
                consider(member, victim->GetObjectGuid(), kFromSelection);
            consider(member, member->GetTargetGuid(), kFromSelection);
        }
        for (Player* member : run.members)
        {
            if (!usable(member))
                continue;
            for (Unit* attacker : member->GetAttackers())
                if (attacker)
                    consider(member, attacker->GetObjectGuid(), kFromAttackers);
            ReadCachedGuidList(PlayerbotAIStorage::Instance().GetAI(member), "attackers", cached);
            for (ObjectGuid const& guid : cached)
                consider(member, guid, kFromAttackers);
        }
        // Then what the sight readers see, until the candidate budget is reached (the pure layer picks and caps).
        for (Player* reader : run.sightReaders)
        {
            if (run.candidates.size() >= budget)
                break;
            if (!usable(reader))
                continue;
            ReadCachedGuidList(PlayerbotAIStorage::Instance().GetAI(reader), "possible targets", cached);
            for (ObjectGuid const& guid : cached)
            {
                if (run.candidates.size() >= budget)
                    break;
                consider(reader, guid, kFromSight);
            }
        }
    }
}

bool IsCommandedBy(Player* commander, Player* bot)
{
    if (!commander || !bot)
        return false;
    RemoteCommander::Binding const* binding =
        CommanderBindings().FindByCommander(commander->GetObjectGuid().GetRawValue());
    if (!binding)
        return false;
    Player* lead = FindCharacter(binding->leadGuid);
    Group* group = lead ? lead->GetGroup() : nullptr;
    return group && group->GetId() == binding->groupId && group->IsMember(bot->GetObjectGuid());
}

void NoteCommanderThreat(Player* bot)
{
    if (bot)
        CommanderThreatBots().Note(bot->GetObjectGuid().GetRawValue());
}

bool IsCommanderThreat(Player* bot)
{
    return bot && CommanderThreatBots().Holds(bot->GetObjectGuid().GetRawValue());
}

void HoldCommanderAutonomy(Player* bot, PlayerbotAI* ai, bool fresh, uint32_t listHeld)
{
    if (!bot || !ai || bot->InBattleGround() || !IsInCommandedGroup(bot))
        return;
    uint32 present = listHeld;
    for (unsigned i = 0; i < RemoteCommander::kCommanderAutonomyCount; ++i)
        if (ai->HasStrategy(RemoteCommander::CommanderAutonomyStrategy(i), BotState::BOT_STATE_NON_COMBAT))
            present |= 1u << i;
    uint32 const strip = CommanderAutonomyBots().Hold(bot->GetObjectGuid().GetRawValue(), present, fresh);
    if (strip)
        ai->ChangeStrategy(RemoteCommander::CommanderAutonomyChange(strip, '-'), BotState::BOT_STATE_NON_COMBAT);
}

uint32_t CommanderAutonomyWithheld(Player* bot)
{
    return bot ? CommanderAutonomyBots().Withheld(bot->GetObjectGuid().GetRawValue()) : 0;
}

void CommanderAutonomyMapChange(Player* bot, PlayerbotAI* ai)
{
    if (!bot || !ai || OnInstanceMap(bot) || bot->InBattleGround() || IsInCommandedGroup(bot))
        return;
    uint32 const mask = CommanderAutonomyBots().TakeDeferred(bot->GetObjectGuid().GetRawValue());
    if (mask && !ai->HasRealPlayerMaster())
        ai->ChangeStrategy(RemoteCommander::CommanderAutonomyChange(mask, '+'), BotState::BOT_STATE_NON_COMBAT);
}

bool IsInCommandedGroup(Player* bot)
{
    if (!bot || !bot->GetGroup() || CommanderBindings().Empty())
        return false;
    for (uint64 commanderRaw : CommanderBindings().Commanders())
    {
        RemoteCommander::Binding const* binding = CommanderBindings().FindByCommander(commanderRaw);
        Player* lead = binding ? FindCharacter(binding->leadGuid) : nullptr;
        Group* group = lead ? lead->GetGroup() : nullptr;
        if (group && group->GetId() == binding->groupId && group->IsMember(bot->GetObjectGuid()))
            return true;
    }
    return false;
}

bool HasCommanderBinding(uint64_t commanderRaw)
{
    return CommanderBindings().FindByCommander(commanderRaw) != nullptr;
}

// Release a binding that no longer holds (its hires go back to the hirer), or,
// when the commander has no binding at all, return any hires still marked as
// bound to one.
void SettleCommanderBinding(uint64_t commanderRaw)
{
    if (CommanderBindings().FindByCommander(commanderRaw))
        StandingBinding(commanderRaw);
    else
        HireLifecycle::Instance().EndCommand(::ObjectGuid(commanderRaw));
}

void ClearRemoteCommanders()
{
    CommanderOrderLog().Clear();
    CommanderBindings().Clear();
    CommanderViews().clear();
    SavedFollowTargets().clear();
    CommanderThreatBots().Clear();
    CommanderAutonomyBots().Clear();
    ClearCommanderTasks();
    s_commanderSweepMs = 0;
}


} // namespace BotCommands
} // namespace TortoiseBots
