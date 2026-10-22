// Standalone test for the `.autogear` real-player rules: spec name and alias
// parsing for every class, the permission rule (GM / for-all flag / self),
// the ilvl clamp (lower-only below max level, raise allowed at max level),
// the unique-copy rule, the active-set rule and the target-state refusals.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_player_autogear_policy.cpp -o /tmp/test_player_autogear_policy
//   /tmp/test_player_autogear_policy

#include "../runtime/PlayerAutogearPolicy.h"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

// Class ids: warrior 1, paladin 2, hunter 3, rogue 4, priest 5, shaman 7,
// mage 8, warlock 9, druid 11.

static void TestSpecParsing()
{
    // Every class: the display words from the issue table parse to the
    // shipped weightscale names.
    CHECK(MatchAutogearSpec(1, "arms") == "arms");
    CHECK(MatchAutogearSpec(1, "fury") == "fury");
    CHECK(MatchAutogearSpec(1, "protection") == "prot");
    CHECK(MatchAutogearSpec(2, "holy") == "holy");
    CHECK(MatchAutogearSpec(2, "protection") == "prot");
    CHECK(MatchAutogearSpec(2, "retribution") == "retrib");
    CHECK(MatchAutogearSpec(3, "beast") == "beast");
    CHECK(MatchAutogearSpec(3, "marksmanship") == "marks");
    CHECK(MatchAutogearSpec(3, "survival") == "surv");
    CHECK(MatchAutogearSpec(4, "assassination") == "assas");
    CHECK(MatchAutogearSpec(4, "combat") == "combat");
    CHECK(MatchAutogearSpec(4, "subtlety") == "subtle");
    CHECK(MatchAutogearSpec(5, "discipline") == "disc");
    CHECK(MatchAutogearSpec(5, "holy") == "holy");
    CHECK(MatchAutogearSpec(5, "shadow") == "shadow");
    CHECK(MatchAutogearSpec(7, "elemental") == "elem");
    CHECK(MatchAutogearSpec(7, "enhancement") == "enhance");
    CHECK(MatchAutogearSpec(7, "enhancement-tank") == "shamantank");
    CHECK(MatchAutogearSpec(7, "restoration") == "resto");
    CHECK(MatchAutogearSpec(8, "arcane") == "arcane");
    CHECK(MatchAutogearSpec(8, "fire") == "fire");
    CHECK(MatchAutogearSpec(8, "frost") == "frost");
    CHECK(MatchAutogearSpec(9, "affliction") == "afflic");
    CHECK(MatchAutogearSpec(9, "demonology") == "demo");
    CHECK(MatchAutogearSpec(9, "destruction") == "destro");
    CHECK(MatchAutogearSpec(11, "balance") == "balance");
    CHECK(MatchAutogearSpec(11, "feral") == "feraldps");
    CHECK(MatchAutogearSpec(11, "feral-tank") == "feraltank");
    CHECK(MatchAutogearSpec(11, "restoration") == "resto");

    // Hyphen and space are the same token, case-insensitive.
    CHECK(MatchAutogearSpec(7, "enhancement tank") == "shamantank");
    CHECK(MatchAutogearSpec(7, "ENHANCEMENT-TANK") == "shamantank");
    CHECK(MatchAutogearSpec(11, "Feral  Tank") == "feraltank");
    CHECK(MatchAutogearSpec(2, "  protection ") == "prot");

    // Short aliases.
    CHECK(MatchAutogearSpec(7, "enh") == "enhance");
    CHECK(MatchAutogearSpec(7, "enh tank") == "shamantank");
    CHECK(MatchAutogearSpec(1, "prot") == "prot");
    CHECK(MatchAutogearSpec(7, "resto") == "resto");
    CHECK(MatchAutogearSpec(11, "bear") == "feraltank");
    CHECK(MatchAutogearSpec(2, "ret") == "retrib");
    CHECK(MatchAutogearSpec(3, "mm") == "marks");
    CHECK(MatchAutogearSpec(4, "sub") == "subtle");
    CHECK(MatchAutogearSpec(9, "destro") == "destro");

    // A spec of another class is refused the same way as an unknown word.
    CHECK(MatchAutogearSpec(8, "fury").empty());
    CHECK(MatchAutogearSpec(7, "feral-tank").empty());
    CHECK(MatchAutogearSpec(1, "shadow").empty());
    CHECK(MatchAutogearSpec(8, "nonsense").empty());
    CHECK(MatchAutogearSpec(8, "").empty());

    // The ambiguity split: a spec word is a spec word, a name is not.
    CHECK(IsAutogearSpecToken("enhancement"));
    CHECK(IsAutogearSpecToken("protection"));
    CHECK(!IsAutogearSpecToken("Thrall"));

    // Every class lists its specs, and every listed display word parses.
    for (uint8_t classId : {uint8_t(1), uint8_t(2), uint8_t(3), uint8_t(4), uint8_t(5),
                            uint8_t(7), uint8_t(8), uint8_t(9), uint8_t(11)})
    {
        std::string const list = ListAutogearSpecs(classId);
        CHECK(!list.empty());
        size_t start = 0;
        while (start < list.size())
        {
            size_t const comma = list.find(", ", start);
            std::string const word = list.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
            CHECK(!MatchAutogearSpec(classId, word).empty());
            if (comma == std::string::npos)
                break;
            start = comma + 2;
        }
    }
}

