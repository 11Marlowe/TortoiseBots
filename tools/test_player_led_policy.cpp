// Standalone check: the player-led (hybrid) dungeon run (runtime/PlayerLedPolicy.h, 2026-10-24). A real player leads
// his own party and walks; the run leader pulls the plan's packs near him. S1: the start words (`player [tank <Bot>]`),
// the bind gate and the player-led binding's release rule.
//   g++ -std=c++17 -Wall -Wextra tools/test_player_led_policy.cpp -o /tmp/test_player_led_policy && /tmp/test_player_led_policy
#include "../runtime/PlayerLedPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

namespace PL = TortoiseBots::PlayerLed;
using namespace TortoiseBots::DungeonRun;
using ai::DungeonPlan;
using ai::PlanStep;
using ai::PlanStepKind;
namespace RC = TortoiseBots::RemoteCommander;

// ---------------------------------------------------------------------------
// S1: the start words
// ---------------------------------------------------------------------------

static void TestStartWords()
{
    // No `player`: untouched, bot-led (the words go on lower-cased, as before).
    PL::StartArgs a = PL::TakeStart("Uldaman from Keystone");
    CHECK(!a.playerLed && a.ok && a.rest == "uldaman from keystone");
    a = PL::TakeStart("");
    CHECK(!a.playerLed && a.ok && a.rest.empty());
    // `player` alone, after a plan, after `from`, in any case.
    a = PL::TakeStart("player");
    CHECK(a.playerLed && a.ok && a.rest.empty() && a.tank.empty());
    a = PL::TakeStart("uldaman PLAYER");
    CHECK(a.playerLed && a.ok && a.rest == "uldaman");
    a = PL::TakeStart("uldaman from step 12 player");
    CHECK(a.playerLed && a.ok && a.rest == "uldaman from step 12");
    // `tank <Bot>`: the name as typed.
    a = PL::TakeStart("uldaman player tank Tanky");
    CHECK(a.playerLed && a.ok && a.tank == "Tanky" && a.rest == "uldaman");
    a = PL::TakeStart("  player   Tank\tGrumbo ");
    CHECK(a.playerLed && a.ok && a.tank == "Grumbo");
    // Anything else after `player` is refused (no guess).
    CHECK(!PL::TakeStart("player tank").ok);
    CHECK(!PL::TakeStart("player Tanky").ok);
    CHECK(!PL::TakeStart("player tank Tanky now").ok);
    CHECK(!PL::TakeStart("player from 3").ok);
}

// ---------------------------------------------------------------------------
// S1: the bind gate
// ---------------------------------------------------------------------------

static PL::Member BotMember(const std::string& name, bool tank = false)
{
    PL::Member m;
    m.name = name;
    m.bot = true;
    m.controllable = true;
    m.liveBot = true;
    m.tankRole = tank;
    return m;
}

static PL::Member PlayerMember(const std::string& name)
{
    PL::Member m;
    m.name = name;
    return m;
}

static PL::BindFacts Party(std::vector<PL::Member> members)
{
    PL::BindFacts f;
    f.enabled = true;
    f.grouped = true;
    f.commanderLeads = true;
    f.members = std::move(members);
    return f;
}

static void TestBindPicksTheTank()
{
    // The first tank-role bot, whatever its slot.
    PL::BindFacts f = Party({BotMember("Healy"), BotMember("Tanky", true), BotMember("Stabby")});
    PL::BindVerdict v = PL::CheckBind(f);
    CHECK(v.refusal == PL::Refusal::None && v.tank == 1);
    // A named tank wins over the role, matched without case.
    f.tankName = "stabby";
    v = PL::CheckBind(f);
    CHECK(v.refusal == PL::Refusal::None && v.tank == 2);
    // A named member that is not there, or a real player named as the tank.
    f.tankName = "Nobody";
    CHECK(PL::CheckBind(f).refusal == PL::Refusal::TankNotInParty);
    f.members.push_back(PlayerMember("Baerthe"));
    f.tankName = "Baerthe";
    CHECK(PL::CheckBind(f).refusal == PL::Refusal::TankNotBot);
    // No tank-role bot and none named.
    PL::BindFacts none = Party({BotMember("Healy"), BotMember("Stabby")});
    CHECK(PL::CheckBind(none).refusal == PL::Refusal::NoTank);
    CHECK(PL::CheckBind(none).tank == -1);
}

