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

    std::cout << "All formation station policy tests passed.\n";
    return 0;
}
