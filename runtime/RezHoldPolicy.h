#pragma once

// Rez hold: when a party member dies and a living resurrector remains, does
// the run leader HOLD for the resurrection, WAIT for the rezzer's mana, SEND
// the revive order (`cmd revive party`: release and corpse run), or GIVE UP?
//
// The run only HOLDS. It sends no order to the resurrector: the bots' own AI
// (PartyMemberToResurrect and the rez actions) decides who raises whom, and a
// resurrector drinks by itself when it is out of mana. The elected rezzer and
// target in the Result name who the run expects to act, for the status note
// and the trace; nothing else reads them.
//
// Derived from jrad7/mod-dungeon-clear
// src/Ai/Dungeon/DungeonClear/Util/DcRezDecision.h:Decide @ da0bdfbc
// (AGPL-3.0-or-later), rewritten for the TortoiseBots run leader and vanilla
// 1.12. What was kept: the verdict is a pure function of a plain roster
// snapshot; the expected rezzer is elected deterministically so every tick
// (and every reader of a trace) gets the same answer and a rezzer who died
// simply drops out of the next evaluation; healers before non-healers; the
// dead healer, then the dead tank, then group order; a fight freezes the
// recovery clock; a full wipe, no living resurrector and a clock that ran out
// all end the hold. What was dropped: the AzerothCore pieces (instance
// "encounter in progress" refusal, raid wipe fractions and entrance regroup,
// parallel rezzer/corpse pairs, the no-rezzer in-fight grace, death knights).
// What was added: vanilla classes only (Priest Resurrection, Paladin
// Redemption, Shaman Ancestral Spirit; a Druid's Rebirth only when no living
// priest, paladin or shaman is near, as the bots' own Rebirth trigger has it),
// a mana gate (a rezzer short of the spell's mana drinks first: WaitMana), a
// corpse too far to walk to, a budget growing with the corpses a rezzer has to
// raise one after the other (a resurrection takes a 10 s cast), and a cast
// that is under way finishing before the hold gives up.
//
// Nothing here touches a core type: runtime/DungeonRunPolicy.h feeds it from
// the facts the glue read this tick (behind AiPlayerbot.DungeonRunRezHold).
// Unit-tested in tools/test_rez_hold_policy.cpp.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace TortoiseBots {
namespace RezHold {

constexpr std::uint32_t kHoldMs = 60000;          // a rezzer that can act gets this long (the 10-20 RFC priest raised the
                                                  // mage at her corpse in 57 s; the old fixed 15 s released her first)
constexpr std::uint32_t kPerExtraCorpseMs = 12000; // ... plus this for each corpse beyond the rezzers (a 10 s cast and the walk)
constexpr std::uint32_t kHoldMaxMs = 120000;      // ... at most this
constexpr std::uint32_t kManaWaitMs = 90000;      // a rezzer short of mana is waited for this long in all, then the revive goes out
constexpr std::uint32_t kCastGraceMs = 12000;     // a resurrection under way (a cast, or a request not yet accepted) may finish this long
                                                  // after the budget, once: the cast is not thrown away for the clock
constexpr float kCorpseMaxYd = 80.0f;             // a corpse farther than this from the party is not walked to

enum class RezClass : std::uint8_t
{
    None,    // no resurrection spell (or it is on cooldown / without its reagent: the glue says so by choosing None)
    Priest,  // Resurrection
    Paladin, // Redemption
    Shaman,  // Ancestral Spirit
    Druid    // Rebirth: a battle rez, castable out of combat too (30 min cooldown, a reagent)
};

// One bot of the party, dead or alive.
struct Member
{
    bool dead = false;
    std::string name;               // for the status note only
    RezClass rez = RezClass::None;  // living members: the spell they could raise the dead with now
    bool blocksDruid = false;       // living priest, paladin or shaman on the map near the party, whatever it can cast
                                    // now: the bots' Rebirth trigger counts it, so the Druid stands aside for it
    bool healer = false;            // a healer rezzes first, and a dead healer is raised first
    bool tank = false;              // a dead tank is raised after the healer
    bool inCombat = false;          // living: carries the combat flag
    std::uint32_t mana = 0;         // living rezzers: current mana
    std::uint32_t rezCost = 0;      // ... and the mana of their best rez spell (0: unknown, taken as enough)
    float corpseYd = -1.0f;         // dead: how far its body lies from the party (-1: not known, taken as near)
};

struct Inputs
{
    std::uint32_t holdMs = 0;       // out-of-combat time since the hold began (the regroup's clock)
    std::uint32_t ableMs = 0;       // ... of which the time a rezzer could act (the glue restarts it while one drinks):
                                    // the hold budget runs on this, so a rezzer that was waited for mana gets its own
    bool rezInProgress = false;     // a party rezzer is casting a resurrection, or one waits for its target to accept
    bool partyEngaged = false;      // a fight is on: the clock stands still
    bool reviveSent = false;        // `revive party` already went out: the hold is over for good
};

enum class Verdict : std::uint8_t
{
    None,        // nobody is dead
    Hold,        // a rezzer can act: the run waits and sends no revive
    WaitMana,    // the rezzer is short of mana: the run waits while it drinks
    SendRevive,  // the hold is over: `cmd revive party` releases the dead
    GiveUp       // nobody is left alive to raise anyone: the wipe recovery
};

enum class Reason : std::uint8_t
{
    NoDeaths,
    Recovering,      // Hold: a rezzer with mana is elected
    RezzerBusy,      // Hold: every rezzer is in a fight and cannot cast yet (the run regroups only out of combat,
                     //   so the run never sees it; kept so the policy is complete on its own)
    Fighting,        // Hold: the clock stands still during a fight (likewise never seen in the run)
    Casting,         // Hold: the budget is spent but a resurrection is under way; it may finish
    LowMana,         // WaitMana
    Wipe,            // GiveUp: everyone is dead
    NoRezzer,        // SendRevive: no living member has a resurrection
    CorpseTooFar,    // SendRevive: every body lies beyond kCorpseMaxYd
    TimedOut,        // SendRevive: kHoldMs (and the extras) went by
    ManaTimedOut,    // SendRevive: kManaWaitMs went by with no rezzer back up
    AlreadySent      // SendRevive: the revive was sent already (the old rule: once)
};

struct Result
{
    Verdict verdict = Verdict::None;
    Reason reason = Reason::NoDeaths;
    int rezzerIdx = -1;  // the rezzer the run expects to act (or to drink in WaitMana); status note only
    int targetIdx = -1;  // the dead member it expects to be raised first; status note only
};

inline const char* VerdictName(Verdict v)
{
    switch (v)
    {
        case Verdict::None: return "none";
        case Verdict::Hold: return "hold";
        case Verdict::WaitMana: return "wait-mana";
        case Verdict::SendRevive: return "send-revive";
        default: return "give-up";
    }
}

inline const char* ReasonName(Reason r)
{
    switch (r)
    {
        case Reason::NoDeaths: return "no deaths";
        case Reason::Recovering: return "a resurrection is under way";
        case Reason::RezzerBusy: return "the resurrectors are in a fight";
        case Reason::Fighting: return "a fight is on";
        case Reason::Casting: return "a resurrection is being cast";
        case Reason::LowMana: return "the resurrector is short of mana";
        case Reason::Wipe: return "everyone is dead";
        case Reason::NoRezzer: return "no living resurrector";
        case Reason::CorpseTooFar: return "the bodies lie too far";
        case Reason::TimedOut: return "the resurrection took too long";
        case Reason::ManaTimedOut: return "the resurrector did not get its mana back in time";
        default: return "the revive was sent already";
    }
}

// Target order: a dead healer, then a dead tank, then group order, among the
// bodies within reach. -1 when none.
inline int PickTarget(const std::vector<Member>& members)
{
    int firstDead = -1, deadTank = -1;
    for (std::size_t i = 0; i < members.size(); ++i)
    {
        const Member& m = members[i];
        if (!m.dead || m.corpseYd > kCorpseMaxYd)
            continue;
        if (m.healer)
            return static_cast<int>(i);
        if (deadTank < 0 && m.tank)
            deadTank = static_cast<int>(i);
        if (firstDead < 0)
            firstDead = static_cast<int>(i);
    }
    return deadTank >= 0 ? deadTank : firstDead;
}

// The hold's budget: kHoldMs, longer for each body a rezzer has to raise after
// another one, capped at kHoldMaxMs.
inline std::uint32_t HoldBudgetMs(std::size_t bodies, std::size_t rezzers)
{
    std::size_t const per = rezzers > 0 ? rezzers : 1;
    std::size_t const extra = bodies > per ? bodies - per : 0;
    std::uint32_t const total = kHoldMs + static_cast<std::uint32_t>(extra) * kPerExtraCorpseMs;
    return total > kHoldMaxMs ? kHoldMaxMs : total;
}

// The verdict for this tick.
inline Result Decide(const Inputs& in, const std::vector<Member>& members)
{
    Result r;
    std::size_t dead = 0, alive = 0;
    for (const Member& m : members)
        (m.dead ? dead : alive)++;
    if (dead == 0)
        return r;
    if (alive == 0)
    {
        r.verdict = Verdict::GiveUp;
        r.reason = Reason::Wipe;
        return r;
    }
    // The revive went out already: it is not taken back (the release is done;
    // the corpse run and the regroup's own ceiling take it from here).
    if (in.reviveSent)
    {
        r.verdict = Verdict::SendRevive;
        r.reason = Reason::AlreadySent;
        return r;
    }

    // Who could raise anyone: a living Priest, Paladin or Shaman; a Druid only
    // when none of those three lives near (blocksDruid: counted by class, as the
    // Rebirth trigger does, not by what the bot can cast this second). His
    // Rebirth is a 30 minute cooldown and a reagent, the others' spells are
    // free of both.
    bool anyNormal = false, anyDruid = false, druidBlocked = false;
    for (const Member& m : members)
    {
        if (m.dead)
            continue;
        if (m.blocksDruid)
            druidBlocked = true;
        if (m.rez == RezClass::Priest || m.rez == RezClass::Paladin || m.rez == RezClass::Shaman)
            anyNormal = true;
        else if (m.rez == RezClass::Druid)
            anyDruid = true;
    }
    bool const useDruid = !anyNormal && !druidBlocked;
    if (!anyNormal && !(anyDruid && useDruid))
    {
        r.verdict = Verdict::SendRevive;
        r.reason = Reason::NoRezzer;
        return r;
    }

    r.targetIdx = PickTarget(members);
    if (r.targetIdx < 0)
    {
        r.verdict = Verdict::SendRevive;
        r.reason = Reason::CorpseTooFar;
        return r;
    }

    // Election among the eligible class set: able now (out of a fight unless it
    // is the Druid's battle rez, mana for the spell), healers before the rest,
    // then group order. A rezzer in a fight is busy, one short of mana is
    // drinking; neither is gone.
    int able = -1, ableHealer = -1, drinking = -1, busy = -1;
    std::size_t rezzers = 0;
    for (std::size_t i = 0; i < members.size(); ++i)
    {
        const Member& m = members[i];
        if (m.dead || m.rez == RezClass::None)
            continue;
        bool const normal = m.rez != RezClass::Druid;
        if (normal != anyNormal) // the Druid is passed over while a normal class can cast
            continue;
        ++rezzers;
        int const idx = static_cast<int>(i);
        if (m.inCombat && m.rez != RezClass::Druid)
        {
            if (busy < 0)
                busy = idx;
        }
        else if (m.mana < m.rezCost) // an unknown cost (0) is never short
        {
            if (drinking < 0)
                drinking = idx;
        }
        else
        {
            if (able < 0)
                able = idx;
            if (m.healer && ableHealer < 0)
                ableHealer = idx;
        }
    }

    std::size_t bodies = 0;
    for (const Member& m : members)
        if (m.dead && m.corpseYd <= kCorpseMaxYd)
            ++bodies;
    std::uint32_t const budget = HoldBudgetMs(bodies, rezzers);

    if (able >= 0)
    {
        r.rezzerIdx = ableHealer >= 0 ? ableHealer : able;
        if (in.partyEngaged)
        {
            r.verdict = Verdict::Hold;
            r.reason = Reason::Fighting;
        }
        else if (in.ableMs >= budget)
        {
            // Spent, but a cast under way finishes first (once: kCastGraceMs).
            bool const finishing = in.rezInProgress && in.ableMs < budget + kCastGraceMs;
            r.verdict = finishing ? Verdict::Hold : Verdict::SendRevive;
            r.reason = finishing ? Reason::Casting : Reason::TimedOut;
        }
        else
        {
            r.verdict = Verdict::Hold;
            r.reason = Reason::Recovering;
        }
        return r;
    }
    if (drinking >= 0)
    {
        r.rezzerIdx = drinking;
        if (in.partyEngaged)
        {
            r.verdict = Verdict::WaitMana;
            r.reason = Reason::Fighting;
        }
        else if (in.holdMs >= kManaWaitMs)
        {
            r.verdict = Verdict::SendRevive;
            r.reason = Reason::ManaTimedOut;
        }
        else
        {
            r.verdict = Verdict::WaitMana;
            r.reason = Reason::LowMana;
        }
        return r;
    }
    // Only busy rezzers (a fight with the Priest in it): they are not gone.
    r.rezzerIdx = busy;
    if (in.partyEngaged)
    {
        r.verdict = Verdict::Hold;
        r.reason = Reason::Fighting;
    }
    else if (in.holdMs >= budget)
    {
        r.verdict = Verdict::SendRevive;
        r.reason = Reason::TimedOut;
    }
    else
    {
        r.verdict = Verdict::Hold;
        r.reason = Reason::RezzerBusy;
    }
    return r;
}

} // namespace RezHold
} // namespace TortoiseBots
