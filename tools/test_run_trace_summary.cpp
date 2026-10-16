// Standalone tests for the run trace's summary layer (runtime/RunTraceSummary.h,
// issue #986): the fields the dashboard's Dungeons tab reads from
// <run_id>.summary.json (outcome, bosses with kill times, wipes and deaths with
// positions, clear_s, mode, build, driver), and the `driver=` argument check.
//
// The header includes the standard library and the plan table's pure header
// only, so this compiles without a game header.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_run_trace_summary.cpp -o /tmp/test_run_trace_summary
//   /tmp/test_run_trace_summary

#include "../runtime/RunTraceSummary.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace TortoiseBots::RunTrace;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)
#define CHECK_EQ(a, b) do { \
    ++checks; \
    std::string const va = (a); \
    std::string const vb = (b); \
    if (va != vb) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n  got:  %s\n  want: %s\n", __FILE__, __LINE__, #a, va.c_str(), vb.c_str()); \
        std::exit(1); \
    } \
} while (0)

static bool Has(std::string const& s, char const* part)
{
    return s.find(part) != std::string::npos;
}

static std::vector<BossProgress> ThreeBosses()
{
    std::vector<BossProgress> v(3);
    v[0].entry = 8127; v[0].name = "Antu'sul";
    v[1].entry = 7272; v[1].name = "Theka the Martyr";
    v[2].entry = 7267; v[2].name = "Velratha";
    return v;
}

static void TestDriver()
{
    CHECK(IsDriverToken("driver=sonnet:medium:zf-1"));
    CHECK(IsDriverToken("DRIVER=x"));
    CHECK(!IsDriverToken("driver"));
    CHECK(!IsDriverToken("party"));
    CHECK(!IsDriverToken("drive=x"));
    CHECK(DriverRefusal("sonnet:medium:zf-ab-1-s2") == nullptr);
    CHECK(DriverRefusal("claude-opus-4.6:high:run_7+x") == nullptr);
    CHECK(DriverRefusal("human") == nullptr);
    CHECK(DriverRefusal("") != nullptr);
    CHECK(DriverRefusal("a b") != nullptr);
    CHECK(DriverRefusal("a\"b") != nullptr);
    CHECK(DriverRefusal("a/b") != nullptr);
    CHECK(DriverRefusal(std::string(kMaxDriverLen, 'a')) == nullptr);
    CHECK(DriverRefusal(std::string(kMaxDriverLen + 1, 'a')) != nullptr);
    // Not given: a player-led run is "human", anything else "unknown".
    CHECK_EQ(EffectiveDriver("", RunMode::Observe), "human");
    CHECK_EQ(EffectiveDriver("", RunMode::Commander), "unknown");
    CHECK_EQ(EffectiveDriver("", RunMode::RunLeader), "unknown");
    CHECK_EQ(EffectiveDriver("haiku:max:r1", RunMode::Observe), "haiku:max:r1");
}

static void TestMode()
{
    CHECK_EQ(ModeName(RunMode::Observe), "observe");
    CHECK_EQ(ModeName(RunMode::Commander), "commander");
    CHECK_EQ(ModeName(RunMode::RunLeader), "run_leader");
    CHECK(StrongerMode(RunMode::Commander, RunMode::Observe) == RunMode::Commander);
    CHECK(StrongerMode(RunMode::Observe, RunMode::RunLeader) == RunMode::RunLeader);
    RunDigest d;
    d.NoteMode(RunMode::RunLeader);
    d.NoteMode(RunMode::Observe);  // a later weaker sign does not downgrade it
    CHECK(d.Mode() == RunMode::RunLeader);
    CHECK(Has(d.Render(), "\"mode\":\"run_leader\""));
}

static void TestOutcomeRule()
{
    // cleared needs a boss list and every boss on it killed
    CHECK(DecideOutcome(true, 3, 3) == Outcome::Cleared);
    CHECK(DecideOutcome(true, 3, 2) == Outcome::Partial);
    CHECK(DecideOutcome(true, 3, 1) == Outcome::Partial);
    CHECK(DecideOutcome(true, 3, 0) == Outcome::Abandoned);
    CHECK(DecideOutcome(true, 0, 0) == Outcome::Abandoned);
    // without a plan nothing proves a full clear
    CHECK(DecideOutcome(false, 3, 3) == Outcome::Partial);
    CHECK(DecideOutcome(false, 1, 1) == Outcome::Partial);
    CHECK(DecideOutcome(false, 0, 0) == Outcome::Abandoned);
    CHECK_EQ(OutcomeLabel(Outcome::Cleared), "cleared");
    CHECK_EQ(OutcomeLabel(Outcome::Partial), "partial");
    CHECK_EQ(OutcomeLabel(Outcome::Abandoned), "abandoned");
}

