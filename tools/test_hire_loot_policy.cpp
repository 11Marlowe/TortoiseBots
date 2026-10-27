// Standalone test for the hire loot policy: hired companions loot by default,
// and AiPlayerbot.HireLoot = 0 switches them off. Non-hires always pass.
//
// Build and run (from the repo root):
//   g++ -std=c++17 -Wall -Wextra tools/test_hire_loot_policy.cpp -o /tmp/test_hire_loot_policy
//   /tmp/test_hire_loot_policy

#include "../runtime/policy/hire/HireLootPolicy.h"

#include <cassert>
#include <iostream>

using namespace TortoiseBots;

static void TestHireLoot()
{
    // Default (flag on): hires loot. Flag off: hires do not.
    assert(HireMayLoot(true, true));
    assert(!HireMayLoot(true, false));
}

static void TestNonHiresUnaffected()
{
    // Pool bots and owned alts keep looting whatever the flag says.
    assert(HireMayLoot(false, true));
    assert(HireMayLoot(false, false));
}

int main()
{
    std::cout << "Starting TortoiseBots hire loot tests...\n";
    TestHireLoot();
    std::cout << "  [PASS] hires loot only when the flag is on\n";
    TestNonHiresUnaffected();
    std::cout << "  [PASS] non-hires unaffected by the flag\n";
    std::cout << "All hire loot tests passed.\n";
    return 0;
}
