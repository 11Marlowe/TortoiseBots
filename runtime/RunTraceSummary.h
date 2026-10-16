#pragma once

// Run trace, summary layer (issue #986): the fields of <run_id>.summary.json
// that the dashboard's Dungeons tab reads (tools/observability/internal/
// dungeons): outcome, bosses with kill times, wipes and deaths with positions,
// clear_s, mode, build and driver.
//
// Plain data and the standard library only (plus the plan table's pure header),
// so tools/test_run_trace_summary.cpp builds it without a game header. The
// recorder (runtime/RunTrace.cpp) feeds a RunDigest from events it already sees
// (a bot's death edge in the frame sample, a boss corpse found by guid, the run
// leader's plan); nothing here reads the database or scans the world, and the
// JSON is rendered once, when the summary is written. Field meanings:
// docs/guides/run-trace.md, "Summary fields for the dashboard".
//
// Every field is additive: the dashboard reads a summary tolerantly, and an
// older summary without them still parses.

#include "RunTraceRecords.h"
#include "../ai/playerbot/DungeonPlanPolicy.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace TortoiseBots {
namespace RunTrace {

// Bosses and spots kept per run (the dashboard reads at most 40 and 200).
constexpr size_t kMaxBosses = 40;
constexpr size_t kMaxSpots = 200;
// `driver=` value: <model>:<effort>:<run>, at most this long.
constexpr size_t kMaxDriverLen = 96;
// A party needs this many bots in scope for "everyone is dead" to be a wipe
// (one traced bot dying is a death, not a wipe).
constexpr size_t kWipeMinBots = 2;

// ---------------------------------------------------------------------------
// Mode: who drives the run. Ordered by how much we know, so a later, stronger
// sign (the run leader starting after the trace did) wins.
// ---------------------------------------------------------------------------

enum class RunMode : uint8_t
{
    Observe = 0,    // a player plays; the trace only watches
    Commander = 1,  // a bound remote commander commands the group
    RunLeader = 2   // the module's run leader (`.bot run`) walks the plan
};

inline char const* ModeName(RunMode m)
{
    switch (m)
    {
        case RunMode::Commander: return "commander";
        case RunMode::RunLeader: return "run_leader";
        default: return "observe";
    }
}

inline RunMode StrongerMode(RunMode a, RunMode b)
{
    return static_cast<uint8_t>(a) >= static_cast<uint8_t>(b) ? a : b;
}

// ---------------------------------------------------------------------------
// Driver: `.bot trace start <run_id> ... driver=<model>:<effort>:<run>`.
// Self-reported, never verified. Letters, digits and . _ - : + only.
// ---------------------------------------------------------------------------

inline bool IsDriverToken(std::string_view token)
{
    constexpr char kKey[] = "driver=";
    if (token.size() < sizeof(kKey) - 1)
        return false;
    for (size_t i = 0; i < sizeof(kKey) - 1; ++i)
    {
        char c = token[i];
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
        if (c != kKey[i])
            return false;
    }
    return true;
}

// Null when `value` is a usable driver, else why not.
inline char const* DriverRefusal(std::string_view value)
{
    if (value.empty())
        return "driver= needs a value: driver=<model>:<effort>:<run>";
    if (value.size() > kMaxDriverLen)
        return "driver= is too long (at most 96 characters).";
    for (char c : value)
    {
        bool const ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '.' || c == '_' || c == '-' || c == ':' || c == '+';
        if (!ok)
            return "driver= takes letters, digits and . _ - : + only.";
    }
    return nullptr;
}

// What the summary says when no driver was given: a player-led run is "human",
// anything else "unknown" (issue #986).
inline std::string EffectiveDriver(std::string const& given, RunMode mode)
{
    if (!given.empty())
        return given;
    return mode == RunMode::Observe ? "human" : "unknown";
}

// ---------------------------------------------------------------------------
// Plan bosses: the boss list a run is judged against.
// ---------------------------------------------------------------------------

struct BossProgress
{
    uint32_t entry = 0;
    std::string name;
    int64_t killedMs = -1;  // ms since the run started; -1 = not killed (yet)
};

struct Spot
{
    uint32_t tMs = 0;
    uint32_t map = 0;
    float x = 0, y = 0, z = 0;
    std::string bot;   // deaths: the bot's name
    std::string role;  // tank | heal | dps (a wipe: the role of the first to fall)
    uint32_t dead = 0; // wipes: how many bots were dead
};

// What the glue knows about the run besides what the recorder sees.
struct Context
{
    RunMode mode = RunMode::Observe;
    std::string planName;
    std::vector<uint32_t> bossEntries;  // the plan's boss steps, in plan order
};

// The plan's bosses for a party of `team` (ai::kPlanFlagAlliance / Horde, 0 =
// both): the `entry` of every Boss step, each once.
inline std::vector<uint32_t> PlanBossEntries(ai::DungeonPlan const& plan, std::uint32_t team)
{
    std::vector<uint32_t> out;
    for (ai::PlanStep const& step : plan.steps)
    {
        if (step.kind != ai::PlanStepKind::Boss || step.entry == 0 || !ai::StepForTeam(step, team))
            continue;
        if (std::find(out.begin(), out.end(), step.entry) == out.end() && out.size() < kMaxBosses)
            out.push_back(step.entry);
    }
    return out;
}

// ---------------------------------------------------------------------------
// Outcome. Against the plan's boss list:
//   cleared    every boss on the list was killed (needs a list: a run without
//              a plan never claims a full clear)
//   partial    at least one boss was killed, not all
//   abandoned  no boss was killed
// ("abandoned" is the dashboard's name for an aborted run.)
// ---------------------------------------------------------------------------

enum class Outcome : uint8_t { Cleared, Partial, Abandoned };

inline char const* OutcomeLabel(Outcome o)
{
    switch (o)
    {
        case Outcome::Cleared: return "cleared";
        case Outcome::Partial: return "partial";
        default: return "abandoned";
    }
}

inline Outcome DecideOutcome(bool planKnown, size_t bossesTotal, size_t bossesKilled)
{
    if (bossesKilled == 0)
        return Outcome::Abandoned;
    if (planKnown && bossesTotal > 0 && bossesKilled >= bossesTotal)
        return Outcome::Cleared;
    return Outcome::Partial;
}

// One traced bot, as the frame sample saw it, for wipe detection.
struct MemberState
{
    bool alive = true;
    Spot lastDeath;  // where it last died (read only when it is dead)
};

// ---------------------------------------------------------------------------
// The digest of one run, in memory.
// ---------------------------------------------------------------------------

class RunDigest
{
public:
    void Reset()
    {
        *this = RunDigest();
    }

