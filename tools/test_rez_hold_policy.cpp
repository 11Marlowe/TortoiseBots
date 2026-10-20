#include "../runtime/RezHoldPolicy.h"

#include <cstdlib>
#include <iostream>
#include <string>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using namespace TortoiseBots::RezHold;

namespace
{
Member Alive(RezClass rez = RezClass::None, bool healer = false, bool tank = false)
{
    Member m;
    m.rez = rez;
    m.healer = healer;
    m.tank = tank;
    return m;
}

Member Dead(bool healer = false, bool tank = false, float yd = -1.0f)
{
    Member m;
    m.dead = true;
    m.healer = healer;
    m.tank = tank;
    m.corpseYd = yd;
    return m;
}

// warrior tank (0), priest healer (1), mage (2), rogue (3), hunter (4)
std::vector<Member> Party()
{
    return {Alive(RezClass::None, false, true), Alive(RezClass::Priest, true), Alive(), Alive(), Alive()};
}
} // namespace

int main()
{
    Inputs in;

    // Nobody dead: nothing to decide.
    {
        Result r = Decide(in, Party());
        CHECK(r.verdict == Verdict::None && r.reason == Reason::NoDeaths);
        CHECK(r.rezzerIdx == -1 && r.targetIdx == -1);
        CHECK(Decide(in, {}).verdict == Verdict::None);
    }

    // The mage dies, the priest lives with mana: hold, the priest raises the mage.
    {
        std::vector<Member> p = Party();
        p[2] = Dead();
        p[1].mana = 500;
        p[1].rezCost = 160;
        Result r = Decide(in, p);
        CHECK(r.verdict == Verdict::Hold && r.reason == Reason::Recovering);
        CHECK(r.rezzerIdx == 1 && r.targetIdx == 2);
        // A rezzer with no cost known is taken as able.
        p[1].mana = 0;
        p[1].rezCost = 0;
        CHECK(Decide(in, p).verdict == Verdict::Hold);
        // The mana line is inclusive: exactly the cost is enough, one short is not.
        p[1].mana = 160;
        p[1].rezCost = 160;
        CHECK(Decide(in, p).verdict == Verdict::Hold);
        p[1].mana = 159;
        CHECK(Decide(in, p).verdict == Verdict::WaitMana);
    }

    // The budget: the hold goes on until kHoldMs, then the revive goes out.
    {
        std::vector<Member> p = Party();
        p[2] = Dead();
        in.holdMs = kHoldMs - 1;
        CHECK(Decide(in, p).verdict == Verdict::Hold);
        in.holdMs = kHoldMs;
        Result r = Decide(in, p);
        CHECK(r.verdict == Verdict::SendRevive && r.reason == Reason::TimedOut);
        CHECK(r.rezzerIdx == 1 && r.targetIdx == 2);
        // A fight freezes the clock: no timeout, whatever the clock reads.
        in.partyEngaged = true;
        in.holdMs = kHoldMs * 10;
        r = Decide(in, p);
        CHECK(r.verdict == Verdict::Hold && r.reason == Reason::Fighting);
        in = Inputs();
    }

    // Bodies beyond the rezzers stretch the budget (kPerExtraCorpseMs each), up to the cap.
    {
        std::vector<Member> p = Party();
        p[2] = Dead();
        p[3] = Dead();
        p[4] = Dead();
        in.holdMs = kHoldMs + 2 * kPerExtraCorpseMs - 1; // 3 bodies, 1 rezzer: 2 extra
        CHECK(Decide(in, p).verdict == Verdict::Hold);
        in.holdMs = kHoldMs + 2 * kPerExtraCorpseMs;
        CHECK(Decide(in, p).reason == Reason::TimedOut);
        // Two rezzers share the bodies: only one extra.
        p[0] = Alive(RezClass::Paladin, false, true);
        in.holdMs = kHoldMs + kPerExtraCorpseMs - 1;
        CHECK(Decide(in, p).verdict == Verdict::Hold);
        in.holdMs = kHoldMs + kPerExtraCorpseMs;
        CHECK(Decide(in, p).reason == Reason::TimedOut);
        // The cap.
        CHECK(HoldBudgetMs(100, 1) == kHoldMaxMs);
        CHECK(HoldBudgetMs(1, 1) == kHoldMs);
        CHECK(HoldBudgetMs(0, 0) == kHoldMs);
        CHECK(HoldBudgetMs(3, 0) == kHoldMs + 2 * kPerExtraCorpseMs);
        in = Inputs();
    }

    // Mana: the only rezzer is short of it. Wait while it drinks, up to kManaWaitMs.
    {
        std::vector<Member> p = Party();
        p[2] = Dead();
        p[1].mana = 40;
        p[1].rezCost = 160;
        in.holdMs = kHoldMs + 5000; // past the plain budget: mana waits longer
        Result r = Decide(in, p);
        CHECK(r.verdict == Verdict::WaitMana && r.reason == Reason::LowMana);
        CHECK(r.rezzerIdx == 1 && r.targetIdx == 2);
        in.holdMs = kManaWaitMs - 1;
        CHECK(Decide(in, p).verdict == Verdict::WaitMana);
        in.holdMs = kManaWaitMs;
        r = Decide(in, p);
        CHECK(r.verdict == Verdict::SendRevive && r.reason == Reason::ManaTimedOut);
        // In a fight the clock stands still here too.
        in.partyEngaged = true;
        r = Decide(in, p);
        CHECK(r.verdict == Verdict::WaitMana && r.reason == Reason::Fighting);
        in = Inputs();
        // A second rezzer with mana wins over the one that drinks.
        p[3] = Alive(RezClass::Shaman);
        p[3].mana = 300;
        p[3].rezCost = 130;
        r = Decide(in, p);
        CHECK(r.verdict == Verdict::Hold && r.rezzerIdx == 3);
    }

    // No living resurrector: send the revive at once (the old rule).
    {
        std::vector<Member> p = Party();
        p[1] = Dead(true);
        Result r = Decide(in, p);
        CHECK(r.verdict == Verdict::SendRevive && r.reason == Reason::NoRezzer);
        CHECK(r.targetIdx == -1);
    }

    // A full wipe is given up on, whatever else the inputs say.
    {
        std::vector<Member> p = {Dead(), Dead(true), Dead()};
        Result r = Decide(in, p);
        CHECK(r.verdict == Verdict::GiveUp && r.reason == Reason::Wipe);
        in.reviveSent = true;
        CHECK(Decide(in, p).verdict == Verdict::GiveUp);
        in = Inputs();
    }

    // The revive went out already: it is not taken back.
    {
        std::vector<Member> p = Party();
        p[2] = Dead();
        in.reviveSent = true;
        Result r = Decide(in, p);
        CHECK(r.verdict == Verdict::SendRevive && r.reason == Reason::AlreadySent);
        in = Inputs();
    }

    // The elected rezzer: a healer before a non-healer, then group order.
    {
        std::vector<Member> p = {Alive(RezClass::Paladin, false, true), Alive(RezClass::Shaman),
            Alive(RezClass::Priest, true), Alive(), Dead()};
        Result r = Decide(in, p);
        CHECK(r.rezzerIdx == 2); // the healer, not the earlier paladin
        p[2] = Dead(true);
        r = Decide(in, p);
        CHECK(r.rezzerIdx == 0); // group order among non-healers
        CHECK(r.targetIdx == 2); // the dead healer first
    }

    // The target: dead healer, then dead tank, then group order.
    {
        std::vector<Member> p = {Alive(RezClass::Priest, true), Dead(), Dead(false, true), Dead(true), Alive()};
        CHECK(Decide(in, p).targetIdx == 3);
        p[3] = Alive();
        CHECK(Decide(in, p).targetIdx == 2);
        p[2] = Alive();
        CHECK(Decide(in, p).targetIdx == 1);
    }

    // A rezzer in a fight cannot cast: held while it is the only one, never gone.
    {
        std::vector<Member> p = Party();
        p[2] = Dead();
        p[1].inCombat = true;
        Result r = Decide(in, p);
        CHECK(r.verdict == Verdict::Hold && r.reason == Reason::RezzerBusy && r.rezzerIdx == 1);
        in.holdMs = kHoldMs;
        r = Decide(in, p);
        CHECK(r.verdict == Verdict::SendRevive && r.reason == Reason::TimedOut);
        in.partyEngaged = true;
        r = Decide(in, p);
        CHECK(r.verdict == Verdict::Hold && r.reason == Reason::Fighting);
        in = Inputs();
        // A free rezzer wins over the busy one.
        p[3] = Alive(RezClass::Paladin);
        r = Decide(in, p);
        CHECK(r.verdict == Verdict::Hold && r.reason == Reason::Recovering && r.rezzerIdx == 3);
    }

    // A Shaman alone is a rezzer of the normal kind: a Druid beside it is passed over.
    {
        std::vector<Member> p = {Alive(RezClass::Shaman), Alive(RezClass::Druid), Dead()};
        p[1].mana = 1;
        Result r = Decide(in, p);
        CHECK(r.verdict == Verdict::Hold && r.rezzerIdx == 0);
        p[0].rez = RezClass::Paladin;
        CHECK(Decide(in, p).rezzerIdx == 0);
    }

    // Bodies out of reach do not stretch the budget.
    {
        std::vector<Member> p = Party();
        p[2] = Dead();
        p[3] = Dead(false, false, kCorpseMaxYd + 50.0f);
        p[4] = Dead(false, false, kCorpseMaxYd + 50.0f);
        in.holdMs = kHoldMs;
        CHECK(Decide(in, p).reason == Reason::TimedOut);
        in = Inputs();
    }

    // The Druid: Rebirth is used only when no Priest, Paladin or Shaman lives.
    {
        std::vector<Member> p = {Alive(RezClass::Druid), Alive(), Dead()};
        Result r = Decide(in, p);
        CHECK(r.verdict == Verdict::Hold && r.rezzerIdx == 0 && r.targetIdx == 2);
        // ... it may cast in a fight, a normal class may not.
        p[0].inCombat = true;
        CHECK(Decide(in, p).reason == Reason::Recovering);
        p[0].inCombat = false;
        // A priest alive: the Druid is passed over, even for its mana (none needed) ...
        p[1] = Alive(RezClass::Priest, true);
        r = Decide(in, p);
        CHECK(r.rezzerIdx == 1);
        // ... and the priest short of mana is waited for, the Druid does not step in.
        p[1].mana = 1;
        p[1].rezCost = 160;
        r = Decide(in, p);
        CHECK(r.verdict == Verdict::WaitMana && r.rezzerIdx == 1);
        // The glue marks a Druid on cooldown as None: then there is no rezzer.
        p[0].rez = RezClass::None;
        p[1] = Alive();
        CHECK(Decide(in, p).reason == Reason::NoRezzer);
    }

    // Bodies far from the party are not walked to.
    {
        std::vector<Member> p = Party();
        p[2] = Dead(false, false, kCorpseMaxYd + 1.0f);
        Result r = Decide(in, p);
        CHECK(r.verdict == Verdict::SendRevive && r.reason == Reason::CorpseTooFar);
        // The line is inclusive; unknown (-1) counts as near.
        p[2].corpseYd = kCorpseMaxYd;
        CHECK(Decide(in, p).verdict == Verdict::Hold);
        p[2].corpseYd = -1.0f;
        CHECK(Decide(in, p).verdict == Verdict::Hold);
        // One far body, one near: the near one is the target, the far one is skipped.
        p[3] = Dead(true, false, 500.0f);
        r = Decide(in, p);
        CHECK(r.verdict == Verdict::Hold && r.targetIdx == 2);
    }

    CHECK(std::string(VerdictName(Verdict::Hold)) == "hold");
    CHECK(std::string(ReasonName(Reason::LowMana)).size() > 0);

    std::cout << "rez hold policy: ok\n";
    return 0;
}
