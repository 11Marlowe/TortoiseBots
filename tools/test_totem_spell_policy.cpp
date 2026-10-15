// Standalone check for runtime/TotemSpellPolicy.h:
//   g++ -std=c++17 -Wall -Wextra tools/test_totem_spell_policy.cpp -o /tmp/t && /tmp/t
#include "../runtime/TotemSpellPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using namespace TortoiseBots;

static bool Works(std::uint32_t a, std::uint32_t b = 0, std::uint32_t c = 0)
{
    std::uint32_t const effects[3] = {a, b, c};
    return SpellWorksOnTotem(effects, 3);
}

int main()
{
    // Immune: a DoT or curse (Corruption, Curse of Agony: APPLY_AURA only),
    // a sting, Polymorph (aura + aura), Rain of Fire (persistent area aura),
    // a taunt.
    CHECK(!Works(6));
    CHECK(!Works(6, 6));
    CHECK(!Works(27));
    CHECK(!Works(114, 6));
    // Works: Frostbolt (school damage + a slow aura), Shadow Bolt and Arcane
    // Shot (school damage), a weapon strike, auto attack, a leech.
    CHECK(Works(2, 6));
    CHECK(Works(2));
    CHECK(Works(58));
    CHECK(Works(121, 6));
    CHECK(Works(78));
    CHECK(Works(9));
    // No effects at all: left to the cast checks.
    CHECK(Works(0));
    std::cout << "test_totem_spell_policy: ok\n";
    return 0;
}
