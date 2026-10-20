// Standalone test for runtime/BotAppearancePolicy.h (valid random-bot looks).
// No core, no DB: checks the per-race/gender ranges the generator rolls from.
#include "../runtime/BotAppearancePolicy.h"

#include <cstdio>

using namespace TortoiseBots;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #cond); } } while (0)

int main()
{
    BotAppearanceCounts c{};

    // Goblin male: the bots that showed no head had face/skin/hair values
    // above these counts (the old generator rolled 0..7 for every race).
    CHECK(BotAppearanceCountsFor(9, 0, c));
    CHECK(c.skin == 3 && c.face == 4 && c.hairStyle == 14 && c.hairColor == 5 && c.facialHair == 10);

    // Goblin female: a smaller facial hair set than male.
    CHECK(BotAppearanceCountsFor(9, 1, c));
    CHECK(c.skin == 3 && c.face == 6 && c.hairStyle == 17 && c.hairColor == 5 && c.facialHair == 5);

    // Every playable race/gender has at least one valid value per field, so
    // urand(0, count - 1) is always a legal roll.
    for (uint8_t race = 1; race <= 10; ++race)
        for (uint8_t gender = 0; gender <= 1; ++gender)
        {
            CHECK(BotAppearanceCountsFor(race, gender, c));
            CHECK(c.skin >= 1 && c.face >= 1 && c.hairStyle >= 1 && c.hairColor >= 1 && c.facialHair >= 1);
        }

    // Unknown race or gender is refused, not defaulted.
    CHECK(!BotAppearanceCountsFor(0, 0, c));
    CHECK(!BotAppearanceCountsFor(11, 0, c));
    CHECK(!BotAppearanceCountsFor(9, 2, c));

    if (failures == 0)
        std::printf("test_bot_appearance_policy ok\n");
    return failures ? 1 : 0;
}
