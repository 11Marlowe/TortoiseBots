#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace TortoiseBots
{

// Durable claim record for wandering bots adopted into a player's guild.
struct ClaimedBotRecord
{
    uint32_t botGuidLow = 0;
    uint32_t ownerAccountId = 0;
    uint32_t ownerPlayerGuidLow = 0;
    uint32_t guildId = 0;
};

// Lifecycle manager for claimed guild bots (Issue #489).
// In-memory cache synced with the `tortoise_bots_claimed` database table.
class ClaimLifecycle
{
public:
    static ClaimLifecycle& Instance();

    void Initialize();

    bool Claim(uint32_t botGuidLow, uint32_t ownerAccountId, uint32_t ownerPlayerGuidLow, uint32_t guildId);
    bool Unclaim(uint32_t botGuidLow);

    bool IsClaimed(uint32_t botGuidLow) const;
    bool IsClaimedByPlayer(uint32_t botGuidLow, uint32_t playerGuidLow) const;
    bool IsClaimedByAccount(uint32_t botGuidLow, uint32_t accountId) const;

    uint32_t GetOwnerPlayerGuid(uint32_t botGuidLow) const;
    uint32_t GetOwnerAccountId(uint32_t botGuidLow) const;
    uint32_t GetGuildId(uint32_t botGuidLow) const;

    std::vector<uint32_t> GetClaimedBotsForPlayer(uint32_t ownerPlayerGuidLow) const;
    std::vector<uint32_t> GetClaimedBotsForGuild(uint32_t guildId) const;

    void OnGuildDelMember(uint32_t botGuidLow);
    void OnGuildDisband(uint32_t guildId);

private:
    ClaimLifecycle() = default;

    mutable std::mutex m_mutex;
    std::unordered_map<uint32_t, ClaimedBotRecord> m_claimedByBotGuid;
    bool m_initialized = false;
};

} // namespace TortoiseBots