static void TestPermissions()
{
    using P = PlayerAutogearPermission;

    // Flag off: GM-only.
    CHECK(DecidePlayerAutogearPermission(false, false, true, false) == P::GmOnly);
    CHECK(DecidePlayerAutogearPermission(false, false, false, false) == P::GmOnly);
    // GMs gear anyone whatever the flag.
    CHECK(DecidePlayerAutogearPermission(true, false, false, false) == P::Allowed);
    CHECK(DecidePlayerAutogearPermission(true, true, false, false) == P::Allowed);
    // Flag on: self only.
    CHECK(DecidePlayerAutogearPermission(false, true, true, false) == P::Allowed);
    CHECK(DecidePlayerAutogearPermission(false, true, false, false) == P::NotSelf);
    // Challenge-mode target: only a GM, whatever the flag and self rule.
    CHECK(DecidePlayerAutogearPermission(false, true, true, true) == P::ChallengeGmOnly);
    CHECK(DecidePlayerAutogearPermission(true, false, true, true) == P::Allowed);
}

static void TestIlvlClamp()
{
    // Below max level: the request may only lower the default ceiling; a
    // higher value is clamped to the default.
    CHECK(ResolvePlayerAutogearIlvl(30, 60, 0, 31).effective == 31);
    CHECK(!ResolvePlayerAutogearIlvl(30, 60, 0, 31).clamped);
    CHECK(ResolvePlayerAutogearIlvl(30, 60, 15, 31).effective == 15);
    CHECK(ResolvePlayerAutogearIlvl(30, 60, 999, 31).effective == 31);
    CHECK(ResolvePlayerAutogearIlvl(30, 60, 999, 31).clamped);
    CHECK(ResolvePlayerAutogearIlvl(30, 60, 31, 31).effective == 31);
    CHECK(ResolvePlayerAutogearIlvl(30, 60, 31, 31).clamped);

    // At max level: a higher value raises the ceiling (no clamp).
    CHECK(ResolvePlayerAutogearIlvl(60, 60, 90, 61).effective == 90);
    CHECK(!ResolvePlayerAutogearIlvl(60, 60, 90, 61).clamped);
    // Lowering still works at max level.
    CHECK(ResolvePlayerAutogearIlvl(60, 60, 15, 61).effective == 15);
    // The default stands when nothing is requested.
    CHECK(ResolvePlayerAutogearIlvl(60, 60, 0, 61).effective == 61);
}

static void TestUniqueAndSets()
{
    CHECK(AutogearUniqueAllowed(false, 0));
    CHECK(AutogearUniqueAllowed(false, 3));
    CHECK(AutogearUniqueAllowed(true, 0));
    CHECK(!AutogearUniqueAllowed(true, 1));
    CHECK(!AutogearUniqueAllowed(true, 2));

    uint32_t const spells[] = {7999, 7998, 0, 0, 0, 0, 0, 0};
    uint32_t const thresholds[] = {2, 5, 0, 0, 0, 0, 0, 0};
    CHECK(AutogearSetBonusActive(2, spells, thresholds, 8));
    CHECK(AutogearSetBonusActive(5, spells, thresholds, 8));
    CHECK(!AutogearSetBonusActive(1, spells, thresholds, 8));
    CHECK(AutogearSetBonusActive(4, spells, thresholds, 8));
    // Threshold rows without a spell never count.
    uint32_t const spellLess[] = {0, 0, 0, 0, 0, 0, 0, 0};
    CHECK(!AutogearSetBonusActive(5, spellLess, thresholds, 8));
}

static void TestTargetState()
{
    using S = PlayerAutogearTargetState;
    CHECK(DecidePlayerAutogearTargetState(true, false, false) == S::Ready);
    CHECK(DecidePlayerAutogearTargetState(false, false, false) == S::Dead);
    CHECK(DecidePlayerAutogearTargetState(true, true, false) == S::InCombat);
    CHECK(DecidePlayerAutogearTargetState(true, false, true) == S::Shapeshifted);
    CHECK(DecidePlayerAutogearTargetState(true, true, true) == S::Shapeshifted);
}

static void TestQualityCeiling()
{
    // Spawn band: rare top, whatever the level — a raise below max level
    // cannot happen (the clamp refuses it first).
    CHECK(AutogearMaxQuality(30, 60, false) == 3);
    CHECK(AutogearMaxQuality(59, 60, true) == 3);
    CHECK(AutogearMaxQuality(30, 60, true) == 3);
    // Raised level-60 ceiling: the best gear in the pool, epic included.
    CHECK(AutogearMaxQuality(60, 60, true) == 4);
    // Level 60 without a raise keeps the spawn band.
    CHECK(AutogearMaxQuality(60, 60, false) == 3);
}

static void TestRunCounts()
{
    PlayerAutogearRunCounts counts;
    // Empty slot filled by the backstop: an equip, nothing replaced.
    CountPlayerAutogearRecord(counts, 0, 1687, false);
    // Normal upgrade: old piece to the bags.
    CountPlayerAutogearRecord(counts, 825, 1687, false);
    // Piece freed for a two-hander, mailed (full bags).
    CountPlayerAutogearRecord(counts, 7, 0, true);
    CHECK(counts.slotsEquipped == 2);
    CHECK(counts.replaced == 2);
    CHECK(counts.mailed == 1);
}

int main()
{
    TestSpecParsing();
    TestPermissions();
    TestIlvlClamp();
    TestQualityCeiling();
    TestUniqueAndSets();
    TestTargetState();
    TestRunCounts();
    std::printf("test_player_autogear_policy: %d checks passed\n", checks);
    return 0;
}
