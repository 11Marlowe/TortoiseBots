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

    // ---- Follow near a keep-out (HoldFollowOutsideKeepout) ----------------
    //
    // While the follow goal lies inside a keep-out, the follower must never be
    // sent toward it across the hold ring. Rings around the creature, centre
    // measure, Creeping Sludge values in brackets:
    //   keepout   [8.0]  inside: the avoid-aoe step-out owns the bot
    //   hazard    [9.0]  8..9: back off along the creature-bot line to `backoff`
    //   hold      [9.5]  where an approaching follower stops (and the goal test)
    //   backoff   [10.0] where a back-off lands
    //   holdOuter [10.5] 9..10.5: hold (stop a running follow, issue nothing)
    //   release   [10.0] once engaged, the goal must clear this to follow again
    // Every move target lands inside the hold band with at least 0.5 yd to
    // either edge, so a height snap, a short arrival or the creature creeping
    // at 2 yd/s cannot flip hold/follow/step-out from one tick to the next.
    constexpr float kCreatureAuraFollowHoldMargin = 0.5f;    // hold ring past the hazard
    constexpr float kCreatureAuraFollowBackoffMargin = 1.0f; // back-off ring past the hazard
    constexpr float kCreatureAuraFollowOuterMargin = 1.5f;   // hold band edge past the hazard
    constexpr float kCreatureAuraFollowReleaseMargin = 1.0f; // engaged goal release past the hazard

    struct CreatureAuraFollowRings
    {
        float keepout, hazard, hold, backoff, holdOuter, release;
    };

    inline CreatureAuraFollowRings CreatureAuraFollowRingsFor(CreatureAuraKeepout const& row)
    {
        float const hazard = CreatureAuraHazardRadius(row);
        return { row.keepoutRadius, hazard, hazard + kCreatureAuraFollowHoldMargin,
            hazard + kCreatureAuraFollowBackoffMargin, hazard + kCreatureAuraFollowOuterMargin,
            hazard + kCreatureAuraFollowReleaseMargin };
    }

    // First point where the segment from B (outside the circle) to P enters
    // the circle (centre C, radius R). False when B is not outside, or the
    // segment never reaches the circle. 2D.
    inline bool SegmentEntersCircle(float bx, float by, float px, float py,
        float cx, float cy, float R, float& hx, float& hy)
    {
        float const fx = bx - cx, fy = by - cy;
        float const c = fx * fx + fy * fy - R * R;
        if (c <= 0.0f)
            return false; // B not outside
        float const dx = px - bx, dy = py - by;
        float const a = dx * dx + dy * dy;
        if (a <= 0.0f)
            return false;
        float const b = 2.0f * (fx * dx + fy * dy);
        float const disc = b * b - 4.0f * a * c;
        if (disc < 0.0f)
            return false; // the line misses the circle
        float const t = (-b - std::sqrt(disc)) / (2.0f * a);
        if (t < 0.0f || t > 1.0f)
            return false; // circle behind B, or beyond P
        hx = bx + dx * t;
        hy = by + dy * t;
        return true;
    }

    enum class CreatureAuraFollowStep
    {
        Follow,   // goal clear of the keep-out: native follow
        StepOut,  // bot inside the keep-out: stop following, the step-out moves it
        BackOff,  // bot inside the hazard: move to (hx, hy), outward on the creature-bot line
        Hold,     // bot in the hold band: stop a running follow, issue no move
        Approach, // bot outside the band: move to (hx, hy), never inside the hold ring
    };

    // One follow decision against one keep-out creature (centre C) for a bot
    // at B whose follow goal is P. `engaged`: the previous decision against
    // this creature was not Follow (goal hysteresis: an engaged follower
    // resumes only once the goal is past the release ring).
    inline CreatureAuraFollowStep PlanCreatureAuraFollow(CreatureAuraKeepout const& row, bool engaged,
        float bx, float by, float px, float py, float cx, float cy, float& hx, float& hy)
    {
        CreatureAuraFollowRings const rings = CreatureAuraFollowRingsFor(row);
        float const gdx = px - cx, gdy = py - cy;
        float const goalLimit = engaged ? rings.release : rings.hold;
        if (gdx * gdx + gdy * gdy > goalLimit * goalLimit)
            return CreatureAuraFollowStep::Follow;

        float const fx = bx - cx, fy = by - cy;
        float const d = std::sqrt(fx * fx + fy * fy);
        if (d <= rings.keepout)
            return CreatureAuraFollowStep::StepOut;
        if (d < rings.hazard)
        {
            // d > keepout > 0, so the direction is defined.
            hx = cx + fx / d * rings.backoff;
            hy = cy + fy / d * rings.backoff;
            return CreatureAuraFollowStep::BackOff;
        }
        if (d <= rings.holdOuter)
            return CreatureAuraFollowStep::Hold;
        // Outside the band: walk to where the straight line to the goal meets
        // the hold ring. An engaged goal between the hold and release rings
        // may be reachable without touching the ring; then walk to it.
        if (!SegmentEntersCircle(bx, by, px, py, cx, cy, rings.hold, hx, hy))
        {
            hx = px;
            hy = py;
        }
        return CreatureAuraFollowStep::Approach;
    }

    // ---- Chase near a keep-out (HoldChaseOutsideKeepout) ------------------
    //
    // A chase (reach melee, reach spell, ...) is planned with the same bands,
    // the goal being the point the chase wants the bot at: `distance` from the
    // target toward the bot, i.e. the target itself for a melee chase
    // (distance 0). A melee bot whose target is a keep-out creature, or stands
    // within the hold ring of one, so stops outside the ring instead of
    // chasing into the pulse; a ranged chase whose goal lies well outside the
    // ring is left alone.
    inline void CreatureAuraChaseGoal(float bx, float by, float tx, float ty, float distance,
        float& gx, float& gy)
    {
        float const dx = bx - tx, dy = by - ty;
        float const d = std::sqrt(dx * dx + dy * dy);
        if (distance <= 0.0f || d <= 0.0f)
        {
            gx = tx;
            gy = ty;
            return;
        }
        float const k = (distance < d ? distance : d) / d;
        gx = tx + dx * k;
        gy = ty + dy * k;
    }

    // ---- Step-out landing near other keep-outs -----------------------------
    //
    // Radius a step-out landing must clear around a listed creature other than
    // the one the bot steps away from (sludges stand in groups): its keep-out
    // on the first pass, only its pulse on the relaxed second pass, so a bot
    // in a cluster with no landing outside every keep-out still never steps
    // into another pulse.
    inline float CreatureAuraLandingClearance(CreatureAuraKeepout const& row, bool relaxed)
    {
        return relaxed ? row.pulseRadius : row.keepoutRadius;
    }
} // namespace ai
