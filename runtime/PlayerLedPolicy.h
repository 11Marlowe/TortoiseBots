#pragma once

// Player-led (hybrid) dungeon runs (owner 2026-10-23, ~/TortoiseBots-audit/PLAYER-LED-HYBRID-DESIGN-2026-10-23.md):
// a real player leads his own party of bots (and up to three other players) and walks wherever he wants; the run
// leader (runtime/DungeonRunPolicy.h) follows where he stands and does the pulls, raid marks, crowd control, boss
// tactics, rests and revives. Started with `.bot run start [plan] player [tank <Bot>]`, behind
// AiPlayerbot.DungeonRunPlayerLed (default 0) on top of AiPlayerbot.DungeonSelfClear and AiPlayerbot.RemoteCommander
// (the run's orders are `.bot cmd` orders).
//
// Kept apart from DungeonRunPolicy.h on purpose: the bot-led step machine is unchanged, and this header only adds
// what the player-led mode decides (the start words, the bind gate, and from slice S2 on how a player's order is
// classed, from S3 on which plan step is current). Plain data and the standard library only, so
// tools/test_player_led_policy.cpp runs it standalone.

#include "DungeonRunPolicy.h"
#include "RemoteCommanderPolicy.h"

#include <cctype>
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace TortoiseBots {
namespace PlayerLed {

using ai::PlanStep;
using ai::PlanStepKind;

inline std::string Lower(std::string v)
{
    for (char& c : v)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return v;
}

// ---------------------------------------------------------------------------
// S1: the start words
// ---------------------------------------------------------------------------

// `.bot run start [plan] [from <leg|step N>] player [tank <Bot>]`: the word `player` (any case) asks for the player-led
// mode; `tank <Bot>` after it names the tank bot. rest: the words before `player`, for DungeonRun::ParseStartArgs
// (lower case, as the glue hands that parser its argument). ok false: a word after `player` that is not
// `tank <Bot>`, or `tank` without a name. A start without the word is untouched (playerLed false, rest the input).
struct StartArgs
{
    bool playerLed = false;
    bool ok = true;
    std::string tank; // as typed (a bot name is matched without case by the glue)
    std::string rest;
};

inline StartArgs TakeStart(const std::string& raw)
{
    StartArgs out;
    std::vector<std::string> words;
    std::string word;
    for (char c : raw)
    {
        if (c == ' ' || c == '\t')
        {
            if (!word.empty())
                words.push_back(word);
            word.clear();
        }
        else
            word += c;
    }
    if (!word.empty())
        words.push_back(word);
    std::size_t at = words.size();
    for (std::size_t i = 0; i < words.size(); ++i)
        if (Lower(words[i]) == "player")
        {
            at = i;
            break;
        }
    for (std::size_t i = 0; i < at; ++i)
        out.rest += (out.rest.empty() ? "" : " ") + Lower(words[i]);
    if (at == words.size())
        return out;
    out.playerLed = true;
    std::size_t const after = words.size() - at - 1;
    if (after == 0)
        return out;
    if (after == 2 && Lower(words[at + 1]) == "tank")
    {
        out.tank = words[at + 2];
        return out;
    }
    out.ok = false;
    return out;
}

// ---------------------------------------------------------------------------
// S1: the bind gate
// ---------------------------------------------------------------------------

// One member of the commander's party besides himself: a bot, or another real player (owner 2026-10-23: 1 to 4
// players, the rest bots).
struct Member
{
    std::string name;
    bool bot = false;           // a module bot (a real player otherwise: he stays, he is never ordered)
    bool controllable = false;  // CanControlBot: the commander's own bot, his hire, his claim, or a GM's
    bool liveBot = false;       // live module-owned Headless bot with an AI
    bool hired = false;         // hired by somebody other than the commander (RemoteCommander::HireBlocksBind)
    bool inOtherBinding = false;
    bool tankRole = false;      // PlayerbotAI::IsTank
};

struct BindFacts
{
    bool enabled = false;               // DungeonRunPlayerLed, DungeonSelfClear and RemoteCommander all on
    bool commanderIsBot = false;
    bool commanderAlreadyBound = false; // a binding of his stands (a player-led one left over is released first)
    bool grouped = false;               // the commander is in a party
    bool raidOrBg = false;
    bool commanderLeads = false;        // ... and leads it
    bool groupBound = false;            // another commander's binding holds that group
    std::vector<Member> members;        // every member but the commander
    std::string tankName;               // `tank <Bot>`, empty: the first tank-role bot
};

enum class Refusal : std::uint8_t
{
    None,
    Disabled,
    CommanderIsBot,
    AlreadyBound,
    NotGrouped,
    Raid,
    NotLeader,
    GroupBound,
    NoBots,
    TooLarge,
    NotControllable,
    NotLive,
    Hired,
    OtherBinding,
    TankNotInParty,
    TankNotBot,
    NoTank,
};

struct BindVerdict
{
    Refusal refusal = Refusal::None;
    int tank = -1; // index into BindFacts::members
};

// Fail closed: every bot of the party is the commander's to control, no member is in another binding, the
// commander leads a party (never a raid) of at most five, and one bot is the tank.
inline BindVerdict CheckBind(const BindFacts& f)
{
    BindVerdict v;
    auto refuse = [&](Refusal r) { v.refusal = r; v.tank = -1; return v; };
    if (!f.enabled)
        return refuse(Refusal::Disabled);
    if (f.commanderIsBot)
        return refuse(Refusal::CommanderIsBot);
    if (f.commanderAlreadyBound)
        return refuse(Refusal::AlreadyBound);
    if (!f.grouped)
        return refuse(Refusal::NotGrouped);
    if (f.raidOrBg)
        return refuse(Refusal::Raid);
    if (!f.commanderLeads)
        return refuse(Refusal::NotLeader);
    if (f.groupBound)
        return refuse(Refusal::GroupBound);
    if (f.members.size() + 1 > RemoteCommander::kMaxGroupSize)
        return refuse(Refusal::TooLarge);
    std::size_t bots = 0;
    for (const Member& m : f.members)
    {
        if (!m.bot)
            continue;
        ++bots;
        if (m.hired)
            return refuse(Refusal::Hired);
        if (!m.controllable)
            return refuse(Refusal::NotControllable);
        if (!m.liveBot)
            return refuse(Refusal::NotLive);
        if (m.inOtherBinding)
            return refuse(Refusal::OtherBinding);
    }
    if (bots == 0)
        return refuse(Refusal::NoBots);
    if (!f.tankName.empty())
    {
        std::string const want = Lower(f.tankName);
        for (std::size_t i = 0; i < f.members.size(); ++i)
            if (Lower(f.members[i].name) == want)
            {
                if (!f.members[i].bot)
                    return refuse(Refusal::TankNotBot);
                v.tank = static_cast<int>(i);
                return v;
            }
        return refuse(Refusal::TankNotInParty);
    }
    for (std::size_t i = 0; i < f.members.size(); ++i)
        if (f.members[i].bot && f.members[i].tankRole)
        {
            v.tank = static_cast<int>(i);
            return v;
        }
    return refuse(Refusal::NoTank);
}

inline const char* RefusalCode(Refusal r)
{
    switch (r)
    {
        case Refusal::None: return "ok";
        case Refusal::Disabled: return "disabled";
        case Refusal::CommanderIsBot: return "commander-is-bot";
        case Refusal::AlreadyBound: return "already-bound";
        case Refusal::NotGrouped: return "not-grouped";
        case Refusal::Raid: return "raid";
        case Refusal::NotLeader: return "not-leader";
        case Refusal::GroupBound: return "group-bound";
        case Refusal::NoBots: return "no-bots";
        case Refusal::TooLarge: return "group-too-large";
        case Refusal::NotControllable: return "not-controllable";
        case Refusal::NotLive: return "not-live";
        case Refusal::Hired: return "hired";
        case Refusal::OtherBinding: return "bot-bound";
        case Refusal::TankNotInParty: return "tank-not-in-party";
        case Refusal::TankNotBot: return "tank-not-bot";
        case Refusal::NoTank: return "no-tank";
    }
    return "rejected";
}

inline const char* RefusalText(Refusal r)
{
    switch (r)
    {
        case Refusal::None: return "";
        case Refusal::Disabled: return "The player-led run is off (AiPlayerbot.DungeonRunPlayerLed = 1, AiPlayerbot.DungeonSelfClear = 1 and AiPlayerbot.RemoteCommander = 1 turn it on).";
        case Refusal::CommanderIsBot: return "A bot cannot lead a player-led run.";
        case Refusal::AlreadyBound: return "You command a bound group already: .bot commander unbind first (a player-led run makes its own binding).";
        case Refusal::NotGrouped: return "A player-led run needs your own party: invite your bots first.";
        case Refusal::Raid: return "A player-led run is for a party, not a raid.";
        case Refusal::NotLeader: return "You must lead the party to start a player-led run.";
        case Refusal::GroupBound: return "Another commander holds this party.";
        case Refusal::NoBots: return "Your party has no bot for the run to lead.";
        case Refusal::TooLarge: return "A party is five at most.";
        case Refusal::NotControllable: return "Your party holds a bot you do not control.";
        case Refusal::NotLive: return "A bot of your party is not ready (offline or between maps).";
        case Refusal::Hired: return "Your party holds another player's hired companion: only your own bots may be in the run.";
        case Refusal::OtherBinding: return "A bot of your party is in another commander's group binding.";
        case Refusal::TankNotInParty: return "The named tank is not in your party.";
        case Refusal::TankNotBot: return "The named tank is a player: name one of your bots (tank <Bot>).";
        case Refusal::NoTank: return "No tank bot in your party: name one with tank <Bot>.";
    }
    return "Request rejected.";
}

// ---------------------------------------------------------------------------
// The player-led state the glue keeps beside the run (DungeonRunSlot)
// ---------------------------------------------------------------------------

struct State
{
    bool on = false;   // this run is player-led
    std::string tank;  // the tank bot's name (the binding's lead bot)
    // S2: the player's orders. stay: the bots stand on his `stay` (no new pull until his `follow` or `come`);
    // playerFight: his own fight order (attack, pull, ...) since the last tick: the run's pending pull is dropped
    // and the fight is his (the next tick adopts the plan step of what the party fights).
    bool stay = false;
    bool playerFight = false;
    std::string lastOrder; // the last order he gave, for the status note
    // S3: the step picker. done: the plan's pull and boss steps judged cleared (or skipped, or left out of reach):
    // it takes the place of the bot-led forward cursor. picked: the step the run works on (Run::step), or none
    // (idle: the player walks, nothing near). stillMs: how long the player has stood still. pullYd: the
    // AiPlayerbot.DungeonRunPlayerLedPullYd the glue read at the start.
    std::set<std::size_t> done;
    std::size_t picked = static_cast<std::size_t>(-1);
    std::uint32_t stillMs = 0;
    float pullYd = 40.0f;
    bool started = false; // the steps before a partway start (`from`) are taken as done on the first pick
};

// ---------------------------------------------------------------------------
// S2: the player's own orders never pause a player-led run
// ---------------------------------------------------------------------------

// The design (section 4): movement orders are obeyed while the run goes on; a fight order is the player's pull; a
// control order (rest, release, revive, a corpse run) is his to give; anything else is noted. Only `.bot run pause`
// and `.bot run stop` pause or stop a player-led run.
enum class OrderClass : std::uint8_t { Move, Fight, Control, Other };

inline OrderClass ClassifyOrder(const std::string& word)
{
    std::string const w = Lower(word);
    for (const char* m : {"follow", "stay", "come", "formation", "flee", "guard", "move", "passive"})
        if (w == m)
            return OrderClass::Move;
    for (const char* m : {"attack", "pull", "pullback", "focus", "cc", "aoe", "sap", "tank", "taunt", "interrupt",
             "assist", "kill"})
        if (w == m)
            return OrderClass::Fight;
    for (const char* m : {"rest", "release", "revive", "corpse", "summon", "eat", "drink", "resurrect", "rez"})
        if (w == m)
            return OrderClass::Control;
    return OrderClass::Other;
}

inline const char* OrderClassName(OrderClass c)
{
    switch (c)
    {
        case OrderClass::Move: return "move";
        case OrderClass::Fight: return "fight";
        case OrderClass::Control: return "control";
        case OrderClass::Other: return "other";
    }
    return "other";
}

// The order word of the glue's labels (PauseDungeonRunForPlayer's `order`): "action <intent> ...", "cmd <verb> ...",
// "chat '<word>' to <bot>". Lower case; empty when none.
inline std::string OrderWord(const std::string& label)
{
    auto firstWord = [](const std::string& s) {
        std::size_t const b = s.find_first_not_of(" \t");
        if (b == std::string::npos)
            return std::string();
        std::size_t const e = s.find_first_of(" \t", b);
        return s.substr(b, e == std::string::npos ? std::string::npos : e - b);
    };
    if (label.compare(0, 7, "action ") == 0)
        return Lower(firstWord(label.substr(7)));
    if (label.compare(0, 4, "cmd ") == 0)
        return Lower(firstWord(label.substr(4)));
    if (label.compare(0, 6, "chat '") == 0)
    {
        std::size_t const end = label.find('\'', 6);
        return Lower(firstWord(label.substr(6, end == std::string::npos ? std::string::npos : end - 6)));
    }
    return Lower(firstWord(label));
}

// The player's order to his party while a player-led run runs (instead of DungeonRun::PauseForPlayer, which the
// bot-led run keeps). The run is never paused here. True when the run took note of it (it runs or is paused).
//   - Move: obeyed by the bots; `stay` holds the run's next pull until `follow` or `come`.
//   - Fight: the player's pull. A pull of the run's own that is gathering or not yet answered by a fight is
//     dropped back to the step's start (the bots' fight and the next tick's step pick take it from there).
//   - Control and other orders: obeyed by the bots, noted.
inline bool NotePlayerOrder(State& pl, DungeonRun::Run& r, const std::string& label)
{
    if (!pl.on || r.state == DungeonRun::State::Done)
        return false;
    std::string const word = OrderWord(label);
    OrderClass const c = ClassifyOrder(word);
    pl.lastOrder = word;
    if (c == OrderClass::Move)
    {
        if (word == "stay")
            pl.stay = true;
        else if (word == "follow" || word == "come")
            pl.stay = false;
    }
    if (c == OrderClass::Fight)
    {
        pl.playerFight = true;
        if (r.state == DungeonRun::State::Running &&
            (r.phase == DungeonRun::Phase::Gather || r.phase == DungeonRun::Phase::AwaitPull))
            DungeonRun::EnterPhase(r, DungeonRun::Phase::Begin);
    }
    if (r.state == DungeonRun::State::Running)
        r.note = "your order (" + (word.empty() ? std::string("?") : word) + ", " + OrderClassName(c) +
            "): obeyed, the run goes on" + (pl.stay ? " (no pull while the bots stay: follow to go on)" : "");
    return true;
}

// `.bot run mode` and the start's answer.
inline std::string ModeText(const State& pl, bool runExists)
{
    if (!runExists)
        return "no run";
    if (!pl.on)
        return "bot-led: the lead bot walks the plan";
    return "player-led: you walk, the run pulls" + (pl.tank.empty() ? std::string() : " (tank " + pl.tank + ")");
}

// ---------------------------------------------------------------------------
// S3: where the player stands picks the step; the run never walks
// ---------------------------------------------------------------------------

constexpr std::size_t kNone = static_cast<std::size_t>(-1);
constexpr std::uint32_t kStillMs = 2000;      // the player stands still this long before the run pulls (owner Q2)
constexpr float kBotsNearYd = 20.0f;          // ... and every living bot is this close to him (the gather waits, it never moves him)
constexpr float kDropYd = 60.0f;              // a picked pack whose pull is not out yet is dropped once he is farther than this from it
constexpr float kNearDz = 15.0f;              // a pack more than this above or below him is on another floor: not near
constexpr float kEngagedSlackYd = 15.0f;      // an engaged unit within the step's radius plus this of its point belongs to it

// The leading player as the glue reads him once a tick (never ordered: the run only looks at him).
struct Anchor
{
    bool read = false;          // found in the world
    float x = 0.0f, y = 0.0f, z = 0.0f;
    bool onMap = true;          // on the plan's map
    bool alive = true;
    bool ghost = false;
    bool moving = false;        // he moved since the last tick (or is moving now)
    bool partyInCombat = false; // a living bot of the party (or its pet) is in combat
    float botsMaxYd = 0.0f;     // the living bot farthest from him (the gather's measure)
    std::uint32_t team = 0;     // the party's team (ai::kPlanFlagAlliance / kPlanFlagHorde, 0 unknown)
};

// A unit in combat with the party (on a bot, a pet or a player), as the glue reads it.
struct Engaged
{
    std::uint32_t entry = 0;
    float x = 0.0f, y = 0.0f, z = 0.0f;
};

inline bool Pickable(const PlanStep& s)
{
    return s.kind == PlanStepKind::Pull || s.kind == PlanStepKind::Boss;
}

inline bool StepHasEntry(const PlanStep& s, std::uint32_t entry)
{
    if (s.kind == PlanStepKind::Boss && s.entry == entry)
        return true;
    for (std::uint32_t e : s.entries)
        if (e == entry)
            return true;
    return false;
}

inline float Dz(float a, float b)
{
    return a > b ? a - b : b - a;
}

inline float YdFrom(const PlanStep& s, const Anchor& a)
{
    return ai::PlanDistance(s.x, s.y, s.z, a.x, a.y, a.z);
}

inline bool Near(const PlanStep& s, const Anchor& a, float yd)
{
    return Dz(s.z, a.z) <= kNearDz && YdFrom(s, a) <= yd;
}

inline bool Open(const DungeonRun::Run& r, const State& pl, std::size_t i)
{
    return r.plan && i < r.plan->steps.size() && Pickable(r.plan->steps[i]) && !pl.done.count(i);
}

// Rule (a): the unfinished pull or boss step whose units the party fights (by entry, near the step's point). The
// player pulled it, or it came to him. The nearest such step; kNone when none.
inline std::size_t EngagedStep(const DungeonRun::Run& r, const State& pl, const std::vector<Engaged>& engaged)
{
    std::size_t best = kNone;
    float bestYd = 0.0f;
    for (std::size_t i = 0; r.plan && i < r.plan->steps.size(); ++i)
    {
        if (!Open(r, pl, i))
            continue;
        const PlanStep& s = r.plan->steps[i];
        float const reach = std::max(s.radius, ai::kPlanDefaultPackRadius) + kEngagedSlackYd;
        for (const Engaged& u : engaged)
        {
            if (!StepHasEntry(s, u.entry))
                continue;
            float const yd = ai::PlanDistance(s.x, s.y, s.z, u.x, u.y, u.z);
            if (yd <= reach && (best == kNone || yd < bestYd))
            {
                best = i;
                bestYd = yd;
            }
        }
    }
    return best;
}

// Rule (b): the nearest unfinished pull or boss step whose point is within maxYd of the player (straight line,
// same floor) and is for the party's team. kNone when none.
inline std::size_t NearestStep(const DungeonRun::Run& r, const State& pl, const Anchor& a, float maxYd)
{
    std::size_t best = kNone;
    float bestYd = 0.0f;
    for (std::size_t i = 0; r.plan && i < r.plan->steps.size(); ++i)
    {
        if (!Open(r, pl, i))
            continue;
        const PlanStep& s = r.plan->steps[i];
        if (a.team && !ai::StepForTeam(s, a.team))
            continue;
        if (!Near(s, a, maxYd))
            continue;
        float const yd = YdFrom(s, a);
        if (best == kNone || yd < bestYd)
        {
            best = i;
            bestYd = yd;
        }
    }
    return best;
}

inline bool AllDone(const DungeonRun::Run& r, const State& pl)
{
    for (std::size_t i = 0; r.plan && i < r.plan->steps.size(); ++i)
        if (Open(r, pl, i))
            return false;
    return true;
}

// Between the picked step's pulls: running, out of a fight, a regroup, a hold and an event, at the step's start or
// its gather (no pull out, no rest, no task).
inline bool BetweenPulls(const DungeonRun::Run& r)
{
    return r.state == DungeonRun::State::Running && !r.fighting && !r.regroup && r.hold.empty() &&
        !DungeonRun::InEvent(r) && (r.phase == DungeonRun::Phase::Begin || r.phase == DungeonRun::Phase::Gather);
}

// ... and no pull of it has gone out yet: the run may still change its mind about which pack it is.
inline bool BeforeItsPull(const DungeonRun::Run& r)
{
    return BetweenPulls(r) && r.pulls == 0;
}

// The run moved off the picked step (DungeonRun::NextStep: cleared, skipped, left out of reach): it is done.
inline void NoteDone(DungeonRun::Run& r, State& pl)
{
    if (pl.picked != kNone && r.step != pl.picked)
    {
        pl.done.insert(pl.picked);
        pl.picked = kNone;
    }
}

// The run goes to plan step idx (no walk: the party is where the player brought it), fresh as a new step.
inline void Jump(DungeonRun::Run& r, std::size_t idx)
{
    r.step = idx;
    r.pendingMissing = 0;
    r.autoRegroups = 0;
    r.fightMs = 0;
    DungeonRun::ClearApproachStand(r);
    DungeonRun::ResetStep(r);
}

inline std::string StepName(const DungeonRun::Run& r, std::size_t i)
{
    const PlanStep& s = r.plan->steps[i];
    std::string const& leg = r.plan->LegName(s.leg);
    return std::string(s.kind == PlanStepKind::Boss ? "boss" : "pack") + " (step " + std::to_string(i + 1) +
        ", leg " + (leg.empty() ? std::to_string(s.leg) : leg) + ")";
}

// Where a point lies from the player, as a compass word (the 1.12 world: +x north, +y west).
inline std::string Compass(const Anchor& a, float x, float y)
{
    float const north = x - a.x, east = a.y - y;
    if (std::fabs(north) < 1.0f && std::fabs(east) < 1.0f)
        return "here";
    static const char* names[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    float deg = std::atan2(east, north) * 57.29578f;
    if (deg < 0.0f)
        deg += 360.0f;
    return names[static_cast<int>((deg + 22.5f) / 45.0f) % 8];
}

// Before the glue reads the step's facts (it reads them for Run::step): the step the run works on this tick.
//   - Mid-fight, in a regroup, a hold or an event: the step stays.
//   - A picked step whose pull is not out yet is changed for the step of what the party fights (rule a); one
//     between its pulls is dropped (left open, not done) once the player is farther than kDropYd from it out of
//     combat.
//   - None picked: the step of what the party fights, else the nearest open pack or boss within pullYd of the
//     player (rule b). None: idle (the player walks, Tick only notes the next pack).
// True when Run::step changed.
inline bool Pick(DungeonRun::Run& r, State& pl, const Anchor& a, const std::vector<Engaged>& engaged)
{
    if (!pl.on || !r.plan || r.state != DungeonRun::State::Running)
        return false;
    NoteDone(r, pl);
    if (!pl.started)
    {
        pl.started = true;
        for (std::size_t i = 0; i < r.step && i < r.plan->steps.size(); ++i)
            if (Pickable(r.plan->steps[i]))
                pl.done.insert(i);
    }
    if (r.fighting || r.regroup || !r.hold.empty() || DungeonRun::InEvent(r))
        return false;
    std::size_t const fought = engaged.empty() ? kNone : EngagedStep(r, pl, engaged);
    if (pl.picked != kNone)
    {
        if (!BetweenPulls(r))
            return false;
        if (BeforeItsPull(r) && fought != kNone && fought != pl.picked)
        {
            Jump(r, fought);
            pl.picked = fought;
            r.note = "the party fights " + StepName(r, fought) + ": taken as the current step";
            return true;
        }
        float const dropYd = std::max(kDropYd, pl.pullYd + 20.0f);
        if (a.read && !a.partyInCombat && !Near(r.plan->steps[pl.picked], a, dropYd))
            pl.picked = kNone; // he walked on: the pack waits for him (not done)
        else
            return false;
    }
    std::size_t idx = fought;
    if (idx == kNone && a.read && a.onMap && a.alive && !a.ghost)
        idx = NearestStep(r, pl, a, pl.pullYd);
    if (idx == kNone)
        return false;
    Jump(r, idx);
    pl.picked = idx;
    return true;
}

// Why the picked step's pull waits for the player, or "" when it may go out. Only asked out of combat, between
// pulls, with nobody dead (the bot-led rules take everything else).
inline std::string NotReadyNote(const DungeonRun::Run& r, const State& pl, const DungeonRun::Facts& f, const Anchor& a)
{
    std::string const what = pl.picked != kNone ? StepName(r, pl.picked) : std::string("the pack");
    if (!a.read)
        return "holding: where you stand is not known";
    if (!a.onMap)
        return "holding: you are not in the plan's dungeon";
    if (!a.alive || a.ghost)
        return "holding: you are dead: a resurrection or your corpse run first";
    if (pl.stay)
        return "holding " + what + ": your bots stay (follow to go on)";
    if (a.moving || pl.stillMs < kStillMs)
        return "pull ready: " + what + ", stand still to pull";
    if (a.botsMaxYd > kBotsNearYd)
        return "pull ready: " + what + ", waiting for the bots to come within " + DungeonRun::Num(kBotsNearYd) +
            " yd of you (the farthest is " + DungeonRun::Num(a.botsMaxYd) + " yd)";
    if (!f.seen)
        return "pull ready: " + what + ", the tank does not see it yet: come closer";
    return std::string();
}

// The note while no step is picked: the next open pack or boss, how far and which way.
inline std::string IdleNote(const DungeonRun::Run& r, const State& pl, const Anchor& a)
{
    if (!a.read)
        return "waiting: where you stand is not known";
    if (!a.onMap)
        return "waiting: you are not in the plan's dungeon";
    std::size_t best = kNone;
    float bestYd = 0.0f;
    for (std::size_t i = 0; r.plan && i < r.plan->steps.size(); ++i)
    {
        if (!Open(r, pl, i) || (a.team && !ai::StepForTeam(r.plan->steps[i], a.team)))
            continue;
        float const yd = YdFrom(r.plan->steps[i], a);
        if (best == kNone || yd < bestYd)
        {
            best = i;
            bestYd = yd;
        }
    }
    if (best == kNone)
        return "waiting: no open pack for your team";
    const PlanStep& s = r.plan->steps[best];
    return "next: " + StepName(r, best) + ", " + DungeonRun::Num(bestYd) + " yd " + Compass(a, s.x, s.y) +
        " (pulled when you stand within " + DungeonRun::Num(pl.pullYd) + " yd of it)";
}

// An order the bot-led step machine gave that a player-led run never sends: a walk of the party or the tank
// (`move`, a bot's `walk`, the island `escape`, the stranded `rescue` teleport): the player owns movement. A `rest`
// while the player walks is not sent either (the bots follow him; the gate keeps the next pull until rested).
inline bool Dropped(const std::string& order, const Anchor& a)
{
    for (const char* head : {"move ", "walk ", "escape ", "rescue "})
        if (order.compare(0, std::char_traits<char>::length(head), head) == 0)
            return true;
    return order == "rest" && a.moving;
}

// The player-led tick in place of DungeonRun::Tick (the glue calls Pick first, then reads the facts for Run::step).
// The bot-led step machine runs the picked step as it runs any pull or boss (the rest gate, the cc marks, kill
// first, the fight ceilings, the regroup and revives); this only keeps it from walking and from pulling before the
// player stands still near his bots. Without a picked step nothing goes out.
inline std::string Tick(DungeonRun::Run& r, State& pl, const DungeonRun::Facts& in, const Anchor& a,
    std::uint32_t dt = DungeonRun::kTickMs, const DungeonRun::RestLines& lines = DungeonRun::RestLines())
{
    if (!pl.on)
        return DungeonRun::Tick(r, in, dt, lines);
    pl.stillMs = a.read && !a.moving ? pl.stillMs + dt : 0;
    bool const playerFight = pl.playerFight;
    pl.playerFight = false;
    if (r.state != DungeonRun::State::Running)
        return DungeonRun::Tick(r, in, dt, lines);
    // The anchor is the player: no walk back, no anchor walk, no tank walk back to its guard stand.
    r.walkBackDue = false;
    r.walkBack = false;
    r.guardReturn = false;
    r.anchorKnown = false;
    NoteDone(r, pl);
    if (pl.picked == kNone)
    {
        if (AllDone(r, pl))
        {
            r.state = DungeonRun::State::Done;
            std::string const counts = DungeonRun::WipeCounts(r);
            r.note = "every pack and boss of the plan is down" + (counts.empty() ? std::string() : " (" + counts + ")");
            return std::string();
        }
        r.note = IdleNote(r, pl, a);
        return std::string();
    }
    // The bots' distance for the gather is the player's (the run never gathers them on the tank).
    DungeonRun::Facts f = in;
    if (a.read)
        f.maxMemberYd = a.botsMaxYd;
    bool const quiet = !f.anyInCombat && !r.regroup && !r.fighting && r.hold.empty() && !DungeonRun::InEvent(r) &&
        (r.phase == DungeonRun::Phase::Begin || r.phase == DungeonRun::Phase::Gather) && f.dead == 0 &&
        DungeonRun::PartyDead(f) == 0;
    if (quiet)
    {
        std::string why = playerFight ? "your pull: the run waits for the fight" : NotReadyNote(r, pl, f, a);
        if (!why.empty())
        {
            if (r.phase != DungeonRun::Phase::Begin)
                DungeonRun::EnterPhase(r, DungeonRun::Phase::Begin);
            r.note = why;
            return std::string();
        }
    }
    std::string order = DungeonRun::Tick(r, f, dt, lines);
    if (!order.empty() && Dropped(order, a))
    {
        r.note += (r.note.empty() ? "" : "; ") + std::string("not sent: ") + order.substr(0, order.find(' ')) +
            " (you lead the way)";
        order.clear();
    }
    // The bot-led machine ends the run after the plan's last step; a player-led run ends when every pack is down.
    std::size_t const before = pl.picked;
    NoteDone(r, pl);
    if (r.state == DungeonRun::State::Done && !AllDone(r, pl))
    {
        r.state = DungeonRun::State::Running;
        r.note = (before != kNone ? StepName(r, before) + " done" : std::string("step done")) + ": on to the next one near you";
    }
    else if (r.state == DungeonRun::State::Done)
    {
        std::string const counts = DungeonRun::WipeCounts(r);
        r.note = "every pack and boss of the plan is down" + (counts.empty() ? std::string() : " (" + counts + ")");
    }
    return order;
}

// `.bot run skip` with no step picked: the open pack or boss nearest the player is left out (counted not cleared).
// True when one was. With a step picked the glue skips it as ever (DungeonRun::Skip; NoteDone marks it).
inline bool SkipNearest(DungeonRun::Run& r, State& pl, const Anchor& a)
{
    if (!pl.on || pl.picked != kNone || !a.read || r.state == DungeonRun::State::Done)
        return false;
    std::size_t const idx = NearestStep(r, pl, a, 100000.0f);
    if (idx == kNone)
        return false;
    pl.done.insert(idx);
    r.notCleared.insert(idx);
    r.note = "skipped " + StepName(r, idx);
    return true;
}

} // namespace PlayerLed
} // namespace TortoiseBots