    void SetDriver(std::string d) { driver_ = std::move(d); }
    void SetBuild(std::string b) { build_ = std::move(b); }
    void NoteMode(RunMode m) { mode_ = StrongerMode(mode_, m); }
    RunMode Mode() const { return mode_; }

    // The party stands on an instance map (a dungeon or raid): only then does
    // the summary carry an outcome (an open-world trace is not a dungeon run).
    void NoteDungeon(uint32_t map) { if (map) dungeonMap_ = map; }
    uint32_t DungeonMap() const { return dungeonMap_; }

    bool HasPlan() const { return hasPlan_; }
    // Adopts a plan's boss list once (the first one wins), merging any boss the
    // recorder saw before the plan was known.
    void NotePlan(std::string const& name, std::vector<BossProgress> bosses)
    {
        if (hasPlan_ || bosses.empty())
            return;
        for (BossProgress const& seen : bosses_)
            for (BossProgress& planned : bosses)
                if (planned.entry == seen.entry && planned.killedMs < 0)
                    planned.killedMs = seen.killedMs;
        hasPlan_ = true;
        plan_ = name;
        bosses_ = std::move(bosses);
        if (bosses_.size() > kMaxBosses)
            bosses_.resize(kMaxBosses);
    }

    // Whether `entry` is on the boss list (with a plan).
    bool IsPlanBoss(uint32_t entry) const
    {
        return hasPlan_ && Find(entry) != nullptr;
    }

    // Without a plan the boss list is whatever boss the bots met. Returns true
    // when it is new.
    bool NoteSeenBoss(uint32_t entry, std::string const& name)
    {
        if (hasPlan_ || entry == 0 || Find(entry) || bosses_.size() >= kMaxBosses)
            return false;
        BossProgress b;
        b.entry = entry;
        b.name = name;
        bosses_.push_back(std::move(b));
        return true;
    }

    // First kill of a boss wins. False for an unknown boss or a repeat.
    bool NoteBossKill(uint32_t entry, uint32_t tMs)
    {
        BossProgress* b = Find(entry);
        if (!b || b->killedMs >= 0)
            return false;
        b->killedMs = tMs;
        return true;
    }

    void NoteDeath(Spot spot)
    {
        if (deaths_.size() < kMaxSpots)
            deaths_.push_back(std::move(spot));
    }

    // Once per frame with every traced bot: everyone dead (at least
    // kWipeMinBots bots) is one wipe, until somebody is alive again. The spot
    // is the centroid of where they fell. True when a wipe was recorded.
    bool NoteFrame(uint32_t tMs, std::vector<MemberState> const& members)
    {
        bool allDead = members.size() >= kWipeMinBots;
        for (MemberState const& m : members)
            if (m.alive)
            {
                allDead = false;
                break;
            }
        if (!allDead)
        {
            if (!members.empty())
                wipeActive_ = false;
            return false;
        }
        if (wipeActive_)
            return false;
        wipeActive_ = true;
        Spot s;
        s.tMs = tMs;
        s.map = members[0].lastDeath.map;
        MemberState const* first = &members[0];
        for (MemberState const& m : members)
        {
            if (m.lastDeath.tMs < first->lastDeath.tMs)
                first = &m;
            s.x += m.lastDeath.x;
            s.y += m.lastDeath.y;
            s.z += m.lastDeath.z;
        }
        float const n = static_cast<float>(members.size());
        s.x /= n;
        s.y /= n;
        s.z /= n;
        s.dead = static_cast<uint32_t>(members.size());
        s.role = first->lastDeath.role;  // the first to fall (the dashboard colours a wipe by it)
        if (wipes_.size() < kMaxSpots)
            wipes_.push_back(std::move(s));
        return true;
    }

