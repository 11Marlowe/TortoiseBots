#pragma once

// The run's item-need facts from the party's bags (spec item-leg-skip C2/C2b/C2c, review round 2 Opus 1/2): the glue
// (commands/Commander.cpp ReadRunNeedFacts) reads the party once a tick (RunNeedParty: the lead bot first, then the
// group's other members, real players with their SKILL_LOCKPICKING) and hands it here; the run's gates
// (DungeonRun::LegNeedGate, DoorKeyOpen, DoorNeedHold) read the facts this fills. Plain data + std only.

#include "DungeonRunPolicy.h"
#include "RunItemNeedPolicy.h"

#include <algorithm>
#include <string>
#include <vector>

namespace TortoiseBots
{
namespace RunItemNeed
{

// The items of the current step's leg the gates watch (Optional and Door rows), distinct, in row order.
inline std::vector<std::uint32_t> LegNeedItems(const DungeonRun::Run& r)
{
    std::vector<std::uint32_t> items;
    const ai::PlanStep* s = DungeonRun::Current(r);
    if (!s || !r.plan)
        return items;
    for (const ai::PlanLegNeed* need : r.plan->NeedsOfLeg(s->leg))
        if (need && need->item &&
            (need->mode == ai::LegNeedMode::Optional || need->mode == ai::LegNeedMode::Door) &&
            std::find(items.begin(), items.end(), need->item) == items.end())
            items.push_back(need->item);
    return items;
}

// Fills, for the current step's leg:
// - legNeedHolders: per Optional/Door item the member whose bags hold it ("" none). Optional: the first bot (party
//   order: the lead bot first), else the first real player. Door: the first REAL PLAYER (only a player's key opens a
//   door for the party), else the first bot (so the hold's note knows someone carries it), else "".
// - doorNeedLockpick: the best SKILL_LOCKPICKING among the real players (bots never pick a lock).
// - doorNeedCarrier: the first bot carrying the leg's first door item (the one that opens it with
//   AiPlayerbot.DungeonRunBotKeysForPlayers = 1), "" none.
// - legNeedRead: true (the bags were read this tick).
inline void FillLegNeedFacts(const DungeonRun::Run& r, const std::vector<Member>& party, DungeonRun::Facts& f)
{
    f.legNeedHolders.clear();
    f.doorNeedCarrier.clear();
    f.doorNeedLockpick = 0;
    const ai::PlanStep* s = DungeonRun::Current(r);
    if (!s || !r.plan)
        return;
    for (const Member& m : party)
        if (!m.bot)
            f.doorNeedLockpick = std::max(f.doorNeedLockpick, m.lockpick);
    auto first = [&party](std::uint32_t item, bool wantBot) -> std::string {
        for (const Member& m : party)
            if (m.bot == wantBot && Carries(m, item))
                return m.name;
        return std::string();
    };
    bool doorSeen = false;
    for (const ai::PlanLegNeed* need : r.plan->NeedsOfLeg(s->leg))
    {
        if (!need || !need->item)
            continue;
        if (need->mode == ai::LegNeedMode::Optional)
        {
            std::string holder = first(need->item, true);
            if (holder.empty())
                holder = first(need->item, false);
            f.legNeedHolders.push_back({need->item, holder});
        }
        else if (need->mode == ai::LegNeedMode::Door)
        {
            std::string holder = first(need->item, false);
            if (holder.empty())
                holder = first(need->item, true);
            f.legNeedHolders.push_back({need->item, holder});
            if (!doorSeen)
            {
                doorSeen = true;
                f.doorNeedCarrier = first(need->item, true);
            }
        }
    }
    f.legNeedRead = true;
}

} // namespace RunItemNeed
} // namespace TortoiseBots
