#include "../runtime/policy/hire/LftPartyAcceptPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using TortoiseBots::LftPartyBotMayAccept;

int main()
{
    std::cout << "Starting TortoiseBots LFT party accept tests...\n";

    CHECK(LftPartyBotMayAccept(false, false));   // alt on the owner's account
    std::cout << "  [PASS] alt on the owner's account may accept\n";
    CHECK(!LftPartyBotMayAccept(true, false));   // pool bot that is not the human's: never
    std::cout << "  [PASS] unrelated pool bot never accepts\n";
    CHECK(LftPartyBotMayAccept(true, true));     // hire (#1284): random record, hired
    std::cout << "  [PASS] hired mercenary may accept\n";
    CHECK(LftPartyBotMayAccept(false, true));
    std::cout << "  [PASS] non-pool hired record may accept\n";

    std::cout << "TortoiseBots LFT party accept tests passed.\n";
    return 0;
}
