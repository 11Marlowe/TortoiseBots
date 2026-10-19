// Standalone test for the raid quest chain rule (ai/playerbot/RaidQuestChainPolicy.h).
// A quest is raid when it is a direct raid quest, or when an earlier quest in
// its chain is a chain source (raid map or raid-exclusive start). Raid-type and
// raid-scale quests never taint successors. Cyclic chain data terminates. The
// direct and chain-source verdicts live in RandomItemMgr; here each quest
// carries its verdict.
//
// Build and run (from the repo root):
// g++ -std=c++17 -Wall -Wextra tools/test_raid_quest_chain_policy.cpp -o /tmp/test_raid_quest_chain
// /tmp/test_raid_quest_chain

#include "../ai/playerbot/RaidQuestChainPolicy.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>

using namespace TortoiseBots;

namespace
{
struct TestWorld
{
    QuestPredecessors predecessors;
    std::set<uint32_t> direct;      // direct raid quests (type 62, raid-scale, raid map, raid-exclusive start)
    std::set<uint32_t> chainSource; // raid map or raid-exclusive start: taints successors

    bool Reaches(uint32_t questId) const
    {
        return ReachesRaidQuest(questId,
            [this](uint32_t id) { return direct.count(id) != 0; },
            [this](uint32_t id) { return chainSource.count(id) != 0; },
            predecessors);
    }
};

// Onyxia-like: 7490 is started by a raid-exclusive item (chain source) and 7491 follows it.
void TestChainSourceTaintsFollowUp()
{
    TestWorld w;
    w.direct = {7490};
    w.chainSource = {7490};
    RecordQuestChainLinks(w.predecessors, 7490, 0, 7491, 0);
    assert(w.Reaches(7491));
    assert(w.Reaches(7490));
}

// Raid-map predecessor (8288) taints its follow-up.
void TestRaidMapPredecessorTaintsFollowUp()
{
    TestWorld w;
    w.direct = {8288};
    w.chainSource = {8288};
    RecordQuestChainLinks(w.predecessors, 8586, 8288, 0, 0);
    assert(w.Reaches(8586));
}

// Raid-type (62) predecessor is raid itself but does not taint its follow-up.
void TestRaidTypePredecessorDoesNotTaint()
{
    TestWorld w;
    w.direct = {4735};
    RecordQuestChainLinks(w.predecessors, 4736, 4735, 0, 0);
    assert(w.Reaches(4735));
    assert(!w.Reaches(4736));
}

// Raid-scale predecessor is raid itself but does not taint its follow-up.
void TestRaidScalePredecessorDoesNotTaint()
{
    TestWorld w;
    w.direct = {9000};
    RecordQuestChainLinks(w.predecessors, 9001, 9000, 0, 0);
    assert(w.Reaches(9000));
    assert(!w.Reaches(9001));
}

void TestOrdinaryChainIsNotRaid()
{
    TestWorld w;
    RecordQuestChainLinks(w.predecessors, 101, 100, 0, 0);
    RecordQuestChainLinks(w.predecessors, 102, 101, 0, 0);
    assert(!w.Reaches(100));
    assert(!w.Reaches(101));
    assert(!w.Reaches(102));
}

// 200 and 201 point at each other; the chain source at 201 still reaches 200 and the walk ends.
void TestCycleTerminates()
{
    TestWorld w;
    RecordQuestChainLinks(w.predecessors, 200, 201, 0, 0);
    RecordQuestChainLinks(w.predecessors, 201, 200, 0, 0);
    assert(!w.Reaches(200));
    w.chainSource = {201};
    assert(w.Reaches(200));
}

// 301 is a chain source following plain 300: the successor must not taint its predecessor.
void TestSuccessorDoesNotTaintPredecessor()
{
    TestWorld w;
    w.direct = {301};
    w.chainSource = {301};
    RecordQuestChainLinks(w.predecessors, 301, 300, 0, 0);
    assert(!w.Reaches(300));
    assert(w.Reaches(301));
}

void TestChainLinkDirections()
{
    QuestPredecessors p;
    RecordQuestChainLinks(p, 10, 0, 11, 0);     // NextQuestId: 11 follows 10
    RecordQuestChainLinks(p, 12, 0, 0, 13);     // NextQuestInChain: 13 follows 12
    RecordQuestChainLinks(p, 21, -20, 0, 0);    // negative PrevQuestId: 20 precedes 21
    assert(p[11] == std::vector<uint32_t>{10});
    assert(p[13] == std::vector<uint32_t>{12});
    assert(p[21] == std::vector<uint32_t>{20});
    assert(p.find(10) == p.end());
}
}

int main()
{
    TestChainSourceTaintsFollowUp();
    TestRaidMapPredecessorTaintsFollowUp();
    TestRaidTypePredecessorDoesNotTaint();
    TestRaidScalePredecessorDoesNotTaint();
    TestOrdinaryChainIsNotRaid();
    TestCycleTerminates();
    TestSuccessorDoesNotTaintPredecessor();
    TestChainLinkDirections();
    std::cout << "raid quest chain policy: all tests passed\n";
    return 0;
}