    size_t BossCount() const { return bosses_.size(); }
    size_t BossesKilled() const
    {
        size_t n = 0;
        for (BossProgress const& b : bosses_)
            if (b.killedMs >= 0)
                ++n;
        return n;
    }
    size_t WipeCount() const { return wipes_.size(); }
    size_t DeathCount() const { return deaths_.size(); }
    std::vector<BossProgress> const& Bosses() const { return bosses_; }

    // The run counts as a dungeon run: it stood on an instance map, or it
    // knows a boss.
    bool IsDungeonRun() const { return dungeonMap_ != 0 || !bosses_.empty(); }

    // The summary members (no braces, no leading comma), rendered once when
    // the summary is written. Keys: mode, build, driver, plan, dungeon,
    // outcome, bosses, clear_s, wipes, deaths.
    std::string Render() const
    {
        std::string out;
        auto add = [&out](std::string const& member)
        {
            if (!out.empty())
                out += ',';
            out += member;
        };
        auto str = [](char const* key, std::string const& v)
        {
            return std::string("\"") + key + "\":\"" + JsonEscape(v) + "\"";
        };
        add(str("mode", ModeName(mode_)));
        add(str("build", build_));
        add(str("driver", EffectiveDriver(driver_, mode_)));
        if (hasPlan_ && !plan_.empty())
            add(str("plan", plan_));
        if (dungeonMap_)
            add("\"dungeon\":{\"map\":" + std::to_string(dungeonMap_) + "}");
        if (IsDungeonRun())
        {
            size_t const killed = BossesKilled();
            Outcome const outcome = DecideOutcome(hasPlan_, bosses_.size(), killed);
            add(str("outcome", OutcomeLabel(outcome)));
            std::string bosses = "\"bosses\":[";
            int64_t lastKill = 0;
            for (size_t i = 0; i < bosses_.size(); ++i)
            {
                BossProgress const& b = bosses_[i];
                if (i)
                    bosses += ',';
                bosses += "{\"entry\":" + std::to_string(b.entry) + ",\"name\":\"" + JsonEscape(b.name) +
                    "\",\"killed_at_s\":" + (b.killedMs >= 0 ? FixedStr(static_cast<double>(b.killedMs) / 1000.0, 1) : "null") + "}";
                lastKill = std::max(lastKill, b.killedMs);
            }
            bosses += ']';
            add(bosses);
            if (outcome == Outcome::Cleared)
                add("\"clear_s\":" + FixedStr(static_cast<double>(lastKill) / 1000.0, 1));
        }
        add("\"wipes\":" + SpotsJson(wipes_, false));
        add("\"deaths\":" + SpotsJson(deaths_, true));
        return out;
    }

private:
    BossProgress* Find(uint32_t entry)
    {
        for (BossProgress& b : bosses_)
            if (b.entry == entry)
                return &b;
        return nullptr;
    }
    BossProgress const* Find(uint32_t entry) const
    {
        for (BossProgress const& b : bosses_)
            if (b.entry == entry)
                return &b;
        return nullptr;
    }

    static std::string SpotsJson(std::vector<Spot> const& spots, bool deaths)
    {
        std::string out = "[";
        for (size_t i = 0; i < spots.size(); ++i)
        {
            Spot const& s = spots[i];
            if (i)
                out += ',';
            out += "{\"t_s\":" + FixedStr(static_cast<double>(s.tMs) / 1000.0, 1) + ",\"map\":" + std::to_string(s.map) +
                ",\"x\":" + FixedStr(s.x, 1) + ",\"y\":" + FixedStr(s.y, 1) + ",\"z\":" + FixedStr(s.z, 1);
            if (deaths)
            {
                out += ",\"bot\":\"" + JsonEscape(s.bot) + "\"";
                if (!s.role.empty())
                    out += ",\"role\":\"" + JsonEscape(s.role) + "\"";
            }
            else
            {
                if (!s.role.empty())
                    out += ",\"role\":\"" + JsonEscape(s.role) + "\"";
                out += ",\"dead\":" + std::to_string(s.dead);
            }
            out += '}';
        }
        out += ']';
        return out;
    }

    std::string driver_;
    std::string build_;
    RunMode mode_ = RunMode::Observe;
    uint32_t dungeonMap_ = 0;
    bool hasPlan_ = false;
    std::string plan_;
    std::vector<BossProgress> bosses_;
    std::vector<Spot> wipes_;
    std::vector<Spot> deaths_;
    bool wipeActive_ = false;
};

} // namespace RunTrace
} // namespace TortoiseBots
