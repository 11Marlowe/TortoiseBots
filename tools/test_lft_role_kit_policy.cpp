#include "../runtime/LftRoleKitPolicy.h"

#include <cstdlib>
#include <iostream>
#include <cstdint>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using TortoiseBots::LftRoleKitInputs;
using TortoiseBots::LftKitNeedsRoleAssignment;
using TortoiseBots::LftRoleMaskForLatentShamanTank;
using TortoiseBots::kLftRoleTank;
using TortoiseBots::kLftRoleHealer;
using TortoiseBots::kLftRoleDamage;

namespace
{
    bool Assign(uint8_t queued, uint8_t talents, uint8_t kit)
    {
        return LftKitNeedsRoleAssignment({queued, talents, kit});
    }

    uint8_t Mask(bool isShaman, uint8_t roles, bool keyTalents)
    {
        return LftRoleMaskForLatentShamanTank(isShaman, roles, keyTalents);
    }
}

int main()
{
    std::cout << "Starting TortoiseBots LFT role kit tests...\n";

    // (1) The issue #966 incident: feral druid (masks Tank|DPS, kit Cat) is
    // queued into the tank slot with its own spec covering it. The kit must
    // be handed the role or the group leans on a cat.
    uint8_t const feralMask = kLftRoleTank | kLftRoleDamage;
    CHECK(Assign(kLftRoleTank, feralMask, kLftRoleDamage));
    CHECK(Assign(kLftRoleTank, feralMask, 0)); // bare kit too: cannot play the slot
    std::cout << "  [PASS] feral druid queued tank while cat kit installed\n";

    // (2) Same druid queued DPS: the cat kit already plays that slot.
    CHECK(!Assign(kLftRoleDamage, feralMask, kLftRoleDamage));
    std::cout << "  [PASS] feral druid queued dps keeps its kit untouched\n";

    // (3) Single-role spec picks: the kit already matches the slot.
    CHECK(!Assign(kLftRoleTank, kLftRoleTank, kLftRoleTank));       // protection warrior
    CHECK(!Assign(kLftRoleHealer, kLftRoleHealer, kLftRoleHealer)); // holy priest
    CHECK(!Assign(kLftRoleHealer, kLftRoleHealer,
                  kLftRoleHealer | kLftRoleDamage));                // healer with an offdps helper
    CHECK(!Assign(kLftRoleDamage, kLftRoleDamage, kLftRoleDamage)); // retribution paladin
    std::cout << "  [PASS] single-role spec picks skip the handoff\n";

    // (4) Kit carries the slot but the spec has NOT been asked for it: a bear
    // kit with no queued role stays where it is.
    CHECK(!Assign(kLftRoleDamage, feralMask, kLftRoleTank | kLftRoleDamage));
    std::cout << "  [PASS] kit covering the slot never re-assigns\n";

    // (5) Not a natural pick (talent mask cannot fill the slot): the
    // eligibility gate already rejected it; borrowing assigns independently.
    CHECK(!Assign(kLftRoleHealer, kLftRoleDamage, 0));
    CHECK(!Assign(kLftRoleTank, kLftRoleHealer | kLftRoleDamage, 0));
    std::cout << "  [PASS] off-spec slot never triggers the kit handoff\n";

    // (6) No queued role: nothing to honour.
    CHECK(!Assign(0, feralMask, 0));
    std::cout << "  [PASS] empty queued role is a no-op\n";

    // (7) A druid kit that has already been handed the bear role covers the
    // slot, so the pick leaves it alone.
    CHECK(!Assign(kLftRoleTank, feralMask, kLftRoleTank));
    std::cout << "  [PASS] bear kit without a queued role stays put\n";

    // (8) Issue #1125: a shaman that reads tank from its levelling build
    // before the key talents is kept fillable as damage - the tank slot stays
    // closed to it, but without the DPS bit every slot would be.
    CHECK(Mask(true, kLftRoleTank, false) == (kLftRoleTank | kLftRoleDamage));
    // With the key talents the read is a real tank: damage does not apply.
    CHECK(Mask(true, kLftRoleTank, true) == kLftRoleTank);
    // A damage or healer shaman is untouched either way.
    CHECK(Mask(true, kLftRoleDamage, false) == kLftRoleDamage);
    CHECK(Mask(true, kLftRoleHealer, false) == kLftRoleHealer);
    CHECK(Mask(true, kLftRoleTank | kLftRoleHealer, true) == (kLftRoleTank | kLftRoleHealer));
    // Non-shamans never change.
    CHECK(Mask(false, kLftRoleTank, false) == kLftRoleTank);
    const uint8_t latentMask = Mask(true, kLftRoleTank, false);
    CHECK(Assign(kLftRoleDamage, latentMask, kLftRoleTank)); // tank kit cannot play the queued DPS slot
    CHECK(!Assign(kLftRoleDamage, latentMask, kLftRoleDamage));
    std::cout << "  [PASS] latent shaman tank stays fillable as damage\n";

    std::cout << "TortoiseBots LFT role kit tests passed.\n";
    return 0;
}
