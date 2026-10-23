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
    // A real player's role is never the tank's: the bot after him is.
    PL::Member tankPlayer = PlayerMember("Baerthe");
    tankPlayer.tankRole = true;
    PL::BindFacts withPlayer = Party({tankPlayer, BotMember("Grumbo", true)});
    CHECK(PL::CheckBind(withPlayer).tank == 1);
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
    // Five in all with the commander: four others fit, five do not.
    PL::BindFacts four = Party({BotMember("Tanky", true), BotMember("A"), BotMember("B"), PlayerMember("C")});
    CHECK(PL::CheckBind(four).refusal == PL::Refusal::None);
    four.members.push_back(BotMember("D"));
    CHECK(PL::CheckBind(four).refusal == PL::Refusal::TooLarge);
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

// ---------------------------------------------------------------------------
// S3: the step picker and the player-led tick
// ---------------------------------------------------------------------------

static PlanStep At(PlanStepKind kind, float x, std::vector<std::uint32_t> entries = {}, std::uint32_t entry = 0)
{
    PlanStep s;
    s.kind = kind;
    s.x = x;
    s.entries = entries;
    s.entry = entry;
    ai::NormalizePlanStep(s);
    return s;
}

// move, pull 4851 at x 40, move, pull 4852 at x 120, boss 100 at x 200.
static DungeonPlan Route()
{
    DungeonPlan plan;
    plan.steps = {At(PlanStepKind::Move, 0), At(PlanStepKind::Pull, 40, {4851}), At(PlanStepKind::Move, 80),
        At(PlanStepKind::Pull, 120, {4852}), At(PlanStepKind::Boss, 200, {}, 100)};
    return plan;
}

static PL::Anchor Player(float x)
{
    PL::Anchor a;
    a.read = true;
    a.x = x;
    a.botsMaxYd = 3.0f;
    return a;
}

static UnitFact Unit(std::uint64_t guid, std::uint32_t entry, float dist)
{
    UnitFact u;
    u.guid = guid;
    u.entry = entry;
    u.distLead = dist;
    return u;
}

// A party of five bots near the tank, the commander among the players, the step in sight.
static Facts Seen(std::vector<UnitFact> units)
{
    Facts f;
    f.total = 5;
    f.alive = 5;
    f.seen = true;
    f.carrier = "Tanky";
    MemberFact tank;
    tank.name = "Tanky";
    f.members = {tank};
    f.units = std::move(units);
    PlayerFact me;
    me.name = "Hypeer";
    me.commander = true;
    me.yd = 4.0f;
    f.players = {me};
    return f;
}

static PL::State Led()
{
    PL::State pl;
    pl.on = true;
    return pl;
}

static bool Walks(const std::string& order)
{
    return order.compare(0, 5, "move ") == 0 || order.compare(0, 5, "walk ") == 0 ||
        order.compare(0, 7, "escape ") == 0 || order.compare(0, 7, "rescue ") == 0;
}

// Far from every pack: idle, nothing goes out, the note names the next pack and its way.
static void TestIdleNamesTheNextPack()
{
    DungeonPlan plan = Route();
    Run r = Start(&plan);
    PL::State pl = Led();
    PL::Anchor a = Player(-10); // 50 yd south of the first pack
    CHECK(!PL::Pick(r, pl, a, {}));
    CHECK(pl.picked == PL::kNone);
    for (int i = 0; i < 400; ++i)
        CHECK(PL::Tick(r, pl, Seen({}), a).empty());
    CHECK(r.state == State::Running); // no idle ceiling: the player walks
    CHECK(r.note.find("next: pack (step 2") != std::string::npos);
    CHECK(r.note.find("50.0 yd N") != std::string::npos);
    // The bot-led run on the same facts walks its first move (the difference the mode makes).
    Run b = Start(&plan);
    Facts away = Seen({});
    away.leadX = -50;
    std::string walked;
    for (int i = 0; i < 3 && walked.empty(); ++i)
        walked = Tick(b, away);
    CHECK(Walks(walked));
}

