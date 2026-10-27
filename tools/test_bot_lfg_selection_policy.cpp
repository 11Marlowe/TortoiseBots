// Standalone tests for the bot LFG selection policy (bot LFG part 2):
// which world bots may form a bot LFG group and which group forms.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_bot_lfg_selection_policy.cpp -o /tmp/lfg2_sel
//   /tmp/lfg2_sel

#include "../runtime/policy/lfg/BotLfgSelectionPolicy.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace TortoiseBots::BotLfg;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

static CandidateFacts Free()
{
    CandidateFacts f;
    f.liveHeadless = true; f.worldBot = true; f.leaseFree = true; f.alive = true; f.level = 20;
    return f;
}

static void TestCandidateCheck()
{
    CHECK(CheckCandidate(Free()) == Skip::None);
    auto with = [](void (*edit)(CandidateFacts&)) { CandidateFacts f = Free(); edit(f); return CheckCandidate(f); };
    CHECK(with([](CandidateFacts& f) { f.liveHeadless = false; }) == Skip::NotLive);
    CHECK(with([](CandidateFacts& f) { f.worldBot = false; }) == Skip::NotWorldBot);
    CHECK(with([](CandidateFacts& f) { f.playerMaster = true; }) == Skip::PlayerMaster);
    CHECK(with([](CandidateFacts& f) { f.hardcore = true; }) == Skip::Hardcore);
    CHECK(with([](CandidateFacts& f) { f.grouped = true; }) == Skip::Grouped);
    CHECK(with([](CandidateFacts& f) { f.bound = true; }) == Skip::Bound);
    CHECK(with([](CandidateFacts& f) { f.leaseFree = false; }) == Skip::Leased);
    CHECK(with([](CandidateFacts& f) { f.lftQueued = true; }) == Skip::Queued);
    CHECK(with([](CandidateFacts& f) { f.battleground = true; }) == Skip::Battleground);
    CHECK(with([](CandidateFacts& f) { f.onInstanceMap = true; }) == Skip::InInstance);
    CHECK(with([](CandidateFacts& f) { f.alive = false; }) == Skip::Dead);
    CHECK(with([](CandidateFacts& f) { f.busy = true; }) == Skip::Busy);
    CHECK(with([](CandidateFacts& f) { f.level = 9; }) == Skip::LowLevel);
    CHECK(with([](CandidateFacts& f) { f.level = 10; }) == Skip::None);
    // first failing check wins: a grouped bot with a player master reports the master
    CHECK(with([](CandidateFacts& f) { f.grouped = true; f.playerMaster = true; }) == Skip::PlayerMaster);
    CHECK(std::string(SkipCode(Skip::Leased)) == "leased");
    CHECK(std::string(SkipCode(Skip::None)) == "ok");
}

static void TestBandAndTeam()
{
    CHECK(InBand(17, 17, 24) && InBand(24, 17, 24) && !InBand(16, 17, 24) && !InBand(25, 17, 24));
    CHECK(TeamAllowed(3, kAlliance) && TeamAllowed(3, kHorde) && TeamAllowed(2, kHorde) && !TeamAllowed(2, kAlliance));
    CHECK(!TeamAllowed(3, 0));
    CHECK(std::string(TeamName(kHorde)) == "horde" && std::string(RoleName(Role::Healer)) == "healer");
}

static Candidate C(uint64_t guid, bool t, bool h, bool d, bool same = true, float dist = 10.0f)
{
    Candidate c;
    c.guid = guid; c.level = 20; c.team = kAlliance; c.tank = t; c.healer = h; c.damage = d; c.sameMap = same; c.distance = dist;
    return c;
}

static void TestSort()
{
    std::vector<Candidate> v = {C(5, 0, 0, 1, false, 1.0f), C(4, 0, 0, 1, true, 50.0f), C(3, 0, 0, 1, true, 5.0f),
        C(2, 0, 0, 1, true, 5.0f)};
    SortCandidates(v);
    CHECK(v[0].guid == 2 && v[1].guid == 3 && v[2].guid == 4 && v[3].guid == 5); // same map first, then distance, then guid
}

