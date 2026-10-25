#pragma once

// Player-led (hybrid) dungeon runs (owner 2026-10-23, ~/TortoiseBots-audit/PLAYER-LED-HYBRID-DESIGN-2026-10-23.md):
// a real player leads his own party of bots (and up to three other players) and walks wherever he wants; the run
// leader (runtime/DungeonRunPolicy.h) follows where he stands and does the pulls, raid marks, crowd control, boss
// tactics, rests and revives. Started with `.bot run start [plan] player [tank <Bot>]`, behind
// AiPlayerbot.DungeonRunPlayerLed (default 0) on top of AiPlayerbot.DungeonSelfClear (the run's orders are given by the run leader).

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
#include <functional>
#include <map>
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
    bool enabled = false;               // DungeonRunPlayerLed and DungeonSelfClear on
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
        case Refusal::Disabled: return "The player-led run is off (AiPlayerbot.DungeonRunPlayerLed = 1 and AiPlayerbot.DungeonSelfClear = 1 turn it on).";
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

// Review round 1: the bots a player-led binding commands are the ones the commander controls (his own, his hires,
// his claims). A bot that joins the party later and is not his (another player's companion) is a party member the
// run never orders, like a real player. A bot-led binding commands every live bot of its group, as before.
inline bool CommandsBot(bool playerLed, bool commanderControls)
{
    return !playerLed || commanderControls;
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
    // S4: where the glue last read the player (MovedSince).
    float lastX = 0.0f, lastY = 0.0f, lastZ = 0.0f;
    bool lastKnown = false;
    // S5: the player away (off the plan's map) or down (a ghost): the run put the bots on stay where they stood
    // (held, and why); awayMs counts the time away, awayMinutes is AiPlayerbot.DungeonRunPlayerLedAwayMinutes.
    enum class Held : std::uint8_t { None, Away, Down };
    Held held = Held::None;
    std::uint32_t awayMs = 0;
    std::uint32_t awayMinutes = 10;
    std::uint32_t team = 0; // the party's team as last read (AllDone leaves the other team's steps out)
    // S6: the step done last (an event's chain goes on from it) and the steps the path check turned down, with the
    // time since (they are asked again after kPathRetryMs).
    std::size_t lastDone = static_cast<std::size_t>(-1);
    std::map<std::size_t, std::uint32_t> pathRefused; // step -> ms since the path check turned it down
    // Review round 1: the event chain is walking its gate moves (an exact move, a move_direct and its launch, a
    // move naming an area trigger): those moves go out (ChainMove).
    bool inChain = false;
    // ... the straggler ladder of the start gate: how long a bot has stood farther than kBotsNearYd from the
    // player, and the summons sent this step (by name), at most kStraggleSummons each.
    std::uint32_t straggleMs = 0;
    std::map<std::string, std::uint32_t> straggleSummons;
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
        {
            pl.stay = false;
            pl.held = State::Held::None; // his own follow ends the run's hold too
        }
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
    std::string farthestBot;    // ... its name (the straggler ladder's summon)
    std::uint32_t team = 0;     // the party's team (ai::kPlanFlagAlliance / kPlanFlagHorde, 0 unknown)
};

// A unit in combat with the party (on a bot, a pet or a player), as the glue reads it.
struct Engaged
{
    std::uint32_t entry = 0;
    float x = 0.0f, y = 0.0f, z = 0.0f;
};

// S4: the player moved since the last read (more than kMovedYd), and the read is kept for the next tick. The first
// read is no move. The glue also counts the client's own moving flag.
constexpr float kMovedYd = 0.5f;

inline bool MovedSince(State& pl, float x, float y, float z)
{
    bool const moved = pl.lastKnown && ai::PlanDistance(pl.lastX, pl.lastY, pl.lastZ, x, y, z) > kMovedYd;
    pl.lastX = x;
    pl.lastY = y;
    pl.lastZ = z;
    pl.lastKnown = true;
    return moved;
}

inline bool IsFight(const PlanStep& s)
{
    return s.kind == PlanStepKind::Pull || s.kind == PlanStepKind::Boss;
}

