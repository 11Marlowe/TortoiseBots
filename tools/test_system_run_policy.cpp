// Standalone tests for the system run policy (bot LFG part 1):
// the server-owned bind gate, the joined-player pause step, the GM debug
// command grammar, and the server log line.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_system_run_policy.cpp -o /tmp/test_system_run_policy
//   /tmp/test_system_run_policy

#include "../runtime/SystemRunPolicy.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace TortoiseBots::SystemRun;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

static BindFacts Good()
{
    BindFacts f;
    f.enabled = true;
    f.lead = Member{true, true, false, false, false};
    f.leadGrouped = true;
    f.leadLeads = true;
    f.members = {Member{true, true, false, false, false}, Member{true, true, false, false, false}};
    return f;
}

static void TestSystemBindGate()
{
    CHECK(CheckBind(Good()) == Refusal::None);
    auto with = [](void (*edit)(BindFacts&)) { BindFacts f = Good(); edit(f); return CheckBind(f); };
    CHECK(with([](BindFacts& f) { f.enabled = false; }) == Refusal::Disabled);
    CHECK(with([](BindFacts& f) { f.lead.liveBot = false; }) == Refusal::LeadNotWorldBot);
    CHECK(with([](BindFacts& f) { f.lead.hired = true; f.lead.worldBot = false; }) == Refusal::LeadHired);
    CHECK(with([](BindFacts& f) { f.lead.worldBot = false; }) == Refusal::LeadNotWorldBot);
    CHECK(with([](BindFacts& f) { f.lead.inOtherBinding = true; }) == Refusal::LeadBound);
    CHECK(with([](BindFacts& f) { f.leadLeads = false; }) == Refusal::NotLeader);
    CHECK(with([](BindFacts& f) { f.raidOrBg = true; }) == Refusal::Raid);
    CHECK(with([](BindFacts& f) { f.realPlayerInGroup = true; }) == Refusal::HasPlayer);
    CHECK(with([](BindFacts& f) { f.groupBound = true; }) == Refusal::GroupBound);
    CHECK(with([](BindFacts& f) { f.members.clear(); }) == Refusal::NoMembers);
    CHECK(with([](BindFacts& f) { f.leadGrouped = false; f.leadLeads = false; }) == Refusal::None); // a lead alone with joiners
    CHECK(with([](BindFacts& f) { Member const m = f.members[0]; f.members.assign(4, m); }) == Refusal::None);
    CHECK(with([](BindFacts& f) { Member const m = f.members[0]; f.members.assign(5, m); }) == Refusal::TooLarge);
    CHECK(with([](BindFacts& f) { f.members[1].liveBot = false; }) == Refusal::MemberNotWorldBot);
    CHECK(with([](BindFacts& f) { f.members[1].hired = true; f.members[1].worldBot = false; }) == Refusal::MemberHired);
    CHECK(with([](BindFacts& f) { f.members[1].worldBot = false; }) == Refusal::MemberNotWorldBot);
    CHECK(with([](BindFacts& f) { f.members[1].inOtherBinding = true; }) == Refusal::MemberBound);
    CHECK(with([](BindFacts& f) { f.members[1].inOtherGroup = true; }) == Refusal::MemberGrouped);
    CHECK(std::string(RefusalCode(Refusal::None)) == "ok");
    CHECK(std::string(RefusalCode(Refusal::HasPlayer)) == "group-has-player");
    CHECK(std::string(RefusalCode(Refusal::MemberGrouped)) == "member-grouped");
    CHECK(std::string(RefusalCode(Refusal::LeadNotWorldBot)) == "no-bot");
}

static void TestJoinStep()
{
    CHECK(DecideJoinStep(true, false, false, true) == JoinStep::Pause);
    CHECK(DecideJoinStep(true, false, false, false) == JoinStep::None);
    CHECK(DecideJoinStep(false, true, true, false) == JoinStep::Resume);
    CHECK(DecideJoinStep(false, true, true, true) == JoinStep::None);
    CHECK(DecideJoinStep(false, true, false, false) == JoinStep::None); // another pause is not ours to end
    CHECK(DecideJoinStep(false, true, false, true) == JoinStep::None);
    CHECK(DecideJoinStep(false, false, true, false) == JoinStep::None); // done
}

static void TestStaleJoinFlag()
{
    // join -> paused+flag, GM resume, player leaves before the tick: the tick
    // clears the stale flag while the run goes with no player in the group,
    // so a later dead-member pause must NOT auto-resume.
    CHECK(DecideJoinStep(true, false, false, true) == JoinStep::Pause);
    bool flag = true; // the tick paused the run for the joined player
    flag = JoinFlagAfterScan(true /*running*/, false /*realPlayer*/, flag); // GM resumed, player left
    CHECK(!flag);
    CHECK(DecideJoinStep(false, true, flag, false) == JoinStep::None); // a dead-member pause stays paused
    CHECK(DecideJoinStep(false, true, true, false) == JoinStep::Resume); // without the clear it would resume
}

