#pragma once

#include "ScriptObjects.h"

namespace TortoiseBots
{

// Bridges Penqle's generic packet hooks into the mature PlayerbotAI packet
// queues. The core does not know why a session is headless or which AI owns a
// player; that interpretation stays here.
class BotPacketAdapter final : public ServerScript
{
public:
    BotPacketAdapter();

    bool CanPacketSend(WorldSession* session, WorldPacket const& packet) override;
    bool CanPacketReceive(WorldSession* session, WorldPacket const& packet) override;

    // World thread, first thing every tick (BotHostAdapter::OnUpdate, before
    // the command drain): records this thread as the world thread, so packet
    // hooks running on it hand packets to the AI at once (issue #844).
    static void MarkWorldThread();
    // World thread only (BotHostAdapter::OnUpdate, before the bot update):
    // hands the packets queued by the hooks since the last tick to the bots'
    // AIs - chat (issue #839) and every other bot / master packet an AI
    // handles (issue #844).
    static void DrainDeferredPackets();
    // World thread, at shutdown: forget queued packets.
    static void ClearDeferredPackets();

private:
    void DispatchMasterIncoming(WorldSession* session, WorldPacket const& packet);
    // Raid anti-spam: when more than kMimicSummaryBots bots share the master,
    // the mimic actions stay silent and the master gets one summary line.
    static constexpr uint32_t kMimicSummaryBots = 4;
    void MaybeSendMimicSummary(Player* master, WorldPacket const& packet);
};

} // namespace TortoiseBots