// Owner 2026-10-23: 1 to 4 real players, the rest bots, in one party.
static void TestBindWithPlayers()
{
    // 1 player (the commander) + 4 bots.
    PL::BindFacts f = Party({BotMember("Tanky", true), BotMember("Healy"), BotMember("Stabby"), BotMember("Zappy")});
    CHECK(PL::CheckBind(f).refusal == PL::Refusal::None);
    // 2 players + 3 bots, 4 players + 1 bot (the tank alone).
    f = Party({BotMember("Tanky", true), PlayerMember("Baerthe"), BotMember("Healy"), BotMember("Zappy")});
    CHECK(PL::CheckBind(f).refusal == PL::Refusal::None);
    f = Party({PlayerMember("Baerthe"), PlayerMember("Daddycool"), PlayerMember("Sagiroth"), BotMember("Tanky", true)});
    PL::BindVerdict const v = PL::CheckBind(f);
    CHECK(v.refusal == PL::Refusal::None && v.tank == 3);
    // Only players: nothing to lead.
    f = Party({PlayerMember("Baerthe")});
    CHECK(PL::CheckBind(f).refusal == PL::Refusal::NoBots);
    // Another player's bot state does not matter (he is never ordered); a bot does.
    f = Party({BotMember("Tanky", true), PlayerMember("Baerthe")});
    f.members[1].liveBot = false;
    CHECK(PL::CheckBind(f).refusal == PL::Refusal::None);
}

static void TestBindRefusals()
{
    auto with = [](void (*change)(PL::BindFacts&)) {
        PL::BindFacts f = Party({BotMember("Tanky", true), BotMember("Healy")});
        change(f);
        return PL::CheckBind(f).refusal;
    };
    CHECK(with([](PL::BindFacts&) {}) == PL::Refusal::None);
    CHECK(with([](PL::BindFacts& f) { f.enabled = false; }) == PL::Refusal::Disabled);
    CHECK(with([](PL::BindFacts& f) { f.commanderIsBot = true; }) == PL::Refusal::CommanderIsBot);
    CHECK(with([](PL::BindFacts& f) { f.commanderAlreadyBound = true; }) == PL::Refusal::AlreadyBound);
    CHECK(with([](PL::BindFacts& f) { f.grouped = false; }) == PL::Refusal::NotGrouped);
    CHECK(with([](PL::BindFacts& f) { f.raidOrBg = true; }) == PL::Refusal::Raid);
    CHECK(with([](PL::BindFacts& f) { f.commanderLeads = false; }) == PL::Refusal::NotLeader);
    CHECK(with([](PL::BindFacts& f) { f.groupBound = true; }) == PL::Refusal::GroupBound);
    CHECK(with([](PL::BindFacts& f) { f.members[1].controllable = false; }) == PL::Refusal::NotControllable);
    CHECK(with([](PL::BindFacts& f) { f.members[1].liveBot = false; }) == PL::Refusal::NotLive);
    CHECK(with([](PL::BindFacts& f) { f.members[1].hired = true; }) == PL::Refusal::Hired);
    CHECK(with([](PL::BindFacts& f) { f.members[1].inOtherBinding = true; }) == PL::Refusal::OtherBinding);
    CHECK(with([](PL::BindFacts& f) {
        for (int i = 0; i < 4; ++i)
            f.members.push_back(BotMember("Extra" + std::to_string(i)));
    }) == PL::Refusal::TooLarge);
    // Every refusal has its own code and a text.
    for (PL::Refusal r : {PL::Refusal::Disabled, PL::Refusal::NotLeader, PL::Refusal::NoTank, PL::Refusal::Hired})
    {
        CHECK(std::string(PL::RefusalCode(r)) != "rejected");
        CHECK(std::string(PL::RefusalText(r)).size() > 10);
    }
    CHECK(std::string(PL::RefusalCode(PL::Refusal::NoTank)) == "no-tank");
}