// Within pullYd and still for 2 s with the bots near him: the tank pulls.
static void TestPullWhenNearAndStill()
{
    DungeonPlan plan = Route();
    Run r = Start(&plan);
    PL::State pl = Led();
    PL::Anchor a = Player(10); // 30 yd from the pack
    CHECK(PL::Pick(r, pl, a, {}));
    CHECK(pl.picked == 1 && r.step == 1);
    Facts f = Seen({Unit(11, 4851, 30)});
    // Still for 1 s only: held.
    CHECK(PL::Tick(r, pl, f, a).empty());
    CHECK(r.note.find("stand still") != std::string::npos);
    CHECK(PL::Tick(r, pl, f, a) == "pullback 11");
}

static void TestNoPullWhileHeWalks()
{
    DungeonPlan plan = Route();
    Run r = Start(&plan);
    PL::State pl = Led();
    PL::Anchor a = Player(10);
    a.moving = true;
    PL::Pick(r, pl, a, {});
    Facts f = Seen({Unit(11, 4851, 30)});
    for (int i = 0; i < 300; ++i)
    {
        std::string const order = PL::Tick(r, pl, f, a);
        CHECK(order.empty());
    }
    CHECK(r.state == State::Running);
    CHECK(pl.stillMs == 0);
    // He stops: 2 s later the pull goes out.
    a.moving = false;
    CHECK(PL::Tick(r, pl, f, a).empty());
    CHECK(PL::Tick(r, pl, f, a) == "pullback 11");
}

// The gather waits for the bots near the player; it never sends a gather walk (the tank's distance is not his).
static void TestGatherIsOnThePlayer()
{
    DungeonPlan plan = Route();
    Run r = Start(&plan);
    PL::State pl = Led();
    PL::Anchor a = Player(10);
    a.botsMaxYd = 25.0f;
    PL::Pick(r, pl, a, {});
    Facts f = Seen({Unit(11, 4851, 30)});
    f.maxMemberYd = 30.0f; // far from the tank: a bot-led run would gather on the tank
    for (int i = 0; i < 10; ++i)
        CHECK(PL::Tick(r, pl, f, a).empty());
    CHECK(r.note.find("within 20.0 yd of you") != std::string::npos);
    a.botsMaxYd = 8.0f;
    CHECK(PL::Tick(r, pl, f, a) == "pullback 11");
    // Bot-led on the same facts: a gather walk first.
    Run b = Start(&plan);
    b.step = 1;
    CHECK(Walks(Tick(b, f)));
}

static void TestStayHoldsThePull()
{
    DungeonPlan plan = Route();
    Run r = Start(&plan);
    PL::State pl = Led();
    PL::Anchor a = Player(10);
    PL::Pick(r, pl, a, {});
    PL::NotePlayerOrder(pl, r, "action stay");
    Facts f = Seen({Unit(11, 4851, 30)});
    for (int i = 0; i < 5; ++i)
        CHECK(PL::Tick(r, pl, f, a).empty());
    CHECK(r.note.find("your bots stay") != std::string::npos);
    PL::NotePlayerOrder(pl, r, "action follow");
    CHECK(PL::Tick(r, pl, f, a) == "pullback 11");
}

// A fight order of his holds the run's pull for that tick (his pull comes first).
static void TestHisFightOrderHoldsOneTick()
{
    DungeonPlan plan = Route();
    Run r = Start(&plan);
    PL::State pl = Led();
    PL::Anchor a = Player(10);
    PL::Pick(r, pl, a, {});
    Facts f = Seen({Unit(11, 4851, 30)});
    PL::Tick(r, pl, f, a);
    PL::NotePlayerOrder(pl, r, "action attack");
    CHECK(PL::Tick(r, pl, f, a).empty());
    CHECK(r.note.find("your pull") != std::string::npos);
    CHECK(PL::Tick(r, pl, f, a) == "pullback 11");
}

// A cleared pack is done; the move after it is never run; the next pick is the next pack near him.
static void TestClearedPackIsDoneAndMovesNeverRun()
{
    DungeonPlan plan = Route();
    Run r = Start(&plan);
    PL::State pl = Led();
    PL::Anchor a = Player(10);
    PL::Pick(r, pl, a, {});
    Facts f = Seen({Unit(11, 4851, 30)});
    PL::Tick(r, pl, f, a);
    CHECK(PL::Tick(r, pl, f, a) == "pullback 11");
    Facts fight = f;
    fight.anyInCombat = true;
    fight.leadInCombat = true;
    for (int i = 0; i < 3; ++i)
        CHECK(!Walks(PL::Tick(r, pl, fight, a)));
    // The pack dead: the step ends, the run sits idle on the move after it.
    Facts after = Seen({});
    std::string order;
    for (int i = 0; i < 10 && pl.picked != PL::kNone; ++i)
    {
        PL::Pick(r, pl, a, {});
        order = PL::Tick(r, pl, after, a);
        CHECK(!Walks(order));
    }
    CHECK(pl.picked == PL::kNone);
    CHECK(pl.done.count(1) == 1);
    for (int i = 0; i < 20; ++i)
    {
        CHECK(!PL::Pick(r, pl, a, {}));
        CHECK(PL::Tick(r, pl, after, a).empty());
    }
    CHECK(r.note.find("next: pack (step 4") != std::string::npos);
    // He walks on to the second pack.
    a = Player(90);
    CHECK(PL::Pick(r, pl, a, {}));
    CHECK(r.step == 3 && pl.picked == 3);
}

