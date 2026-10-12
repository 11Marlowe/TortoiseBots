#include "../ai/playerbot/InstanceGuardPolicy.h"

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
    using ai::InstanceGuardBit;
    using ai::InstanceGuardRestore;

    // Review finding: a bot leaving the instance for its real-player master
    // keeps following — no open-world restore, whatever was withheld.
    CHECK(InstanceGuardRestore(0x3F, true, true).empty());
    CHECK(InstanceGuardRestore(0x3F, true, false).empty());
    CHECK(InstanceGuardRestore(InstanceGuardBit(0), true, false).empty());

    // Masterless pool bots get back exactly what the guard held.
    CHECK(InstanceGuardRestore(InstanceGuardBit(0), false, false) == "+grind");
    CHECK(InstanceGuardRestore(0, false, false).empty());

    // "+wander" never comes back next to "follow" (siblings); without follow
    // a wander-mode bot gets it back.
    CHECK(InstanceGuardRestore(InstanceGuardBit(5), false, true).empty());
    CHECK(InstanceGuardRestore(InstanceGuardBit(5), false, false) == "+wander");
    CHECK(InstanceGuardRestore(InstanceGuardBit(0) | InstanceGuardBit(5), false, true) == "+grind");

    std::cout << "instance guard policy: OK\n";
    return 0;
}