// The player-led binding stands while the commander leads the party (not the lead bot), with players in it.
static void TestPlayerLedBindingRule()
{
    RC::Binding b;
    b.commanderGuid = 1;
    b.leadGuid = 2;
    b.groupId = 7;
    b.inParty = true;
    b.playerLed = true;
    RC::BindingState s;
    s.commanderOnline = true;
    s.leadOnline = true;
    s.leadGroupId = 7;
    s.leadIsLeader = false;
    s.commanderIsLeader = true;
    s.realPlayerInGroup = true;
    s.memberCount = 5;
    CHECK(RC::CheckBinding(b, s) == RC::ReleaseReason::None);
    // He hands the lead to somebody (a bot, the tank, another player): released.
    s.commanderIsLeader = false;
    CHECK(RC::CheckBinding(b, s) == RC::ReleaseReason::CommanderNotLeader);
    s.leadIsLeader = true;
    CHECK(RC::CheckBinding(b, s) == RC::ReleaseReason::CommanderNotLeader);
    CHECK(std::string(RC::ReleaseReasonText(RC::ReleaseReason::CommanderNotLeader)).find("player") != std::string::npos);
    // The tank leaves the party, or the party turns into a raid: released as any binding.
    s.commanderIsLeader = true;
    s.leadGroupId = 8;
    CHECK(RC::CheckBinding(b, s) == RC::ReleaseReason::GroupChanged);
    s.leadGroupId = 7;
    s.leadGroupIsRaidOrBg = true;
    CHECK(RC::CheckBinding(b, s) == RC::ReleaseReason::GroupRaid);
    // A bot-led binding keeps its rule: the lead bot leads (the commander's lead says nothing).
    RC::Binding classic = b;
    classic.playerLed = false;
    s.leadGroupIsRaidOrBg = false;
    s.leadIsLeader = false;
    s.commanderIsLeader = true;
    CHECK(RC::CheckBinding(classic, s) == RC::ReleaseReason::LeaderChanged);
    s.leadIsLeader = true;
    s.commanderIsLeader = false;
    CHECK(RC::CheckBinding(classic, s) == RC::ReleaseReason::None);
}

static void TestModeText()
{
    PL::State pl;
    CHECK(PL::ModeText(pl, false) == "no run");
    CHECK(PL::ModeText(pl, true).compare(0, 7, "bot-led") == 0);
    pl.on = true;
    pl.tank = "Tanky";
    CHECK(PL::ModeText(pl, true).find("player-led") == 0);
    CHECK(PL::ModeText(pl, true).find("Tanky") != std::string::npos);
}

// ---------------------------------------------------------------------------
// S2: the player's orders
// ---------------------------------------------------------------------------

static void TestClassifyOrder()
{
    for (const char* w : {"follow", "stay", "come", "Formation", "flee", "guard", "move"})
        CHECK(PL::ClassifyOrder(w) == PL::OrderClass::Move);
    for (const char* w : {"attack", "pull", "pullback", "focus", "cc", "aoe", "sap", "taunt"})
        CHECK(PL::ClassifyOrder(w) == PL::OrderClass::Fight);
    for (const char* w : {"rest", "release", "revive", "corpse", "summon"})
        CHECK(PL::ClassifyOrder(w) == PL::OrderClass::Control);
    for (const char* w : {"", "hello", "who", "stats"})
        CHECK(PL::ClassifyOrder(w) == PL::OrderClass::Other);
    // The glue's labels.
    CHECK(PL::OrderWord("action stay") == "stay");
    CHECK(PL::OrderWord("action pullback 0x1234") == "pullback");
    CHECK(PL::OrderWord("cmd Attack 12") == "attack");
    CHECK(PL::OrderWord("chat 'follow' to Healy") == "follow");
    CHECK(PL::OrderWord("chat '' to Healy").empty());
    CHECK(PL::OrderWord("stay") == "stay");
}