// Rule (a): what the party fights is adopted, even far from where he stands, and wins over a pick not pulled yet.
static void TestEngagedPackIsAdopted()
{
    DungeonPlan plan = Route();
    Run r = Start(&plan);
    PL::State pl = Led();
    PL::Anchor a = Player(-30);
    a.partyInCombat = true;
    PL::Engaged e;
    e.entry = 4852;
    e.x = 118;
    CHECK(PL::Pick(r, pl, a, {e}));
    CHECK(r.step == 3 && pl.picked == 3);
    // An engaged unit of the right entry far from the step's point is no unit of it.
    Run r2 = Start(&plan);
    PL::State pl2 = Led();
    e.x = 170;
    CHECK(!PL::Pick(r2, pl2, a, {e}));
    // A picked pack not pulled yet gives way to the fought one.
    Run r3 = Start(&plan);
    PL::State pl3 = Led();
    PL::Anchor near = Player(10);
    PL::Pick(r3, pl3, near, {});
    CHECK(pl3.picked == 1);
    e.x = 120;
    CHECK(PL::Pick(r3, pl3, near, {e}));
    CHECK(pl3.picked == 3 && r3.step == 3);
    CHECK(r3.note.find("the party fights") != std::string::npos);
    // ... but not one whose pull is out: the step stays.
    r3.pulls = 1;
    PL::Engaged first;
    first.entry = 4851;
    first.x = 40;
    near.partyInCombat = true;
    CHECK(!PL::Pick(r3, pl3, near, {first}));
    CHECK(pl3.picked == 3);
}

// A picked pack he walks away from (60 yd) out of combat is dropped, not done.
static void TestWalkedAwayDropsThePick()
{
    DungeonPlan plan = Route();
    Run r = Start(&plan);
    PL::State pl = Led();
    PL::Pick(r, pl, Player(10), {});
    CHECK(pl.picked == 1);
    CHECK(!PL::Pick(r, pl, Player(-15), {})); // 55 yd: kept
    CHECK(pl.picked == 1);
    CHECK(!PL::Pick(r, pl, Player(-25), {})); // 65 yd: dropped
    CHECK(pl.picked == PL::kNone);
    CHECK(pl.done.count(1) == 0);
    // In combat it stays.
    PL::Pick(r, pl, Player(10), {});
    PL::Anchor fighting = Player(-25);
    fighting.partyInCombat = true;
    PL::Pick(r, pl, fighting, {});
    CHECK(pl.picked == 1);
    // Mid-fight it stays whatever.
    r.fighting = true;
    CHECK(!PL::Pick(r, pl, Player(-200), {}));
    CHECK(pl.picked == 1);
    // A fight with nothing picked (units of no plan step) is not cut by a pick of a pack near him.
    Run idle = Start(&plan);
    PL::State none = Led();
    idle.fighting = true;
    CHECK(!PL::Pick(idle, none, Player(10), {}));
    CHECK(none.picked == PL::kNone && idle.fighting);
    idle.fighting = false;
    idle.regroup = true;
    CHECK(!PL::Pick(idle, none, Player(10), {}));
}

