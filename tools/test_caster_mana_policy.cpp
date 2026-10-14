#include "../ai/playerbot/CasterManaPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        return 1; \
    } \
} while (0)

int main()
{
    std::cout << "Starting caster mana-loop policy tests...\n";

    // Gem ranks recognised low to high; non-gems rejected.
    CHECK(ai::IsManaGemItem(5514));
    CHECK(ai::IsManaGemItem(5513));
    CHECK(ai::IsManaGemItem(8007));
    CHECK(ai::IsManaGemItem(8008));
    CHECK(!ai::IsManaGemItem(20520));
    CHECK(!ai::IsManaGemItem(13444));
    std::cout << "  [PASS] gem ranks recognised, rune/potion rejected\n";

    // Gem-vs-rune arbitration: ready gem wins, spent gem falls to rune.
    CHECK(ai::MagePrefersGemOverRune(true) == true);
    CHECK(ai::MagePrefersGemOverRune(false) == false);
    std::cout << "  [PASS] ready gem preferred, spent gem falls to rune\n";

    // Potion arms at the high-water mark so the 2 min line rolls early.
    CHECK(ai::PotionArmedAtManaPercent(64));
    CHECK(ai::PotionArmedAtManaPercent(40));
    CHECK(!ai::PotionArmedAtManaPercent(65));
    CHECK(!ai::PotionArmedAtManaPercent(100));
    std::cout << "  [PASS] potion arms below 65%\n";

    // Evocation arms with the gem, not as a last rite.
    CHECK(ai::EvocationArmedAtManaPercent(39));
    CHECK(ai::EvocationArmedAtManaPercent(15));
    CHECK(!ai::EvocationArmedAtManaPercent(40));
    std::cout << "  [PASS] evocation arms below 40%\n";

    std::cout << "All caster mana-loop policy checks PASSED!\n";
    return 0;
}
