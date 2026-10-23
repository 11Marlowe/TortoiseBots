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
#include <cstdint>
#include <string>
#include <vector>

namespace TortoiseBots {
namespace PlayerLed {

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

} // namespace PlayerLed
} // namespace TortoiseBots