// The bot-led machine's walks are never sent: the walk through a stealthed pack, and the walk back after a
// regroup (the anchor is the player).
static void TestNoWalkGoesOut()
{
    DungeonPlan plan = Route();
    Run r = Start(&plan);
    PL::State pl = Led();
    PL::Anchor a = Player(10);
    PL::Pick(r, pl, a, {});
    UnitFact hidden = Unit(11, 4851, 30);
    hidden.stealthed = true;
    Facts f = Seen({hidden});
    f.leadX = 5; // the tank 35 yd from the pack's point: the bot-led run walks through it
    PL::Tick(r, pl, f, a);
    std::string const order = PL::Tick(r, pl, f, a);
    CHECK(order.empty());
    CHECK(r.note.find("not sent: move") != std::string::npos);
    Run b = Start(&plan);
    b.step = 1;
    CHECK(Walks(Tick(b, f)));
    // A resume regroups; the walk back a bot-led resume does is not started.
    Run w = Start(&plan);
    PL::State wl = Led();
    PL::Anchor far = Player(110);
    PL::Pick(w, wl, far, {});
    CHECK(wl.picked == 3);
    PausePlayer(w, "paused by .bot run pause");
    CHECK(Resume(w));
    CHECK(w.walkBackDue);
    Facts there = Seen({Unit(12, 4852, 20)});
    there.leadX = 300; // the tank came back far from the step and from the last move point
    for (int i = 0; i < 4; ++i)
    {
        CHECK(!Walks(PL::Tick(w, wl, there, far)));
        CHECK(!w.walkBack && !w.walkBackDue);
    }
}

static void TestTeamFloorAndRange()
{
    DungeonPlan plan = Route();
    plan.steps[1].flags = ai::kPlanFlagHorde;
    Run r = Start(&plan);
    PL::State pl = Led();
    PL::Anchor a = Player(10);
    a.team = ai::kPlanFlagAlliance;
    CHECK(!PL::Pick(r, pl, a, {})); // the Horde pack is not pulled for an Alliance party
    a.team = ai::kPlanFlagHorde;
    CHECK(PL::Pick(r, pl, a, {}));
    // Another floor: 20 yd above.
    DungeonPlan plan2 = Route();
    plan2.steps[1].z = 20;
    Run r2 = Start(&plan2);
    PL::State pl2 = Led();
    CHECK(!PL::Pick(r2, pl2, Player(30), {}));
    // The range is the key's.
    DungeonPlan plan3 = Route();
    Run r3 = Start(&plan3);
    PL::State pl3 = Led();
    pl3.pullYd = 20;
    CHECK(!PL::Pick(r3, pl3, Player(10), {}));
    CHECK(PL::Pick(r3, pl3, Player(25), {}));
    // Exactly at the line is near.
    Run r4 = Start(&plan3);
    PL::State pl4 = Led();
    CHECK(PL::Pick(r4, pl4, Player(0), {})); // 40.0 yd
    Run r5 = Start(&plan3);
    PL::State pl5 = Led();
    CHECK(!PL::Pick(r5, pl5, Player(-0.5f), {}));
}

// Dead, a ghost or outside: no pick, and a picked pull holds.
static void TestPlayerDownOrAway()
{
    DungeonPlan plan = Route();
    PL::Anchor dead = Player(10);
    dead.alive = false;
    Run r = Start(&plan);
    PL::State pl = Led();
    CHECK(!PL::Pick(r, pl, dead, {}));
    PL::Anchor a = Player(10);
    PL::Pick(r, pl, a, {});
    Facts f = Seen({Unit(11, 4851, 30)});
    PL::Tick(r, pl, f, a);
    PL::Anchor outside = a;
    outside.onMap = false;
    CHECK(PL::Tick(r, pl, f, outside).empty());
    CHECK(r.note.find("not in the plan's dungeon") != std::string::npos);
    PL::Anchor ghost = a;
    ghost.ghost = true;
    CHECK(PL::Tick(r, pl, f, ghost).empty());
}

// The plan's last pack first: the bot-led end of plan does not end the run while packs are open; all down does.
static void TestRunEndsWhenAllPacksAreDown()
{
    DungeonPlan plan;
    plan.steps = {At(PlanStepKind::Pull, 40, {4851}), At(PlanStepKind::Move, 80), At(PlanStepKind::Pull, 120, {4852})};
    Run r = Start(&plan);
    PL::State pl = Led();
    PL::Anchor a = Player(110);
    PL::Pick(r, pl, a, {});
    CHECK(pl.picked == 2);
    Facts none = Seen({});
    for (int i = 0; i < 6; ++i)
        PL::Tick(r, pl, none, a);
    CHECK(pl.done.count(2) == 1);
    CHECK(r.state == State::Running);
    a = Player(30);
    CHECK(PL::Pick(r, pl, a, {}));
    for (int i = 0; i < 6; ++i)
        PL::Tick(r, pl, none, a);
    CHECK(pl.done.count(0) == 1);
    PL::Pick(r, pl, a, {});
    PL::Tick(r, pl, none, a);
    CHECK(r.state == State::Done);
    CHECK(r.note.find("every pack and boss") != std::string::npos);
}

