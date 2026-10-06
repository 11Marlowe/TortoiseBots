#include "BotPacketAdapter.h"

#include "../runtime/BotManager.h"
#include "../runtime/PlayerbotAIStorage.h"
#include "playerbot/PlayerbotAI.h"

#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Log.h"
#include "ModuleLog.h"

namespace TortoiseBots
{

BotPacketAdapter::BotPacketAdapter()
    : ServerScript("tortoisebots_packets", {
        SERVERHOOK_CAN_PACKET_SEND,
        SERVERHOOK_CAN_PACKET_RECEIVE })
{
}

bool BotPacketAdapter::CanPacketSend(WorldSession* session, WorldPacket const& packet)
{
    if (!session)
        return true;

    if (session->IsHeadless())
    {
        if (packet.getOpcode() == SMSG_GROUP_INVITE)
            TB_LOG_DEBUG("TortoiseBots: ServerScript CanPacketSend bot outgoing SMSG_GROUP_INVITE from %s", session->GetPlayer() ? session->GetPlayer()->GetName() : "<none>");
        if (PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(session->GetPlayer()))
            ai->HandleBotOutgoingPacket(packet);
        return true;
    }

    Player* master = session->GetPlayer();
    if (!master)
        return true;

    for (Player* bot : BotManager::Instance().GetBotsForMaster(master->GetObjectGuid()))
    {
        if (packet.getOpcode() == SMSG_PARTY_COMMAND_RESULT)
            TB_LOG_DEBUG("TortoiseBots: ServerScript CanPacketSend master SMSG_PARTY_COMMAND_RESULT %s -> bot %s",
                master->GetName(), bot ? bot->GetName() : "<none>");
        if (PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot))
            ai->HandleMasterOutgoingPacket(packet);
    }

    return true;
}

bool BotPacketAdapter::CanPacketReceive(WorldSession* session, WorldPacket const& packet)
{
    if (!session || session->IsHeadless())
        return true;

    DispatchMasterIncoming(session, packet);
    if (packet.getOpcode() == CMSG_USE_ITEM)
        MaybeSendMimicSummary(session->GetPlayer(), packet);
    return true;
}

void BotPacketAdapter::DispatchMasterIncoming(WorldSession* session, WorldPacket const& packet)
{
    if (!session || session->IsHeadless())
        return;

    Player* master = session->GetPlayer();
    if (!master)
        return;

    if (packet.getOpcode() == CMSG_GROUP_UNINVITE ||
        packet.getOpcode() == CMSG_QUESTGIVER_ACCEPT_QUEST ||
        packet.getOpcode() == CMSG_GOSSIP_HELLO ||
        packet.getOpcode() == CMSG_LOOT_ROLL)
    {
        TB_LOG_DEBUG("TortoiseBots: ServerScript CanPacketReceive master opcode %u from %s", packet.getOpcode(), master->GetName());
    }

    for (Player* bot : BotManager::Instance().GetBotsForMaster(master->GetObjectGuid()))
    {
        if (PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot))
            ai->HandleMasterIncomingPacket(packet);
    }
}
void BotPacketAdapter::MaybeSendMimicSummary(Player* master, WorldPacket const& packet)
{
    if (!master)
        return;
    // Decode which bag slot the master used; unreadable packets are ignored.
    WorldPacket copy(packet);
    copy.rpos(0);
    uint8_t bagIndex = 0, slot = 0, spellCount = 0;
    if (copy.size() < 3)
        return;
    copy >> bagIndex >> slot >> spellCount;
    Item* masterItem = master->GetItemByPos(bagIndex, slot);
    if (!masterItem || !masterItem->GetProto())
        return;

    std::vector<Player*> bots = BotManager::Instance().GetBotsForMaster(master->GetObjectGuid());
    if (bots.size() <= kMimicSummaryBots)
        return;

    // Only mimic-enabled bots count; the first one relays the summary so the
    // master sees one line instead of one whisper per bot.
    uint32_t mimicCount = 0;
    Player* relay = nullptr;
    for (Player* bot : bots)
    {
        PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
        if (!ai || !ai->HasStrategy("mimic consumables", BotState::BOT_STATE_NON_COMBAT))
            continue;
        if (!relay)
            relay = bot;
        ++mimicCount;
    }
    if (!relay || mimicCount <= kMimicSummaryBots)
        return;

    ItemPrototype const* proto = masterItem->GetProto();
    std::string what = proto->Name1.empty() ? "consumables" : proto->Name1;
    std::ostringstream out;
    out << "[Raid Mimic]: " << mimicCount << " bots mimicked " << what << " with role equivalents";
    if (PlayerbotAI* relayAi = PlayerbotAIStorage::Instance().GetAI(relay))
        relayAi->TellPlayerNoFacing(master, out.str());
}


} // namespace TortoiseBots
