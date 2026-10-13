#pragma once

namespace TortoiseBots {

// "Latest explicit player order wins over a pending automatic restore."
//
// A summon (`.bot summon`, summon-on-group-accept) puts the bot back on
// follow when it arrives. That restore is automatic, so it only holds while
// no later explicit order replaced it: an explicit stationary order (`stay`,
// `guard`, `hold`, `comestay`) given after the request and before the arrival
// cancels the restore (the bot lands and keeps holding). Such an order given
// before the request never reaches this object: it is created by the request,
// so the summon is the later order and restores follow as before. `come`
// does not count: it sends the bot to the player, which the summon does anyway.
//
// Plain data, no core types, so tools/test_summon_restore_policy.cpp can
// exercise it standalone.
struct SummonRestorePolicy
{
    // An explicit stationary order arrived while the summon was pending.
    void NoteHoldOrder() { holdOrderedSincePending = true; }

    bool ShouldRestoreFollow() const { return !holdOrderedSincePending; }

    bool holdOrderedSincePending = false;
};

} // namespace TortoiseBots
