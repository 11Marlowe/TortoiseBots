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
    // The window only drops "wait for attack" when the pull put it there: a
    // bot that already had the player's own `co +wait for attack` keeps it,
    // and a player who adds it while the window runs makes it theirs.
    //
    // Kept in memory only (PlayerbotAI). A stored strategy snapshot never
    // carries a pull's own window (ReleaseDropsStrategy, PlayerbotDbStore::
    // Save), and a reload drops any window still running, so a relog never
    // comes back held.
    //
    // Pure state, no core includes; exercised by tools/test_pull_join_window_policy.cpp.
    struct PullJoinWindow
    {
        std::time_t start = 0;     // 0: no pull window is running
        bool ownsStrategy = false; // the pull added "wait for attack"

        // A pull holds the bot from `now`. playerWindow: the bot already had a
        // "wait for attack" of the player's own (not one an earlier pull left).
        void Start(std::time_t now, bool playerWindow)
        {
            start = now;
            ownsStrategy = !playerWindow;
        }

        // The pull landed, or the pullback tank is back at the anchor: the
        // join delay runs from now.
        void Restamp(std::time_t now)
        {
            if (start)
                start = now;
        }

        bool Running() const { return start != 0; }

        bool Expired(std::time_t now, unsigned seconds) const
        {
            return start != 0 && now - start >= static_cast<std::time_t>(seconds);
        }

        // Ending this window removes "wait for attack" (the pull's own) rather
        // than only the anchor hold; such a "wait for attack" is also never
        // stored with the bot.
        bool ReleaseDropsStrategy() const { return start != 0 && ownsStrategy; }

        // Someone else added "wait for attack" while the window runs: it is
        // theirs from now on and outlives the window.
        void OnStrategyAdded() { ownsStrategy = false; }

        void Clear()
        {
            start = 0;
            ownsStrategy = false;
        }
    };
}
