#pragma once

// Which world bots may form a bot LFG group and which group forms (bot LFG
// part 2): candidate eligibility, the level band and faction of a proven
// plan, the tank/healer/damage roles, the four-bot composition with one role
// left open for a player, the open-role rotation, how many groups the bots
// online can sustain, and which proven plan forms next.
//
// Everything that decides here is a pure function of plain facts, so
// tools/test_bot_lfg_selection_policy.cpp exercises it standalone: the
// candidate check, the band and team checks, candidate order, the full and
// open compositions, the role pick, the rotation, the group count, the form
// cadence and the plan pick. The glue is runtime/BotLfgService.cpp.
//
// Plain data and the standard library only.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace TortoiseBots {
namespace BotLfg {

enum class Role : uint8_t { Tank, Healer, Damage, None };

inline char const* RoleName(Role role)
{
    switch (role)
    {
        case Role::Tank:
            return "tank";
        case Role::Healer:
            return "healer";
        case Role::Damage:
            return "damage";
        default:
            return "none";
    }
}

constexpr uint8_t kAlliance = 1, kHorde = 2;

inline char const* TeamName(uint8_t team)
{
    if (team == kAlliance)
        return "alliance";
    if (team == kHorde)
        return "horde";
    return "any";
}

constexpr uint32_t kFormIntervalMs = 60000;
constexpr uint8_t kMinBotLevel = 10;

struct CandidateFacts
{
    bool liveHeadless = false;
    bool worldBot = false;
    bool playerMaster = false;
    bool hardcore = false;
    bool grouped = false;
    bool bound = false;
    bool leaseFree = false;
    bool lftQueued = false;
    bool battleground = false;
    bool onInstanceMap = false;
    bool alive = false;
    bool busy = false;
    uint8_t level = 0;
};

enum class Skip : uint8_t
{
    None,
    NotLive,
    NotWorldBot,
    PlayerMaster,
    Hardcore,
    Grouped,
    Bound,
    Leased,
    Queued,
    Battleground,
    InInstance,
    Dead,
    Busy,
    LowLevel
};

inline Skip CheckCandidate(CandidateFacts const& f)
{
    if (!f.liveHeadless)
        return Skip::NotLive;
    if (!f.worldBot)
        return Skip::NotWorldBot;
    if (f.playerMaster)
        return Skip::PlayerMaster;
    if (f.hardcore)
        return Skip::Hardcore;
    if (f.grouped)
        return Skip::Grouped;
    if (f.bound)
        return Skip::Bound;
    if (!f.leaseFree)
        return Skip::Leased;
    if (f.lftQueued)
        return Skip::Queued;
    if (f.battleground)
        return Skip::Battleground;
    if (f.onInstanceMap)
        return Skip::InInstance;
    if (!f.alive)
        return Skip::Dead;
    if (f.busy)
        return Skip::Busy;
    if (f.level < kMinBotLevel)
        return Skip::LowLevel;
    return Skip::None;
}

inline char const* SkipCode(Skip skip)
{
    switch (skip)
    {
        case Skip::None:
            return "ok";
        case Skip::NotLive:
            return "not-live";
        case Skip::NotWorldBot:
            return "not-world-bot";
        case Skip::PlayerMaster:
            return "player-master";
        case Skip::Hardcore:
            return "hardcore";
        case Skip::Grouped:
            return "grouped";
        case Skip::Bound:
            return "bound";
        case Skip::Leased:
            return "leased";
        case Skip::Queued:
            return "queued";
        case Skip::Battleground:
            return "battleground";
        case Skip::InInstance:
            return "in-instance";
        case Skip::Dead:
            return "dead";
        case Skip::Busy:
            return "busy";
        default:
            return "low-level";
    }
}

inline bool InBand(uint8_t level, uint8_t minLevel, uint8_t maxLevel)
{
    return level >= minLevel && level <= maxLevel;
}

inline bool TeamAllowed(uint8_t teamsMask, uint8_t team)
{
    return team != 0 && (teamsMask & team);
}

struct Candidate
{
    uint64_t guid = 0;
    uint8_t level = 0;
    uint8_t team = 0;
    bool tank = false;
    bool healer = false;
    bool damage = false;
    bool sameMap = false;
    float distance = 0.0f;
};

inline void SortCandidates(std::vector<Candidate>& candidates)
{
    std::sort(candidates.begin(), candidates.end(), [](Candidate const& a, Candidate const& b) {
        if (a.sameMap != b.sameMap)
            return a.sameMap;
        if (a.sameMap && b.sameMap && a.distance != b.distance)
            return a.distance < b.distance;
        return a.guid < b.guid;
    });
}

inline int PickForRole(std::vector<Candidate> const& sorted, Role role, std::vector<bool> const& taken)
{
    if (role == Role::None)
        return -1;
    for (size_t i = 0; i < sorted.size(); ++i)
    {
        if (taken[i])
            continue;
        Candidate const& c = sorted[i];
        bool pure = false;
        switch (role)
        {
            case Role::Tank:
                pure = c.tank && !c.healer;
                break;
            case Role::Healer:
                pure = c.healer && !c.tank;
                break;
            default:
                pure = c.damage && !c.tank && !c.healer;
                break;
        }
        if (pure)
            return static_cast<int>(i);
    }
    for (size_t i = 0; i < sorted.size(); ++i)
    {
        if (taken[i])
            continue;
        Candidate const& c = sorted[i];
        bool able = role == Role::Tank ? c.tank : (role == Role::Healer ? c.healer : c.damage);
        if (able)
            return static_cast<int>(i);
    }
    return -1;
}

struct Pick
{
    size_t index = 0;
    Role role = Role::None;
};

struct Composition
{
    bool ok = false;
    std::vector<Pick> picks;
    int leader = -1;
};

inline int LeaderOf(std::vector<Pick> const& picks)
{
    int firstHealer = -1;
    int firstDamage = -1;
    for (size_t i = 0; i < picks.size(); ++i)
    {
        if (picks[i].role == Role::Tank)
            return static_cast<int>(i);
        if (firstHealer < 0 && picks[i].role == Role::Healer)
            firstHealer = static_cast<int>(i);
        if (firstDamage < 0 && picks[i].role == Role::Damage)
            firstDamage = static_cast<int>(i);
    }
    return firstHealer >= 0 ? firstHealer : firstDamage;
}

inline Composition ComposeFull(std::vector<Candidate> const& sorted)
{
    static Role const kOrder[] = {Role::Tank, Role::Healer, Role::Damage, Role::Damage, Role::Damage};
    Composition out;
    std::vector<bool> taken(sorted.size(), false);
    for (Role role : kOrder)
    {
        int at = PickForRole(sorted, role, taken);
        if (at < 0)
            return out;
        taken[static_cast<size_t>(at)] = true;
        out.picks.push_back(Pick{static_cast<size_t>(at), role});
    }
    out.ok = true;
    out.leader = LeaderOf(out.picks);
    return out;
}

inline Composition ComposeWithOpen(std::vector<Candidate> const& sorted, Role open)
{
    Composition out = ComposeFull(sorted);
    if (!out.ok || open == Role::None)
        return out;
    for (size_t i = out.picks.size(); i-- > 0;)
    {
        if (out.picks[i].role == open)
        {
            out.picks.erase(out.picks.begin() + static_cast<std::ptrdiff_t>(i));
            break;
        }
    }
    out.leader = LeaderOf(out.picks);
    return out;
}

inline Role OpenRoleFor(uint32_t formedCount)
{
    static Role const kRotation[] = {Role::Damage, Role::Tank, Role::Damage, Role::Healer};
    return kRotation[formedCount % 4];
}

inline uint32_t TargetGroups(uint32_t botsOnline, uint32_t per100, uint32_t maxGroups, uint32_t realPlayersOnline,
    bool onlyWithPlayers)
{
    if (onlyWithPlayers && realPlayersOnline == 0)
        return 0;
    uint64_t target = uint64_t(botsOnline) * per100 / 100;
    return target < maxGroups ? static_cast<uint32_t>(target) : maxGroups;
}

inline bool FormDue(uint32_t activeGroups, uint32_t target, uint32_t sinceLastFormMs)
{
    return activeGroups < target && sinceLastFormMs >= kFormIntervalMs;
}

struct PlanOption
{
    std::string plan;
    uint8_t team = 0;
    bool feasible = false;
    bool busy = false;
    bool playerInBand = false;
    bool everFormed = false;
    uint32_t sinceFormedMs = 0;
    uint32_t candidates = 0;
};

inline int PickPlan(std::vector<PlanOption> const& options)
{
    int best = -1;
    for (size_t i = 0; i < options.size(); ++i)
    {
        PlanOption const& o = options[i];
        if (!o.feasible || o.busy)
            continue;
        if (best < 0)
        {
            best = static_cast<int>(i);
            continue;
        }
        PlanOption const& b = options[best];
        bool better = false;
        bool worse = false;
        if (o.playerInBand != b.playerInBand)
        {
            better = o.playerInBand;
            worse = !o.playerInBand;
        }
        else if (o.everFormed != b.everFormed)
        {
            better = !o.everFormed;
            worse = o.everFormed;
        }
        else if (o.sinceFormedMs != b.sinceFormedMs)
        {
            better = o.sinceFormedMs > b.sinceFormedMs;
            worse = o.sinceFormedMs < b.sinceFormedMs;
        }
        else if (o.candidates != b.candidates)
        {
            better = o.candidates > b.candidates;
            worse = o.candidates < b.candidates;
        }
        else if (o.plan != b.plan)
        {
            better = o.plan < b.plan;
            worse = o.plan > b.plan;
        }
        else if (o.team != b.team)
        {
            better = o.team < b.team;
            worse = o.team > b.team;
        }
        if (better && !worse)
            best = static_cast<int>(i);
    }
    return best;
}

} // namespace BotLfg
} // namespace TortoiseBots
