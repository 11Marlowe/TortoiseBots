#pragma once

#include <ctime>

namespace ai
{
    // The join window an ordered pull (`.bot action pull|pullback`) puts on a
    // held DPS bot (PausePartyDpsForPull): the "wait for attack" gate, which
    // keeps the bot out of the fight until the window has run out.
    //
    // The window has its own clock. It starts at the command, restarts when
    // the pull lands or the pullback tank is back (RestampPullParty), and ends
    // when `seconds` ("wait for attack time") have passed since then, in or out
    // of combat. Combat end, death and movement orders only release the
    // anchor ("pull hold" + its stay); they never touch the window. That is
    // what keeps a chain pull's join delay (pull B ordered while A is still
    // dying keeps B's window when A's death ends the fight) and what makes the
    // window always end: it used to be ended only through the anchor marker,
    // so a window whose anchor was released first (the fight over inside the
    // join delay, a death, a follow order) stayed on for good and held the
    // bot for the join delay at the start of every later fight.
    //
    // The pull owns only what it changed:
    // - "wait for attack" in the engines it added it to (one bit per BotState
    //   engine). An engine that already had it keeps the player's own copy,
    //   and any later change to the strategy in an engine (a player's `co +`,
    //   `co -` or `~` toggle, an attack order) makes that engine's copy no
    //   longer the pull's.
    // - "wait for attack time": the value from before the first pull of a
    //   chain is put back when the window ends, so the pull's delay never
    //   replaces the player's own setting.
    //
    // Kept in memory only (PlayerbotAI). A stored snapshot never carries the
    // pull's own copies or its delay (PlayerbotDbStore::Save), and a reload
    // ends any window still running, so a relog never comes back held.
    //
    // Pure state, no core includes; exercised by tools/test_pull_join_window_policy.cpp.
    struct PullJoinWindow
    {
        std::time_t start = 0;         // 0: no pull window is running
        unsigned addedEngines = 0;     // bit n: the pull added "wait for attack" to engine n
        unsigned savedWaitSeconds = 0; // "wait for attack time" before the pull
        bool leadKeyed = false;        // the run leader's pull: released on the tank's lead (PullLeadReleases), not on the timer alone
        unsigned minSeconds = 0;       // the join delay: the earliest lead release after the landing
        bool landed = false;           // Restamp(now, true) ran: the pull landed (ordinary pull) or the pullback tank is back

        static unsigned EngineBit(unsigned engine) { return 1u << engine; }

        // A pull holds the bot from `now`. addedMask: the engines this pull
        // just added "wait for attack" to. previousWaitSeconds: the bot's
        // "wait for attack time" before this pull wrote its delay. A pull
        // ordered while an earlier one still runs (a chain pull) keeps that
        // pull's engines and the value from before the first one.
        void Start(std::time_t now, unsigned addedMask, unsigned previousWaitSeconds, bool leadKeyedIn = false, unsigned minSecondsIn = 0)
        {
            if (start)
                addedEngines |= addedMask;
            else
            {
                addedEngines = addedMask;
                savedWaitSeconds = previousWaitSeconds;
            }
            start = now;
            leadKeyed = leadKeyedIn;
            minSeconds = minSecondsIn;
            landed = false;
        }

        // The pull landed, or the pullback tank is back at the anchor: the
        // join delay runs from now.
        void Restamp(std::time_t now, bool landedNow)
        {
            if (start)
            {
                start = now;
                if (landedNow)
                    landed = true;
            }
        }

        bool Running() const { return start != 0; }

        bool Expired(std::time_t now, unsigned seconds) const
        {
            return start != 0 && now - start >= static_cast<std::time_t>(seconds);
        }

        // Whether the copy of "wait for attack" in this engine is the pull's
        // (removed when the window ends, never stored with the bot).
        bool Added(unsigned engine) const { return start != 0 && (addedEngines & EngineBit(engine)) != 0; }

        // Someone else changed "wait for attack" in these engines (added,
        // removed or toggled it): whatever is there now is not the pull's.
        void Forget(unsigned engineMask) { addedEngines &= ~engineMask; }

        // The player sets "wait for attack time" while the window runs: the
        // pull keeps its delay, and the player's value is the one that comes
        // back when the window ends. False when no window runs (set it now).
        bool DeferPlayerWaitSeconds(unsigned seconds)
        {
            if (!start)
                return false;
            savedWaitSeconds = seconds;
            return true;
        }

        void Clear()
        {
            start = 0;
            addedEngines = 0;
            savedWaitSeconds = 0;
            leadKeyed = false;
            minSeconds = 0;
            landed = false;
        }
    };

    constexpr unsigned kPullLeadCapSeconds = 10;   // a lead-keyed window ends this long after the landing at the latest
    constexpr float kPullLeadPackYd = 30.0f;       // hostiles this close to the tank are "the pack"
    constexpr unsigned kPullLeadPanicHpPct = 60;   // tank below this: release (the kill matters more than the threat)

    struct PullLeadFacts
    {
        bool landed = false;          // PullJoinWindow::landed
        unsigned elapsedSeconds = 0;  // now - window start (start is the landing once landed)
        unsigned minSeconds = 0;      // PullJoinWindow::minSeconds
        bool tankAlive = false;       // a live main tank in the group, on the bot's map
        bool tankLow = false;         // its health < kPullLeadPanicHpPct
        bool botIsVictim = false;     // a live hostile's victim is this bot
        unsigned packUnits = 0;       // live, in-combat, not crowd-controlled group attackers within kPullLeadPackYd of the tank
        unsigned packOnTank = 0;      // ... of them, the ones whose victim is the tank
    };

    // True when a lead-keyed window may end now.
    inline bool PullLeadReleases(PullLeadFacts const& f)
    {
        if (!f.landed)
            return false;                               // the pullback tank is still on its way back: the hard ceiling decides
        if (f.elapsedSeconds >= kPullLeadCapSeconds || !f.tankAlive)
            return true;
        if (f.botIsVictim || f.tankLow)
            return true;                                // jrad7: a hit bot or a failing tank is released at once
        if (f.elapsedSeconds < f.minSeconds)
            return false;
        return f.packOnTank == f.packUnits;             // every mob of the pack is on the tank (none: nothing to wait for)
    }
}
