#pragma once

// Pure pick rules for the shared "interrupt target" value: which casting
// enemy a bot's interrupt should take.
//
// No core includes: callers in strategy/ translate game state into these
// plain inputs, so the rules stay testable in
// tools/test_interrupt_target_policy.cpp without the server.

#include <string>

namespace ai
{
    // BuffClaimRegistry key shared by every interrupt spell, on the victim's
    // guid: a rogue's Kick and a mage's Counterspell must not both commit to
    // the same cast bar, while two rogues' Kicks on different casters never
    // block each other.
    inline std::string InterruptClaimKey()
    {
        return "interrupt";
    }

    // Lower wins. A cast that disables or heals someone (crowd control,
    // sleep, fear, a heal) outranks a plain damage cast. A cast another bot
    // already claimed is rejected outright - two interrupters on one cast
    // bar waste the second cooldown. A claim is only written by a bot whose
    // interrupt is ready right now and expires in seconds, so a dead
    // claimant cannot wedge coverage for long.
    constexpr int kInterruptTargetRejected = -1;

    inline int InterruptTargetTier(bool priorityCast, bool claimedByOther)
    {
        if (claimedByOther)
            return kInterruptTargetRejected;
        return priorityCast ? 0 : 1;
    }
} // namespace ai