// S6: the plan's object and event steps the run does near the player (owner: "the run does them").
inline bool IsTask(const PlanStep& s)
{
    switch (s.kind)
    {
        case PlanStepKind::Open:
        case PlanStepKind::UseItem:
        case PlanStepKind::UseOn:
        case PlanStepKind::Ritual:
        case PlanStepKind::Gossip:
        case PlanStepKind::WaitObject:
        case PlanStepKind::WaitClear:
        case PlanStepKind::Hold:
            return true;
        default:
            return false;
    }
}

// S5: a later step of the plan uses this item (an open or use_on's `item`, a use_item's `entry`, any one's `param`).
inline bool NeededLater(const ai::DungeonPlan& plan, std::size_t idx, std::uint32_t item)
{
    if (!item)
        return false;
    for (std::size_t i = idx + 1; i < plan.steps.size(); ++i)
    {
        const PlanStep& s = plan.steps[i];
        bool const usesItem = s.kind == PlanStepKind::Open || s.kind == PlanStepKind::UseOn;
        if ((usesItem && s.item == item) || (s.kind == PlanStepKind::UseItem && s.entry == item) ||
            ((usesItem || s.kind == PlanStepKind::UseItem) && s.param == item))
            return true;
    }
    return false;
}

// The steps a player-led run does: pulls and bosses (S3), object and event steps (S6), and a loot step only for a
// key item a later step needs (S5: the group's loot is the players'; every other loot step is left out). Moves and
// rest steps never: the player walks, the rest gate runs before every step.
inline bool Pickable(const ai::DungeonPlan& plan, std::size_t i)
{
    if (i >= plan.steps.size())
        return false;
    const PlanStep& s = plan.steps[i];
    if (IsFight(s) || IsTask(s))
        return true;
    return s.kind == PlanStepKind::Loot && NeededLater(plan, i, s.item);
}

