#pragma once

#include <cstdint>

#include "DevourMagicPolicy.h"
#include "SeductionPolicy.h"

// Solo warlock default-pet choice (pool bots only): once the bot knows
// Summon Voidwalker (697, trainer tier 10 in tw_world.spell_template) AND
// holds a Soul Shard (6265, the summon reagent) the autonomous upkeep
// summons the Voidwalker; below that it keeps the Imp. The shard gate keeps
// a shardless bot from queueing a summon that fails isPossible every tick
// (the "no pet" node's summon-imp fallback covers the petless case). The
// rule is a pure function of durable facts and is unit-tested on its own
// (tools/test_warlock_pet_policy.cpp). Owned/hired bots are never touched
// here: the "wrong pet" trigger refuses to fire while a real-player master
// answers for the bot, and the player-chosen strategies stay.
//
// Opt-in spec pet (AiPlayerbot.WarlockPetBySpec, default 0): with the toggle
// on, PvE (open world + dungeon) picks the demon that fits the talent tree
// instead of always the Voidwalker; see DecideWarlockSpecPet below.

namespace TortoiseBots
{

inline constexpr uint32_t WARLOCK_VOIDWALKER_SUMMON_SPELL = 697;
inline constexpr uint32_t WARLOCK_VOIDWALKER_PET_ENTRY = 1860;
inline constexpr uint32_t WARLOCK_IMP_PET_ENTRY = 416;
inline constexpr uint32_t WARLOCK_SUCCUBUS_SUMMON_SPELL = 712;
inline constexpr uint32_t WARLOCK_FELHUNTER_SUMMON_SPELL = 691;
inline constexpr uint32_t WARLOCK_SOUL_SHARD_ITEM = 6265;

enum class WarlockSoloPetDecision
{
    LeaveAlone,
    SummonVoidwalker,
};

struct WarlockSoloPetInputs
{
    bool hasRealPlayerMaster = false; // owned/hired bot: player decides
    bool knowsVoidwalker = false;     // HasSpell(697)
    bool hasSoulShard = false;        // holds a 6265 so the summon can fire
    bool hasPet = false;              // any live pet (GetPet / pet target)
    uint32_t currentPetEntry = 0;     // 0 when petless
};

inline WarlockSoloPetDecision DecideWarlockSoloPet(WarlockSoloPetInputs const& inputs)
{
    if (inputs.hasRealPlayerMaster)
        return WarlockSoloPetDecision::LeaveAlone;
    if (!inputs.knowsVoidwalker)
        return WarlockSoloPetDecision::LeaveAlone;
    // Summon Voidwalker consumes a shard; without one the trigger stays
    // quiet (the "no pet" node's summon-imp fallback covers the petless
    // case) so a shardless bot never queues a summon that fails isPossible.
    if (!inputs.hasSoulShard)
        return WarlockSoloPetDecision::LeaveAlone;
    if (inputs.hasPet && inputs.currentPetEntry == WARLOCK_VOIDWALKER_PET_ENTRY)
        return WarlockSoloPetDecision::LeaveAlone;
    if (inputs.hasPet && inputs.currentPetEntry != 0 &&
        inputs.currentPetEntry != WARLOCK_IMP_PET_ENTRY &&
        inputs.currentPetEntry != WARLOCK_VOIDWALKER_PET_ENTRY)
        return WarlockSoloPetDecision::LeaveAlone;
    return WarlockSoloPetDecision::SummonVoidwalker;
}

// ---------------------------------------------------------------------------
// Spec-fitting pet (opt-in, AiPlayerbot.WarlockPetBySpec). Behaviour ported
// from mod-playerbots (79bd428): the per-spec non-combat strategy (Affliction
// -> felhunter, Demonology -> felguard, Destruction -> imp) and its
// pass-through chain Felguard -> Felhunter -> Succubus -> Voidwalker -> Imp,
// which settles on the first demon the warlock has learned. Turtle 1.18.1 has
// no player Felguard, so the Demonology line lands on the Felhunter exactly as
// the donor chain does. The one deliberate difference is the tank rule: a
// warlock whose group has no tank keeps the Voidwalker, because the pet is
// then the tank (the donor ignores group roles).
//
// Pure function of durable facts; the caller gathers them once per summon
// decision (no per-tick work beyond a few HasSpell/strategy lookups).

enum class WarlockSpec
{
    Unknown,
    Affliction,
    Demonology,
    Destruction,
};

enum class WarlockPetKind
{
    None,
    Imp,
    Voidwalker,
    Succubus,
    Felhunter,
};

inline uint32_t WarlockPetEntry(WarlockPetKind kind)
{
    switch (kind)
    {
        case WarlockPetKind::Imp: return WARLOCK_IMP_PET_ENTRY;
        case WarlockPetKind::Voidwalker: return WARLOCK_VOIDWALKER_PET_ENTRY;
        case WarlockPetKind::Succubus: return WARLOCK_SUCCUBUS_PET_ENTRY;
        case WarlockPetKind::Felhunter: return WARLOCK_FELHUNTER_PET_ENTRY;
        default: return 0;
    }
}

inline WarlockPetKind WarlockPetKindFromEntry(uint32_t entry)
{
    switch (entry)
    {
        case WARLOCK_IMP_PET_ENTRY: return WarlockPetKind::Imp;
        case WARLOCK_VOIDWALKER_PET_ENTRY: return WarlockPetKind::Voidwalker;
        case WARLOCK_SUCCUBUS_PET_ENTRY: return WarlockPetKind::Succubus;
        case WARLOCK_FELHUNTER_PET_ENTRY: return WarlockPetKind::Felhunter;
        default: return WarlockPetKind::None;
    }
}

struct WarlockSpecPetInputs
{
    WarlockSpec spec = WarlockSpec::Unknown;
    bool knowsVoidwalker = false;   // HasSpell(697)
    bool knowsSuccubus = false;     // HasSpell(712)
    bool knowsFelhunter = false;    // HasSpell(691)
    bool hasSoulShard = false;      // holds a 6265; every demon but the Imp needs one
    bool petIsGroupTank = false;    // grouped and no other member fills the tank role
    bool hasPet = false;            // any live pet
    uint32_t currentPetEntry = 0;   // 0 when petless
    bool canSwapLivePet = false;    // pool bot, no player answers for it
};

// The demon the spec wants given what the warlock has learned (donor chain
// without Felguard). Imp is always known. Shards and the tank rule are not
// part of this: they are applied by DecideWarlockSpecPet.
inline WarlockPetKind WarlockSpecPetPreference(WarlockSpec spec, bool knowsVoidwalker, bool knowsSuccubus,
    bool knowsFelhunter)
{
    switch (spec)
    {
        case WarlockSpec::Destruction:
            return WarlockPetKind::Imp;
        case WarlockSpec::Affliction:
        case WarlockSpec::Demonology:
            if (knowsFelhunter)
                return WarlockPetKind::Felhunter;
            if (knowsSuccubus)
                return WarlockPetKind::Succubus;
            if (knowsVoidwalker)
                return WarlockPetKind::Voidwalker;
            return WarlockPetKind::Imp;
        default:
            return WarlockPetKind::None;
    }
}

enum class WarlockSpecPetDecision
{
    LeaveAlone,
    SummonImp,
    SummonVoidwalker,
    SummonSuccubus,
    SummonFelhunter,
};

inline WarlockSpecPetDecision DecideWarlockSpecPet(WarlockSpecPetInputs const& in)
{
    WarlockPetKind want = WarlockSpecPetPreference(in.spec, in.knowsVoidwalker, in.knowsSuccubus, in.knowsFelhunter);
    if (want == WarlockPetKind::None)
        return WarlockSpecPetDecision::LeaveAlone; // spec unknown: the existing rows decide
    // The pet is the group's tank: Voidwalker, as before the toggle.
    if (in.petIsGroupTank && in.knowsVoidwalker)
        want = WarlockPetKind::Voidwalker;

    if (in.hasPet)
    {
        // A live pet is swapped only for pool bots and only when it is one of
        // the four default demons; a player's choice (any bot with a player
        // master) and anything else stays.
        if (!in.canSwapLivePet)
            return WarlockSpecPetDecision::LeaveAlone;
        if (WarlockPetKindFromEntry(in.currentPetEntry) == WarlockPetKind::None)
            return WarlockSpecPetDecision::LeaveAlone;
        if (WarlockPetEntry(want) == in.currentPetEntry)
            return WarlockSpecPetDecision::LeaveAlone;
    }

    // Every demon but the Imp consumes a shard; without one stay quiet and
    // let the "no pet" summon-imp fallback cover a petless warlock.
    if (want != WarlockPetKind::Imp && !in.hasSoulShard)
        return WarlockSpecPetDecision::LeaveAlone;

    switch (want)
    {
        case WarlockPetKind::Imp: return WarlockSpecPetDecision::SummonImp;
        case WarlockPetKind::Voidwalker: return WarlockSpecPetDecision::SummonVoidwalker;
        case WarlockPetKind::Succubus: return WarlockSpecPetDecision::SummonSuccubus;
        case WarlockPetKind::Felhunter: return WarlockSpecPetDecision::SummonFelhunter;
        default: return WarlockSpecPetDecision::LeaveAlone;
    }
}

} // namespace TortoiseBots
