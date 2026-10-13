#include "BotPacketAdapter.h"

#include "../runtime/BotManager.h"
#include "../runtime/PlayerbotAIStorage.h"
#include "../runtime/WorldThreadQueue.h"
#include "playerbot/PlayerbotAI.h"

#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Log.h"
#include "ModuleLog.h"

#include <memory>
#include <vector>

namespace TortoiseBots
{

BotPacketAdapter::BotPacketAdapter()
    : ServerScript("tortoisebots_packets", {
        SERVERHOOK_CAN_PACKET_SEND,
        SERVERHOOK_CAN_PACKET_RECEIVE })
{
}

namespace
{
// Issue #839: SMSG_MESSAGECHAT delivered to a bot's headless session arrives on
// the sender's thread - AsyncPackets for party / raid / guild / whisper,
// ChannelBroadcaster for channels, a map thread for say / yell. Its handler
// (PlayerbotAI::HandleBotOutgoingPacket) runs whisper and party commands and
// writes chat state, so the hook only copies the packet here and the world
// hook runs it before the bot update. 64 per bot per tick is far above what a
// master types; a bot in a flooded channel drops the excess, as its reply
// logic ignores most channel lines anyway.
constexpr size_t kMaxDeferredChatPerBot = 64;

WorldThreadQueue<std::unique_ptr<WorldPacket>>& DeferredBotChat()
{
    static WorldThreadQueue<std::unique_ptr<WorldPacket>> queue(kMaxDeferredChatPerBot);
    return queue;
}

// The chat types PlayerbotAI::HandleBotOutgoingPacket reacts to; everything
// else (system, monster, emote, ...) is ignored there, so it is not copied.
bool IsBotHandledChatType(WorldPacket const& packet)
{
    if (packet.size() < 1)
        return false;
    switch (packet.contents()[0])
    {
        case CHAT_MSG_CHANNEL:
        case CHAT_MSG_SAY:
        case CHAT_MSG_PARTY:
        case CHAT_MSG_YELL:
        case CHAT_MSG_WHISPER:
        case CHAT_MSG_GUILD:
            return true;
        default:
            return false;
    }
}

// Live AI of a bot named by raw guid. The AI storage is locked; an AI is
// removed from it before it is deleted.
PlayerbotAI* FindBotAI(uint64_t botGuid)
{
    return PlayerbotAIStorage::Instance().GetAI(ObjectGuid(botGuid));
}
} // namespace

bool BotPacketAdapter::CanPacketSend(WorldSession* session, WorldPacket const& packet)
{
    if (!session)
        return true;

    if (session->IsHeadless())
    {
        Player* bot = session->GetPlayer();
        if (packet.getOpcode() == SMSG_GROUP_INVITE)
            TB_LOG_DEBUG("TortoiseBots: ServerScript CanPacketSend bot outgoing SMSG_GROUP_INVITE from %s", bot ? bot->GetName() : "<none>");
        PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
        if (!ai)
            return true;

        // Issue #839: chat is copied and handled on the world thread (see
        // DeferredBotChat). Every other opcode keeps its inline path: the
        // spell / knockback cases come from the bot's own map thread or the
        // world thread, which never overlap the AI update, and the rest only
        // enter the mutex-guarded PacketHandlingHelper queue.
        if (packet.getOpcode() == SMSG_MESSAGECHAT)
        {
            if (IsBotHandledChatType(packet))
                DeferredBotChat().Enqueue(bot->GetObjectGuid().GetRawValue(), std::make_unique<WorldPacket>(packet));
            return true;
        }

        ai->HandleBotOutgoingPacket(packet);
        return true;
    }

    Player* master = session->GetPlayer();
    if (!master)
        return true;

    // Issue #839: any thread. Published guid view + locked AI lookup; the AI
    // side only enters its mutex-guarded packet queue.
    for (uint64_t botGuid : BotManager::Instance().GetBotGuidsForMasterAnyThread(master->GetObjectGuid()))
    {
        if (packet.getOpcode() == SMSG_PARTY_COMMAND_RESULT)
            TB_LOG_DEBUG("TortoiseBots: ServerScript CanPacketSend master SMSG_PARTY_COMMAND_RESULT %s -> bot guid %llu",
                master->GetName(), static_cast<unsigned long long>(botGuid));
        if (PlayerbotAI* ai = FindBotAI(botGuid))
            ai->HandleMasterOutgoingPacket(packet);
    }

    return true;
}

void BotPacketAdapter::DrainDeferredPackets()
{
    DeferredBotChat().Drain([](uint64_t botGuid, std::unique_ptr<WorldPacket>& packet)
    {
        // Resolved again on the world thread: a bot that logged out (its AI
        // left the storage) since the copy is skipped.
        if (PlayerbotAI* ai = FindBotAI(botGuid))
            if (ai->GetBot())
                ai->HandleBotOutgoingPacket(*packet);
    });
}

void BotPacketAdapter::ClearDeferredPackets()
{
    DeferredBotChat().Clear();
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

    // Issue #839: runs on whichever thread processes the master's packet
    // (world, map or AsyncPackets); same any-thread view as CanPacketSend.
    for (uint64_t botGuid : BotManager::Instance().GetBotGuidsForMasterAnyThread(master->GetObjectGuid()))
    {
        if (PlayerbotAI* ai = FindBotAI(botGuid))
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

    // CMSG_USE_ITEM is processed on the master's map thread, which never
    // overlaps the world-thread AI update; the bot list still comes from the
    // any-thread view (issue #839).
    std::vector<uint64_t> bots = BotManager::Instance().GetBotGuidsForMasterAnyThread(master->GetObjectGuid());
    if (bots.size() <= kMimicSummaryBots)
        return;

    // Only mimic-enabled bots count; the first one relays the summary so the
    // master sees one line instead of one whisper per bot.
    uint32_t mimicCount = 0;
    PlayerbotAI* relayAi = nullptr;
    for (uint64_t botGuid : bots)
    {
        PlayerbotAI* ai = FindBotAI(botGuid);
        if (!ai || !ai->HasStrategy("mimic consumables", BotState::BOT_STATE_NON_COMBAT))
            continue;
        if (!relayAi)
            relayAi = ai;
        ++mimicCount;
    }
    if (!relayAi || mimicCount <= kMimicSummaryBots)
        return;

    ItemPrototype const* proto = masterItem->GetProto();
    std::string what = proto->Name1.empty() ? "consumables" : proto->Name1;
    std::ostringstream out;
    out << "[Raid Mimic]: " << mimicCount << " bots mimicked " << what << " with role equivalents";
    relayAi->TellPlayerNoFacing(master, out.str());
}


} // namespace TortoiseBots
