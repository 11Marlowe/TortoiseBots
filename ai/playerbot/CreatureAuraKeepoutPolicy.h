#pragma once

// Pure rules for the creature aura keep-out (AiPlayerbot.AvoidCreatureAuras).
// Some creatures carry a permanent damaging aura that pulses around them
// whether or not they are in combat. A bot must not stand, path or chase
// into that circle. The table lists each such creature by map and entry with
// the aura that marks it live, the aura's pulse radius and the keep-out
// radius the bot steps out from (pulse plus a drift margin).
//
// Behaviour reimplemented from the T-imothy tortoise-wow registry
// (modules/mod-dungeon-clear/.../Data/DcHazardRegistry.cpp, 0fb4d7c): every
// role stays out of the Creeping Sludge, and the keep-out is the 5 yd pulse
// plus 3 yd. No code copied. Turtle data: creature_template 12222 auras
// '22638'; spell 22638 is a 2000 ms periodic trigger of 22595, school damage
// at EffectRadiusIndex 8 (5 yd).
// Callers own game state (aura check, distances, the explicit order); this
// header owns the table and the decisions so tools/test_creature_aura_keepout_policy.cpp
// can test them. No core includes.

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace ai
{
    struct CreatureAuraKeepout
    {
        uint32_t mapId;
        uint32_t creatureEntry;
        uint32_t auraSpellId;
        float pulseRadius;   // radius of the damaging pulse (yd)
        float keepoutRadius; // bot steps out when this close (yd)
    };

    // Extra yards the movement hazard reaches past the keep-out radius, so a
    // bot that just stepped out is not chased or followed back to the rim.
    constexpr float kCreatureAuraHazardMargin = 1.0f;

    // Seconds a registered movement hazard lives; re-added every trigger tick
    // while the creature is near, so it lapses soon after the creature dies,
    // leaves or the bot moves on.
    constexpr unsigned kCreatureAuraHazardSeconds = 3;

    constexpr CreatureAuraKeepout kCreatureAuraKeepouts[] = {
        // Maraudon, Creeping Sludge, Poison Shock (180-220 nature every 2 s, 5 yd).
        { 349, 12222, 22638, 5.0f, 8.0f },
    };

    inline CreatureAuraKeepout const* FindCreatureAuraKeepout(uint32_t mapId, uint32_t creatureEntry)
    {
        for (CreatureAuraKeepout const& row : kCreatureAuraKeepouts)
            if (row.mapId == mapId && row.creatureEntry == creatureEntry)
                return &row;
        return nullptr;
    }

    // A listed creature is a keep-out for this bot when the toggle is on, the
    // creature is alive and carries the listed aura, and the player has not
    // ordered this bot to attack exactly that creature (the order wins).
    inline bool CreatureAuraKeepoutApplies(bool toggleOn, CreatureAuraKeepout const* row,
        bool alive, bool hasAura, bool isExplicitAttackTarget)
    {
        return toggleOn && row && alive && hasAura && !isExplicitAttackTarget;
    }

    // Step out while inside the keep-out radius. Both this and the movement
    // hazard radius are measured from the creature's centre (no bounding
    // radius), the way MovementAction's hazard checks measure, so an end
    // point the hazard accepts can never trigger a step-out.
    inline bool InsideCreatureAuraKeepout(CreatureAuraKeepout const& row, float distance)
    {
        return distance <= row.keepoutRadius;
    }

    // Radius registered as a movement hazard (end points and paths stay out).
    inline float CreatureAuraHazardRadius(CreatureAuraKeepout const& row)
    {
        return row.keepoutRadius + kCreatureAuraHazardMargin;
    }

    // Follow stops this far outside the hazard radius, so the hold point is
    // outside both the hazard and the keep-out radius.
    constexpr float kCreatureAuraFollowHoldMargin = 0.5f;

    // Where a follower holds when its follow goal (P) lies inside the hazard
    // circle (centre C, radius r): the point where the straight line from the
    // bot (B) to P first enters the circle grown by the hold margin. 2D.
    // Returns false when the goal is outside the circle (follow normally) or
    // the bot itself is already inside it (the step-out handles that).
    inline bool CreatureAuraFollowHoldPoint(float bx, float by, float px, float py,
        float cx, float cy, float r, float& hx, float& hy)
    {
        float const R = r + kCreatureAuraFollowHoldMargin;
        float const pdx = px - cx, pdy = py - cy;
        if (pdx * pdx + pdy * pdy > R * R)
            return false; // goal outside
        float const fx = bx - cx, fy = by - cy;
        float const c = fx * fx + fy * fy - R * R;
        if (c <= 0.0f)
            return false; // bot inside
        float const dx = px - bx, dy = py - by;
        float const a = dx * dx + dy * dy;
        if (a <= 0.0f)
            return false;
        float const b = 2.0f * (fx * dx + fy * dy);
        float const disc = b * b - 4.0f * a * c;
        if (disc < 0.0f)
            return false; // cannot happen with B outside and P inside
        float t = (-b - std::sqrt(disc)) / (2.0f * a);
        if (t < 0.0f)
            t = 0.0f;
        if (t > 1.0f)
            t = 1.0f;
        hx = bx + dx * t;
        hy = by + dy * t;
        return true;
    }
} // namespace ai
