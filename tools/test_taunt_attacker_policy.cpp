// Standalone regression test for the tank's "taunt the attacker of a
// non-tank" rule (ai/playerbot/strategy/values/TauntAttackerPolicy.h):
// which mob standing on a healer, DPS or off-target party member the tank
// Taunts, which it leaves alone (its own mob, another tank's, a CC'd,
// evading or held CC mark, out of range or sight, one it Taunted < 10 s ago,
// one it wins back by threat alone), the order between two candidates, and
// when Challenging Shout replaces the single Taunt.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_taunt_attacker_policy.cpp -o /tmp/test_taunt_attacker_policy
//   /tmp/test_taunt_attacker_policy

#include "../ai/playerbot/strategy/values/TauntAttackerPolicy.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace ai;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

// A creature hitting a DPS group member, 8 yd from the tank, in Taunt range
// and sight, the victim on top of its threat list.
static TauntAttackerCandidate OnDps()
{
    TauntAttackerCandidate c;
    c.victimInGroup = true;
    c.victimHealthPct = 80.0f;
    c.distance = 8.0f;
    c.inRange = true;
    c.inSight = true;
    c.tankThreat = 100.0f;
    c.victimThreat = 500.0f;
    return c;
}

static TauntAttackerCandidate OnHealer()
{
    TauntAttackerCandidate c = OnDps();
    c.victimIsHealer = true;
    return c;
}

