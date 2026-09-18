// pi-lens-ignore-file: clang:pp_file_not_found,clang:unknown_typename,clang:use_of_undeclared_identifier,clang:unknown_type_name,clang:undeclared_var_use,clang:incomplete_member_access
#include "HireLifecycle.h"
#include "BotManager.h"
#include "BotActivityLease.h"
#include "PlayerbotAIStorage.h"
#include "../host/BotSessionAdapter.h"
#include "../host/ModuleLog.h"
#include "../ai/playerbot/PlayerbotAI.h"
#include "../ai/playerbot/PlayerbotAIConfig.h"
#include "../ai/playerbot/strategy/Event.h"

#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include "Chat.h"
#include "Group/Group.h"
#include "Log.h"

#include <ctime>

namespace TortoiseBots
{

HireLifecycle& HireLifecycle::Instance()
{
    static HireLifecycle instance;
    return instance;
}

void HireLifecycle::Claim(ObjectGuid botGuid, ObjectGuid masterGuid, uint32_t ownerAccountId)
{
    if (botGuid.IsEmpty())
        return;
    HiredRecord record;
    record.botGuid = botGuid;
    record.masterGuid = masterGuid;
    record.ownerAccountId = ownerAccountId;
    record.masterOfflineSince = 0;
    record.greeted = false;
    m_hired[botGuid.GetCounter()] = record;
}

void HireLifecycle::Release(ObjectGuid botGuid)
{
    m_hired.erase(botGuid.GetCounter());
}

bool HireLifecycle::IsHired(ObjectGuid botGuid) const
{
    return m_hired.find(botGuid.GetCounter()) != m_hired.end();
}

ObjectGuid HireLifecycle::GetMaster(ObjectGuid botGuid) const
{
    auto it = m_hired.find(botGuid.GetCounter());
    if (it == m_hired.end())
        return ObjectGuid();
    return it->second.masterGuid;
}

bool HireLifecycle::MasterOnline(HiredRecord const& record) const
{
    Player* master = sObjectAccessor.FindPlayer(record.masterGuid);
    return master && master->IsInWorld() && master->GetSession() &&
        !master->GetSession()->IsHeadless();
}

void HireLifecycle::Dismiss(HiredRecord const& record, char const* reason)
{
    Player* bot = sObjectAccessor.FindPlayer(record.botGuid);
    if (bot && bot->GetGroup())
        bot->GetGroup()->RemoveMember(record.botGuid, 0);

    // Zero-strain vs living-world: with no random pool the hire logs off
    // immediately; with a pool it only logs off past the admin's population
    // target, otherwise it hearths home and rejoins the roaming world.
    bool randomPoolOn = sPlayerbotAIConfig.randomBotAutologin && sPlayerbotAIConfig.maxRandomBots > 0;
    bool overTarget = false;
    if (randomPoolOn)
    {
        uint32_t onlineRandom = 0;
        for (Player* candidate : BotManager::Instance().GetAllBots())
        {
            BotRecord* rec = candidate ? BotManager::Instance().FindBot(candidate->GetObjectGuid()) : nullptr;
            if (rec && rec->random)
                ++onlineRandom;
        }
        uint32_t target = std::min(sPlayerbotAIConfig.minRandomBots, sPlayerbotAIConfig.maxRandomBots);
        if (!target)
            target = sPlayerbotAIConfig.maxRandomBots;
        overTarget = onlineRandom > target;
    }

    if (!randomPoolOn || overTarget)
    {
        TB_LOG_BASIC("TortoiseBots: hired bot %s dismissed (%s); logging off",
            record.botGuid.GetString().c_str(), reason ? reason : "released");
        BotManager::Instance().RemoveBot(record.botGuid, true);
        m_hired.erase(record.botGuid.GetCounter());
        return;
    }

    if (bot && bot->IsInWorld() && bot->IsAlive() && !bot->IsBeingTeleported())
    {
        bot->TeleportToHomebind(0, false);
        TB_LOG_BASIC("TortoiseBots: hired bot %s dismissed (%s); hearths home to the roaming pool",
            bot->GetName(), reason ? reason : "released");
    }
    BotManager::Instance().ClearBotMaster(record.botGuid);
    BotActivityLeaseManager::Instance().ReleaseMaster(record.botGuid.GetCounter());
    m_hired.erase(record.botGuid.GetCounter());
}

void HireLifecycle::OnGroupMemberRemoved(Group* group, ObjectGuid guid)
{
    if (!group || guid.IsEmpty())
        return;
    auto it = m_hired.find(guid.GetCounter());
    if (it == m_hired.end())
        return;
    // Kicked or left by choice: the hire ends. Master-offline grace is only
    // for disconnects (master object gone), never for explicit removals.
    Player* bot = sObjectAccessor.FindPlayer(guid);
    if (bot && BotManager::Instance().IsBot(guid) && !MasterOnline(it->second))
    {
        // Master already offline and someone removed the bot: still dismiss.
        Dismiss(it->second, "removed from group");
        return;
    }
    HiredRecord record = it->second;
    Dismiss(record, "removed from group");
}

void HireLifecycle::OnGroupDisband(Group* group)
{
    if (!group)
        return;
    // The member list is cleared by the core after this hook in some paths;
    // snapshot hired members first, then dismiss.
    std::vector<HiredRecord> departing;
    for (auto const& kv : m_hired)
    {
        if (group->IsMember(kv.second.botGuid))
            departing.push_back(kv.second);
    }
    for (HiredRecord const& record : departing)
        Dismiss(record, "group disbanded");
}

void HireLifecycle::OnMasterLogin(Player* master)
{
    if (!master || !master->GetSession() || master->GetSession()->IsHeadless())
        return;
    ObjectGuid masterGuid = master->GetObjectGuid();
    for (auto& kv : m_hired)
    {
        HiredRecord& record = kv.second;
        if (record.masterGuid != masterGuid)
            continue;
        record.masterOfflineSince = 0;
        Reunite(record, master);
    }
}

void HireLifecycle::OnMasterLogout(Player* master)
{
    if (!master)
        return;
    ObjectGuid masterGuid = master->GetObjectGuid();
    time_t now = time(nullptr);
    for (auto& kv : m_hired)
    {
        HiredRecord& record = kv.second;
        if (record.masterGuid != masterGuid)
            continue;
        if (!record.masterOfflineSince)
            record.masterOfflineSince = now;
        // Guard stance: hold position where the master vanished. The mature
        // "stay" shortcut anchors both strategies and the return position.
        if (Player* bot = sObjectAccessor.FindPlayer(record.botGuid))
        {
            if (PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot))
            {
                ai::Event stayEvent("stay", "", nullptr);
                ai->DoSpecificAction("stay chat shortcut", stayEvent, true);
            }
        }
        TB_LOG_BASIC("TortoiseBots: master %s offline; hired bot %s guards for %us",
            master->GetName(),
            record.botGuid.GetString().c_str(),
            sPlayerbotAIConfig.hireDisconnectGracePeriod);
    }
}

