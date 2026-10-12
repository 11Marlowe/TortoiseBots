#include "../ai/playerbot/PullJoinWindowPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

namespace
{
// A held DPS bot reduced to the state the pull hold touches, wired the way
// PlayerbotAI / PausePartyDpsForPull / PlayerbotDbStore use the policy.
struct Bot
{
    bool waitStrategy = false; // "wait for attack" in its engines
    bool anchor = false;       // posMap "pull hold" + the stay it owns
    unsigned waitSeconds = 10; // "wait for attack time"
    ai::PullJoinWindow window;

    // PlayerbotAI::ChangeStrategy
    void AddWait() { waitStrategy = true; window.OnStrategyAdded(); }
    void DropWait() { waitStrategy = false; window.Clear(); anchor = false; }

    // PausePartyDpsForPull
    void Hold(std::time_t now, unsigned seconds)
    {
        anchor = true;
        waitSeconds = seconds;
        bool const playerWindow = waitStrategy && !window.ReleaseDropsStrategy();
        AddWait();
        window.Start(now, playerWindow);
    }

    // RestampPullParty (landing / pullback tank back)
    void Land(std::time_t now) { if (waitStrategy) window.Restamp(now); }

    // OnCombatEnded, death, SetMovementStrategy: the anchor only.
    void AnchorReleased() { anchor = false; }

    // "pull hold expired" -> "release pull hold" -> PlayerbotAI::EndPullWindow
    bool Tick(std::time_t now)
    {
        if (!waitStrategy || !window.Expired(now, waitSeconds))
            return false;
        if (window.ReleaseDropsStrategy())
            DropWait();
        else
        {
            window.Clear();
            anchor = false;
        }
        return true;
    }

    // PlayerbotDbStore::Save / Load + DropPullAfterReload
    bool StoredWait() const { return waitStrategy && !window.ReleaseDropsStrategy(); }
    void Reload(bool storedWait, bool storedAnchor)
    {
        waitStrategy = storedWait;
        anchor = storedAnchor;
        window.Clear();
        anchor = false;
    }

    // WaitForAttackStrategy::ShouldWait, reduced to the window.
    bool HoldsBack(std::time_t now) const { return waitStrategy && !window.Expired(now, waitSeconds); }
};
}

int main()
{
    std::cout << "Starting TortoiseBots pull join window policy tests...\n";

    // Plain pull: held from the command, the delay restarts at the landing.
    {
        Bot bot;
        bot.Hold(100, 10);
        CHECK(bot.waitStrategy && bot.anchor && bot.window.Running());
        bot.Land(104);
        CHECK(!bot.Tick(113));
        CHECK(bot.Tick(114));
        CHECK(!bot.waitStrategy && !bot.anchor && !bot.window.Running());
    }
    std::cout << "  [PASS] plain pull: window ends join delay after the landing\n";

    // The leak (headless repro L1b): the mob dies inside the window. Combat
    // end releases the anchor; the window must still end on its own clock,
    // and the next fight must not be held.
    {
        Bot bot;
        bot.Hold(100, 60);
        bot.Land(102);
        bot.AnchorReleased(); // fight over at 122
        CHECK(bot.waitStrategy);
        CHECK(!bot.Tick(150));
        CHECK(bot.Tick(162));
        CHECK(!bot.waitStrategy);
        CHECK(!bot.HoldsBack(400)); // a later fight starts unhindered
    }
    std::cout << "  [PASS] window ends after combat end released the anchor\n";

    // A movement order (follow) or a death during the hold drops the anchor
    // only; the window still ends.
    {
        Bot bot;
        bot.Hold(100, 10);
        bot.AnchorReleased();
        CHECK(bot.Tick(110));
        CHECK(!bot.waitStrategy && !bot.window.Running());
    }
    std::cout << "  [PASS] window ends after a movement order or death\n";

    // Chain pull: B ordered while A is still dying. A's death ends the fight
    // (anchor released), B lands later: B's join delay runs from B's landing.
    {
        Bot bot;
        bot.Hold(100, 10);    // pull A
        bot.Land(102);
        bot.Hold(105, 10);    // pull B while A is alive
        CHECK(bot.window.ReleaseDropsStrategy()); // still the pull's own
        bot.AnchorReleased(); // A dies at 107: combat end
        CHECK(!bot.Tick(108));
        bot.Land(109);        // B lands
        CHECK(bot.HoldsBack(118));
        CHECK(!bot.Tick(118));
        CHECK(bot.Tick(119));
        CHECK(!bot.waitStrategy);
    }
    std::cout << "  [PASS] chain pull keeps its join delay across A's death\n";

    // A failed pull frees the party at once (ReleaseHeldPartyNow: wait time 0,
    // clock now).
    {
        Bot bot;
        bot.Hold(100, 10);
        bot.waitSeconds = 0;
        CHECK(bot.Tick(103));
        CHECK(!bot.waitStrategy);
    }
    std::cout << "  [PASS] failed pull releases the hold on the next tick\n";

    // The player's own `co +wait for attack` survives the pull's window.
    {
        Bot bot;
        bot.AddWait();
        bot.Hold(100, 10);
        CHECK(!bot.window.ReleaseDropsStrategy());
        bot.AnchorReleased();
        CHECK(bot.Tick(110));
        CHECK(bot.waitStrategy && !bot.anchor && !bot.window.Running());
    }
    std::cout << "  [PASS] player's own wait for attack outlives the window\n";

    // ...also when the player adds it while the pull's window runs.
    {
        Bot bot;
        bot.Hold(100, 10);
        bot.AddWait();
        CHECK(bot.Tick(110));
        CHECK(bot.waitStrategy);
    }
    std::cout << "  [PASS] wait for attack added during the window stays the player's\n";

    // Attack / flee / `co -wait for attack` end the window at once.
    {
        Bot bot;
        bot.Hold(100, 10);
        bot.DropWait();
        CHECK(!bot.anchor && !bot.window.Running());
        CHECK(!bot.Tick(200));
    }
    std::cout << "  [PASS] explicit attack/flee ends the window\n";

    // Relog / DB restore: a pull's own window is never stored, and a reload
    // drops whatever pull was running.
    {
        Bot bot;
        bot.Hold(100, 10);
        CHECK(!bot.StoredWait());
        bot.Reload(bot.StoredWait(), true);
        CHECK(!bot.waitStrategy && !bot.anchor && !bot.window.Running());

        Bot own;
        own.AddWait();
        own.Hold(100, 10);
        CHECK(own.StoredWait()); // the player's own is stored as usual
        own.Reload(own.StoredWait(), true);
        CHECK(own.waitStrategy && !own.anchor && !own.window.Running());
    }
    std::cout << "  [PASS] nothing of a pull survives a relog\n";

    std::cout << "All TortoiseBots pull join window policy tests passed.\n";
    return 0;
}
