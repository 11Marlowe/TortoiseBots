#pragma once

#include <cstdint>

namespace TortoiseBots
{

// The bot registry (BotManager::m_bots) is keyed by a player guid's counter.
// Only a player guid may be looked up there: every guid's counter is a small
// number, and a creature's or a pet's collides with some bot character's.
// Deadmines r12-19z29 (DEADMINES-PACKS-2026-10-22 Q3): the Remote-Controlled
// Golem summons carry pet guids (high part 0xF140, counters 18, 31, 32), and
// BotManager::IsBot answered "a bot" for them, so ResolveOrderTarget refused
// the tank's pick-up order with "That GUID is a bot or a member of the group"
// (t 1506.3, 2444.0, 2576.1). A guid is a player's when its high 16 bits are
// HIGHGUID_PLAYER (0x0000 in the 1.12 core, src/game/ObjectGuid.h) and it is
// not 0; its counter is then its low 32 bits (ObjectGuid::GetCounter: a player
// guid has no entry part).
constexpr std::uint16_t kBotGuidHighPlayer = 0x0000;

// The raw guid's high part (ObjectGuid::GetHigh).
inline std::uint16_t BotGuidHigh(std::uint64_t raw)
{
    return static_cast<std::uint16_t>((raw >> 48) & 0xFFFF);
}

// True and the registry key when `raw` is a player guid; false for an empty
// guid and for any other kind (creature, pet, game object, item ...).
inline bool BotRegistryKey(std::uint64_t raw, std::uint32_t& key)
{
    if (raw == 0 || BotGuidHigh(raw) != kBotGuidHighPlayer)
        return false;
    key = static_cast<std::uint32_t>(raw & 0xFFFFFFFFull);
    return true;
}

} // namespace TortoiseBots
