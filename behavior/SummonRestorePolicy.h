#pragma once

namespace TortoiseBots {

// "Latest explicit player order wins over a pending automatic restore."
//
// A summon (`.bot summon`, summon-on-group-accept) puts the bot back on
// follow when it arrives. That restore is automatic, so it only holds while
// no later explicit order replaced it: an explicit stationary order (`stay`,
// `guard`, `hold`, `comestay`) given after the request and before the arrival
// cancels the restore (the bot lands and keeps holding). `come` ("come to me,
// then follow") is the latest order too and clears such a hold again, so
// follow is restored. An order given before the request never reaches this
// object: it is created by the request, so the summon is the later order and
// restores follow as before.
//
// Plain data, no core types, so tools/test_summon_restore_policy.cpp can
// exercise it standalone.
struct SummonRestorePolicy
{
    // An explicit stationary order arrived while the summon was pending.
    void NoteHoldOrder() { holdOrderedSincePending = true; }

    // An explicit `come` arrived: the bot is told to come, then follow.
    void NoteComeOrder() { holdOrderedSincePending = false; }

    bool ShouldRestoreFollow() const { return !holdOrderedSincePending; }

    bool holdOrderedSincePending = false;
};

} // namespace TortoiseBots