static void TestFullClear()
{
    RunDigest d;
    d.SetBuild("2026-10-16-v2");
    d.SetDriver("sonnet:medium:zf-ab-1-s2");
    d.NoteMode(RunMode::Commander);
    d.NoteDungeon(209);
    d.NotePlan("zf-full", ThreeBosses());
    CHECK(d.HasPlan());
    CHECK(d.IsPlanBoss(8127));
    CHECK(!d.IsPlanBoss(1234));
    CHECK(d.NoteBossKill(8127, 2280000));
    CHECK(!d.NoteBossKill(8127, 2290000));   // a repeat does not move the time
    CHECK(!d.NoteBossKill(1234, 100));       // not on the list
    CHECK(d.NoteBossKill(7272, 2520000));
    {
        std::string const mid = d.Render();
        CHECK(Has(mid, "\"outcome\":\"partial\""));
        CHECK(!Has(mid, "clear_s"));
        CHECK(Has(mid, "\"killed_at_s\":null"));
    }
    CHECK(d.NoteBossKill(7267, 4260500));
    std::string const s = d.Render();
    CHECK(Has(s, "\"mode\":\"commander\""));
    CHECK(Has(s, "\"build\":\"2026-10-16-v2\""));
    CHECK(Has(s, "\"driver\":\"sonnet:medium:zf-ab-1-s2\""));
    CHECK(Has(s, "\"plan\":\"zf-full\""));
    CHECK(Has(s, "\"dungeon\":{\"map\":209}"));
    CHECK(Has(s, "\"outcome\":\"cleared\""));
    CHECK(Has(s, "{\"entry\":8127,\"name\":\"Antu'sul\",\"killed_at_s\":2280.0}"));
    CHECK(Has(s, "{\"entry\":7267,\"name\":\"Velratha\",\"killed_at_s\":4260.5}"));
    CHECK(Has(s, "\"clear_s\":4260.5"));   // the last boss
    CHECK(d.BossesKilled() == 3);
}

static void TestPartialAndAbandoned()
{
    RunDigest d;
    d.NoteDungeon(209);
    d.NotePlan("zf-full", ThreeBosses());
    CHECK(Has(d.Render(), "\"outcome\":\"abandoned\""));
    d.NoteBossKill(7272, 61000);
    std::string const s = d.Render();
    CHECK(Has(s, "\"outcome\":\"partial\""));
    CHECK(!Has(s, "clear_s"));
    CHECK(Has(s, "{\"entry\":8127,\"name\":\"Antu'sul\",\"killed_at_s\":null}"));
}

static void TestNoPlan()
{
    // Bosses the bots met are the list; a full clear is never claimed.
    RunDigest d;
    d.NoteDungeon(70);
    CHECK(d.NoteSeenBoss(2748, "Archaedas"));
    CHECK(!d.NoteSeenBoss(2748, "Archaedas"));
    CHECK(!d.IsPlanBoss(2748));              // no plan: the recorder decides by rank
    d.NoteBossKill(2748, 9000);
    std::string s = d.Render();
    CHECK(Has(s, "\"outcome\":\"partial\""));
    CHECK(!Has(s, "clear_s"));
    CHECK(!Has(s, "\"plan\""));
    // A plan that arrives later keeps the kill already seen, and the seen-boss list is replaced by the plan's.
    std::vector<BossProgress> planned(2);
    planned[0].entry = 2748; planned[0].name = "Archaedas";
    planned[1].entry = 2749; planned[1].name = "Other";
    d.NotePlan("uld", planned);
    CHECK(d.BossesKilled() == 1);
    CHECK(d.BossCount() == 2);
    d.NoteBossKill(2749, 20000);
    s = d.Render();
    CHECK(Has(s, "\"outcome\":\"cleared\""));
    CHECK(Has(s, "\"clear_s\":20.0"));
    // the first plan wins
    d.NotePlan("other", ThreeBosses());
    CHECK(d.BossCount() == 2);
    CHECK(Has(d.Render(), "\"plan\":\"uld\""));
}