static DungeonPlan OnePull()
{
    DungeonPlan plan;
    PlanStep s;
    s.kind = PlanStepKind::Pull;
    s.x = 40;
    s.entries = {4851};
    ai::NormalizePlanStep(s);
    plan.steps = {s};
    return plan;
}

// The player's orders never pause a player-led run; the bot-led run keeps its pause.
static void TestPlayerOrdersNeverPause()
{
    DungeonPlan plan = OnePull();
    for (const char* label : {"action stay", "action attack", "cmd pullback 11", "chat 'rest' to Healy", "action who"})
    {
        Run r = Start(&plan);
        PL::State pl;
        pl.on = true;
        CHECK(PL::NotePlayerOrder(pl, r, label));
        CHECK(r.state == State::Running);
        CHECK(!PausedByPlayer(r));
        CHECK(r.note.find("obeyed") != std::string::npos);
        // Bot-led: the same order pauses (unchanged).
        Run b = Start(&plan);
        PL::State off;
        CHECK(!PL::NotePlayerOrder(off, b, label));
        CHECK(PauseForPlayer(b, label));
        CHECK(b.state == State::Paused);
    }
    // A paused player-led run stays paused (only `.bot run resume` goes on); a done one takes no note.
    Run r = Start(&plan);
    PL::State pl;
    pl.on = true;
    PausePlayer(r, "paused by .bot run pause");
    CHECK(PL::NotePlayerOrder(pl, r, "action follow"));
    CHECK(r.state == State::Paused && r.note == "paused by .bot run pause");
    r.state = State::Done;
    CHECK(!PL::NotePlayerOrder(pl, r, "action follow"));
}

// `stay` holds the run's pulls until `follow` or `come`; a fight order drops the run's own pending pull.
static void TestStayAndFightOrders()
{
    DungeonPlan plan = OnePull();
    Run r = Start(&plan);
    PL::State pl;
    pl.on = true;
    PL::NotePlayerOrder(pl, r, "action stay");
    CHECK(pl.stay);
    CHECK(r.note.find("follow to go on") != std::string::npos);
    PL::NotePlayerOrder(pl, r, "chat 'attack' to Tanky");
    CHECK(pl.stay); // a fight order is not a move
    PL::NotePlayerOrder(pl, r, "action come");
    CHECK(!pl.stay);
    PL::NotePlayerOrder(pl, r, "action stay");
    PL::NotePlayerOrder(pl, r, "cmd follow");
    CHECK(!pl.stay);
    // The run's pull out (AwaitPull) or gathering: back to the step's start; a pull's fight already on is left.
    for (Phase ph : {Phase::AwaitPull, Phase::Gather})
    {
        r.phase = ph;
        pl.playerFight = false;
        PL::NotePlayerOrder(pl, r, "action pull");
        CHECK(pl.playerFight);
        CHECK(r.phase == Phase::Begin);
    }
    r.phase = Phase::Rest;
    PL::NotePlayerOrder(pl, r, "action focus");
    CHECK(r.phase == Phase::Rest);
    // A move order leaves the run's pull alone.
    r.phase = Phase::AwaitPull;
    pl.playerFight = false;
    PL::NotePlayerOrder(pl, r, "action follow");
    CHECK(r.phase == Phase::AwaitPull && !pl.playerFight);
}

int main()
{
    TestClassifyOrder();
    TestPlayerOrdersNeverPause();
    TestStayAndFightOrders();
    TestStartWords();
    TestBindPicksTheTank();
    TestBindWithPlayers();
    TestBindRefusals();
    TestPlayerLedBindingRule();
    TestModeText();
    std::cout << "test_player_led_policy: all checks passed\n";
    return 0;
}
