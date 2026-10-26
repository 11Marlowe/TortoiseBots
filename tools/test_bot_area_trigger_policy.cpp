// Standalone check for ai/playerbot/BotAreaTriggerPolicy.h (issue #984):
// the inside test matches the core's IsPointInAreaTriggerZone, only listed,
// scripted triggers on dungeon maps (not raids) qualify, only a run/commander
// party or one with a human nearby fires, and a trigger fires once per entry.
//   g++ -std=c++17 -Wall -Wextra tools/test_bot_area_trigger_policy.cpp -o /tmp/test_bot_area_trigger_policy && /tmp/test_bot_area_trigger_policy
#include "../ai/playerbot/BotAreaTriggerPolicy.h"

#include <cstdio>
#include <cstdlib>

using namespace ai;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

static BotAreaTriggerShape Sphere(uint32_t id, float x, float y, float z, float r)
{
    BotAreaTriggerShape s;
    s.id = id; s.x = x; s.y = y; s.z = z; s.radius = r;
    return s;
}

static BotAreaTriggerShape Box(uint32_t id, float x, float y, float z, float bx, float by, float bz, float o)
{
    BotAreaTriggerShape s;
    s.id = id; s.x = x; s.y = y; s.z = z; s.boxX = bx; s.boxY = by; s.boxZ = bz; s.boxOrientation = o;
    return s;
}