static void TestOpenWorldHasNoOutcome()
{
    // A trace that never stood in an instance and met no boss is not a dungeon run.
    RunDigest d;
    d.SetBuild("dev");
    std::string const s = d.Render();
    CHECK(!Has(s, "outcome"));
    CHECK(!Has(s, "bosses"));
    CHECK(!Has(s, "dungeon"));
    CHECK(Has(s, "\"mode\":\"observe\""));
    CHECK(Has(s, "\"driver\":\"human\""));
    CHECK(Has(s, "\"wipes\":[]"));
    CHECK(Has(s, "\"deaths\":[]"));
}

static Spot Fell(uint32_t tMs, float x, float y, float z, char const* bot, char const* role)
{
    Spot s;
    s.tMs = tMs; s.map = 209; s.x = x; s.y = y; s.z = z; s.bot = bot; s.role = role;
    return s;
}

static void TestDeaths()
{
    RunDigest d;
    d.NoteDungeon(209);
    d.NoteDeath(Fell(1612400, 1680.24f, 871.4f, 12.0f, "Aldo", "heal"));
    d.NoteDeath(Fell(1630000, 1678.0f, 874.9f, 12.5f, "Bria", "dps"));
    CHECK(d.DeathCount() == 2);
    std::string const s = d.Render();
    CHECK(Has(s, "{\"t_s\":1612.4,\"map\":209,\"x\":1680.2,\"y\":871.4,\"z\":12.0,\"bot\":\"Aldo\",\"role\":\"heal\"}"));
    CHECK(Has(s, "\"bot\":\"Bria\",\"role\":\"dps\"}"));
    // a name is escaped
    RunDigest e;
    e.NoteDeath(Fell(1000, 1, 2, 3, "Qu\"ote", "tank"));
    CHECK(Has(e.Render(), "\"bot\":\"Qu\\\"ote\""));
    // negative coordinates keep their sign
    RunDigest n;
    n.NoteDeath(Fell(1000, -45.25f, -3.0f, 0.0f, "N", "dps"));
    CHECK(Has(n.Render(), "\"x\":-45.3,\"y\":-3.0,\"z\":0.0"));
    // bounded
    RunDigest big;
    for (size_t i = 0; i < kMaxSpots + 50; ++i)
        big.NoteDeath(Fell(1000, 0, 0, 0, "B", "dps"));
    CHECK(big.DeathCount() == kMaxSpots);
}

static MemberState Member(bool alive, uint32_t tMs, float x, float y, char const* role)
{
    MemberState m;
    m.alive = alive;
    m.lastDeath = Fell(tMs, x, y, 5.0f, "b", role);
    return m;
}

static void TestWipes()
{
    RunDigest d;
    d.NoteDungeon(209);
    // someone alive: no wipe
    CHECK(!d.NoteFrame(1000, { Member(false, 900, 10, 10, "dps"), Member(true, 0, 0, 0, "tank") }));
    // everyone dead: one wipe, at the centroid, role of the first to fall
    CHECK(d.NoteFrame(2000, { Member(false, 900, 10, 20, "heal"), Member(false, 1500, 20, 40, "dps"), Member(false, 1800, 30, 30, "tank") }));
    CHECK(d.WipeCount() == 1);
    // the same wipe is not counted again while they stay dead
    CHECK(!d.NoteFrame(2500, { Member(false, 900, 10, 20, "heal"), Member(false, 1500, 20, 40, "dps"), Member(false, 1800, 30, 30, "tank") }));
    CHECK(d.WipeCount() == 1);
    // back up, then down again: a second wipe
    CHECK(!d.NoteFrame(9000, { Member(true, 0, 0, 0, "heal"), Member(false, 1500, 20, 40, "dps") }));
    CHECK(d.NoteFrame(12000, { Member(false, 11000, 100, 100, "dps"), Member(false, 11500, 120, 100, "tank") }));
    CHECK(d.WipeCount() == 2);
    std::string const s = d.Render();
    CHECK(Has(s, "\"wipes\":[{\"t_s\":2.0,\"map\":209,\"x\":20.0,\"y\":30.0,\"z\":5.0,\"role\":\"heal\",\"dead\":3},"
                 "{\"t_s\":12.0,\"map\":209,\"x\":110.0,\"y\":100.0,\"z\":5.0,\"role\":\"dps\",\"dead\":2}]"));
    // a lone traced bot dying is a death, not a wipe
    RunDigest one;
    CHECK(!one.NoteFrame(1000, { Member(false, 900, 1, 1, "dps") }));
    CHECK(one.WipeCount() == 0);
    // an empty frame changes nothing, and does not end a wipe in progress
    RunDigest e;
    CHECK(e.NoteFrame(1000, { Member(false, 1, 0, 0, "dps"), Member(false, 2, 0, 0, "dps") }));
    CHECK(!e.NoteFrame(1500, {}));
    CHECK(!e.NoteFrame(2000, { Member(false, 1, 0, 0, "dps"), Member(false, 2, 0, 0, "dps") }));
    CHECK(e.WipeCount() == 1);
}

