#pragma once

// A dead bot's own auto release (ai/playerbot/strategy/actions/ReleaseSpiritAction.h,
// AutoReleaseSpiritAction::isUseful) inside the module's two waits:
// - a dungeon crew **inside an instance** waits while a living group member of a
//   resurrecting class is on its map or the crew still fights; only a wipe releases;
//   outside one the stock rules decide;
// - a bot bound to an active dungeon run (a hired run bot on the PlayerMaster lease
//   whose party a run leader is running or holds paused) waits at its body: the run's
//   regroup releases it (a resurrection in the party, then `revive party`), or the
//   player does. Scarlet Monastery r21-19z40e slot 7 (SM-REVIVE-PATH-2026-10-23): the
//   lead bot released 9 s after it died, while two members still fought, and the core
//   raised it at the Graveyard wing's door, far from the run.
// The run question is asked through a registered query, so this header needs neither
// the run leader nor the command layer: until a run leader registers one, no bot is
// held and the stock rules decide as before. Once its run ends, is stopped, its
// binding is released or its time limit fires, the query answers false and the bot
// releases by the stock rules again. World thread only.

#include "BotActivityLease.h"

#include <cstdint>

class Player;

namespace TortoiseBots {
namespace RunReleaseHold {

using Query = bool (*)(Player* bot);

inline Query& RegisteredQuery()
{
    static Query query = nullptr;
    return query;
}

// The run leader registers its "bound to an active run" question once.
inline void Register(Query query)
{
    RegisteredQuery() = query;
}

// True while a registered run leader holds this bot's party in an active run.
inline bool HeldByRun(Player* bot)
{
    Query const query = RegisteredQuery();
    return bot && query && query(bot);
}

enum class AutoRelease : std::uint8_t
{
    Release, // release now
    Wait,    // stay dead at the body
    Stock,   // not the module's call: the stock rules decide
};

inline AutoRelease Decide(BotActivity lease, bool heldByRun, bool crewHasLivingRezzer, bool crewInCombat,
    bool onInstanceMap)
{
    if (lease == BotActivity::Dungeon && onInstanceMap)
        return !crewHasLivingRezzer && !crewInCombat ? AutoRelease::Release : AutoRelease::Wait;
    if (lease == BotActivity::PlayerMaster && heldByRun)
        return AutoRelease::Wait;
    return AutoRelease::Stock;
}

} // namespace RunReleaseHold
} // namespace TortoiseBots