void HireLifecycle::Reunite(HiredRecord& record, Player* master)
{
    if (!master)
        return;
    Player* bot = sObjectAccessor.FindPlayer(record.botGuid);
    if (!bot || !BotManager::Instance().IsControllableBot(bot))
        return;
    if (!bot->IsInSameGroupWith(master))
    {
        Group* masterGroup = master->GetGroup();
        if (masterGroup && masterGroup->isBGGroup())
            masterGroup = master->GetOriginalGroup();
        if (masterGroup && !masterGroup->isRaidGroup() && masterGroup->GetMembersCount() > 4)
            masterGroup->ConvertToRaid();
        auto* previousInvite = bot->GetGroupInvite();
        WorldPacket packet;
        packet << bot->GetName() << uint32(0);
        master->GetSession()->HandleGroupInviteOpcode(packet);
        if (bot->GetGroupInvite() != previousInvite)
        {
            if (PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot))
            {
                ai::Event inviteEvent("group invite", "", master);
                ai->DoSpecificAction("accept invitation", inviteEvent, true);
            }
        }
    }
    if (bot->IsInSameGroupWith(master))
    {
        if (PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot))
        {
            ai::Event followEvent("follow", "", master);
            ai->DoSpecificAction("follow chat shortcut", followEvent, true);
        }
        if (!record.greeted)
        {
            record.greeted = true;
            std::string text = std::string("Welcome back, ") + master->GetName() + ". We stand ready.";
            WorldPacket data;
            ChatHandler::BuildChatPacket(data, bot->GetGroup() && bot->GetGroup()->isRaidGroup() ? CHAT_MSG_RAID : CHAT_MSG_PARTY,
                text.c_str(), LANG_UNIVERSAL, CHAT_TAG_NONE, bot->GetObjectGuid(), bot->GetName());
            if (Group* group = bot->GetGroup())
                group->BroadcastPacket(&data, true);
        }
    }
}

void HireLifecycle::SweepGracePeriod(time_t now)
{
    uint32_t grace = sPlayerbotAIConfig.hireDisconnectGracePeriod;
    for (auto it = m_hired.begin(); it != m_hired.end();)
    {
        HiredRecord& record = it->second;
        // Drop bookkeeping for bots that left the module another way
        // (.bot remove, reclaim, restart without a record).
        if (!BotManager::Instance().FindBot(record.botGuid))
        {
            it = m_hired.erase(it);
            continue;
        }
        if (!record.masterOfflineSince)
        {
            // Passive disconnect detection: the master object vanished without
            // a logout hook (crash). Start the clock on first observation.
            Player* master = sObjectAccessor.FindPlayer(record.masterGuid);
            bool online = master && master->IsInWorld() && master->GetSession() &&
                !master->GetSession()->IsHeadless();
            if (!online && MasterOnline(record) == false)
            {
                // Only start the clock when the master is genuinely gone, not
                // while they are teleporting (no Player object for a tick).
                if (!master)
                    record.masterOfflineSince = now;
            }
            ++it;
            continue;
        }
        if (MasterOnline(record))
        {
            record.masterOfflineSince = 0;
            ++it;
            continue;
        }
        if (now - record.masterOfflineSince < static_cast<time_t>(grace))
        {
            ++it;
            continue;
        }
        HiredRecord expired = record;
        it = m_hired.erase(it);
        Dismiss(expired, "master grace expired");
    }
}

void HireLifecycle::Update(uint32_t diff)
{
    m_updateElapsedMs += diff;
    if (m_updateElapsedMs < 5000)
        return;
    m_updateElapsedMs = 0;
    if (m_hired.empty())
        return;
    SweepGracePeriod(time(nullptr));
}

} // namespace TortoiseBots