int main()
{
    // The plain case: a mob standing on the healer or a DPS is Taunted.
    CHECK(TauntAttackerEligible(OnHealer()));
    CHECK(TauntAttackerEligible(OnDps()));

    // Its victim must be a party member, not the tank itself, not a tank.
    {
        TauntAttackerCandidate c = OnDps();
        c.victimInGroup = false;                   // a stranger or a pet
        CHECK(!TauntAttackerEligible(c));
        c = OnDps();
        c.victimIsSelf = true;                     // already on this tank
        CHECK(!TauntAttackerEligible(c));
        c = OnDps();
        c.victimIsTank = true;                     // an off-tank holds it
        CHECK(!TauntAttackerEligible(c));
    }

    // A player enemy (no threat list), a CC'd or evading mob: never.
    {
        TauntAttackerCandidate c = OnHealer();
        c.attackerIsPlayer = true;
        CHECK(!TauntAttackerEligible(c));
        c = OnHealer();
        c.crowdControlled = true;
        CHECK(!TauntAttackerEligible(c));
        c = OnHealer();
        c.evading = true;
        CHECK(!TauntAttackerEligible(c));
    }

    // A held CC mark only when the existing owner rule lets the tank's Taunt
    // touch it (CcHeldMarkTauntAllowed: not CC'd yet, hitting a healer).
    {
        TauntAttackerCandidate c = OnHealer();
        c.heldCcMark = true;
        CHECK(!TauntAttackerEligible(c));
        c.heldMarkTauntAllowed = true;
        CHECK(TauntAttackerEligible(c));
    }

    // No movement: out of Taunt range or out of sight is not a candidate.
    {
        TauntAttackerCandidate c = OnHealer();
        c.inRange = false;
        CHECK(!TauntAttackerEligible(c));
        c = OnHealer();
        c.inSight = false;
        CHECK(!TauntAttackerEligible(c));
    }

    // At most one Taunt per mob per 10 s.
    {
        TauntAttackerCandidate c = OnHealer();
        c.tauntedRecently = true;
        CHECK(!TauntAttackerEligible(c));
        CHECK(TauntAttackerRepeatMs == 10000u);
        CHECK(TauntedRecently(true, 1000u, 1000u));
        CHECK(TauntedRecently(true, 1000u, 10999u));
        CHECK(!TauntedRecently(true, 1000u, 11000u));
        CHECK(!TauntedRecently(false, 1000u, 1000u));
        // getMSTime wraps: the unsigned difference still counts.
        CHECK(TauntedRecently(true, 0xFFFFF000u, 0x00000100u));
    }

    // The tank wins the mob back by threat alone (>= 110 % of the victim's
    // threat in melee reach, 130 % outside it): no Taunt. Top threat below
    // that margin does not move the mob, so the Taunt still goes out.
    {
        TauntAttackerCandidate c = OnHealer();
        c.tankThreat = 560.0f;                     // 112 % of 500
        c.tankInMelee = true;
        CHECK(TankWinsBackUnaided(c));
        CHECK(!TauntAttackerEligible(c));
        c.tankInMelee = false;                     // needs 650 (130 %)
        CHECK(!TankWinsBackUnaided(c));
        CHECK(TauntAttackerEligible(c));
        c.tankThreat = 660.0f;
        CHECK(TankWinsBackUnaided(c));
        c = OnHealer();
        c.tankThreat = 520.0f;                     // top, but under 110 %
        c.tankInMelee = true;
        CHECK(!TankWinsBackUnaided(c));
        CHECK(TauntAttackerEligible(c));
        // A victim with no threat (a scripted or fixated attack) is not
        // the tank's to take: Taunt would not hold it.
        c = OnHealer();
        c.victimThreat = 0.0f;
        CHECK(TankWinsBackUnaided(c));
        CHECK(!TauntAttackerEligible(c));
    }

    // Order: healer victims first, then the lowest victim health, then the
    // nearest mob.
    {
        TauntAttackerCandidate healer = OnHealer();
        healer.victimHealthPct = 90.0f;
        healer.distance = 25.0f;
        TauntAttackerCandidate dps = OnDps();
        dps.victimHealthPct = 10.0f;
        dps.distance = 3.0f;
        CHECK(TauntAttackerBetter(healer, dps));
        CHECK(!TauntAttackerBetter(dps, healer));

        TauntAttackerCandidate low = OnDps();
        low.victimHealthPct = 30.0f;
        low.distance = 20.0f;
        TauntAttackerCandidate high = OnDps();
        high.victimHealthPct = 60.0f;
        high.distance = 4.0f;
        CHECK(TauntAttackerBetter(low, high));
        CHECK(!TauntAttackerBetter(high, low));

        TauntAttackerCandidate near = OnDps();
        near.distance = 6.0f;
        TauntAttackerCandidate far = OnDps();
        far.distance = 12.0f;
        CHECK(TauntAttackerBetter(near, far));
        CHECK(!TauntAttackerBetter(far, near));
        CHECK(!TauntAttackerBetter(near, near));

        std::vector<TauntAttackerCandidate> list;
        CHECK(TauntAttackerPick(list) == -1);
        list.push_back(dps);
        list.push_back(healer);
        TauntAttackerCandidate own = OnHealer();
        own.victimIsSelf = true;                   // ineligible, never picked
        own.distance = 1.0f;
        list.push_back(own);
        CHECK(TauntAttackerPick(list) == 1);
        list[1].crowdControlled = true;            // healer's mob sheeped: the DPS's
        CHECK(TauntAttackerPick(list) == 0);
        list[0].inRange = false;                   // nothing left
        CHECK(TauntAttackerPick(list) == -1);
    }

    // Challenging Shout replaces the single Taunt when three or more
    // eligible mobs (range/sight/repeat aside: the shout is an area around
    // the tank) stand within its radius.
    {
        CHECK(ChallengingShoutMinAttackers == 3);
        std::vector<TauntAttackerCandidate> list(3, OnHealer());
        CHECK(TauntAttackerShoutCount(list, 10.0f) == 3);
        CHECK(ChallengingShoutOnAttackers(list, 10.0f));
        list[2].distance = 10.5f;                  // outside the shout
        CHECK(TauntAttackerShoutCount(list, 10.0f) == 2);
        CHECK(!ChallengingShoutOnAttackers(list, 10.0f));
        list[2].distance = 9.0f;
        list[2].inRange = false;                   // Taunt range does not matter
        list[1].tauntedRecently = true;            // nor a recent single Taunt
        CHECK(ChallengingShoutOnAttackers(list, 10.0f));
        list[0].victimIsTank = true;               // a tank's mob does not count
        CHECK(!ChallengingShoutOnAttackers(list, 10.0f));
    }

    // Gate: a tank in a group whose Taunt can be cast now (Defensive
    // Stance, off cooldown); anything else keeps the row quiet.
    CHECK(TauntAttackerGate(true, true, true));
    CHECK(!TauntAttackerGate(false, true, true));
    CHECK(!TauntAttackerGate(true, false, true));
    CHECK(!TauntAttackerGate(true, true, false));

    std::printf("taunt attacker policy: %d checks passed\n", checks);
    return 0;
}
