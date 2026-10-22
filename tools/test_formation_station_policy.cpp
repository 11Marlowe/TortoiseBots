#include "../ai/playerbot/FormationStationPolicy.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using ai::FormationStationDistance;
using ai::FormationStationWanted;
using ai::kFormationStationToleranceYd;
using ai::FormationStationMemory;
using ai::FormationStationStep;

int main()
{
    std::cout << "Starting TortoiseBots formation station policy tests...\n";

    // An idle bot next to its master but off its slot walks there: the
    // follow trigger's 5 yd dead zone never fires this close, so without
    // the station trigger a new formation visibly does nothing.
    {
        CHECK(FormationStationWanted(2.0f));
        CHECK(FormationStationWanted(1.5f));
        std::cout << "  [PASS] idle bot off its slot walks there\n";
    }

    // At the slot the bot settles: sub-yard error is the core follow's own
    // dead zone, walking for less would jitter every tick.
    {
        CHECK(!FormationStationWanted(0.0f));
        CHECK(!FormationStationWanted(0.5f));
        CHECK(!FormationStationWanted(kFormationStationToleranceYd));
        std::cout << "  [PASS] bot on its slot stays put\n";
    }

    // Degenerate inputs never fire.
    {
        CHECK(!FormationStationWanted(-1.0f));
        CHECK(!FormationStationWanted(std::nanf("")));
        std::cout << "  [PASS] degenerate distances stay put\n";
    }

    // The distance helper measures the 2D gap the trigger reads.
    {
        CHECK(fabs(FormationStationDistance(0.0f, 0.0f, 3.0f, 4.0f) - 5.0f) < 1e-6f);
        CHECK(FormationStationDistance(1.0f, 1.0f, 1.0f, 1.0f) == 0.0f);
        std::cout << "  [PASS] distance helper measures the 2D gap\n";
    }

    // Once per formation change: a bot that reached its slot is not walked
    // again on later ticks, even though chaos re-rolls the slot each tick.
    {
        FormationStationMemory memory;
        CHECK(FormationStationStep(memory, 1, 0.0f, 0.0f, 5.0f, 0.0f));
        CHECK(!FormationStationStep(memory, 1, 5.0f, 0.0f, 5.5f, 0.3f));
        CHECK(!FormationStationStep(memory, 1, 5.0f, 0.0f, 4.7f, -0.2f));
        std::cout << "  [PASS] reached slot is not walked again in the same formation\n";
    }

    // A walk that never lands gets one retry, then the bot stops until the
    // formation changes: no endless loop.
    {
        FormationStationMemory memory;
        CHECK(FormationStationStep(memory, 1, 0.0f, 0.0f, 5.0f, 0.0f));
        CHECK(FormationStationStep(memory, 1, 0.0f, 0.0f, 5.2f, 0.4f));
        CHECK(!FormationStationStep(memory, 1, 0.0f, 0.0f, 4.8f, -0.3f));
        CHECK(!FormationStationStep(memory, 1, 0.0f, 0.0f, 5.0f, 0.0f));
        std::cout << "  [PASS] a walk that never lands gets one retry, then stops\n";
    }

    // A new formation starts over, also when the same name is set again: the
    // bot walks once more, then settles.
    {
        FormationStationMemory memory;
        CHECK(FormationStationStep(memory, 1, 0.0f, 0.0f, 5.0f, 0.0f));
        CHECK(!FormationStationStep(memory, 1, 5.0f, 0.0f, 5.0f, 0.0f));
        CHECK(FormationStationStep(memory, 2, 5.0f, 0.0f, 0.0f, 6.0f));
        CHECK(!FormationStationStep(memory, 2, 0.0f, 6.0f, 0.0f, 6.0f));
        std::cout << "  [PASS] formation change re-stations once\n";
    }

    // Within the 1 yd tolerance a new formation does not walk the bot.
    {
        FormationStationMemory memory;
        CHECK(!FormationStationStep(memory, 1, 0.0f, 0.0f, 0.9f, 0.0f));
        CHECK(!FormationStationStep(memory, 2, 0.0f, 0.0f, 1.0f, 0.0f));
        CHECK(FormationStationStep(memory, 3, 0.0f, 0.0f, 1.5f, 0.0f));
        std::cout << "  [PASS] new formation respects the 1 yd tolerance\n";
    }

    std::cout << "All formation station policy tests passed.\n";
    return 0;
}
