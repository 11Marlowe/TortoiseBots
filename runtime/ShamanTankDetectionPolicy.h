#pragma once

#include <cstdint>

// Shaman tank detection (issue #1125): a shaman
// (player or bot — the gear and spell reads are identical for both) counts
// as a tank when all three hold: a shield is equipped, the main hand carries
// a Rockbiter imbue, and a key tank talent is known (Totemic Alignment or
// Spirit Armor, either rank). A forced role wins over the read either way:
// an ordered tank needs no gear, an ordered non-tank stays non-tank. A
// detected shaman tank co-tanks: another tank in the group never demotes it,
// and it counts as an existing tank for everyone else (both enforced by the
// AiFactory wiring; the predicate pins the rule by ignoring otherTankPresent).
//
// Pure decision logic; the live input gathering (item slots, HasSpell) lives
// in AiFactory.cpp (IsDetectedShamanTank). Unit-tested on its own
// (tools/test_shaman_tank_detection_policy.cpp).
//
// Key-talent spell ids verified against the core DBCs:
// Totemic Alignment R1/R2 = 51381/51382, Spirit Armor R1/R2 = 45951/45952.
// Read the top rank first; rank 2 in the spell map does not imply rank 1,
// so both ranks are checked.

namespace TortoiseBots
{

constexpr uint32_t kShamanTankTalentTotemicAlignmentR1 = 51381;
constexpr uint32_t kShamanTankTalentTotemicAlignmentR2 = 51382;
constexpr uint32_t kShamanTankTalentSpiritArmorR1 = 45951;
constexpr uint32_t kShamanTankTalentSpiritArmorR2 = 45952;

struct ShamanTankDetectionInputs
{
    bool forcedTank = false;          // stored/bot forced role says TANK
    bool forcedNonTank = false;        // a forced DPS/healer role wins over gear
    bool shieldEquipped = false;      // shield in the off-hand slot
    bool rockbiterOnMainHand = false;  // Rockbiter (totem-type) temp enchant on main hand
    bool totemicAlignment = false;    // HasSpell(51382) || HasSpell(51381)
    bool spiritArmor = false;         // HasSpell(45952) || HasSpell(45951)
    bool otherTankPresent = false;    // warrior/paladin/druid (or second shaman) tank in group
};

inline bool IsShamanTank(ShamanTankDetectionInputs const& inputs)
{
    // An explicit forced role wins over the gear/talent read (a forced role
    // is single-valued upstream, so both flags set is unreachable).
    if (inputs.forcedNonTank)
        return false;
    if (inputs.forcedTank)
        return true;
    // Detection needs all three: shield, Rockbiter, and a key tank talent.
    // Tab 1 is Enhancement for both DPS and tank builds, so the tab never
    // decides here.
    if (!inputs.shieldEquipped || !inputs.rockbiterOnMainHand)
        return false;
    if (!inputs.totemicAlignment && !inputs.spiritArmor)
        return false;
    // Co-tank: another tank present never demotes a detected shaman tank.
    return true;
}

} // namespace TortoiseBots
