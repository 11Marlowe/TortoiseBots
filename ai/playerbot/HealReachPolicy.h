#pragma once

// Pure gate for "party member to heal out of spell range" (the trigger that
// walks a healer to its heal target).
//
// The "party member to heal" value always names a tank, even at full health:
// in a fight that keeps the healer within reach of the tank before the first
// hit lands. Out of combat the same pick made the healer walk to the tank
// instead of following its leader. Holy and discipline priests, holy
// paladins and restoration druids and shamans wire the trigger into their
// non-combat strategies above the follow action. In Uldaman (2026-10-12) a
// holy priest of a full-health party stood 38-47 yd from its leader for over
// a minute on "reach party member to heal" while the others followed.
//
// mod-playerbots (79bd428) only wires this trigger into combat strategies.
// Here the out-of-combat rows stay, for the member who really is hurt after a
// fight: out of combat the reach fires only for a target below the
// almost-full-health line, the same line the out-of-combat heal triggers use.
//
// In combat a healer also stays put when another living healer of the group
// already has the ally in heal range and in sight (issue #904): walking across
// the room to an ally somebody else can heal killed healers (Magmadar, 2026-10-13:
// 18 of 86 healer deaths ended on "reach party member to heal"). When no other
// healer can reach the ally, the walk stays. Out of combat the rule does not
// apply: the healer's walk there only fires for a hurt member anyway.
//
// No core includes; tested in tools/test_heal_reach_policy.cpp.

namespace ai
{
    // True when one other group member covers the ally: a healer (not the
    // requester, not the ally itself), alive, on the same map, within heal
    // range of the ally and with line of sight to it. "maxDistance" is the
    // reach trigger's own limit, so both sides use the same range.
    inline bool HealerCoversAlly(bool isOtherHealer, bool alive, bool sameMap, float distanceToAlly,
        float maxDistance, bool inSight)
    {
        return isOtherHealer && alive && sameMap && inSight && distanceToAlly <= maxDistance;
    }

    inline bool HealReachWanted(bool inCombat, float targetHealthPct, float almostFullHealthPct,
        bool otherHealerCovers = false)
    {
        if (inCombat)
            return !otherHealerCovers;
        return targetHealthPct < almostFullHealthPct;
    }
}