static void TestSystemOrderGate()
{
    // No player and no stale flag: every order is allowed.
    CHECK(DecideSystemOrder("cmd", false, false) == SystemOrder::Allow);
    CHECK(DecideSystemOrder("stop", false, false) == SystemOrder::Allow);
    CHECK(DecideSystemOrder("start rfc", false, false) == SystemOrder::Allow);
    CHECK(DecideSystemOrder("", false, false) == SystemOrder::Allow);
    // A halt or release verb stays allowed with a player in the group or a
    // stale joined-player flag: stop, status, pause and unbind never order or
    // move the player, so a GM always has a way out. A stale flag alone still
    // counts: a resume after the joined player left must not let an order
    // through before the tick clears the stale flag.
    for (char const* verb : {"stop", "status", "pause", "unbind", " STOP ", "Status"})
    {
        CHECK(DecideSystemOrder(verb, true, false) == SystemOrder::Allow);
        CHECK(DecideSystemOrder(verb, false, true) == SystemOrder::Allow);
        CHECK(DecideSystemOrder(verb, true, true) == SystemOrder::Allow);
    }
    // Cmd, resume and every other forwarded order make the bots act: refused
    // while a real player is in the group or the flag is set. An unknown verb
    // (a future addition not classified yet) fails closed.
    for (char const* verb : {"cmd", "start", "start rfc", "resume", "skip", "", "dance", "bind"})
    {
        CHECK(DecideSystemOrder(verb, true, false) == SystemOrder::RefusePlayerInGroup);
        CHECK(DecideSystemOrder(verb, false, true) == SystemOrder::RefusePlayerInGroup);
        CHECK(DecideSystemOrder(verb, true, true) == SystemOrder::RefusePlayerInGroup);
    }
    CHECK(std::string(SystemOrderCode(SystemOrder::RefusePlayerInGroup)) == "player-in-group");
}

static MovedBot Moved(uint64_t priorGroup, bool hadGroup, uint64_t priorLeader)
{
    MovedBot m;
    m.priorGroup = priorGroup;
    m.hadGroup = hadGroup;
    m.priorLeader = priorLeader;
    return m;
}

static void TestBindRestoreTrace()
{
    // A failure after 1 join and after 3 joins restores every moved bot to
    // its prior group and leader (or removes it from the new group when it
    // had none): the trace the glue replays runs in reverse join order.
    CHECK(RestorePlan({Moved(7, true, 70)}).size() == 1);
    CHECK(RestorePlan({Moved(7, true, 70)})[0].group == 7);
    CHECK(RestorePlan({Moved(7, true, 70)})[0].leader == 70);
    CHECK(RestorePlan({Moved(7, true, 70)})[0].hadGroup);
    CHECK(RestorePlan({Moved(0, false, 0), Moved(8, true, 80), Moved(9, true, 90)}).size() == 3);
    CHECK(RestorePlan({Moved(0, false, 0), Moved(8, true, 80), Moved(9, true, 90)})[0].group == 9);
    CHECK(RestorePlan({Moved(0, false, 0), Moved(8, true, 80), Moved(9, true, 90)})[0].leader == 90);
    CHECK(RestorePlan({Moved(0, false, 0), Moved(8, true, 80), Moved(9, true, 90)})[2].group == 0);
    CHECK(!RestorePlan({Moved(0, false, 0), Moved(8, true, 80), Moved(9, true, 90)})[2].hadGroup);
    CHECK(RestorePlan({Moved(0, false, 0), Moved(8, true, 80), Moved(9, true, 90)})[1].group == 8);
    CHECK(RestorePlan({}).empty());
}

static void TestParseArgs()
{
    CHECK(ParseArgs("bind").verb == Verb::Bind && ParseArgs("bind").rest.empty());
    CHECK(ParseArgs("BIND Ana Bob").verb == Verb::Bind && ParseArgs("BIND Ana Bob").rest == "Ana Bob");
    CHECK(ParseArgs("unbind").verb == Verb::Unbind && ParseArgs("unbind x").verb == Verb::Invalid);
    CHECK(ParseArgs("cmd enter near").verb == Verb::Cmd && ParseArgs("cmd enter near").rest == "enter near");
    CHECK(ParseArgs("cmd").verb == Verb::Invalid);
    CHECK(ParseArgs("start rfc").verb == Verb::Forward && ParseArgs("start rfc").rest == "start rfc");
    CHECK(ParseArgs("start").rest == "start");
    for (char const* v : {"pause", "resume", "skip", "stop", "status"})
        CHECK(ParseArgs(v).verb == Verb::Forward && ParseArgs(v).rest == v);
    CHECK(ParseArgs("start rfc player").verb == Verb::Invalid);
    CHECK(ParseArgs("start rfc speed=2").verb == Verb::Invalid);
    CHECK(ParseArgs("").verb == Verb::Invalid && ParseArgs("dance").verb == Verb::Invalid);
    CHECK(ParseArgs("list").verb == Verb::Invalid);
}

static void TestLogLine()
{
    CHECK(LogLine("Tankbot", "TBM:RUN|x") == "TortoiseBots: system run of Tankbot: TBM:RUN|x");
}

int main()
{
    TestSystemBindGate();
    TestJoinStep();
    TestStaleJoinFlag();
    TestSystemOrderGate();
    TestBindRestoreTrace();
    TestParseArgs();
    TestLogLine();
    std::printf("system run policy: %d checks passed\n", checks);
    return 0;
}
