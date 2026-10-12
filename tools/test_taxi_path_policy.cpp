#include "../ai/playerbot/TaxiPathPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

int main()
{
    std::cout << "Starting TortoiseBots taxi path policy tests...\n";

    // The two broken paths are blocked by default.
    CHECK(ai::TaxiPathNeedsCoreRepair(274));
    CHECK(ai::TaxiPathNeedsCoreRepair(355));
    CHECK(!ai::TaxiPathAllowed(274, false));
    CHECK(!ai::TaxiPathAllowed(355, false));
    std::cout << "  [PASS] broken paths blocked by default\n";

    // Every other path flies as before, including 355's reverse (356) and 303.
    CHECK(ai::TaxiPathAllowed(356, false));
    CHECK(ai::TaxiPathAllowed(303, false));
    CHECK(ai::TaxiPathAllowed(1, false));
    CHECK(ai::TaxiPathAllowed(0, false));
    std::cout << "  [PASS] other paths unaffected\n";

    // With the core repair (AllowRepairedTaxiPaths = 1) they are allowed.
    CHECK(ai::TaxiPathAllowed(274, true));
    CHECK(ai::TaxiPathAllowed(355, true));
    std::cout << "  [PASS] flag allows repaired paths\n";

    std::cout << "taxi path policy: OK\n";
    return 0;
}
