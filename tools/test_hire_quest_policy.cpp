// Standalone test for the hire quest-autonomy policy (play-test 2026-10-13):
// hired companions follow their master and never run their own questing /
// travel or talk to NPCs - and never to a <Mercenary Hire> recruiter. Owned
// alts keep their explicit orders: every rule keys off the hire record, so
// non-hires always pass.
//
// Build and run (from the repo root):
//   g++ -std=c++17 -Wall -Wextra tools/test_hire_quest_policy.cpp -o /tmp/test_hire_quest_policy
//   /tmp/test_hire_quest_policy

#include "../runtime/policy/hire/HireQuestPolicy.h"

#include <cassert>
#include <iostream>

using namespace TortoiseBots;

static void TestRecruiterRange()
{
    // Every <Mercenary Hire> recruiter entry is covered.
    assert(IsHireQuestRecruiterEntry(95000));
    assert(IsHireQuestRecruiterEntry(95017));
    assert(IsHireQuestRecruiterEntry(95067));
    assert(!IsHireQuestRecruiterEntry(94999));
    assert(!IsHireQuestRecruiterEntry(95068));
    assert(!IsHireQuestRecruiterEntry(12345));
}

static void TestAutonomyStrip()
{
    // Hires skip the autonomy kit; everyone else keeps today's behaviour.
    assert(HireSkipsAutonomyStrategies(true));
    assert(!HireSkipsAutonomyStrategies(false));
}

static void TestQuestTalk()
{
    // Hires never do quests, not even on explicit orders; non-hires always pass.
    assert(!HireMayDoQuests(true));
    assert(HireMayDoQuests(false));
}

static void TestQuestTravel()
{
    // Hires never request quest travel; non-hires keep today's behaviour.
    assert(!HireMayRequestQuestTravel(true));
    assert(HireMayRequestQuestTravel(false));
}

int main()
{
    std::cout << "Starting TortoiseBots hire quest-autonomy tests...\n";
    TestRecruiterRange();
    std::cout << "  [PASS] recruiter entry range\n";
    TestAutonomyStrip();
    std::cout << "  [PASS] autonomy kit stripped for hires only\n";
    TestQuestTalk();
    std::cout << "  [PASS] quest talk needs an explicit order on hires\n";
    TestQuestTravel();
    std::cout << "  [PASS] quest travel refused for hires\n";
    std::cout << "All hire quest-autonomy tests passed.\n";
    return 0;
}
