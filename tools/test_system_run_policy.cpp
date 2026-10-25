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
    // A system order is refused with player-in-group whenever the bound group
    // has a real player OR the run is still flagged as paused for one: a
    // resume after the joined player left must not let an order through
    // before the tick clears the stale flag.
    CHECK(DecideSystemOrder(false, false) == SystemOrder::Allow);
    CHECK(DecideSystemOrder(true, false) == SystemOrder::RefusePlayerInGroup);
    CHECK(DecideSystemOrder(false, true) == SystemOrder::RefusePlayerInGroup);
    CHECK(DecideSystemOrder(true, true) == SystemOrder::RefusePlayerInGroup);
    CHECK(std::string(SystemOrderCode(SystemOrder::RefusePlayerInGroup)) == "player-in-group");
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
    TestParseArgs();
    TestLogLine();
    std::printf("system run policy: %d checks passed\n", checks);
    return 0;
}