static void TestPartwayStart()
{
    DungeonPlan plan = Route();
    Run r = Start(&plan, 3);
    PL::State pl = Led();
    CHECK(!PL::Pick(r, pl, Player(10), {})); // pack 1 lies before the start: done
    CHECK(pl.done.count(1) == 1);
}

static void TestSkipNearest()
{
    DungeonPlan plan = Route();
    Run r = Start(&plan);
    PL::State pl = Led();
    CHECK(PL::SkipNearest(r, pl, Player(100)));
    CHECK(pl.done.count(3) == 1 && r.notCleared.count(3) == 1);
    CHECK(r.note.find("step 4") != std::string::npos);
    // With a step picked the glue skips it (DungeonRun::Skip) and the next pick marks it done.
    PL::Pick(r, pl, Player(10), {});
    CHECK(!PL::SkipNearest(r, pl, Player(10)));
    CHECK(Skip(r));
    PL::Pick(r, pl, Player(10), {});
    CHECK(pl.done.count(1) == 1);
}

static void TestDroppedOrders()
{
    PL::Anchor a = Player(0);
    CHECK(PL::Dropped("move 1 2 3", a));
    CHECK(PL::Dropped("walk Healy 1 2 3", a));
    CHECK(PL::Dropped("escape 1 2 3", a));
    CHECK(PL::Dropped("rescue 1 2 3", a));
    CHECK(!PL::Dropped("pullback 11", a));
    CHECK(!PL::Dropped("revive party", a));
    CHECK(!PL::Dropped("summon Healy", a));
    CHECK(!PL::Dropped("rest", a));
    a.moving = true;
    CHECK(PL::Dropped("rest", a));
    CHECK(PL::Compass(Player(0), 10, 0) == "N");
    CHECK(PL::Compass(Player(0), 0, -10) == "E");
    CHECK(PL::Compass(Player(0), -10, 10) == "SW");
}

// pl.on false: PlayerLed::Tick is DungeonRun::Tick (the bot-led run is unchanged).
static void TestBotLedUnchanged()
{
    DungeonPlan plan = Route();
    Run a = Start(&plan), b = Start(&plan);
    PL::State off;
    PL::Anchor anywhere = Player(500);
    std::vector<Facts> script = {Seen({}), Seen({Unit(11, 4851, 30)}), Seen({Unit(11, 4851, 30)}), Seen({})};
    script[2].anyInCombat = true;
    for (int round = 0; round < 30; ++round)
    {
        const Facts& f = script[static_cast<std::size_t>(round) % script.size()];
        CHECK(PL::Tick(a, off, f, anywhere) == Tick(b, f));
        CHECK(a.step == b.step && a.note == b.note && a.state == b.state);
    }
    CHECK(!PL::Pick(a, off, Player(10), {}));
}

static void TestMovedSince()
{
    PL::State pl;
    CHECK(!PL::MovedSince(pl, 10, 10, 0)); // the first read
    CHECK(!PL::MovedSince(pl, 10.3f, 10, 0));
    CHECK(PL::MovedSince(pl, 11.0f, 10, 0));
    CHECK(!PL::MovedSince(pl, 11.0f, 10, 0));
    pl.lastKnown = false; // he left the world: the next read starts over
    CHECK(!PL::MovedSince(pl, 50, 50, 0));
}

int main()
{
    TestMovedSince();
    TestIdleNamesTheNextPack();
    TestPullWhenNearAndStill();
    TestNoPullWhileHeWalks();
    TestGatherIsOnThePlayer();
    TestStayHoldsThePull();
    TestHisFightOrderHoldsOneTick();
    TestClearedPackIsDoneAndMovesNeverRun();
    TestEngagedPackIsAdopted();
    TestWalkedAwayDropsThePick();
    TestTeamFloorAndRange();
    TestPlayerDownOrAway();
    TestRunEndsWhenAllPacksAreDown();
    TestPartwayStart();
    TestSkipNearest();
    TestDroppedOrders();
    TestBotLedUnchanged();
    TestNoWalkGoesOut();
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