static void TestCompose()
{
    // druid (tank+heal+dps) must not be eaten as damage while pure dps exist
    std::vector<Candidate> v = {C(1, 1, 1, 1), C(2, 0, 0, 1), C(3, 0, 0, 1), C(4, 0, 0, 1), C(5, 0, 1, 0)};
    Composition full = ComposeFull(v);
    CHECK(full.ok && full.picks.size() == 5);
    CHECK(full.picks[0].role == Role::Tank && full.picks[0].index == 0);
    CHECK(full.picks[1].role == Role::Healer && full.picks[1].index == 4);
    CHECK(full.picks[2].index == 1 && full.picks[3].index == 2 && full.picks[4].index == 3);
    CHECK(full.leader == 0);

    // a pure tank goes before a flexible one
    std::vector<Candidate> w = {C(1, 1, 1, 0), C(2, 1, 0, 0), C(3, 0, 1, 0), C(4, 0, 0, 1), C(5, 0, 0, 1), C(6, 0, 0, 1)};
    Composition fw = ComposeFull(w);
    CHECK(fw.ok && w[fw.picks[0].index].guid == 2 && w[fw.picks[1].index].guid == 3);

    // four dps and a healer: no tank -> not feasible
    std::vector<Candidate> noTank = {C(1, 0, 1, 0), C(2, 0, 0, 1), C(3, 0, 0, 1), C(4, 0, 0, 1), C(5, 0, 0, 1)};
    CHECK(!ComposeFull(noTank).ok);
    // a single druid cannot be tank and healer at once
    std::vector<Candidate> oneDruid = {C(1, 1, 1, 1), C(2, 0, 0, 1), C(3, 0, 0, 1), C(4, 0, 0, 1)};
    CHECK(!ComposeFull(oneDruid).ok);

    Composition openTank = ComposeWithOpen(v, Role::Tank);
    CHECK(openTank.ok && openTank.picks.size() == 4);
    for (Pick const& p : openTank.picks)
        CHECK(p.role != Role::Tank);
    CHECK(openTank.picks[openTank.leader].role == Role::Healer); // no tank -> the healer leads
    Composition openDps = ComposeWithOpen(v, Role::Damage);
    CHECK(openDps.ok && openDps.picks.size() == 4 && openDps.picks.back().index == 2); // the last damage pick is dropped
    CHECK(openDps.picks[openDps.leader].role == Role::Tank);
    CHECK(!ComposeWithOpen(noTank, Role::Tank).ok); // feasibility is the full five, the open one included
}

static void TestPickForRole()
{
    std::vector<Candidate> v = {C(1, 1, 1, 1), C(2, 0, 0, 1), C(3, 0, 1, 0)};
    std::vector<bool> taken(v.size(), false);
    CHECK(PickForRole(v, Role::Damage, taken) == 1);
    CHECK(PickForRole(v, Role::Healer, taken) == 2);
    CHECK(PickForRole(v, Role::Tank, taken) == 0);
    taken[1] = true;
    CHECK(PickForRole(v, Role::Damage, taken) == 0); // only the flexible one is left
    taken[0] = true;
    CHECK(PickForRole(v, Role::Tank, taken) == -1);
    CHECK(PickForRole(v, Role::None, taken) == -1);
}

static void TestOpenRoleRotation()
{
    CHECK(OpenRoleFor(0) == Role::Damage && OpenRoleFor(1) == Role::Tank && OpenRoleFor(2) == Role::Damage &&
        OpenRoleFor(3) == Role::Healer && OpenRoleFor(4) == Role::Damage);
}

static void TestTargetGroups()
{
    CHECK(TargetGroups(500, 1, 5, 1, true) == 5);
    CHECK(TargetGroups(499, 1, 5, 1, true) == 4);
    CHECK(TargetGroups(1000, 1, 5, 1, true) == 5);
    CHECK(TargetGroups(99, 1, 5, 1, true) == 0);
    CHECK(TargetGroups(500, 1, 5, 0, true) == 0);   // nobody to play with
    CHECK(TargetGroups(500, 1, 5, 0, false) == 5);
    CHECK(TargetGroups(500, 0, 5, 3, true) == 0);
    CHECK(TargetGroups(500, 2, 20, 3, true) == 10);
    CHECK(TargetGroups(500, 1, 0, 3, true) == 0);
}

static void TestFormDue()
{
    CHECK(FormDue(0, 1, kFormIntervalMs));
    CHECK(!FormDue(0, 1, kFormIntervalMs - 1));
    CHECK(!FormDue(1, 1, kFormIntervalMs * 10));
    CHECK(!FormDue(2, 1, kFormIntervalMs));
}

static PlanOption Opt(char const* plan, uint8_t team)
{
    PlanOption o;
    o.plan = plan; o.team = team; o.feasible = true; o.candidates = 10;
    return o;
}

static void TestPickPlan()
{
    std::vector<PlanOption> v = {Opt("deadmines", kAlliance), Opt("wailing_caverns", kHorde)};
    v[0].everFormed = true; v[0].sinceFormedMs = 900000;
    CHECK(PickPlan(v) == 1);                              // never formed first
    v[0].playerInBand = true;
    CHECK(PickPlan(v) == 0);                              // a player in band beats rotation
    v[0].busy = true;
    CHECK(PickPlan(v) == 1);                              // a pair with a group is skipped
    v[1].feasible = false;
    CHECK(PickPlan(v) == -1);
    std::vector<PlanOption> w = {Opt("a", kAlliance), Opt("b", kAlliance)};
    w[0].everFormed = w[1].everFormed = true;
    w[0].sinceFormedMs = 1000; w[1].sinceFormedMs = 2000;
    CHECK(PickPlan(w) == 1);                              // longest since formed
    w[1].sinceFormedMs = 1000; w[1].candidates = 11;
    CHECK(PickPlan(w) == 1);                              // more candidates
    w[1].candidates = 10;
    CHECK(PickPlan(w) == 0);                              // name A-Z
    std::vector<PlanOption> t = {Opt("a", kHorde), Opt("a", kAlliance)};
    CHECK(PickPlan(t) == 1);                              // Alliance before Horde
    CHECK(PickPlan({}) == -1);
}

int main()
{
    TestCandidateCheck();
    TestBandAndTeam();
    TestSort();
    TestCompose();
    TestPickForRole();
    TestOpenRoleRotation();
    TestTargetGroups();
    TestFormDue();
    TestPickPlan();
    std::printf("bot lfg selection policy: %d checks passed\n", checks);
    return 0;
}
