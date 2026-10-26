// A dead bot whose dungeon run is active stays at its body (runtime/RunReleaseHold.h;
// SM-REVIVE-PATH-2026-10-23 fix 3). Scarlet Monastery r21-19z40e slot 7: the lead bot,
// a hired bot on the PlayerMaster lease, auto-released 9 s after it died while two
// party members still fought, and the core raised it at the Graveyard wing's door.
// The wiring into AutoReleaseSpiritAction::isUseful is pinned in
// tools/test_run_release_hold_wiring.py.
#include "../runtime/RunReleaseHold.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using namespace TortoiseBots;
using namespace TortoiseBots::RunReleaseHold;

static bool s_answer = false;
static Player* s_asked = nullptr;
static bool FakeQuery(Player* bot)
{
    s_asked = bot;
    return s_answer;
}

int main()
{
    Player* const bot = reinterpret_cast<Player*>(0x1000); // never dereferenced

    // No run leader registered (the module without it, Sagiroth's dev): never held.
    CHECK(!HeldByRun(bot)); // the default, before anything registered
    Register(nullptr);
    CHECK(!HeldByRun(bot));
    CHECK(Decide(BotActivity::PlayerMaster, HeldByRun(bot), false, false) == AutoRelease::Stock);

    // The run leader answers: held while its run is active, not after.
    Register(&FakeQuery);
    s_answer = true;
    CHECK(HeldByRun(bot));
    CHECK(s_asked == bot);
    CHECK(!HeldByRun(nullptr)); // no bot: the query is not asked
    s_answer = false;           // the run ended, was stopped, unbound or hit its time limit
    CHECK(!HeldByRun(bot));

    // A hired run bot (PlayerMaster lease) of an active run waits, fight or not, rezzer or not:
    // the run leader's regroup (a resurrection in the party, then `revive party`) releases it.
    CHECK(Decide(BotActivity::PlayerMaster, true, false, false) == AutoRelease::Wait);
    CHECK(Decide(BotActivity::PlayerMaster, true, true, false) == AutoRelease::Wait);
    CHECK(Decide(BotActivity::PlayerMaster, true, false, true) == AutoRelease::Wait);
    // ... and once its run is over the stock rules decide again (the release works again).
    CHECK(Decide(BotActivity::PlayerMaster, false, false, false) == AutoRelease::Stock);
    CHECK(Decide(BotActivity::PlayerMaster, false, true, true) == AutoRelease::Stock);

    // The Dungeon lease keeps its own gate: wait while a rezzer lives or the crew fights.
    CHECK(Decide(BotActivity::Dungeon, false, true, false) == AutoRelease::Wait);
    CHECK(Decide(BotActivity::Dungeon, false, false, true) == AutoRelease::Wait);
    CHECK(Decide(BotActivity::Dungeon, false, false, false) == AutoRelease::Release);
    CHECK(Decide(BotActivity::Dungeon, true, false, false) == AutoRelease::Release);

    // Random bots and every other lease are not changed, even if a query said "held".
    for (BotActivity other : {BotActivity::Idle, BotActivity::Grinding, BotActivity::Trading,
                              BotActivity::LftQueued, BotActivity::BgQueued})
    {
        CHECK(Decide(other, true, false, false) == AutoRelease::Stock);
        CHECK(Decide(other, false, true, true) == AutoRelease::Stock);
    }

    Register(nullptr);
    std::cout << "run release hold policy: ok\n";
    return 0;
}
