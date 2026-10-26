#pragma once

// Spec item-leg-skip C1 (+D glue contract, F1-6/6b/6c/15-21):
// which items a run needs, who gets them at start, and the ledger glue.
//
// - NeedsFrom: every PlanLegNeed of a leg >= the leg of fromStep (one entry per
//   (leg, item)), plus DERIVED Blocking needs: an open (item), use_on (item) or
//   use_item (entry) step whose item no earlier step (loot with that item,
//   use_item with that item) makes and no need row of its leg covers.
//   "Made earlier" (a loot/use_item step j with fromStep <= j < firstStep
//   making the item) counts the need as met, for the player check too.
// - CheckStart never refuses with a real player (door/blocking warn + hold at
//   the door/use step, unless botKeysForPlayers: rule 5 runs the bot-only
//   branch for every party). Bot-only + giveOn: give every needed item to
//   every bot missing it (by item, then party order; never refuse/warn).
//   Bot-only + giveOn false: doors ignored (bots walk through); an unmade,
//   uncarried Blocking need refuses. Player + flag off: nothing given.
// - DecideGiveTick: the per-tick take-back/give-again glue (D5; rule 4 take-all
//   on player join, take back from a bot that left, auto re-give when alone
//   again); the Commander glue fills GiveTickIn and acts on the plan.
//
// Plain data + std only, includable without core.

