// Standalone test for runtime/BotKindPolicy.h (dashboard kinds, Issue #872).
// No core, no DB: feeds ClassifyBotKind plain values.
#include "../runtime/BotKindPolicy.h"

#include <cassert>
#include <string>

using namespace TortoiseBots;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #cond); } } while (0)

int main()
{
    // Default: wandering random-pool bot.
    CHECK(ClassifyBotKind(false, false, false, false, false) == BotKind::Pool);
    CHECK(std::string(BotKindName(BotKind::Pool)) == "pool");

    // Autonomous alt wins over every bot mark (grouped with others too).
    CHECK(ClassifyBotKind(false, true, false, false, true) == BotKind::Alt);
    CHECK(ClassifyBotKind(false, true, true, true, true) == BotKind::Alt);
    CHECK(std::string(BotKindName(BotKind::Alt)) == "alt");

    // Hire ledger then claim ledger, both above owned/pool.
    CHECK(ClassifyBotKind(false, false, true, true, false) == BotKind::Hired);
    CHECK(ClassifyBotKind(false, false, false, true, false) == BotKind::Claimed);
    CHECK(std::string(BotKindName(BotKind::Hired)) == "hired");
    CHECK(std::string(BotKindName(BotKind::Claimed)) == "claimed");

    // Owned non-random bot with no other mark follows its player.
    CHECK(ClassifyBotKind(false, false, false, false, true) == BotKind::Companion);
    CHECK(std::string(BotKindName(BotKind::Companion)) == "companion");

    // A real player is never a bot kind, whatever the lifecycles say.
    CHECK(ClassifyBotKind(true, true, true, true, true) == BotKind::Player);
    CHECK(std::string(BotKindName(BotKind::Player)) == "player");

    if (failures == 0)
        std::printf("test_bot_kind_policy ok\n");
    return failures ? 1 : 0;
}
