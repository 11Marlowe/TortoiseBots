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
    // sleep, fear, a heal) outranks a plain damage cast among unclaimed
    // casts; a cast another bot already claimed ranks below every free
    // one, because two interrupters on one cast bar waste the second
    // cooldown while a free caster goes unhandled. A claim only lowers
    // the rank, never rejects: if the claiming bot never casts
    // (higher-priority action, death, spell on cooldown), the next bot
    // still takes the cast over.
    inline int InterruptTargetTier(bool priorityCast, bool claimedByOther)
    {
        return (claimedByOther ? 2 : 0) + (priorityCast ? 0 : 1);
    }
} // namespace ai
