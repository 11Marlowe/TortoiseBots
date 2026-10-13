// Standalone regression test for the solo warlock default-pet rule
// (runtime/WarlockPetPolicy.h): once a pool bot knows Summon Voidwalker
// (697) AND holds a Soul Shard (6265, the summon reagent) the upkeep summons
// the Voidwalker; below that it keeps the Imp, and a shardless bot stays
// quiet instead of queueing a summon that fails isPossible. Owned/hired
// bots (live real-player master) are never touched.
//
// Also covers the opt-in spec-fitting pet (AiPlayerbot.WarlockPetBySpec):
// Affliction/Demonology -> Felhunter -> Succubus -> Voidwalker -> Imp by what
// is learned, Destruction -> Imp, a tankless group keeps the Voidwalker, live
// pets only swap for pool bots, a missing shard keeps the existing fallback.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_warlock_pet_policy.cpp -o /tmp/test_warlock_pet
//   /tmp/test_warlock_pet

#include "../runtime/WarlockPetPolicy.h"

#include <cstdio>
#include <cstdlib>
#include <initializer_list>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

static WarlockSoloPetInputs Solo(bool knowsVW, bool hasShard, bool hasPet, uint32_t entry)
{
    WarlockSoloPetInputs inputs;
    inputs.knowsVoidwalker = knowsVW;
    inputs.hasSoulShard = hasShard;
    inputs.hasPet = hasPet;
    inputs.currentPetEntry = entry;
    return inputs;
}

// Petless lvl10+ pool bot that knows the summon and holds a shard: summon.
static void TestPetlessSummonsVoidwalker()
{
    CHECK(DecideWarlockSoloPet(Solo(true, true, false, 0)) == WarlockSoloPetDecision::SummonVoidwalker);
}

// Imp running past the tier with a shard: upgrade to the Voidwalker.
static void TestImpUpgrades()
{
    CHECK(DecideWarlockSoloPet(Solo(true, true, true, WARLOCK_IMP_PET_ENTRY)) ==
        WarlockSoloPetDecision::SummonVoidwalker);
}

// Voidwalker already out: leave it alone (no resummon loop).
static void TestVoidwalkerLeftAlone()
{
    CHECK(DecideWarlockSoloPet(Solo(true, true, true, WARLOCK_VOIDWALKER_PET_ENTRY)) ==
        WarlockSoloPetDecision::LeaveAlone);
}

// Below the tier (no 697): the Imp stays, petless stays petless.
static void TestBelowTierKeepsImp()
{
    CHECK(DecideWarlockSoloPet(Solo(false, true, true, WARLOCK_IMP_PET_ENTRY)) ==
        WarlockSoloPetDecision::LeaveAlone);
    CHECK(DecideWarlockSoloPet(Solo(false, true, false, 0)) == WarlockSoloPetDecision::LeaveAlone);
}

// Shardless at/above the tier: stay quiet so no failed summon is queued
// (petless falls back to the "no pet" summon-imp node instead).
static void TestShardlessStaysQuiet()
{
    CHECK(DecideWarlockSoloPet(Solo(true, false, true, WARLOCK_IMP_PET_ENTRY)) ==
        WarlockSoloPetDecision::LeaveAlone);
    CHECK(DecideWarlockSoloPet(Solo(true, false, false, 0)) == WarlockSoloPetDecision::LeaveAlone);
}

// Exotic current pet (Succubus/Felhunter from a manual order): never override.
static void TestExoticPetLeftAlone()
{
    CHECK(DecideWarlockSoloPet(Solo(true, true, true, 1863)) == WarlockSoloPetDecision::LeaveAlone);
    CHECK(DecideWarlockSoloPet(Solo(true, true, true, 417)) == WarlockSoloPetDecision::LeaveAlone);
}

// Owned/hired bot with a live master: the player decides, always.
static void TestOwnedBotLeftAlone()
{
    WarlockSoloPetInputs inputs = Solo(true, true, false, 0);
    inputs.hasRealPlayerMaster = true;
    CHECK(DecideWarlockSoloPet(inputs) == WarlockSoloPetDecision::LeaveAlone);
    inputs = Solo(true, true, true, WARLOCK_IMP_PET_ENTRY);
    inputs.hasRealPlayerMaster = true;
    CHECK(DecideWarlockSoloPet(inputs) == WarlockSoloPetDecision::LeaveAlone);
}

