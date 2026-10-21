// Standalone regression test for ai/playerbot/TalentSpellSavePolicy.h - the
// "a talent rank spell must never stay dependent" rule the module re-asserts in
// PlayerbotFactory::EnsureTalentSpellsSaved (called from InitClassLevelSpells
// and from BotManager::OnPlayerLogin).
//
// The rule decides which spellbook entries the guard rewrites, so both failure
// modes matter: a false negative keeps the reported defect (a 51-point shaman
// tank hire loses Lightning Strike 51387 out of character_spell and the armory
// reads 15/35 while the live spellbook says 15/36 - Mortal Strike 12294,
// Bloodthirst 23881, Shield Slam 23922, Moonkin Form 24858 and friends behave
// the same), and a false positive would start writing spells that are *meant* to
// stay derived (the trainer ranks 52420/21551/... below a rank the bot knows).
// Both are covered by the exhaustive input table below.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_talent_spell_save_policy.cpp -o /tmp/test_talent_save
//   /tmp/test_talent_save

#include "../ai/playerbot/TalentSpellSavePolicy.h"

#include <cstdio>
#include <cstdlib>

using namespace ai;

static int checks = 0;

#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

// Every combination of the four inputs the guard reads, with the expected
// verdict; the table is what a rule change has to keep passing.
static void CheckExhaustiveTable()
{
    struct Row
    {
        bool talent;      // GetTalentSpellCost(spell) != 0
        bool dependent;   // learned as a by-product of another spell
        bool removed;     // state == PLAYERSPELL_REMOVED
        bool disabled;    // first rank known but the talent not spent
        bool expected;
        char const* what; // the case, in the terms the callers meet
    };

    Row const rows[] = {
        { true,  true,  false, false, true,  "live talent rank spell the chain pass marked dependent (51387)" },
        { true,  false, false, false, false, "live talent rank spell already saved (the guard is idempotent)" },
        { true,  true,  true,  false, false, "removed talent spell (not in the spellbook any more)" },
        { true,  true,  false, true,  false, "disabled talent spell (first rank only, talent not spent)" },
        { true,  false, true,  false, false, "removed, not dependent (nothing to repair)" },
        { true,  false, false, true,  false, "disabled, not dependent (nothing to repair)" },
        { true,  true,  true,  true,  false, "removed and disabled (nothing to repair)" },
        { true,  false, true,  true,  false, "removed and disabled, not dependent" },
        { false, true,  false, false, false, "dependent trainer rank (52420 under 52422) - derived, stays unsaved" },
        { false, false, false, false, false, "plain known spell" },
        { false, true,  true,  false, false, "removed non-talent spell" },
        { false, true,  false, true,  false, "disabled non-talent spell" },
        { false, false, true,  false, false, "removed, not dependent, non-talent" },
        { false, false, false, true,  false, "disabled, not dependent, non-talent" },
        { false, true,  true,  true,  false, "removed and disabled non-talent" },
        { false, false, true,  true,  false, "removed and disabled, not dependent, non-talent" },
    };

    for (Row const& row : rows)
    {
        TalentSpellSaveInputs entry{ row.talent, row.dependent, row.removed, row.disabled };
        if (TalentSpellMustStaySaved(entry) != row.expected)
        {
            std::fprintf(stderr, "FAIL (%s): talent=%d dependent=%d removed=%d disabled=%d expected=%d\n",
                row.what, row.talent, row.dependent, row.removed, row.disabled, row.expected);
            std::exit(1);
        }
        ++checks;
    }
}

int main()
{
    CheckExhaustiveTable();

    std::printf("test_talent_spell_save_policy: %d checks passed\n", checks);
    return 0;
}