int main()
{
    // at_zumrah (962): sphere radius 10 at (1909.27, 1015.11, 11.5155), map 209.
    BotAreaTriggerShape const zumrah = Sphere(962, 1909.27f, 1015.11f, 11.5155f, 10.0f);
    CHECK(IsInsideAreaTriggerShape(zumrah, 1909.27f, 1015.11f, 11.5155f, 0.0f));
    CHECK(IsInsideAreaTriggerShape(zumrah, 1918.0f, 1015.11f, 11.5155f, 0.0f));   // 8.7 yd
    CHECK(!IsInsideAreaTriggerShape(zumrah, 1920.5f, 1015.11f, 11.5155f, 0.0f));  // 11.2 yd
    CHECK(IsInsideAreaTriggerShape(zumrah, 1920.5f, 1015.11f, 11.5155f, 2.0f));   // within the margin
    CHECK(!IsInsideAreaTriggerShape(zumrah, 1909.27f, 1015.11f, 23.0f, 0.0f));    // 3-D: too high

    // Axis box 20 x 10 x 4 at the origin.
    BotAreaTriggerShape const box = Box(1, 0, 0, 0, 20.0f, 10.0f, 4.0f, 0.0f);
    CHECK(IsInsideAreaTriggerShape(box, 9.0f, 4.0f, 1.5f, 0.0f));
    CHECK(!IsInsideAreaTriggerShape(box, 11.0f, 0.0f, 0.0f, 0.0f));
    CHECK(!IsInsideAreaTriggerShape(box, 0.0f, 6.0f, 0.0f, 0.0f));
    CHECK(!IsInsideAreaTriggerShape(box, 0.0f, 0.0f, 2.5f, 0.0f));
    CHECK(IsInsideAreaTriggerShape(box, 11.0f, 0.0f, 0.0f, 2.0f));

    // The same box turned a quarter: its long side now runs along y.
    BotAreaTriggerShape const turned = Box(2, 0, 0, 0, 20.0f, 10.0f, 4.0f, 1.5707963f);
    CHECK(IsInsideAreaTriggerShape(turned, 0.0f, 9.0f, 0.0f, 0.0f));
    CHECK(!IsInsideAreaTriggerShape(turned, 9.0f, 0.0f, 0.0f, 0.0f));
    CHECK(IsInsideAreaTriggerShape(turned, 4.0f, -9.0f, 0.0f, 0.0f));

    // Filter: listed + scripted + dungeon map, and not a teleport, tavern or BG entrance.
    CHECK(IsBotFireableAreaTrigger(true, true, true, false, false, false));
    CHECK(!IsBotFireableAreaTrigger(false, true, true, false, false, false)); // not on the allowlist
    CHECK(!IsBotFireableAreaTrigger(true, false, true, false, false, false)); // no script (quest explore)
    CHECK(!IsBotFireableAreaTrigger(true, true, false, false, false, false)); // open world or a raid
    CHECK(!IsBotFireableAreaTrigger(true, true, true, true, false, false));   // entrance / exit
    CHECK(!IsBotFireableAreaTrigger(true, true, true, false, true, false));   // tavern
    CHECK(!IsBotFireableAreaTrigger(true, true, true, false, false, true));   // BG entrance

    // The index holds every trigger that could fire (scripted, dungeon, no teleport, tavern or BG
    // entrance), listed or not; which of them fire is decided per check (IsBotAreaTriggerArmed).
    CHECK(IsBotIndexableAreaTrigger(true, true, false, false, false));
    CHECK(!IsBotIndexableAreaTrigger(false, true, false, false, false));
    CHECK(!IsBotIndexableAreaTrigger(true, false, false, false, false));
    CHECK(!IsBotIndexableAreaTrigger(true, true, true, false, false));
    CHECK(!IsBotIndexableAreaTrigger(true, true, false, true, false));
    CHECK(!IsBotIndexableAreaTrigger(true, true, false, false, true));

    // Armed: a listed trigger, or one the bot's active run has armed (a move step's `entry` the run has
    // reached and that has not fired yet; null = none). Zul'Farrak's plan walks into 962 at leg 10 seq 101.
    std::vector<uint32_t> const runArmed = {962};
    std::vector<uint32_t> const none;
    CHECK(IsBotAreaTriggerArmed(true, 962, nullptr));        // listed, no run: as before
    CHECK(!IsBotAreaTriggerArmed(false, 962, nullptr));      // unlisted, no run: refused
    CHECK(!IsBotAreaTriggerArmed(false, 962, &none));        // unlisted, the run armed nothing (or it fired)
    CHECK(IsBotAreaTriggerArmed(false, 962, &runArmed));     // unlisted, the run armed it
    CHECK(!IsBotAreaTriggerArmed(false, 1526, &runArmed));   // another trigger during the run: refused
    CHECK(IsBotAreaTriggerArmed(true, 1526, &runArmed));     // listed stays listed
    std::vector<uint32_t> const two = {963, 962};
    CHECK(IsBotAreaTriggerArmed(false, 962, &two) && IsBotAreaTriggerArmed(false, 963, &two));
    CHECK(!IsBotAreaTriggerArmed(false, 0, &two));

    // The allowlist: 962 is the audited opt-in; the shipped default is empty.
    std::vector<uint32_t> const defaults = {962};
    CHECK(IsListedAreaTrigger(defaults, 962));
    CHECK(!IsListedAreaTrigger(defaults, 1526)); // at_ring_of_law (BRD arena)
    CHECK(!IsListedAreaTrigger(defaults, 1786)); // at_shadowforge_bridge
    CHECK(!IsListedAreaTrigger(defaults, 2046)); // UBRS stadium
    CHECK(!IsListedAreaTrigger(std::vector<uint32_t>{}, 962)); // empty: off

    // Party gate.
    CHECK(IsBotAreaTriggerPartyAllowed(true, false));   // all-bot run / commander party
    CHECK(IsBotAreaTriggerPartyAllowed(false, true));   // a human within 40 yd
    CHECK(IsBotAreaTriggerPartyAllowed(true, true));
    CHECK(!IsBotAreaTriggerPartyAllowed(false, false)); // follower alone, no run

    // Edge detection: fires once on entry, not while inside, again after leaving.
    BotAreaTriggerState st;
    CHECK(st.Update(209, 7, {}, {}).empty());
    std::vector<uint32_t> e = st.Update(209, 7, {962}, {962});
    CHECK(e.size() == 1 && e[0] == 962);
    CHECK(st.Update(209, 7, {962}, {962}).empty());
    // On the edge (outside, but inside the leave margin): still counts as inside.
    CHECK(st.Update(209, 7, {}, {962}).empty());
    CHECK(st.Update(209, 7, {962}, {962}).empty());
    // Left past the margin, then back: fires again.
    CHECK(st.Update(209, 7, {}, {}).empty());
    e = st.Update(209, 7, {962}, {962});
    CHECK(e.size() == 1 && e[0] == 962);
    // A new instance of the same map: the trigger fires again there.
    e = st.Update(209, 8, {962}, {962});
    CHECK(e.size() == 1 && e[0] == 962);
    // Two triggers at once, one already inside.
    e = st.Update(209, 8, {962, 963}, {962, 963});
    CHECK(e.size() == 1 && e[0] == 963);
    // Clear (dead, ungrouped, left the instance) forgets them.
    st.Clear();
    e = st.Update(209, 8, {962}, {962});
    CHECK(e.size() == 1 && e[0] == 962);

    // Step: 962 fires for an all-bot run party ...
    {
        BotAreaTriggerState run;
        int asked = 0;
        auto runParty = [&]() { ++asked; return IsBotAreaTriggerPartyAllowed(true, false); };
        CHECK(run.Step(209, 1, {}, {}, runParty).empty());
        CHECK(asked == 0); // not asked while outside every trigger
        e = run.Step(209, 1, {962}, {962}, runParty);
        CHECK(e.size() == 1 && e[0] == 962);
        CHECK(run.Step(209, 1, {962}, {962}, runParty).empty());
    }
    // ... but not for a grouped bot with no run and no human nearby,
    {
        BotAreaTriggerState alone;
        auto noRun = []() { return IsBotAreaTriggerPartyAllowed(false, false); };
        CHECK(alone.Step(209, 1, {962}, {962}, noRun).empty());
        CHECK(alone.Step(209, 1, {962}, {962}, noRun).empty());
        // and it fires once the player arrives while the bot still stands there.
        auto human = []() { return IsBotAreaTriggerPartyAllowed(false, true); };
        e = alone.Step(209, 1, {962}, {962}, human);
        CHECK(e.size() == 1 && e[0] == 962);
    }

    // A script that refused (OnAreaTrigger false: at_zumrah with Zum'rah not within 30 yd yet) is tried
    // again while the bot stays inside, every kBotAreaTriggerRetryMs (2 s = 4 checks), never every check;
    // one that ran (true) is not tried again while the bot stays inside.
    {
        BotAreaTriggerState rs;
        CHECK(kBotAreaTriggerRetryMs == 2000 && kBotAreaTriggerRetryMs % kBotAreaTriggerCheckMs == 0);
        std::vector<uint32_t> fire = rs.Update(209, 7, {962}, {962});
        CHECK(fire == std::vector<uint32_t>{962});
        rs.Refused(962);                                        // false: keep trying
        CHECK(rs.Update(209, 7, {962}, {962}).empty());          // 0.5 s
        CHECK(rs.Update(209, 7, {962}, {962}).empty());          // 1.0 s
        CHECK(rs.Update(209, 7, {962}, {962}).empty());          // 1.5 s
        fire = rs.Update(209, 7, {962}, {962});                  // 2.0 s: again
        CHECK(fire == std::vector<uint32_t>{962});
        // true this time: no Refused, so it is not tried again while the bot stays inside.
        for (int i = 0; i < 20; ++i)
            CHECK(rs.Update(209, 7, {962}, {962}).empty());

        // Always refused: bounded to one try per 2 s while inside (40 checks = 20 s -> 10 tries).
        BotAreaTriggerState always;
        int tries = 0;
        for (int i = 0; i < 40; ++i)
            for (uint32_t id : always.Update(209, 7, {962}, {962}))
            {
                CHECK(id == 962);
                ++tries;
                always.Refused(id);
            }
        CHECK(tries == 10);

        // A retry waits at the margin (near but not inside) and fires once the bot is inside again.
        BotAreaTriggerState edge;
        edge.Update(209, 7, {962}, {962});
        edge.Refused(962);
        for (int i = 0; i < 6; ++i)
            CHECK(edge.Update(209, 7, {}, {962}).empty());
        CHECK(edge.Update(209, 7, {962}, {962}) == std::vector<uint32_t>{962});

        // Leaving drops the retry; walking in again fires as an entry, at once.
        BotAreaTriggerState gone;
        gone.Update(209, 7, {962}, {962});
        gone.Refused(962);
        CHECK(gone.Update(209, 7, {}, {}).empty());
        CHECK(gone.Update(209, 7, {962}, {962}) == std::vector<uint32_t>{962});
        // A new instance or a party that may not fire forgets the retry.
        BotAreaTriggerState moved;
        moved.Update(209, 7, {962}, {962});
        moved.Refused(962);
        CHECK(moved.Update(209, 8, {}, {}).empty());
        for (int i = 0; i < 6; ++i)
            CHECK(moved.Update(209, 8, {}, {962}).empty());
        BotAreaTriggerState barred;
        barred.Update(209, 7, {962}, {962});
        barred.Refused(962);
        CHECK(barred.Step(209, 7, {962}, {962}, [] { return false; }).empty());
        CHECK(barred.retry.empty());
        // A short step out (beyond the margin) and back: the entry fires; when it runs (no Refused) the
        // old retry is gone and nothing fires again while the bot stays.
        BotAreaTriggerState back;
        back.Update(209, 7, {962}, {962});
        back.Refused(962);
        CHECK(back.Update(209, 7, {}, {}).empty());
        CHECK(back.Update(209, 7, {962}, {962}) == std::vector<uint32_t>{962});
        for (int i = 0; i < 6; ++i)
            CHECK(back.Update(209, 7, {962}, {962}).empty());
        // A new instance with the bot inside the same trigger id: an entry there; the old retry is gone.
        BotAreaTriggerState other;
        other.Update(209, 7, {962}, {962});
        other.Refused(962);
        CHECK(other.Update(209, 8, {962}, {962}) == std::vector<uint32_t>{962});
        for (int i = 0; i < 6; ++i)
            CHECK(other.Update(209, 8, {962}, {962}).empty());
        // Refused again restarts the 2 s wait.
        BotAreaTriggerState again;
        again.Update(209, 7, {962}, {962});
        again.Refused(962);
        CHECK(again.Update(209, 7, {962}, {962}).empty());
        CHECK(again.Update(209, 7, {962}, {962}).empty());
        again.Refused(962);
        CHECK(again.Update(209, 7, {962}, {962}).empty());
        CHECK(again.Update(209, 7, {962}, {962}).empty());
        CHECK(again.Update(209, 7, {962}, {962}).empty());
        CHECK(again.Update(209, 7, {962}, {962}) == std::vector<uint32_t>{962});
        // Refused on a trigger the bot is not inside of schedules nothing.
        BotAreaTriggerState none;
        none.Refused(963);
        for (int i = 0; i < 6; ++i)
            CHECK(none.Update(209, 7, {962}, {962}).size() == (i == 0 ? 1u : 0u));
    }

    std::printf("test_bot_area_trigger_policy: %d checks passed\n", checks);
    return 0;
}
