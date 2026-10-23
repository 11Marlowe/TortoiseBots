// Standalone test for the issue #490 (bug part) hire source guard:
// hired bots never auto-take PvP/rep-flagged gear unless the matching knob
// allows it; pool bots and owned alts are untouched.
//
// Build and run (from the repo root):
//   g++ -std=c++17 -Wall -Wextra tools/test_hire_source_guard_policy.cpp -o /tmp/test_hire_source_guard
//   /tmp/test_hire_source_guard

#include "../runtime/policy/hire/HireSourceGuardPolicy.h"

#include <cassert>
#include <iostream>
#include <vector>

using namespace TortoiseBots;

static void TestNonHireAlwaysPasses()
{
    // Pool bots keep organic behaviour: flagged gear is wearable.
    assert(MayHireAutoTakeSource(kSourceFlagPvp, false, false, false));
    assert(MayHireAutoTakeSource(kSourceFlagRep, false, false, false));
    assert(MayHireAutoTakeSource(kSourceFlagPvp | kSourceFlagRep, false, false, false));
    assert(MayHireAutoTakeSource(0, false, false, false));
}

static void TestHireDefaultsBlock()
{
    // Default knobs (both 0): any flagged item is refused.
    assert(!MayHireAutoTakeSource(kSourceFlagPvp, true, false, false));
    assert(!MayHireAutoTakeSource(kSourceFlagRep, true, false, false));
    assert(!MayHireAutoTakeSource(kSourceFlagPvp | kSourceFlagRep, true, false, false));
    // Unflagged gear always passes.
    assert(MayHireAutoTakeSource(0, true, false, false));
}

static void TestKnobsAreIndependent()
{
    // PvP knob opens PvP only, not rep.
    assert(MayHireAutoTakeSource(kSourceFlagPvp, true, true, false));
    assert(!MayHireAutoTakeSource(kSourceFlagRep, true, true, false));
    assert(!MayHireAutoTakeSource(kSourceFlagPvp | kSourceFlagRep, true, true, false));
    // Rep knob opens rep only, not PvP.
    assert(MayHireAutoTakeSource(kSourceFlagRep, true, false, true));
    assert(!MayHireAutoTakeSource(kSourceFlagPvp, true, false, true));
    assert(!MayHireAutoTakeSource(kSourceFlagPvp | kSourceFlagRep, true, false, true));
    // Both knobs open everything.
    assert(MayHireAutoTakeSource(kSourceFlagPvp | kSourceFlagRep, true, true, true));
}

static void TestChoiceFilter()
{
    // Gated choice dropped when an ungated one exists.
    std::vector<uint32_t> best = {0, 1, 2};
    uint8_t flags[3] = {0, kSourceFlagPvp, 0};
    std::vector<uint32_t> kept = FilterHireGatedRewardChoices(best, flags, 3, true, false, false);
    assert(kept.size() == 2 && kept[0] == 0 && kept[1] == 2);
    // Non-hire: nothing filtered.
    kept = FilterHireGatedRewardChoices(best, flags, 3, false, false, false);
    assert(kept.size() == 3);
    // All gated: fall back to the plain ranking so the quest completes.
    uint8_t allGated[3] = {kSourceFlagPvp, kSourceFlagRep, kSourceFlagPvp};
    kept = FilterHireGatedRewardChoices(best, allGated, 3, true, false, false);
    assert(kept.size() == 3);
    // Knob open: gated choice survives.
    kept = FilterHireGatedRewardChoices(best, flags, 3, true, true, false);
    assert(kept.size() == 3);
    // Out-of-range index never escapes.
    std::vector<uint32_t> bad = {0, 7};
    kept = FilterHireGatedRewardChoices(bad, flags, 3, true, false, false);
    assert(kept.size() == 1 && kept[0] == 0);
}

int main()
{
    std::cout << "Starting TortoiseBots hire source-guard tests...\n";
    TestNonHireAlwaysPasses();
    std::cout << "  [PASS] non-hires always pass\n";
    TestHireDefaultsBlock();
    std::cout << "  [PASS] hire defaults block flagged gear\n";
    TestKnobsAreIndependent();
    std::cout << "  [PASS] knobs gate their own flag only\n";
    TestChoiceFilter();
    std::cout << "  [PASS] quest-choice filter with all-gated fallback\n";
    std::cout << "All hire source-guard tests passed.\n";
    return 0;
}
