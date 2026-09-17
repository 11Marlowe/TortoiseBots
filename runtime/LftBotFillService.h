#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class Player;
class ObjectGuid;

namespace TortoiseBots
{

// Default-off bounded fill of human-waiting LFT queues with live random Headless bots.
// The service observes the copy-only native queue API from core #416,
// identifies human groups/instances and missing 1 tank / 1 healer / 3 dps roles,
// then filters in-memory Headless random candidates by the authoritative
// Soromeister/LFT v0.0.3.3 LFT.allDungeons minLevel/maxLevel range,
// team, hardcore, group, state, and AiFactory spec role before QueuePlayer.
// Native core owns offers, acceptance, cancellation, and group formation; the
// service auto-accepts only its own Headless participants. Unknown dungeon ranges
// fail closed. No second queue, DB tick, addon protocol, or role hook.
class LftBotFillService
{
public:
    static LftBotFillService& Instance();

    void Initialize();
    void Update(uint32_t diff);
    void Shutdown();

    // Activity-lease eviction hook (issue #89): synchronously leaves the
    // native LFT queue, clears the forced role, and drops pending tracking.
    // Must not touch the lease map; the manager owns the transition.
    void OnLeaseEvicted(uint32_t guidLow);

private:
    LftBotFillService() = default;
    ~LftBotFillService() = default;

    bool IsEligibleCandidate(Player* bot) const;
    uint8 GetBotRoleMask(Player const* bot) const;
    bool IsModuleOwnedHeadlessBot(Player const* bot) const;
    void ReconcilePending(bool cancelAll, std::vector<std::string> const* activeInstances = nullptr);
    void AcceptPendingOffers();
    void ClearForcedRole(uint32 guidLow);
    // Issue #189 Phase 4: natural-role eligibility (no borrowing by default)
    // plus best-shield-from-bags equip for shield-class tanks. Returns the
    // skip reason when the bot must not fill the role, empty when it may.
    std::string RoleMismatchReason(Player* bot, uint8 needRole) const;
    bool EquipBestShieldFromBags(Player* bot) const;

    bool m_initialized = false;
    uint32_t m_elapsedMs = 0;
    // guidLow -> instance we queued the bot for (single instance, the one we filled)
    std::unordered_map<uint32_t, std::string> m_pending;
    // "instance:role:reason" -> skip count for this tick, logged once per tick.
    std::unordered_map<std::string, uint32_t> m_skipReasons;
};

} // namespace TortoiseBots
