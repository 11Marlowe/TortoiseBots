#pragma once

// One LFT level-range table for the LFT fill and bot LFG groups; source
// Soromeister/LFT v0.0.3.3 LFT.allDungeons; unknown and invalid ranges fail
// closed; plain data and the standard library only.

#include <cctype>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace TortoiseBots
{

struct LftLevelRange
{
    uint8_t minLevel;
    uint8_t maxLevel;
};

inline std::string NormalizeLftToken(std::string const& raw)
{
    std::string normalized;
    normalized.reserve(raw.size());
    for (unsigned char character : raw)
        if (std::isalnum(character))
            normalized.push_back(static_cast<char>(std::tolower(character)));
    return normalized;
}

inline std::unordered_map<std::string, LftLevelRange> const& LftDungeonRangeTable()
{
    // Source: Soromeister/LFT v0.0.3.3 LFT.allDungeons (authoritative queue availability).
    // Exact codes/names/minLevel/maxLevel from the addon; unknown/corrupt ranges fail closed.
    // Keys are NormalizeLftToken(raw) = alnum lowercased, so addon codes and normalized
    // display names both resolve. No DBC fallback, no invented Tortoise-only ranges beyond this list.
    static std::unordered_map<std::string, LftLevelRange> const ranges = {
        {"ragefirechasm", {13, 18}}, {"rfc", {13, 18}},
        {"wailingcaverns", {17, 24}}, {"wc", {17, 24}},
        {"thedeadmines", {17, 24}}, {"deadmines", {17, 24}}, {"dm", {17, 24}},
        {"shadowfangkeep", {22, 30}}, {"sfk", {22, 30}},
        {"thestockade", {22, 30}}, {"stockade", {22, 30}}, {"stocks", {22, 30}},
        {"blackfathomdeeps", {23, 32}}, {"bfd", {23, 32}},
        {"scarletmonasterygraveyard", {27, 36}}, {"smgy", {27, 36}},
        {"scarletmonasterylibrary", {28, 39}}, {"smlib", {28, 39}},
        {"gnomeregan", {29, 38}}, {"gnomer", {29, 38}},
        {"razorfenkraul", {29, 38}}, {"rfk", {29, 38}},
        {"thecrescentgrove", {32, 38}}, {"crescentgrove", {32, 38}}, {"tcg", {32, 38}},
        {"scarletmonasteryarmory", {32, 41}}, {"smarmory", {32, 41}},
        {"scarletmonasterycathedral", {35, 45}}, {"smcath", {35, 45}},
        {"razorfendowns", {36, 46}}, {"rfd", {36, 46}},
        {"uldaman", {40, 51}}, {"ulda", {40, 51}},
        {"gilneascity", {42, 50}}, {"gilneas", {42, 50}},
        {"zulfarrak", {44, 54}}, {"zulfarak", {44, 54}}, {"zf", {44, 54}},
        {"maraudonorange", {47, 55}}, {"maraorange", {47, 55}},
        {"maraudonpurple", {45, 55}}, {"marapurple", {45, 55}},
        {"maraudonprincess", {47, 55}}, {"maraprincess", {47, 55}},
        {"templeofatalhakkar", {50, 60}}, {"st", {50, 60}},
        {"hateforgequarry", {50, 60}}, {"hfq", {50, 60}},
        {"blackrockdepths", {52, 60}}, {"brd", {52, 60}},
        {"blackrockdepthsarena", {52, 60}}, {"brdarena", {52, 60}},
        {"blackrockdepthsemperor", {54, 60}}, {"brdemp", {54, 60}},
        {"lowerblackrockspire", {55, 60}}, {"lbrs", {55, 60}},
        {"diremauleast", {55, 60}}, {"dme", {55, 60}},
        {"diremaulnorth", {57, 60}}, {"dmn", {57, 60}},
        {"diremaultribute", {57, 60}}, {"dmt", {57, 60}},
        {"diremaulwest", {57, 60}}, {"dmw", {57, 60}},
        {"scholomance", {58, 60}}, {"scholo", {58, 60}},
        {"stratholmeundeaddistrict", {58, 60}}, {"stratud", {58, 60}},
        {"stratholmescarletbastion", {58, 60}}, {"stratlive", {58, 60}},
        {"karazhancrypt", {58, 60}}, {"kc", {58, 60}},
        {"cavernsoftimeblackmorass", {60, 60}}, {"cotbm", {60, 60}},
        {"stormwindvault", {60, 60}}, {"swv", {60, 60}}
    };
    return ranges;
}

inline LftLevelRange const* LftDungeonRange(std::string const& raw)
{
    std::unordered_map<std::string, LftLevelRange> const& ranges = LftDungeonRangeTable();
    auto it = ranges.find(NormalizeLftToken(raw));
    if (it == ranges.end() || it->second.minLevel == 0 ||
        it->second.minLevel > it->second.maxLevel || it->second.maxLevel > 60)
        return nullptr;
    return &it->second;
}

} // namespace TortoiseBots