// A use_item step has no place (an item used from the bags): it is near wherever the player stands.
inline bool Placeless(const PlanStep& s)
{
    return s.kind == PlanStepKind::UseItem && s.x == 0.0f && s.y == 0.0f && s.z == 0.0f;
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

inline bool Open(const DungeonRun::Run& r, const State& pl, std::size_t i)
{
    return r.plan && Pickable(*r.plan, i) && !pl.done.count(i);
}

// S6: an object, event or loot step waits for every pickable step before it in its leg (the plan's leg order): the
// packs that guard it, the door before the lever, the ritual before its hold. Pulls and bosses wait for nothing.
inline bool Gated(const DungeonRun::Run& r, const State& pl, std::size_t i)
{
    const PlanStep& s = r.plan->steps[i];
    if (IsFight(s))
        return false;
    for (std::size_t j = 0; j < i; ++j)
        if (r.plan->steps[j].leg == s.leg && Open(r, pl, j) && (!pl.team || ai::StepForTeam(r.plan->steps[j], pl.team)))
            return true;
    // A hold is never picked first: its event's start step (the ritual, open, use or gossip before it, any leg) is
    // done first, or the chain takes it (FollowChain). A hold with no start step waits for its leg only.
    if (s.kind == PlanStepKind::Hold)
    {
        std::size_t const start = DungeonRun::EventStartStep(*r.plan, i);
        if (start != i && !pl.done.count(start))
            return true;
    }
    return false;
}

// Where a step is done: a loot step at the corpse its boss step saw down (Run::corpses), else the plan's point.
inline void StepSpot(const DungeonRun::Run& r, std::size_t i, float& x, float& y, float& z)
{
    const PlanStep& s = r.plan->steps[i];
    x = s.x;
    y = s.y;
    z = s.z;
    if (s.kind != PlanStepKind::Loot)
        return;
    auto const corpse = r.corpses.find(s.entry);
    if (corpse != r.corpses.end() && corpse->second.guid)
    {
        x = corpse->second.x;
        y = corpse->second.y;
        z = corpse->second.z;
    }
}

inline bool SpotNear(const DungeonRun::Run& r, std::size_t i, const Anchor& a, float yd, float* outYd = nullptr)
{
    if (Placeless(r.plan->steps[i]))
    {
        if (outYd)
            *outYd = 0.0f;
        return true;
    }
    float x = 0.0f, y = 0.0f, z = 0.0f;
    StepSpot(r, i, x, y, z);
    float const d = ai::PlanDistance(x, y, z, a.x, a.y, a.z);
    if (outYd)
        *outYd = d;
    return Dz(z, a.z) <= kNearDz && d <= yd;
}

// S6: the cheap path check of the pick (the design: a path no longer than 1.5 times the straight line, so a pack
// behind a wall or on the floor below is not pulled). noPath: no navmesh path; pathYd <= 0: not read (no check).
constexpr float kPathRatio = 1.5f;
constexpr std::uint32_t kPathRetryMs = 10000; // a step the check turned down is asked again after this
constexpr int kPathChecksPerTick = 2;
constexpr std::uint32_t kStraggleMs = 15000;     // a bot this long beyond kBotsNearYd from the player is summoned to him
constexpr std::uint32_t kStraggleSummons = 2;    // ... at most this often a step per bot

inline bool PathShortEnough(bool noPath, float pathYd, float straightYd)
{
    if (noPath)
        return false;
    if (pathYd <= 0.0f)
        return true;
    return pathYd <= kPathRatio * std::max(straightYd, 1.0f);
}

// The glue's path check of one candidate step: true when the party may go there (PathShortEnough). Empty: no check.
using PathCheck = std::function<bool(std::size_t step)>;

// Rule (a): the unfinished pull or boss step whose units the party fights (by entry, near the step's point). The
// player pulled it, or it came to him. The nearest such step; kNone when none.
inline std::size_t EngagedStep(const DungeonRun::Run& r, const State& pl, const std::vector<Engaged>& engaged)
{
    std::size_t best = kNone;
    float bestYd = 0.0f;
    for (std::size_t i = 0; r.plan && i < r.plan->steps.size(); ++i)
    {
        if (!Open(r, pl, i) || !IsFight(r.plan->steps[i]) ||
            (pl.team && !ai::StepForTeam(r.plan->steps[i], pl.team)))
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

// Rule (b): the nearest open step (a pull or boss; an object, event or loot step whose leg gates are done) whose
// spot is within maxYd of the player (straight line, same floor) and is for the party's team. With a path check the
// nearest candidates are asked (at most kPathChecksPerTick a tick; a refused one is not asked again for
// kPathRetryMs). kNone when none.
inline std::size_t NearestStep(const DungeonRun::Run& r, State& pl, const Anchor& a, float maxYd,
    const PathCheck& pathOk = PathCheck())
{
    std::vector<std::pair<float, std::size_t>> candidates;
    for (std::size_t i = 0; r.plan && i < r.plan->steps.size(); ++i)
    {
        if (!Open(r, pl, i) || Gated(r, pl, i) || pl.pathRefused.count(i))
            continue;
        const PlanStep& s = r.plan->steps[i];
        if (a.team && !ai::StepForTeam(s, a.team))
            continue;
        float yd = 0.0f;
        if (!SpotNear(r, i, a, maxYd, &yd))
            continue;
        candidates.push_back({yd, i});
    }
    std::sort(candidates.begin(), candidates.end());
    int asked = 0;
    for (const auto& c : candidates)
    {
        if (!pathOk || Placeless(r.plan->steps[c.second]))
            return c.second;
        if (asked++ >= kPathChecksPerTick)
            return kNone; // asked enough this tick: the next tick goes on
        if (pathOk(c.second))
            return c.second;
        pl.pathRefused[c.second] = 0;
    }
    return kNone;
}

inline bool ForTeam(const DungeonRun::Run& r, const State& pl, std::size_t i)
{
    return !pl.team || ai::StepForTeam(r.plan->steps[i], pl.team);
}

// Every open step for the party's team is done (another team's pack is never pulled, so it does not hold the end).
inline bool AllDone(const DungeonRun::Run& r, const State& pl)
{
    for (std::size_t i = 0; r.plan && i < r.plan->steps.size(); ++i)
        if (Open(r, pl, i) && ForTeam(r, pl, i))
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

// The run moved off the picked step (DungeonRun::NextStep: cleared, skipped, left out of reach): it is done. A run
// sent back to an earlier step (an event that failed or a wipe in a hold: DungeonRun::EventFailed, RearmEventOnWipe
// go back to the event's start step) follows it: that step and the ones up to the picked one are open again.
inline void NoteDone(DungeonRun::Run& r, State& pl)
{
    if (pl.picked == kNone || r.step == pl.picked)
        return;
    if (r.step < pl.picked)
    {
        for (std::size_t i = r.step; i <= pl.picked; ++i)
            pl.done.erase(i);
        pl.picked = r.step;
        return;
    }
    pl.done.insert(pl.picked);
    pl.lastDone = pl.picked;
    pl.picked = kNone;
}

// S6: an event's chain goes on by itself. When the step just done started an event (a ritual, an open, a use, a
// gossip) and the run's next step is that event's hold, or the moves leading to it, the hold is the current step
// at once (the moves are left out: the player brought the party). An open's wait_object right after it is taken
// the same way. True when it took one.
// Review round 1 (Codex): a move between an event's start and its hold that the event needs is a gate move and is
// walked, not left out: an exact waypoint, a move_direct (a gap the navmesh does not bridge), the move right before
// a move_direct (its launch), and a move naming an area trigger (entry, fix/plan-gossip-allow). Plain route moves
// are passive: the player brought the party.
inline bool GateMove(const ai::DungeonPlan& plan, std::size_t i)
{
    if (i >= plan.steps.size())
        return false;
    const PlanStep& s = plan.steps[i];
    if (s.kind == PlanStepKind::MoveDirect)
        return true;
    if (s.kind != PlanStepKind::Move)
        return false;
    if (s.entry != 0 || (s.flags & ai::kPlanFlagExact) || s.exactLaunch)
        return true;
    return i + 1 < plan.steps.size() && plan.steps[i + 1].kind == PlanStepKind::MoveDirect;
}

inline bool FollowChain(DungeonRun::Run& r, State& pl)
{
    if (pl.picked != kNone || pl.lastDone == kNone || r.step != pl.lastDone + 1 || !r.plan ||
        r.state != DungeonRun::State::Running)
    {
        if (pl.picked == kNone)
            pl.inChain = false;
        return false;
    }
    const PlanStep& doneStep = r.plan->steps[pl.lastDone];
    const PlanStep* cur = DungeonRun::Current(r);
    if (!cur)
        return false;
    bool const starts = doneStep.kind == PlanStepKind::Ritual || doneStep.kind == PlanStepKind::Open ||
        doneStep.kind == PlanStepKind::UseItem || doneStep.kind == PlanStepKind::Gossip ||
        doneStep.kind == PlanStepKind::UseOn;
    // ... or a gate move of the chain just walked: the chain goes on from it.
    bool const chainMove = pl.inChain &&
        (doneStep.kind == PlanStepKind::Move || doneStep.kind == PlanStepKind::MoveDirect);
    if (!starts && !chainMove)
    {
        pl.inChain = false;
        return false;
    }
    if (doneStep.kind == PlanStepKind::Open && cur->kind == PlanStepKind::WaitObject && !pl.done.count(r.step))
    {
        pl.picked = r.step;
        return true;
    }
    if (!DungeonRun::EventHoldAhead(r) && cur->kind != PlanStepKind::Hold)
    {
        pl.inChain = false;
        return false;
    }
    while ((cur = DungeonRun::Current(r)) != nullptr &&
        (cur->kind == PlanStepKind::Move || cur->kind == PlanStepKind::MoveDirect))
    {
        if (GateMove(*r.plan, r.step))
        {
            // Walked and checked by the bot-led move step (its exact, direct and trigger rules) before the hold.
            pl.picked = r.step;
            pl.inChain = true;
            r.note = "the event's way: step " + std::to_string(r.step + 1) + " (" + ai::PlanStepRowKindName(*cur) +
                ") is walked before the hold";
            return true;
        }
        DungeonRun::NextStep(r, "the event's moves are left out: you brought the party");
    }
    pl.inChain = false;
    if (!cur || cur->kind != PlanStepKind::Hold || pl.done.count(r.step))
        return false;
    pl.picked = r.step;
    r.note = "the event is on: holding " + std::string("step ") + std::to_string(r.step + 1);
    return true;
}

// The chain's gate move is the current step: its walk goes out (the one place besides the hold's spot where the run
// moves the party in a player-led run).
inline bool ChainMove(const DungeonRun::Run& r, const State& pl)
{
    const PlanStep* s = DungeonRun::Current(r);
    return pl.inChain && pl.picked == r.step && s &&
        (s->kind == PlanStepKind::Move || s->kind == PlanStepKind::MoveDirect);
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

inline void NewPick(State& pl)
{
    pl.straggleMs = 0;
    pl.straggleSummons.clear();
}

inline std::string StepName(const DungeonRun::Run& r, std::size_t i)
{
    const PlanStep& s = r.plan->steps[i];
    std::string const& leg = r.plan->LegName(s.leg);
    std::string const what = s.kind == PlanStepKind::Boss ? "boss" : s.kind == PlanStepKind::Pull ? "pack"
                                                                                                 : ai::PlanStepKindName(s.kind);
    return what + " (step " + std::to_string(i + 1) +
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
inline bool Pick(DungeonRun::Run& r, State& pl, const Anchor& a, const std::vector<Engaged>& engaged,
    const PathCheck& pathOk = PathCheck())
{
    if (!pl.on || !r.plan || r.state != DungeonRun::State::Running)
        return false;
    std::size_t const stepBefore = r.step;
    if (a.team)
        pl.team = a.team;
    NoteDone(r, pl);
    if (!pl.started)
    {
        pl.started = true;
        for (std::size_t i = 0; i < r.step && i < r.plan->steps.size(); ++i)
            if (Pickable(*r.plan, i))
                pl.done.insert(i);
    }
    if (FollowChain(r, pl))
        return r.step != stepBefore;
    // The event under way or leading in (the picked step is in one): the step stays. With nothing picked the
    // cursor is stale and says nothing (an event on was taken by FollowChain).
    if (r.fighting || r.regroup || !r.hold.empty() || (pl.picked != kNone && DungeonRun::InEvent(r)) ||
        (pl.picked != kNone && r.eventOn))
        return false;
    std::size_t const fought = engaged.empty() ? kNone : EngagedStep(r, pl, engaged);
    if (pl.picked != kNone)
    {
        if (!BetweenPulls(r))
            return false;
        if (BeforeItsPull(r) && fought != kNone && fought != pl.picked)
        {
            Jump(r, fought);
            NewPick(pl);
            pl.picked = fought;
            r.note = "the party fights " + StepName(r, fought) + ": taken as the current step";
            return true;
        }
        float const dropYd = std::max(kDropYd, pl.pullYd + 20.0f);
        if (a.read && !a.partyInCombat && !SpotNear(r, pl.picked, a, dropYd))
            pl.picked = kNone; // he walked on: the pack waits for him (not done)
        else
            return false;
    }
    std::size_t idx = fought;
    if (idx == kNone && a.read && a.onMap && a.alive && !a.ghost && pl.held == State::Held::None)
        idx = NearestStep(r, pl, a, pl.pullYd, pathOk);
    if (idx == kNone)
        return false;
    Jump(r, idx);
    NewPick(pl);
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
    // The nearest step the run would take (a pack, or an object step whose gates are done), and the nearest pack.
    std::size_t best = kNone, pack = kNone;
    float bestYd = 0.0f, packYd = 0.0f;
    for (std::size_t i = 0; r.plan && i < r.plan->steps.size(); ++i)
    {
        if (!Open(r, pl, i) || (a.team && !ai::StepForTeam(r.plan->steps[i], a.team)))
            continue;
        float yd = 0.0f;
        SpotNear(r, i, a, 0.0f, &yd);
        if (IsFight(r.plan->steps[i]) && (pack == kNone || yd < packYd))
        {
            pack = i;
            packYd = yd;
        }
        if (!Gated(r, pl, i) && (best == kNone || yd < bestYd))
        {
            best = i;
            bestYd = yd;
        }
    }
    if (best == kNone)
        return "waiting: no open step for your team";
    float x = 0.0f, y = 0.0f, z = 0.0f;
    StepSpot(r, best, x, y, z);
    std::string note = "next: " + StepName(r, best) + ", " + DungeonRun::Num(bestYd) + " yd " + Compass(a, x, y) +
        " (the run does it when you stand within " + DungeonRun::Num(pl.pullYd) + " yd of it)";
    // A door, lever or event between him and the packs: say so (the next packs may lie behind it).
    if (!IsFight(r.plan->steps[best]) && pack != kNone && pack > best)
        note += "; the packs of later steps may lie behind it (nearest pack: " + StepName(r, pack) + ", " +
            DungeonRun::Num(packYd) + " yd)";
    return note;
}

// An order the bot-led step machine gave that a player-led run never sends: a walk of the party or the tank
// (`move`, a bot's `walk`, the island `escape`, the stranded `rescue` teleport): the player owns movement. A `rest`
// while the player walks is not sent either (the bots follow him; the gate keeps the next pull until rested).
// S6: the one walk a player-led run sends itself (besides the task orders, whose bots walk to their object or NPC
// on their own: `cmd open`, `use`, `ritual`, `gossip`, `loot`): the tank onto the current hold's own spot, which the
// event needs (DungeonRun::HoldStep). The player is never moved.
inline bool BotWalkAllowed(const DungeonRun::Run& r, const std::string& order)
{
    const PlanStep* s = DungeonRun::Current(r);
    return s && s->kind == PlanStepKind::Hold && order == DungeonRun::MoveOrder(s->x, s->y, s->z, false);
}

inline bool Dropped(const std::string& order, const Anchor& a, const DungeonRun::Run* r = nullptr,
    const State* pl = nullptr)
{
    if (r && BotWalkAllowed(*r, order))
        return false;
    if (r && pl && ChainMove(*r, *pl) && (order.compare(0, 5, "move ") == 0 || order.compare(0, 5, "walk ") == 0))
        return false;
    for (const char* head : {"move ", "walk ", "escape ", "rescue "})
        if (order.compare(0, std::char_traits<char>::length(head), head) == 0)
            return true;
    return order == "rest" && a.moving;
}

// S5: the player down (a ghost) or away (off the plan's dungeon). "-" when neither holds the tick (the tick goes
// on), else the tick's order ("" or one `stay` / `follow`). A fight goes on as ever (the bots' fight, the run's
// bounds while a step is picked); a body (dead, not released) is waited for without a stay, so a resurrector of
// the party can reach it.
inline std::string DownOrAway(DungeonRun::Run& r, State& pl, const DungeonRun::Facts& f, const Anchor& a,
    std::uint32_t dt)
{
    bool const fight = f.anyInCombat && pl.picked != kNone;
    if (!a.read)
        return pl.held == State::Held::None ? "-" : std::string();
    // Down first (review round 1, owner rule "the party holds where the player fell"): dead or a ghost, in the
    // instance or out of it (a released ghost walks from a graveyard outside), he is down, not away. The away clock
    // does not run while he is dead.
    if (a.ghost || !a.alive)
    {
        pl.awayMs = 0;
        if (fight)
            return "-";
        if (!a.ghost)
        {
            r.note = "you are dead: waiting for a resurrection (a bot of the party may raise you)";
            return std::string();
        }
        r.note = "you are a ghost: the party holds where you fell; come back to it";
        if (pl.held != State::Held::Down)
        {
            bool const send = pl.held == State::Held::None;
            pl.held = State::Held::Down;
            if (send)
                return "stay";
        }
        return std::string();
    }
    // Alive and out of the instance: away (after a fall too: the party keeps holding where he fell).
    if (!a.onMap)
    {
        pl.awayMs += dt;
        std::uint32_t const capMs = pl.awayMinutes * 60000u;
        if (pl.awayMs >= capMs)
        {
            r.state = DungeonRun::State::Done;
            r.note = "stopped: you were away from the plan's dungeon for " + std::to_string(pl.awayMinutes) +
                " min (the bots stay where they hold)";
            return std::string();
        }
        if (fight)
            return "-";
        std::uint32_t const leftMin = (capMs - pl.awayMs + 59999u) / 60000u;
        r.note = std::string(pl.held == State::Held::Down ? "you are alive outside the dungeon: the party holds where you fell"
                                                           : "paused: you left the plan's dungeon: the bots hold here") +
            "; back within " + std::to_string(leftMin) + " min the run goes on, then it stops";
        if (pl.held == State::Held::None)
        {
            pl.held = State::Held::Away;
            return "stay";
        }
        return std::string();
    }
    pl.awayMs = 0;
    if (pl.held == State::Held::None)
        return "-";
    // Back inside after a fall: the party follows him again only once he stands within kBotsNearYd of it (never
    // across the instance from the portal); until then the note says where it holds.
    if (pl.held == State::Held::Down && a.botsMaxYd > kBotsNearYd)
    {
        float const yd = ai::PlanDistance(a.x, a.y, a.z, f.leadX, f.leadY, f.leadZ);
        r.note = "you are alive: your party holds where you fell, " + DungeonRun::Num(yd) + " yd " +
            Compass(a, f.leadX, f.leadY) + " of you: come back to it";
        return std::string();
    }
    pl.held = State::Held::None;
    r.note = "you are back: the bots follow you again";
    return "follow";
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
    if (a.team)
        pl.team = a.team;
    // Each refused step is asked again kPathRetryMs after its own refusal.
    for (auto it = pl.pathRefused.begin(); it != pl.pathRefused.end();)
    {
        it->second += dt;
        it = it->second >= kPathRetryMs ? pl.pathRefused.erase(it) : std::next(it);
    }
    if (r.state != DungeonRun::State::Running)
        return DungeonRun::Tick(r, in, dt, lines);
    // The anchor is the player: no walk back, no anchor walk, no tank walk back to its guard stand.
    r.walkBackDue = false;
    r.walkBack = false;
    r.guardReturn = false;
    r.anchorKnown = false;
    NoteDone(r, pl);
    FollowChain(r, pl);
    // S5: away from the plan's dungeon, or a ghost. Out of a fight the bots hold where they stand ("stay" once);
    // back (or alive again with his bots near), they follow him again ("follow" once). Away longer than
    // awayMinutes: the run stops with a note (the glue releases the binding; the bots keep their stay).
    std::string const down = DownOrAway(r, pl, in, a, dt);
    if (down != "-")
        return down;
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
    // The gate holds a step at its start: a pull, and an object or event step before its order (the event's own
    // waves and its hold are never held).
    const PlanStep* cur = DungeonRun::Current(r);
    bool const quiet = !f.anyInCombat && !r.regroup && !r.fighting && r.hold.empty() && !r.eventOn &&
        cur && cur->kind != PlanStepKind::Hold && !ChainMove(r, pl) && !r.orderSent &&
        (r.phase == DungeonRun::Phase::Begin || r.phase == DungeonRun::Phase::Gather) && f.dead == 0 &&
        DungeonRun::PartyDead(f) == 0;
    if (quiet)
    {
        // Review round 1: the straggler ladder on the player. A bot farther than kBotsNearYd from him for
        // kStraggleMs is summoned to him (cmd summon: the point is the player), at most kStraggleSummons times a
        // step; then the note names it.
        if (!playerFight && a.read && a.alive && !a.ghost && a.onMap && !pl.stay && !a.moving &&
            a.botsMaxYd > kBotsNearYd && !a.farthestBot.empty())
        {
            pl.straggleMs += dt;
            if (pl.straggleMs >= kStraggleMs)
            {
                pl.straggleMs = 0;
                std::uint32_t& sent = pl.straggleSummons[a.farthestBot];
                if (sent < kStraggleSummons)
                {
                    ++sent;
                    r.note = "summoning " + a.farthestBot + " to you (" + DungeonRun::Num(a.botsMaxYd) + " yd away for " +
                        std::to_string(kStraggleMs / 1000) + " s)";
                    return "summon " + a.farthestBot;
                }
                r.note = "waiting for " + a.farthestBot + " (" + DungeonRun::Num(a.botsMaxYd) +
                    " yd from you, summoned " + std::to_string(kStraggleSummons) + " times): bring it to you, or .bot run skip";
                return std::string();
            }
        }
        else
            pl.straggleMs = 0;
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
    if (!order.empty() && Dropped(order, a, &r, &pl))
    {
        r.note += (r.note.empty() ? "" : "; ") + std::string("not sent: ") + order.substr(0, order.find(' ')) +
            " (you lead the way)";
        order.clear();
    }
    // The bot-led machine ends the run after the plan's last step; a player-led run ends when every pack is down.
    std::size_t const before = pl.picked;
    NoteDone(r, pl);
    FollowChain(r, pl);
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
    // The player asked: the nearest open step for his team, a gated one too (a step behind another team's boss or
    // a door the party cannot open is skipped this way); no path check.
    std::size_t idx = kNone;
    float bestYd = 0.0f;
    for (std::size_t i = 0; r.plan && i < r.plan->steps.size(); ++i)
    {
        if (!Open(r, pl, i) || !ForTeam(r, pl, i))
            continue;
        float yd = 0.0f;
        SpotNear(r, i, a, 0.0f, &yd);
        if (idx == kNone || yd < bestYd)
        {
            idx = i;
            bestYd = yd;
        }
    }
    if (idx == kNone)
        return false;
    pl.done.insert(idx);
    r.notCleared.insert(idx);
    r.note = "skipped " + StepName(r, idx);
    return true;
}

} // namespace PlayerLed
} // namespace TortoiseBots
