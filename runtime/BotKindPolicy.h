#pragma once

// Dashboard kind labels (Issue #872): every bot/character row in the
// observability dashboard shows its kind, and the bot list filters by it.
//
// Header-only and free of core/database types on purpose: the precedence is
// exercised standalone in tools/test_bot_kind_policy.cpp. The emitter feeds
// this function plain values from the lifecycles it already queries.
#include <cstdint>

namespace TortoiseBots
{

// pool: wandering random-pool bot. hired: temporary mercenary on a managed
// pool account (hire ledger). claimed: pool bot adopted into a player guild.
// alt: owned alt playing on its own (autonomous alt-bot). companion: owned
// alt following its player. player: a real player's own character.
enum class BotKind : uint8_t
{
    Pool,
    Hired,
    Claimed,
    Alt,
    Companion,
    Player,
};

inline char const* BotKindName(BotKind kind)
{
    switch (kind)
    {
        case BotKind::Hired:     return "hired";
        case BotKind::Claimed:   return "claimed";
        case BotKind::Alt:       return "alt";
        case BotKind::Companion: return "companion";
        case BotKind::Player:    return "player";
        default:                 return "pool";
    }
}

// Precedence: a real player is never a bot kind; an autonomous alt stays an
// alt even while grouped; hired and claimed are pool-account characters with
// a ledger row, so they outrank companion/pool; a non-random character with
// no other mark is a companion (an owned alt following its player).
inline BotKind ClassifyBotKind(bool isPlayer, bool autonomous, bool hired, bool claimed, bool owned)
{
    if (isPlayer)
        return BotKind::Player;
    if (autonomous)
        return BotKind::Alt;
    if (hired)
        return BotKind::Hired;
    if (claimed)
        return BotKind::Claimed;
    if (owned)
        return BotKind::Companion;
    return BotKind::Pool;
}

} // namespace TortoiseBots
