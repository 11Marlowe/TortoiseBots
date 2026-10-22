#pragma once

// Idle formation station-keeping: a follow slot is usually closer to the
// master than the follow trigger's 5 yd dead zone, so a bot standing next
// to its master never re-fires "update follow" and never walks to its
// formation slot. Changing the formation (queue, circle, shield, ...) then
// visibly does nothing until the party moves far enough to break the dead
// zone. This header owns the pure decision (should this idle bot walk to
// its formation slot now), the trigger owns the game state, so the rule
// stays testable in tools/test_formation_station_policy.cpp. No core
// includes.

#include <cmath>

namespace ai
{
    // How far (2D, yards) a bot may stand from its formation slot before it
    // counts as out of place. Below this the core follow's own dead zone
    // absorbs the error; walking for less would jitter the bot every tick.
    // Well under the follow trigger's 5 yd dead zone, so an idle bot inside
    // that zone still corrects to its slot.
    constexpr float kFormationStationToleranceYd = 1.0f;

    // Pure decision: out of place when beyond the tolerance. The caller
    // passes the bot's current 2D distance to its own formation slot.
    inline bool FormationStationWanted(float slotDistance2d)
    {
        if (slotDistance2d != slotDistance2d)
            return false;
        if (slotDistance2d < 0.0f)
            return false;
        return slotDistance2d > kFormationStationToleranceYd;
    }

    // 2D distance between two points. The caller passes bot and slot
    // coordinates; kept here so the test exercises the same math.
    inline float FormationStationDistance(float botX, float botY, float slotX, float slotY)
    {
        float const dx = botX - slotX;
        float const dy = botY - slotY;
        return std::sqrt(dx * dx + dy * dy);
    }

    // Walks allowed per formation change: the first walk, plus one retry when
    // the bot did not reach the slot it aimed at (a cast, a stop, a path that
    // fell short). Then the bot stops until the formation changes, so a walk
    // that never lands cannot repeat forever.
    constexpr int kFormationStationWalksPerChange = 2;

    // Per-bot station memory for the formation the bot was last set to.
    // Chaos re-rolls its slot every 3 s, so the memory keeps the slot the last
    // walk aimed at: the next check asks whether the bot reached that slot,
    // not whether it sits on today's re-rolled one.
    struct FormationStationMemory
    {
        unsigned int formation = 0; // serial of the formation this memory is for
        int walks = 0;              // walks issued for that formation
        bool settled = false;       // no more walks until the formation changes
        float walkX = 0.0f;
        float walkY = 0.0f;
    };

    // One check tick. Returns true when the bot should walk to the slot now.
    // A new formation serial starts the memory over. A bot already on its slot
    // settles without walking; a bot whose last walk reached its slot settles too.
    inline bool FormationStationStep(FormationStationMemory& memory, unsigned int formation,
        float botX, float botY, float slotX, float slotY)
    {
        if (memory.formation != formation)
        {
            memory = FormationStationMemory();
            memory.formation = formation;
        }
        if (memory.settled)
            return false;

        if (memory.walks == 0)
        {
            if (!FormationStationWanted(FormationStationDistance(botX, botY, slotX, slotY)))
            {
                memory.settled = true;
                return false;
            }
        }
        else if (!FormationStationWanted(FormationStationDistance(botX, botY, memory.walkX, memory.walkY)) ||
            memory.walks >= kFormationStationWalksPerChange)
        {
            // The last walk reached its slot, or the retry is already spent.
            memory.settled = true;
            return false;
        }

        ++memory.walks;
        memory.walkX = slotX;
        memory.walkY = slotY;
        return true;
    }
}
