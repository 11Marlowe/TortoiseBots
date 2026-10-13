// Standalone check for ai/playerbot/CreatureAuraKeepoutPolicy.h:
//   g++ -std=c++17 -Wall -Wextra tools/test_creature_aura_keepout_policy.cpp -o /tmp/test_cak && /tmp/test_cak
#include "../ai/playerbot/CreatureAuraKeepoutPolicy.h"
#include "../ai/playerbot/AvoidAoePolicy.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using namespace ai;

int main()
{
    std::cout << "Starting TortoiseBots creature aura keep-out tests...\n";

    // Table: the Creeping Sludge row, Turtle ids, only on Maraudon.
    {
        CreatureAuraKeepout const* row = FindCreatureAuraKeepout(349, 12222);
        CHECK(row != nullptr);
        CHECK(row->auraSpellId == 22638);
        CHECK(row->pulseRadius == 5.0f);
        CHECK(row->keepoutRadius == 8.0f);
        CHECK(FindCreatureAuraKeepout(0, 12222) == nullptr);   // other map
        CHECK(FindCreatureAuraKeepout(349, 12221) == nullptr); // Noxious Slime: no aura, not listed
        CHECK(FindCreatureAuraKeepout(349, 0) == nullptr);
    }
    std::cout << "  [PASS] table lookup by map and entry\n";

    // Every row keeps out past its pulse and fits the avoid-aoe radius cap,
    // so the existing strafe-first step-out handles it.
    for (CreatureAuraKeepout const& row : kCreatureAuraKeepouts)
    {
        CHECK(row.keepoutRadius > row.pulseRadius);
        CHECK(IsAvoidableAoeRadius(row.keepoutRadius));
        CHECK(CreatureAuraHazardRadius(row) > row.keepoutRadius);
        CHECK(IsAvoidableAoeRadius(CreatureAuraHazardRadius(row)));
    }
    std::cout << "  [PASS] keep-out wider than the pulse, inside the 15 yd cap\n";

    // Applies: toggle, row, alive, aura, and no explicit order on it.
    {
        CreatureAuraKeepout const* row = FindCreatureAuraKeepout(349, 12222);
        CHECK(CreatureAuraKeepoutApplies(true, row, true, true, false));
        CHECK(!CreatureAuraKeepoutApplies(false, row, true, true, false)); // toggle off: today's behaviour
        CHECK(!CreatureAuraKeepoutApplies(true, nullptr, true, true, false));
        CHECK(!CreatureAuraKeepoutApplies(true, row, false, true, false)); // dead
        CHECK(!CreatureAuraKeepoutApplies(true, row, true, false, false)); // aura gone
        CHECK(!CreatureAuraKeepoutApplies(true, row, true, true, true));   // player ordered the attack
    }
    std::cout << "  [PASS] applies only with toggle, live aura and no explicit order\n";

    // Step-out band and hysteresis: step out inside 8, paths kept outside 9.
    {
        CreatureAuraKeepout const& row = *FindCreatureAuraKeepout(349, 12222);
        CHECK(InsideCreatureAuraKeepout(row, 4.0f));  // melee reach: inside the pulse
        CHECK(InsideCreatureAuraKeepout(row, 8.0f));
        CHECK(!InsideCreatureAuraKeepout(row, 8.1f));
        CHECK(CreatureAuraHazardRadius(row) == 9.0f);
        // The existing escape step clears the keep-out rim from inside the pulse.
        CHECK(AoeEscapeStep(row.keepoutRadius, 8.0f) == 8.0f);
        CHECK(AoeEscapeStep(row.keepoutRadius, 15.0f) == 9.0f);
        CHECK(kCreatureAuraHazardSeconds >= 2);
    }
    std::cout << "  [PASS] step out inside 8 yd, hazard keeps paths 9 yd away\n";

    // Player order: the ordered creature is never a keep-out for that bot,
    // whatever its state; any other listed creature still is.
    {
        CreatureAuraKeepout const* row = FindCreatureAuraKeepout(349, 12222);
        bool const ordered = true;
        CHECK(!CreatureAuraKeepoutApplies(true, row, true, true, ordered));
        CHECK(CreatureAuraKeepoutApplies(true, row, true, true, !ordered));
    }
    std::cout << "  [PASS] attack order exempts only the ordered creature\n";

    // Follow near a keep-out. Sludge at the origin; rings 8 / 9 / 9.5 / 10 / 10.5.
    CreatureAuraKeepout const& sludge = *FindCreatureAuraKeepout(349, 12222);
    using Step = CreatureAuraFollowStep;
    auto plan = [&](bool engaged, float bx, float by, float px, float py, float cx, float cy,
                    float& hx, float& hy) {
        return PlanCreatureAuraFollow(sludge, engaged, bx, by, px, py, cx, cy, hx, hy);
    };
    auto dist = [](float x, float y) { return std::sqrt(x * x + y * y); };
    {
        CreatureAuraFollowRings const r = CreatureAuraFollowRingsFor(sludge);
        CHECK(r.keepout == 8.0f && r.hazard == 9.0f && r.hold == 9.5f);
        CHECK(r.backoff == 10.0f && r.holdOuter == 10.5f && r.release == 10.0f);
        // Every move target sits inside the hold band with margin both ways.
        CHECK(r.hold - r.hazard >= 0.5f && r.holdOuter - r.hold >= 0.5f);
        CHECK(r.backoff - r.hazard >= 0.5f && r.holdOuter - r.backoff >= 0.5f);
        CHECK(r.keepout < r.hazard && r.release > r.hold);
    }
    std::cout << "  [PASS] follow rings nest with 0.5 yd margins\n";

    // Bot east of the sludge, master (goal) 3 yd west of it: every distance
    // from 8 to 10.5 either backs off outward or holds; none follows.
    {
        float hx = 0.0f, hy = 0.0f;
        CHECK(plan(false, 4.0f, 0.0f, -3.0f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::StepOut);
        CHECK(plan(false, 8.0f, 0.0f, -3.0f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::StepOut);
        float const backoff[] = { 8.01f, 8.25f, 8.5f, 8.75f, 8.99f };
        for (float d : backoff)
        {
            hx = hy = 0.0f;
            CHECK(plan(false, d, 0.0f, -3.0f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::BackOff);
            // Outward along the sludge-bot line, to the back-off ring.
            CHECK(std::fabs(hx - 10.0f) < 0.01f && std::fabs(hy) < 0.01f);
        }
        float const hold[] = { 9.0f, 9.25f, 9.49f, 9.5f, 9.51f, 10.0f, 10.5f };
        for (float d : hold)
        {
            CHECK(plan(false, d, 0.0f, -3.0f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::Hold);
            CHECK(plan(true, d, 0.0f, -3.0f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::Hold);
        }
        // Oblique bot positions hold by distance too, not by axis.
        CHECK(plan(false, 9.49f * 0.6f, 9.49f * 0.8f, -3.0f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::Hold);
        CHECK(plan(false, 8.5f * 0.6f, -8.5f * 0.8f, -3.0f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::BackOff);
        CHECK(std::fabs(dist(hx, hy) - 10.0f) < 0.01f && hx > 0.0f && hy < 0.0f);
    }
    std::cout << "  [PASS] 8-10.5 yd: back off below 9, hold from 9 (incl. 9.49), step-out at 8\n";

    // Approach from outside the band: stop on the hold ring on the way in,
    // and the arrival (even a hair short after the height snap) holds.
    {
        float hx = 0.0f, hy = 0.0f;
        CHECK(plan(false, 20.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::Approach);
        CHECK(std::fabs(hx - 9.5f) < 0.01f && std::fabs(hy) < 0.01f);
        CHECK(plan(false, 10.6f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::Approach);
        CHECK(std::fabs(hx - 9.5f) < 0.01f);
        // Arrived 0.4 yd short or long of the hold point: hold.
        CHECK(plan(true, 9.1f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::Hold);
        CHECK(plan(true, 9.9f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::Hold);
        // Oblique approach: the entry point lies on the hold ring, bot side.
        CHECK(plan(false, -20.0f, 5.0f, 0.0f, 2.0f, 0.0f, 0.0f, hx, hy) == Step::Approach);
        CHECK(std::fabs(dist(hx, hy) - 9.5f) < 0.01f && hx < 0.0f);
        // The hold point is outside the hazard and the keep-out radius.
        CHECK(!InsideCreatureAuraKeepout(sludge, 9.5f) && 9.5f > CreatureAuraHazardRadius(sludge));
    }
    std::cout << "  [PASS] approach stops on the 9.5 yd ring; arrival +-0.4 yd holds\n";

    // A sludge creeping (2 yd/s, 0.25 yd per tick) toward a bot holding at
    // 9.5 yd, the master 3 yd behind it: hold, back off along the same line
    // before the step-out radius, hold again; never follow, never step out.
    {
        float bx = 9.5f;
        float const by = 0.0f;
        float hx = 0.0f, hy = 0.0f;
        int backOffs = 0;
        float closest = 100.0f;
        for (int i = 0; i <= 40; ++i)
        {
            float const cx = 0.25f * i; // sludge moves east, toward the bot
            float const d = bx - cx;
            closest = std::min(closest, d);
            Step const s = plan(true, bx, by, cx - 3.0f, 0.0f, cx, 0.0f, hx, hy);
            CHECK(s == Step::Hold || s == Step::BackOff);
            CHECK((s == Step::Hold) == (d >= 9.0f));
            if (s == Step::BackOff)
            {
                // On the sludge-bot line, 10 yd from the sludge.
                CHECK(std::fabs(hy) < 0.01f && std::fabs((hx - cx) - 10.0f) < 0.01f);
                bx = hx; // the bot lands there before the next tick
                ++backOffs;
            }
        }
        CHECK(backOffs >= 4);
        CHECK(closest > sludge.keepoutRadius + 0.5f); // never near the step-out
    }
    std::cout << "  [PASS] creeping sludge: hold, back off on the line, hold; no step-out\n";

    // Goal leaving the circle, with hysteresis: not engaged, the goal counts
    // inside up to 9.5 yd; engaged, it must clear 10 yd before follow resumes.
    {
        float hx = 0.0f, hy = 0.0f;
        CHECK(plan(false, 9.5f, 0.0f, 9.4f, 1.0f, 0.0f, 0.0f, hx, hy) != Step::Follow); // goal 9.45
        CHECK(plan(false, 9.5f, 0.0f, -9.8f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::Follow); // goal 9.8, fresh
        CHECK(plan(true, 9.5f, 0.0f, -9.8f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::Hold);    // goal 9.8, engaged
        CHECK(plan(true, 9.5f, 0.0f, -10.0f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::Hold);   // on the release ring
        CHECK(plan(true, 9.5f, 0.0f, -10.01f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::Follow); // left the circle
        CHECK(plan(true, 4.0f, 0.0f, -12.0f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::Follow); // goal gone: the step-out alone
        // Engaged bot outside the band, goal between 9.5 and 10 on its own
        // side: walk to the goal (the straight line never meets the hold ring).
        CHECK(plan(true, 20.0f, 0.0f, 9.8f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::Approach);
        CHECK(std::fabs(hx - 9.8f) < 0.01f && std::fabs(hy) < 0.01f);
        // ...and on the far side: stop where the line meets the hold ring.
        CHECK(plan(true, 20.0f, 0.0f, -9.8f, 0.0f, 0.0f, 0.0f, hx, hy) == Step::Approach);
        CHECK(std::fabs(hx - 9.5f) < 0.01f);
    }
    std::cout << "  [PASS] goal hysteresis: follow resumes only once the goal clears 10 yd\n";

    // Segment entry helper.
    {
        float hx = 0.0f, hy = 0.0f;
        CHECK(SegmentEntersCircle(20.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 9.5f, hx, hy));
        CHECK(std::fabs(hx - 9.5f) < 0.01f);
        CHECK(!SegmentEntersCircle(5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 9.5f, hx, hy));    // B inside
        CHECK(!SegmentEntersCircle(20.0f, 0.0f, 12.0f, 0.0f, 0.0f, 0.0f, 9.5f, hx, hy));  // stops short
        CHECK(!SegmentEntersCircle(20.0f, 12.0f, -20.0f, 12.0f, 0.0f, 0.0f, 9.5f, hx, hy)); // misses
        CHECK(!SegmentEntersCircle(20.0f, 0.0f, 30.0f, 0.0f, 0.0f, 0.0f, 9.5f, hx, hy));  // circle behind
    }
    std::cout << "  [PASS] segment entry point\n";

    std::cout << "All creature aura keep-out tests passed.\n";
    return 0;
}
