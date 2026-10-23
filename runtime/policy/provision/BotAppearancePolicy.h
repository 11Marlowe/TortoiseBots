#pragma once

// Valid appearance ranges for random bots, per race and gender.
//
// The client only draws a head/hair/face that exists for the race and gender:
// a value outside its range shows as a missing head or the wrong race's head.
// Each field's valid values are exactly 0..count-1 (the counts were derived
// from CharSections.dbc, CharacterFacialHairStyles.dbc and CharHairGeosets.dbc
// and every field is contiguous). The core does not validate these values on
// character creation, so the generator must stay inside them.
//
// Header-only and free of core types on purpose: tools/test_bot_appearance_policy.cpp
// checks the table standalone.
#include <cstdint>

namespace TortoiseBots
{

struct BotAppearanceCounts
{
    uint8_t skin;
    uint8_t face;
    uint8_t hairStyle;
    uint8_t hairColor;
    uint8_t facialHair;
};

// Fills `out` with the number of valid values per field for race (1..10,
// Turtle ids) and gender (0 male, 1 female). Returns false for any other
// race or gender; the caller must not create that character.
inline bool BotAppearanceCountsFor(uint8_t race, uint8_t gender, BotAppearanceCounts& out)
{
    struct Row
    {
        BotAppearanceCounts male;
        BotAppearanceCounts female;
    };
    // Index = race - 1: Human, Orc, Dwarf, NightElf, Undead, Tauren, Gnome, Troll, Goblin, HighElf.
    static constexpr Row kRows[] = {
        {{10, 12, 17, 10, 9},  {10, 15, 26, 10, 7}},   // Human
        {{9, 10, 13, 8, 11},   {9, 9, 14, 8, 7}},      // Orc (skins 9-10 have no face textures)
        {{14, 10, 17, 10, 21}, {14, 10, 19, 10, 6}},   // Dwarf
        {{11, 10, 12, 10, 14}, {11, 9, 12, 10, 10}},   // NightElf
        {{6, 10, 16, 11, 17},  {6, 10, 17, 11, 8}},    // Undead
        {{20, 5, 13, 9, 9},    {12, 4, 12, 9, 7}},     // Tauren
        {{6, 7, 12, 10, 12},   {6, 7, 12, 10, 7}},     // Gnome
        {{12, 5, 10, 10, 14},  {12, 6, 10, 10, 10}},   // Troll
        {{3, 4, 14, 5, 10},    {3, 6, 17, 5, 5}},      // Goblin
        {{11, 10, 16, 16, 11}, {11, 10, 20, 16, 11}},  // HighElf
    };
    if (race < 1 || race > 10 || gender > 1)
        return false;
    Row const& row = kRows[race - 1];
    out = gender == 0 ? row.male : row.female;
    return true;
}

} // namespace TortoiseBots
