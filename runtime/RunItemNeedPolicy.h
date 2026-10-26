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
// - GivenLedger + DecideGiveTick: the per-tick take-back/give-again glue
//   (D5; rule 4 take-all on player join, auto re-give when alone again).
//   The Commander glue calls DecideGiveTick(giveRun, realPlayer, ledgerEmpty,
//   running, done, system): TakeAll (non-system join: take, then the glue
//   rechecks CheckStart for the pause warning) / TakeAllAndRecheck (system
//   join: take, the system L6 watch rechecks) / GiveAgain (alone again,
//   ledger empty, running) / None. With DungeonRunBotKeysForPlayers = 1 the
//   caller skips the take (rule 5: the bots keep carrying).
//
// Plain data + std only, includable without core.

#include "../ai/playerbot/DungeonPlanPolicy.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
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

// The run's given items (D4-D7): bot name -> (item, count given). Counts merge
// (the REPLACE INTO ledger row holds the running count); takeBot/takeAll drop
// the records (the glue destroys up to the recorded count and deletes the row;
// a bot not in the world keeps its ledger row for the login cleanup).
struct GivenLedger
{
    void add(const std::string& bot, std::uint32_t item, std::uint32_t n = 1)
    {
        if (bot.empty() || !item || !n)
            return;
        for (auto& have : records_[bot])
            if (have.first == item)
            {
                have.second += n;
                return;
            }
        records_[bot].push_back(std::make_pair(item, n));
    }

    void takeBot(const std::string& bot) { records_.erase(bot); }

    void takeAll() { records_.clear(); }

    bool empty() const { return records_.empty(); }

    // Takes (returns and drops) one bot's records.
    std::vector<std::pair<std::uint32_t, std::uint32_t>> takeAllFor(const std::string& bot)
    {
        auto it = records_.find(bot);
        if (it == records_.end())
            return {};
        std::vector<std::pair<std::uint32_t, std::uint32_t>> out = it->second;
        records_.erase(it);
        return out;
    }

private:
    std::unordered_map<std::string, std::vector<std::pair<std::uint32_t, std::uint32_t>>> records_;
};

enum class GiveTick : std::uint8_t
{
    None = 0,
    TakeAll,           // a real player joined a non-system run: take everything back at once
    TakeAllAndRecheck, // ... of a system run: take back, the system L6 watch rechecks
    GiveAgain,         // bots alone again, ledger empty, run running: give again
};

// Pure per-tick glue (D5). With DungeonRunBotKeysForPlayers = 1 the caller
// skips the take (rule 5: the bots keep carrying for the party).
inline GiveTick DecideGiveTick(
    bool giveRun, bool realPlayer, bool ledgerEmpty, bool running, bool done, bool system)
{
    if (!giveRun || !running || done)
        return GiveTick::None;
    if (realPlayer)
        return ledgerEmpty ? GiveTick::None : (system ? GiveTick::TakeAllAndRecheck : GiveTick::TakeAll);
    return ledgerEmpty ? GiveTick::GiveAgain : GiveTick::None;
}

} // namespace RunItemNeed
} // namespace TortoiseBots
