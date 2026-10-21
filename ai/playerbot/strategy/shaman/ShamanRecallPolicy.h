#pragma once

// Pure policy for out-of-combat Totemic Recall (mod-playerbots parity
// SHM-5): recall standing totems for mana when nothing is fighting, but
// never destroy the mana-tide cooldown totem for pennies. Recall needs the
// spell trained and at least one totem down.

namespace ai
{
    // autoRowsArmed: the automatic totem rows would drop the same set again
    // a few ticks later (they are the only reason a totem was standing),
    // so the pair recall -> re-drop -> recall just burns mana and never
    // settles (issue #1125: a tank looping out of combat, and the same
    // pairing for every shaman). The refund is only worth taking when the
    // set is not about to be rebuilt: while the rows are quiet, i.e. in
    // manual drop mode, or outside the pre-pull window in prepull mode.
    inline bool TotemicRecallShouldFire(bool hasSpell, bool anyTotemDown, bool manaTideDown,
        bool botInCombat, bool anyMemberInCombat, bool prepullWanted = false, bool autoRowsArmed = false)
    {
        if (!hasSpell || !anyTotemDown)
            return false;
        if (manaTideDown)
            return false;
        if (botInCombat || anyMemberInCombat)
            return false;
        if (prepullWanted)
            return false;
        if (autoRowsArmed)
            return false;
        return true;
    }
}
