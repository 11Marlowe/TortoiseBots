#pragma once

// Pure policy for out-of-combat Totemic Recall (mod-playerbots parity
// SHM-5): recall standing totems for mana when nothing is fighting, but
// never destroy the mana-tide cooldown totem for pennies. Recall needs the
// spell trained and at least one totem down.
//
// There is no recall -> re-drop loop to guard against any more: the
// automatic totem rows only drop in combat or in the pre-pull window
// (TotemAutoRowsFire), so an out-of-combat recall is never answered by a
// re-drop — the set stays recalled until the next fight. The gate is
// deliberately state-based: a "recalled" latch cleared by a single combat
// tick would let a flicker restart the loop the rows can no longer feed
// (issue #1125: a drop/recall pair burning mana out of combat).

namespace ai
{
    // prepullWanted: a fight is imminent (ready-check anchor live or the
    // force-rebuff window open), so the rows own the standing set.
    inline bool TotemicRecallShouldFire(bool hasSpell, bool anyTotemDown, bool manaTideDown,
        bool botInCombat, bool anyMemberInCombat, bool prepullWanted = false)
    {
        if (!hasSpell || !anyTotemDown)
            return false;
        if (manaTideDown)
            return false;
        if (botInCombat || anyMemberInCombat)
            return false;
        if (prepullWanted)
            return false;
        return true;
    }
}
