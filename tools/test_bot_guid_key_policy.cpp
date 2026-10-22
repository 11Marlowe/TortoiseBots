// Standalone test for runtime/BotGuidKey.h (the bot registry's key).
// Deadmines r12-19z29 (DEADMINES-PACKS-2026-10-22 Q3): the Remote-Controlled
// Golem summons' pet guids were classified as bots by their counter, and the
// tank's pick-up order at them was refused as "a bot or a member of the group".
#include "../runtime/BotGuidKey.h"

#include <cstdint>
#include <cstdio>

using namespace TortoiseBots;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #cond); } } while (0)

int main()
{
    std::uint32_t key = 0;

    // A player guid: the high part 0, the key its low 32 bits.
    CHECK(BotRegistryKey(18ull, key) && key == 18);
    CHECK(BotRegistryKey(0x00000000FFFFFFFFull, key) && key == 0xFFFFFFFFu);
    CHECK(BotGuidHigh(18ull) == kBotGuidHighPlayer);

    // The three refused orders' targets (t 1506.3, 2444.0, 2576.1), decimal as
    // the run leader sent them: pet guids (0xF140), never a registry key,
    // though their counters (18, 31, 32) are small player counters too.
    std::uint64_t const golems[] = {17383894575189327890ull, 17383894575994634271ull, 17383894576128852000ull};
    std::uint64_t const hex[] = {0xf140000327000012ull, 0xf14000035700001full, 0xf14000035f000020ull};
    for (int i = 0; i < 3; ++i)
    {
        CHECK(golems[i] == hex[i]);
        CHECK(BotGuidHigh(golems[i]) == 0xF140);
        key = 7;
        CHECK(!BotRegistryKey(golems[i], key) && key == 7); // the key is left alone
    }

    // A creature (0xF130, the focused 622 3569), a game object (0xF110) and an
    // item-ish high part (0x4000): not players either.
    CHECK(!BotRegistryKey(17379390972458251625ull, key));
    CHECK(17379390972458251625ull == 0xf13000026e013569ull);
    CHECK(!BotRegistryKey(0xf110000000000001ull, key));
    CHECK(!BotRegistryKey(0x4000000000000012ull, key));
    // A guid with only the lowest high bit set is not a player's.
    CHECK(!BotRegistryKey(0x0001000000000012ull, key));
    // The empty guid is nobody.
    CHECK(!BotRegistryKey(0ull, key));

    if (failures == 0)
        std::printf("test_bot_guid_key_policy ok\n");
    return failures ? 1 : 0;
}