// Pin the server-data constants so a stray edit faces this test.
static void TestConstants()
{
    CHECK(WARLOCK_VOIDWALKER_SUMMON_SPELL == 697);
    CHECK(WARLOCK_VOIDWALKER_PET_ENTRY == 1860);
    CHECK(WARLOCK_IMP_PET_ENTRY == 416);
    CHECK(WARLOCK_SOUL_SHARD_ITEM == 6265);
}

// ---- spec-fitting pet (AiPlayerbot.WarlockPetBySpec) -----------------------

static WarlockSpecPetInputs Spec(WarlockSpec spec, int level, bool shard = true)
{
    WarlockSpecPetInputs in;
    in.spec = spec;
    in.knowsVoidwalker = level >= 10;
    in.knowsSuccubus = level >= 20;
    in.knowsFelhunter = level >= 30;
    in.hasSoulShard = shard;
    return in;
}

static void TestSpecPreferenceMapping()
{
    // Affliction and Demonology share the donor chain (no Felguard in 1.12).
    for (WarlockSpec spec : { WarlockSpec::Affliction, WarlockSpec::Demonology })
    {
        CHECK(WarlockSpecPetPreference(spec, false, false, false) == WarlockPetKind::Imp);
        CHECK(WarlockSpecPetPreference(spec, true, false, false) == WarlockPetKind::Voidwalker);
        CHECK(WarlockSpecPetPreference(spec, true, true, false) == WarlockPetKind::Succubus);
        CHECK(WarlockSpecPetPreference(spec, true, true, true) == WarlockPetKind::Felhunter);
    }
    // Destruction is the Imp at every level.
    CHECK(WarlockSpecPetPreference(WarlockSpec::Destruction, false, false, false) == WarlockPetKind::Imp);
    CHECK(WarlockSpecPetPreference(WarlockSpec::Destruction, true, true, true) == WarlockPetKind::Imp);
    CHECK(WarlockSpecPetPreference(WarlockSpec::Unknown, true, true, true) == WarlockPetKind::None);
}

static void TestSpecPetlessSolo()
{
    CHECK(DecideWarlockSpecPet(Spec(WarlockSpec::Affliction, 60)) == WarlockSpecPetDecision::SummonFelhunter);
    CHECK(DecideWarlockSpecPet(Spec(WarlockSpec::Demonology, 25)) == WarlockSpecPetDecision::SummonSuccubus);
    CHECK(DecideWarlockSpecPet(Spec(WarlockSpec::Affliction, 15)) == WarlockSpecPetDecision::SummonVoidwalker);
    CHECK(DecideWarlockSpecPet(Spec(WarlockSpec::Demonology, 5)) == WarlockSpecPetDecision::SummonImp);
    CHECK(DecideWarlockSpecPet(Spec(WarlockSpec::Destruction, 60)) == WarlockSpecPetDecision::SummonImp);
    // Spec unknown (no class strategy yet): existing rows decide.
    CHECK(DecideWarlockSpecPet(Spec(WarlockSpec::Unknown, 60)) == WarlockSpecPetDecision::LeaveAlone);
}

static void TestSpecTankRuleKeepsVoidwalker()
{
    WarlockSpecPetInputs in = Spec(WarlockSpec::Affliction, 60);
    in.petIsGroupTank = true;
    CHECK(DecideWarlockSpecPet(in) == WarlockSpecPetDecision::SummonVoidwalker);
    in = Spec(WarlockSpec::Destruction, 60);
    in.petIsGroupTank = true;
    CHECK(DecideWarlockSpecPet(in) == WarlockSpecPetDecision::SummonVoidwalker);
    // Tank rule needs the Voidwalker learned; below 10 the chain is unchanged.
    in = Spec(WarlockSpec::Affliction, 5);
    in.petIsGroupTank = true;
    CHECK(DecideWarlockSpecPet(in) == WarlockSpecPetDecision::SummonImp);
}

static void TestSpecShardGate()
{
    // Shardless: every demon but the Imp stays quiet (the "no pet" summon-imp
    // fallback covers the petless case); the Imp needs none.
    CHECK(DecideWarlockSpecPet(Spec(WarlockSpec::Affliction, 60, false)) == WarlockSpecPetDecision::LeaveAlone);
    CHECK(DecideWarlockSpecPet(Spec(WarlockSpec::Destruction, 60, false)) == WarlockSpecPetDecision::SummonImp);
}

