// Issue #1003: realistic bot names (runtime/BotNames.h, runtime/BotNameData.h).
// Exercises pool parsing, the style weights and every race/class/gender combo
// standalone. Pass --print to see a sample per style.
#include "../runtime/BotNames.h"
#include "../runtime/BotNameData.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <random>
#include <set>
#include <string>

using namespace TortoiseBots;

namespace {

struct Rng
{
    std::mt19937 gen{1003};
    uint32_t operator()(uint32_t lo, uint32_t hi) { return std::uniform_int_distribution<uint32_t>(lo, hi)(gen); }
};

BotNamePools BuiltIn()
{
    BotNamePools pools;
    std::string text;
    for (char const* section : kBotNameData)
        text += section;
    assert(pools.Parse(text) > 1500);
    return pools;
}

bool Valid(std::string const& name)
{
    return !name.empty() && NormalizeBotName(name) == name;
}

void TestWeights()
{
    uint32_t w[NAME_STYLE_COUNT] = { 1, 2, 3 };
    assert(ParseBotNameStyleWeights("55,20,25", w) && w[0] == 55 && w[1] == 20 && w[2] == 25);
    assert(ParseBotNameStyleWeights(" 100, 0 ,0", w) && w[0] == 100 && w[1] == 0 && w[2] == 0);
    assert(!ParseBotNameStyleWeights("0,0,0", w) && w[0] == 100);
    assert(!ParseBotNameStyleWeights("50,50", w));
    assert(!ParseBotNameStyleWeights("1,2,3,4", w));
    assert(!ParseBotNameStyleWeights("a,b,c", w));
    assert(!ParseBotNameStyleWeights("10,-5,20", w));
    assert(!ParseBotNameStyleWeights("", w));
}

void TestNormalize()
{
    assert(NormalizeBotName("kIDneyShotz") == "Kidneyshotz");
    assert(NormalizeBotName("A").empty());
    assert(NormalizeBotName("Thirteenchars").empty());
    assert(NormalizeBotName("Zul'jin").empty());
    assert(NormalizeBotName("Leeroy2").empty());
}

void TestParse()
{
    BotNamePools pools;
    assert(pools.Parse("# comment\n[Fun] Pewpew, Bad'name Toolongnameforwow\n[suffix] z\n[fun] x") == 2);
    assert(pools.Words("fun").size() == 1 && pools.Words("fun")[0] == "pewpew");
    assert(pools.Words("suffix").size() == 1);
}

void TestAllCombos(bool print)
{
    BotNamePools pools = BuiltIn();
    Rng rng;
    uint8_t const races[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    uint8_t const classes[] = { 1, 2, 3, 4, 5, 7, 8, 9, 11 };
    char const* const style_names[] = { "rp", "compound", "fun" };
    for (uint32_t style = 0; style < NAME_STYLE_COUNT; ++style)
    {
        uint32_t w[NAME_STYLE_COUNT] = {};
        w[style] = 1;
        std::set<std::string> seen;
        uint32_t made = 0;
        for (uint8_t race : races)
            for (uint8_t cls : classes)
                for (uint8_t gender = 0; gender < 2; ++gender)
                    for (int i = 0; i < 20; ++i)
                    {
                        std::string name = pools.Generate(race, cls, gender, w, rng);
                        assert(Valid(name));
                        if (style == NAME_STYLE_RP)
                        {
                            std::string key = std::string("rp:") + BotNameRaceKey(race) + (gender ? ":female" : ":male");
                            std::string lower = name;
                            lower[0] = char(lower[0] - 'A' + 'a');
                            for (std::string const& sample : pools.Words(key))
                                assert(sample != lower);
                        }
                        seen.insert(name);
                        ++made;
                    }
        // Plenty of distinct names per style for a large pool.
        assert(seen.size() * 3 > made);
        if (print)
        {
            std::printf("%s (%zu distinct of %u):", style_names[style], seen.size(), made);
            int shown = 0;
            for (std::string const& name : seen)
                if (shown++ % (seen.size() / 25 + 1) == 0)
                    std::printf(" %s", name.c_str());
            std::printf("\n");
        }
    }
    std::set<std::string> dwarves;
    uint32_t rp_only[NAME_STYLE_COUNT] = { 1, 0, 0 };
    for (int i = 0; i < 30; ++i)
        dwarves.insert(pools.Generate(3, 1, 0, rp_only, rng));
    if (print)
    {
        std::printf("dwarf male rp:");
        for (std::string const& n : dwarves)
            std::printf(" %s", n.c_str());
        std::printf("\n");
    }
}

void TestEmptyPools()
{
    BotNamePools pools;
    Rng rng;
    uint32_t w[NAME_STYLE_COUNT] = { 55, 20, 25 };
    assert(pools.Generate(1, 1, 0, w, rng).empty());
    pools.Parse("[rp:human:male] Aldric Bram Cedric Doran Edwin Garret");
    for (int i = 0; i < 50; ++i)
        assert(Valid(pools.Generate(7, 4, 1, w, rng))); // gnome female rogue falls back to human rp
}

} // namespace

int main(int argc, char** argv)
{
    TestWeights();
    TestNormalize();
    TestParse();
    TestEmptyPools();
    TestAllCombos(argc > 1 && std::strcmp(argv[1], "--print") == 0);
    std::printf("test_bot_names_policy: OK\n");
    return 0;
}
