#pragma once

// Boost-readiness policy (issue #1166: priest boost triggers fire while the
// spell is not known or on cooldown, every tick).
//
// Boost triggers arm on a missing aura. A cooldown spell is missing its aura
// for almost the whole cooldown, and an untrained spell never has one, so
// without a readiness gate the row is armed every combat tick and the cast
// dies in isPossible - the spell audit measured 1,817 impossible berserking
// and 2,416 impossible ascendance frames in ~4,200 priest combat frames. The
// engine falls through to the next action, so no cast is lost, but the queue
// churns and the log fills with IMPOSSIBLE verdicts.
//
// mod-playerbots has the same hole: its BoostTrigger checks balance only
// (GenericTriggers.cpp:325-335) and never spell known/cooldown. Donor dodges
// the unknown-spell half at construction time (racials rows are built only
// under bot->HasSpell, RacialsStrategy.cpp:56-61) and gates the one cooldown
// trigger it noticed per-hand (ShadowfiendTrigger adds
// !bot->HasSpellCooldown(34433), PriestTriggers.cpp:46). Neither survives a
// live pool that starts at level 1 and climbs: readiness must be checked at
// trigger time.
//
// The predicates below are pure so they can be unit-tested; call sites pass
// the spellbook/cooldown facts: known = the "spell id" value is non-zero (it
// walks the bot's own spellbook, pet spells as fallback) and ready =
// sServerFacade.IsSpellReady(bot, spellId).

namespace ai
{
    // A boost trigger may arm only while its spell is trained and off cooldown.
    inline bool BoostReady(bool spellKnown, bool spellReady)
    {
        return spellKnown && spellReady;
    }

    // The shared "generic boost" racial row offers berserking then blood fury
    // to every class. Arm it only when one of the two is castable right now:
    // a bot trained in neither - or with its racial on cooldown - must not
    // arm the row at all, or both actions die in isPossible every tick.
    inline bool RacialBoostReady(bool firstCastable, bool secondCastable)
    {
        return firstCastable || secondCastable;
    }
}