static void TestPlanBossEntries()
{
    ai::DungeonPlan plan;
    auto step = [&plan](ai::PlanStepKind kind, uint32_t entry, uint32_t flags)
    {
        ai::PlanStep s;
        s.kind = kind;
        s.entry = entry;
        s.flags = flags;
        plan.steps.push_back(s);
    };
    step(ai::PlanStepKind::Move, 0, 0);
    step(ai::PlanStepKind::Pull, 111, 0);          // a pack, not a boss
    step(ai::PlanStepKind::Boss, 8127, 0);
    step(ai::PlanStepKind::Boss, 8127, 0);         // the same boss twice: once
    step(ai::PlanStepKind::Boss, 7272, ai::kPlanFlagAlliance);
    step(ai::PlanStepKind::Boss, 7273, ai::kPlanFlagHorde);
    step(ai::PlanStepKind::Loot, 5000, 0);
    std::vector<uint32_t> const both = PlanBossEntries(plan, 0);
    CHECK(both.size() == 3 && both[0] == 8127 && both[1] == 7272 && both[2] == 7273);
    std::vector<uint32_t> const alliance = PlanBossEntries(plan, ai::kPlanFlagAlliance);
    CHECK(alliance.size() == 2 && alliance[0] == 8127 && alliance[1] == 7272);
    std::vector<uint32_t> const horde = PlanBossEntries(plan, ai::kPlanFlagHorde);
    CHECK(horde.size() == 2 && horde[0] == 8127 && horde[1] == 7273);
    CHECK(PlanBossEntries(ai::DungeonPlan(), 0).empty());
}

static void TestSummaryMembers()
{
    // The digest's members go into the summary after the counters, before "oracles", and never ahead of
    // "status" (MarkSummaryIncomplete reads the first "status" key).
    Builder b("run-9");
    Header h;
    h.runId = "run-9";
    h.tbVersion = "2026-10-16-v2";
    h.startUnix = 100;
    RunDigest d;
    d.SetBuild(h.tbVersion);
    d.SetDriver("opus:high:zf-1");
    d.NoteDungeon(209);
    d.NotePlan("zf-full", ThreeBosses());
    d.NoteBossKill(8127, 5000);
    Counters c;
    std::string const s = b.BuildSummary(h, "complete", "stop", 160, 60000, c, "run-9.jsonl", std::string(), d.Render());
    CHECK(s.front() == '{' && s.back() == '}');
    CHECK(Has(s, "\"oracles\":[]}"));
    size_t const status = s.find("\"status\"");
    size_t const mode = s.find("\"mode\"");
    size_t const counters = s.find("\"counters\"");
    size_t const oracles = s.find("\"oracles\"");
    CHECK(status != std::string::npos && mode != std::string::npos);
    CHECK(status < counters && counters < mode && mode < oracles);
    CHECK(s.find("\"status\"", status + 1) == std::string::npos);
    CHECK(s.find("\"end_reason\"", s.find("\"end_reason\"") + 1) == std::string::npos);
    CHECK(Has(s, "\"tb_version\":\"2026-10-16-v2\""));  // the old key stays
    CHECK(Has(s, "\"build\":\"2026-10-16-v2\""));
    CHECK(Has(s, "\"driver\":\"opus:high:zf-1\""));
    // Old callers pass no members and get the old summary byte for byte.
    std::string const old = b.BuildSummary(h, "complete", "stop", 160, 60000, c, "run-9.jsonl");
    CHECK(!Has(old, "\"mode\""));
    CHECK(Has(old, "\"dropped\":0,\"truncated\":false},\"oracles\":[]}"));
    // Members of an error summary come after the error, still before oracles.
    std::string const err = b.BuildSummary(h, "failed", "stop", 160, 60000, c, "run-9.jsonl", "disk full", d.Render());
    CHECK(err.find("\"error\"") < err.find("\"mode\""));
}

int main()
{
    TestDriver();
    TestMode();
    TestOutcomeRule();
    TestFullClear();
    TestPartialAndAbandoned();
    TestNoPlan();
    TestOpenWorldHasNoOutcome();
    TestDeaths();
    TestWipes();
    TestPlanBossEntries();
    TestSummaryMembers();
    std::printf("test_run_trace_summary: %d checks passed\n", checks);
    return 0;
}