#include "../ai/playerbot/DungeonPlanPolicy.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace TortoiseBots
{
namespace RunItemNeed
{

struct NeedItem
{
    std::uint32_t item = 0;
    std::string name;
    ai::LegNeedMode mode = ai::LegNeedMode::None;
    std::uint16_t pick = 0;
    std::uint32_t object = 0;
    std::string how;
    std::size_t firstStep = 0; // the plan index of the first step of the leg (rows) or the step (derived)
    std::string where;         // "leg <name|n>" or "step <n>"
};

struct Member
{
    std::string name;
    bool bot = false;
    std::uint32_t lockpick = 0; // SKILL_LOCKPICKING value, real players only (0 none)
    std::vector<std::uint32_t> carries; // the need items in its bags (bank not counted)
};

struct GiveOrder
{
    std::string bot;
    std::uint32_t item = 0;
};

struct StartVerdict
{
    bool refuse = false;
    std::string warn;                 // start warning (doors/blocking with a player; joined with " ")
    std::string text;                 // the refusal line (D3 verdict.text; empty unless refuse)
    std::vector<GiveOrder> give;
};

inline bool BotOnly(const std::vector<Member>& party)
{
    for (const Member& m : party)
        if (!m.bot)
            return false;
    return true;
}

inline bool Carries(const Member& m, std::uint32_t item)
{
    return std::find(m.carries.begin(), m.carries.end(), item) != m.carries.end();
}

// The leg's label for notes: its name, or its number without one.
inline std::string NeedLegLabel(const ai::DungeonPlan& plan, std::uint8_t leg)
{
    const std::string& name = plan.LegName(leg);
    return name.empty() ? std::to_string(leg) : name;
}

inline std::string NeedDisplay(const NeedItem& need)
{
    return need.name.empty() ? "item " + std::to_string(need.item)
                             : need.name + " (item " + std::to_string(need.item) + ")";
}

inline std::vector<NeedItem> NeedsFrom(const ai::DungeonPlan& plan, std::size_t fromStep)
{
    std::vector<NeedItem> out;
    if (fromStep < plan.steps.size())
    {
        std::uint8_t const fromLeg = plan.steps[fromStep].leg;
        for (const ai::PlanLegNeed& need : plan.needs)
        {
            if (need.leg < fromLeg)
                continue;
            NeedItem n;
            n.item = need.item;
            n.name = need.name;
            n.mode = need.mode;
            n.pick = need.pick;
            n.object = need.object;
            n.how = need.how;
            n.firstStep = plan.steps.size();
            for (std::size_t i = 0; i < plan.steps.size(); ++i)
                if (plan.steps[i].leg == need.leg)
                {
                    n.firstStep = i;
                    break;
                }
            n.where = "leg " + NeedLegLabel(plan, need.leg);
            out.push_back(n);
        }
    }
    for (std::size_t i = fromStep; i < plan.steps.size(); ++i)
    {
        const ai::PlanStep& s = plan.steps[i];
        std::uint32_t item = 0;
        if (s.kind == ai::PlanStepKind::Open || s.kind == ai::PlanStepKind::UseOn)
            item = s.item;
        else if (s.kind == ai::PlanStepKind::UseItem)
            item = s.entry;
        else
            continue;
        if (!item)
            continue;
        bool covered = false;
        for (const ai::PlanLegNeed& need : plan.needs)
            if (need.leg == s.leg && need.item == item)
            {
                covered = true;
                break;
            }
        if (covered)
            continue;
        bool made = false;
        for (std::size_t j = fromStep; j < i; ++j)
        {
            const ai::PlanStep& m = plan.steps[j];
            if ((m.kind == ai::PlanStepKind::Loot && m.item == item) ||
                (m.kind == ai::PlanStepKind::UseItem && m.item == item))
            {
                made = true;
                break;
            }
        }
        if (made)
            continue;
        NeedItem n;
        n.item = item;
        n.mode = ai::LegNeedMode::Blocking;
        n.firstStep = i;
        n.where = "step " + std::to_string(i + 1);
        out.push_back(n);
    }
    return out;
}

// A loot/use_item step j with fromStep <= j < need.firstStep making the item:
// the run makes it itself, so the need is met (for the player check too).
inline bool MadeEarlier(const ai::DungeonPlan& plan, std::size_t fromStep, const NeedItem& need)
{
    for (std::size_t j = fromStep; j < need.firstStep && j < plan.steps.size(); ++j)
    {
        const ai::PlanStep& m = plan.steps[j];
        if ((m.kind == ai::PlanStepKind::Loot && m.item == need.item) ||
            (m.kind == ai::PlanStepKind::UseItem && m.item == need.item))
            return true;
    }
    return false;
}

inline StartVerdict CheckStart(const ai::DungeonPlan& plan, std::size_t fromStep, const std::vector<Member>& party,
    bool giveOn, bool botKeysForPlayers = false)
{
    StartVerdict v;
    std::vector<NeedItem> const needs = NeedsFrom(plan, fromStep);
    bool const botOnly = BotOnly(party);
    if (giveOn && (botOnly || botKeysForPlayers))
    {
        std::vector<std::uint32_t> items; // distinct, by item
        for (const NeedItem& n : needs)
        {
            if (MadeEarlier(plan, fromStep, n))
                continue;
            if (std::find(items.begin(), items.end(), n.item) == items.end())
                items.push_back(n.item);
        }
        std::sort(items.begin(), items.end());
        for (std::uint32_t item : items)
            for (const Member& m : party)
                if (m.bot && !Carries(m, item))
                    v.give.push_back(GiveOrder{m.name, item});
        return v;
    }
    if (botOnly)
    {
        for (const NeedItem& n : needs)
        {
            if (n.mode != ai::LegNeedMode::Blocking || MadeEarlier(plan, fromStep, n))
                continue;
            bool carried = false;
            for (const Member& m : party)
                if (m.bot && Carries(m, n.item))
                {
                    carried = true;
                    break;
                }
            if (carried)
                continue;
            v.refuse = true;
            v.text = "This run needs " + NeedDisplay(n) + " for " + n.where +
                ": no bot of the party carries it and AiPlayerbot.DungeonRunGiveItems is 0. Give it to a bot (.additem " +
                std::to_string(n.item) + "), then start again.";
            return v;
        }
        return v;
    }
    std::string warn;
    for (const NeedItem& n : needs)
    {
        if (n.mode == ai::LegNeedMode::Optional)
            continue;
        if (n.mode == ai::LegNeedMode::Door)
        {
            bool playerCarries = false, rogue = false, anyoneCarries = false;
            for (const Member& m : party)
            {
                if (Carries(m, n.item))
                {
                    anyoneCarries = true;
                    if (!m.bot)
                        playerCarries = true;
                }
                if (!m.bot && n.pick > 0 && m.lockpick >= n.pick)
                    rogue = true;
            }
            if (playerCarries || rogue || MadeEarlier(plan, fromStep, n))
                continue;
            std::string const pickText = n.pick > 0 ? " or has Lockpicking " + std::to_string(n.pick) : "";
            std::string const howText =
                !n.how.empty() && !anyoneCarries ? " Get it: " + n.how + "." : "";
            warn += std::string(warn.empty() ? "" : " ") + "This run needs " + NeedDisplay(n) + " for " + n.where +
                ": its door is locked and no player of the party carries the key" + pickText +
                ". The bots will stop at the door for you to open it." + howText;
        }
        else if (n.mode == ai::LegNeedMode::Blocking)
        {
            bool anyoneCarries = false;
            for (const Member& m : party)
                if (Carries(m, n.item))
                {
                    anyoneCarries = true;
                    break;
                }
            if (anyoneCarries || MadeEarlier(plan, fromStep, n))
                continue;
            warn += std::string(warn.empty() ? "" : " ") + "This run needs " + NeedDisplay(n) + " for " + n.where +
                ": nobody in the party carries it. The run will stop at the step for you to hand it over.";
        }
    }
    v.warn = warn;
    return v;
}

// The items the glue reads the party's bags for (RunNeedParty): every item of
// NeedsFrom (need rows and derived blocking needs), distinct, ascending, so a
// carrier of a derived item counts as one.
inline std::vector<std::uint32_t> PlanNeedItems(const ai::DungeonPlan& plan, std::size_t fromStep)
{
    std::vector<std::uint32_t> items;
    for (const NeedItem& n : NeedsFrom(plan, fromStep))
        if (n.item && std::find(items.begin(), items.end(), n.item) == items.end())
            items.push_back(n.item);
    std::sort(items.begin(), items.end());
    return items;
}

// The start's glue plan (D3), all `.bot run start` acts on: refuse (bot-only,
// giveOn 0, an uncarried blocking need) with the refusal LINE (the verdict's
// text), else start with the warning on the note, the give orders, and
// giveRun = giveOn && (bot-only || botKeysForPlayers).
struct StartPlan
{
    bool refuse = false;
    std::string refusal;
    std::string warn;
    std::vector<GiveOrder> give;
    bool giveRun = false;
};

inline StartPlan PlanStart(const ai::DungeonPlan& plan, std::size_t fromStep, const std::vector<Member>& party,
    bool giveOn, bool botKeysForPlayers)
{
    StartVerdict const v = CheckStart(plan, fromStep, party, giveOn, botKeysForPlayers);
    StartPlan p;
    p.refuse = v.refuse;
    p.refusal = v.refuse ? v.text : std::string();
    p.warn = v.warn;
    p.give = v.give;
    p.giveRun = giveOn && (BotOnly(party) || botKeysForPlayers);
    return p;
}

// The per-tick given-item glue (spec D5; review round 2 Opus 4/5): what TickDungeonRun does with the run's given
// items this tick. The glue fills GiveTickIn from the slot (its given bots), the binding (system run) and the lead
// bot's group, calls DecideGiveTick and acts on every field of the plan, in this order: takeBots, takeAll,
// pauseForPlayer, giveAgain.
struct GiveTickIn
{
    bool giveRun = false;    // the run gives (bot-only at start, or DungeonRunBotKeysForPlayers = 1)
    bool realPlayer = false; // a real player is in the lead bot's group now
    bool botKeys = false;    // AiPlayerbot.DungeonRunBotKeysForPlayers (read live)
    bool system = false;     // a system run (its own L6 pause handles a player joining)
    bool running = false;    // the run's state is Running
    bool done = false;       // the run's state is Done
    std::vector<std::uint64_t> givenBots; // the bots holding the run's given items (the slot's records)
    std::vector<std::uint64_t> groupBots; // every member of the lead bot's group now (online or not)
};

struct GiveTickPlan
{
    std::vector<std::uint64_t> takeBots; // given bots no longer in the group: take back from each, this tick
    bool takeAll = false;        // done, or a real player is in the party (flag 0): take back every given item
    bool pauseForPlayer = false; // a non-system running run whose items were taken THIS tick for a joined player:
                                 // the glue rechecks CheckStart from the current step and pauses with its warning
    bool giveAgain = false;      // nothing given any more, no real player (or the flag on), running: give again
};

inline GiveTickPlan DecideGiveTick(const GiveTickIn& in)
{
    GiveTickPlan plan;
    std::vector<std::uint64_t> kept;
    for (std::uint64_t bot : in.givenBots)
        if (std::find(in.groupBots.begin(), in.groupBots.end(), bot) == in.groupBots.end())
            plan.takeBots.push_back(bot); // the bot left the party (rule 1)
        else
            kept.push_back(bot);
    if (in.done)
    {
        plan.takeAll = !kept.empty();
        return plan;
    }
    if (in.giveRun && in.realPlayer && !in.botKeys && !kept.empty())
    {
        plan.takeAll = true; // rule 4: at once, the tick the player is seen
        plan.pauseForPlayer = !in.system && in.running;
        return plan;
    }
    plan.giveAgain = in.giveRun && in.running && kept.empty() && (!in.realPlayer || in.botKeys);
    return plan;
}

} // namespace RunItemNeed
} // namespace TortoiseBots
