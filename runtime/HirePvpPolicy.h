#pragma once

// Hired companions are PvE-only unless AiPlayerbot.HireAllowPvP is set. Pure,
// core-free rules for the battleground boundary, unit-tested standalone
// (tools/test_hire_pvp_policy.cpp). Callers own the messaging and the native
// queue/BG calls; this header only decides.

namespace TortoiseBots
{

// A hire ends when its master is inside a battleground, or when the hire
// itself is (a group queue can carry it in before the sweep dequeues it).
inline bool ShouldDismissHireOnBattleground(bool hireAllowPvP, bool masterInBattleground,
    bool hireInBattleground)
{
    if (hireAllowPvP)
        return false;
    return masterInBattleground || hireInBattleground;
}

// Warn the master once per queue entry; never while already inside (the entry
// notice owns that case).
inline bool ShouldWarnMasterOnBgQueue(bool hireAllowPvP, bool masterInQueue, bool masterInBattleground)
{
    if (hireAllowPvP || masterInBattleground)
        return false;
    return masterInQueue;
}

// A queued hire leaves the queue again; one already inside is dismissed, not
// dequeued.
inline bool ShouldDequeueHireFromBgQueue(bool hireAllowPvP, bool hireQueued, bool hireInBattleground)
{
    if (hireAllowPvP || hireInBattleground)
        return false;
    return hireQueued;
}

// No hiring while the requester is in a battleground or its queue.
inline bool MayHireForPvpState(bool hireAllowPvP, bool requesterInBattleground, bool requesterInQueue)
{
    if (hireAllowPvP)
        return true;
    return !requesterInBattleground && !requesterInQueue;
}

} // namespace TortoiseBots
