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

// Issue #844: every other packet a bot's AI reacts to (bot outgoing, master
// outgoing and master incoming) is copied here too and handed to the AI on the
// world thread. A packet hook may run on AsyncPackets or ChannelBroadcaster
// while the world thread deletes that bot's AI (logout, removal), and the AI
// storage lock covers only the lookup, so no hook keeps a PlayerbotAI pointer.
// Only opcodes with an AI handler are copied (PlayerbotAI::Handles*Opcode);
// those are rare next to chat (invites, loot, quest updates, cast results),
// so 256 per bot per tick is never reached in play and only bounds a flood.
// A master packet is copied once and shared by the bots it is queued for.
constexpr size_t kMaxDeferredPacketsPerBot = 256;

enum class PacketRoute : uint8_t
{
    BotOutgoing,
    MasterOutgoing,
    MasterIncoming,
};

struct DeferredPacket
{
    PacketRoute route;
    std::shared_ptr<WorldPacket const> packet;
};

WorldThreadQueue<DeferredPacket>& DeferredBotPackets()
{
    static WorldThreadQueue<DeferredPacket> queue(kMaxDeferredPacketsPerBot);
    return queue;
}

// The world thread deletes AIs, so a hook running on it can hand a packet to
// the AI at once, as before: the AI's own casts, hire provisioning (which drops
// its queued level-up echo right after the jump) and world-processed packets
// keep their timing. Learned on the first drain; until then every hook defers.
WorldThreadMark& WorldThread()
{
    static WorldThreadMark mark;
    return mark;
}

bool OnWorldThread()
{
    return WorldThread().IsCurrent();
}

// Live AI of a bot named by raw guid. Only where no AI can be deleted while
// the pointer is used: the world thread (the only one that deletes AIs) or a
// map thread (the world thread waits in MapManager::Update meanwhile).
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
        // Issue #844: any thread. Off the world thread nothing here
        // dereferences the bot's Player or AI; the storage maps the session's
        // Player pointer to the guid the deferred copy is keyed by, and only
        // bots with an AI have one.
        uint16 const opcode = packet.getOpcode();
        if (!PlayerbotAI::HandlesBotOutgoingOpcode(opcode))
            return true;
        ObjectGuid botGuid;
        if (!PlayerbotAIStorage::Instance().GetBotGuid(session->GetPlayer(), botGuid))
            return true;
        if (opcode == SMSG_GROUP_INVITE)
            TB_LOG_DEBUG("TortoiseBots: ServerScript CanPacketSend bot outgoing SMSG_GROUP_INVITE to bot guid %u", botGuid.GetCounter());

        if (opcode != SMSG_MESSAGECHAT && OnWorldThread())
        {
            if (PlayerbotAI* ai = FindBotAI(botGuid.GetRawValue()))
                ai->HandleBotOutgoingPacket(packet);
            return true;
        }

        // Issue #839: chat has its own per-bot FIFO and cap, so a flooded
        // channel cannot crowd out invites or loot.
        if (opcode == SMSG_MESSAGECHAT)
        {
            if (IsBotHandledChatType(packet))
                DeferredBotChat().Enqueue(botGuid.GetRawValue(), std::make_unique<WorldPacket>(packet));
            return true;
        }

        DeferredBotPackets().Enqueue(botGuid.GetRawValue(),
            DeferredPacket{ PacketRoute::BotOutgoing, std::make_shared<WorldPacket const>(packet) });
        return true;
    }

    Player* master = session->GetPlayer();
    if (!master)
        return true;
    if (!PlayerbotAI::HandlesMasterOutgoingOpcode(packet.getOpcode()))
        return true;

    // Issue #839 / #844: any thread. Published guid view; off the world
    // thread the copy is handed to each bot's AI by the world-thread drain.
    bool const onWorldThread = OnWorldThread();
    std::shared_ptr<WorldPacket const> copy;
    for (uint64_t botGuid : BotManager::Instance().GetBotGuidsForMasterAnyThread(master->GetObjectGuid()))
    {
        if (packet.getOpcode() == SMSG_PARTY_COMMAND_RESULT)
            TB_LOG_DEBUG("TortoiseBots: ServerScript CanPacketSend master SMSG_PARTY_COMMAND_RESULT %s -> bot guid %llu",
                master->GetName(), static_cast<unsigned long long>(botGuid));
        if (onWorldThread)
        {
            if (PlayerbotAI* ai = FindBotAI(botGuid))
                ai->HandleMasterOutgoingPacket(packet);
            continue;
        }
        if (!copy)
            copy = std::make_shared<WorldPacket const>(packet);
        DeferredBotPackets().Enqueue(botGuid, DeferredPacket{ PacketRoute::MasterOutgoing, copy });
    }

    return true;
}

void BotPacketAdapter::DrainDeferredPackets()
{
    WorldThread().MarkCurrent();

    // Issue #844: resolved again on the world thread, so a bot whose AI left
    // the storage (logout, removal) since the copy is skipped. Each bot's
    // packets reach its AI in arrival order.
    DeferredBotPackets().Drain([](uint64_t botGuid, DeferredPacket& deferred)
    {
        PlayerbotAI* ai = FindBotAI(botGuid);
        if (!ai || !ai->GetBot())
            return;
        switch (deferred.route)
        {
            case PacketRoute::BotOutgoing:
                ai->HandleBotOutgoingPacket(*deferred.packet);
                break;
            case PacketRoute::MasterOutgoing:
                ai->HandleMasterOutgoingPacket(*deferred.packet);
                break;
            case PacketRoute::MasterIncoming:
                ai->HandleMasterIncomingPacket(*deferred.packet);
                break;
        }
    });

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
    DeferredBotPackets().Clear();
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

    if (!PlayerbotAI::HandlesMasterIncomingOpcode(packet.getOpcode()))
        return;

    // Issue #839 / #844: runs on whichever thread processes the master's
    // packet (world, map or AsyncPackets); same any-thread view and deferred
    // copy as CanPacketSend.
    bool const onWorldThread = OnWorldThread();
    std::shared_ptr<WorldPacket const> copy;
    for (uint64_t botGuid : BotManager::Instance().GetBotGuidsForMasterAnyThread(master->GetObjectGuid()))
    {
        if (onWorldThread)
        {
            if (PlayerbotAI* ai = FindBotAI(botGuid))
                ai->HandleMasterIncomingPacket(packet);
            continue;
        }
        if (!copy)
            copy = std::make_shared<WorldPacket const>(packet);
        DeferredBotPackets().Enqueue(botGuid, DeferredPacket{ PacketRoute::MasterIncoming, copy });
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

    // CMSG_USE_ITEM is PACKET_PROCESS_MAP: this runs on the master's map
    // thread, and the world thread waits in MapManager::Update while map
    // threads run, so it can neither update nor delete an AI meanwhile
    // (issue #844: the one hook path that still reads AIs directly). The bot
    // list still comes from the any-thread view (issue #839).
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