static void TestSpecLivePetSwap()
{
    // Pool bot: a default demon that does not fit is swapped; a fitting one stays.
    WarlockSpecPetInputs in = Spec(WarlockSpec::Affliction, 60);
    in.hasPet = true;
    in.canSwapLivePet = true;
    in.currentPetEntry = WARLOCK_VOIDWALKER_PET_ENTRY;
    CHECK(DecideWarlockSpecPet(in) == WarlockSpecPetDecision::SummonFelhunter);
    in.currentPetEntry = WARLOCK_FELHUNTER_PET_ENTRY;
    CHECK(DecideWarlockSpecPet(in) == WarlockSpecPetDecision::LeaveAlone);
    // No ping-pong with the tank rule: Voidwalker out under a tankless group stays.
    in.currentPetEntry = WARLOCK_VOIDWALKER_PET_ENTRY;
    in.petIsGroupTank = true;
    CHECK(DecideWarlockSpecPet(in) == WarlockSpecPetDecision::LeaveAlone);
    // Shardless pool bot keeps its live demon (no swap to the Imp).
    in = Spec(WarlockSpec::Affliction, 60, false);
    in.hasPet = true;
    in.canSwapLivePet = true;
    in.currentPetEntry = WARLOCK_VOIDWALKER_PET_ENTRY;
    CHECK(DecideWarlockSpecPet(in) == WarlockSpecPetDecision::LeaveAlone);
    // Destruction pool bot with a Voidwalker swaps to the Imp (no shard needed).
    in = Spec(WarlockSpec::Destruction, 60, false);
    in.hasPet = true;
    in.canSwapLivePet = true;
    in.currentPetEntry = WARLOCK_VOIDWALKER_PET_ENTRY;
    CHECK(DecideWarlockSpecPet(in) == WarlockSpecPetDecision::SummonImp);
    // A player-owned bot never swaps a live pet.
    in = Spec(WarlockSpec::Affliction, 60);
    in.hasPet = true;
    in.canSwapLivePet = false;
    in.currentPetEntry = WARLOCK_IMP_PET_ENTRY;
    CHECK(DecideWarlockSpecPet(in) == WarlockSpecPetDecision::LeaveAlone);
    // An unknown creature (not one of the four demons) is left alone.
    in.canSwapLivePet = true;
    in.currentPetEntry = 12345;
    CHECK(DecideWarlockSpecPet(in) == WarlockSpecPetDecision::LeaveAlone);
}

static void TestSpecConstants()
{
    CHECK(WARLOCK_SUCCUBUS_SUMMON_SPELL == 712);
    CHECK(WARLOCK_FELHUNTER_SUMMON_SPELL == 691);
    CHECK(WARLOCK_SUCCUBUS_PET_ENTRY == 1863);
    CHECK(WARLOCK_FELHUNTER_PET_ENTRY == 417);
    CHECK(WarlockPetEntry(WarlockPetKind::Imp) == 416);
    CHECK(WarlockPetKindFromEntry(417) == WarlockPetKind::Felhunter);
    CHECK(WarlockPetKindFromEntry(0) == WarlockPetKind::None);
}

int main()
{
    std::printf("Starting warlock pet policy tests...\n");
    TestPetlessSummonsVoidwalker();
    std::printf("  [PASS] petless summons voidwalker\n");
    TestImpUpgrades();
    std::printf("  [PASS] imp upgrades\n");
    TestVoidwalkerLeftAlone();
    std::printf("  [PASS] voidwalker left alone\n");
    TestBelowTierKeepsImp();
    std::printf("  [PASS] below tier keeps imp\n");
    TestShardlessStaysQuiet();
    std::printf("  [PASS] shardless stays quiet\n");
    TestExoticPetLeftAlone();
    std::printf("  [PASS] exotic pet left alone\n");
    TestOwnedBotLeftAlone();
    std::printf("  [PASS] owned bot left alone\n");
    TestConstants();
    std::printf("  [PASS] constants\n");
    TestSpecPreferenceMapping();
    std::printf("  [PASS] spec preference mapping\n");
    TestSpecPetlessSolo();
    std::printf("  [PASS] spec petless solo\n");
    TestSpecTankRuleKeepsVoidwalker();
    std::printf("  [PASS] spec tank rule keeps voidwalker\n");
    TestSpecShardGate();
    std::printf("  [PASS] spec shard gate\n");
    TestSpecLivePetSwap();
    std::printf("  [PASS] spec live pet swap\n");
    TestSpecConstants();
    std::printf("  [PASS] spec constants\n");
    std::printf("All warlock pet policy checks PASSED (%d assertions)!\n", checks);
    return 0;
}
