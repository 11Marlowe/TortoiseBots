#pragma once

// Issue #1125: which combat/noncombat kit pieces a shaman
// tank bot installs. The tank strategy itself is "tank shaman";
// this policy only decides the kit lists so AiFactory, the `.bot role`
// handler, and the tests share one definition instead of drifting.
//
// Pure policy: no core/engine headers, unit-tested standalone
// (tools/test_shaman_tank_kit_policy.cpp).

#include <cstdint>
#include <string>
#include <vector>

namespace TortoiseBots
{

// Combat kit for a shaman with a tank role: the new spec strategy plus the
// shared tank-framework pieces (warrior `:575` / bear `:667` pattern), then
// the shared shaman pieces that still apply. "aoe" is a placeholder strategy:
// the tank's Fire Nova / Magma rows live in the "aoe tank shaman *" variant,
// which the shaman strategy updater only installs while both the spec and
// "aoe" are present (the pack tools were dead without it). Deliberately NOT
// "dps assist" (threat race), NOT "behind" (tanks face the mob), NOT threat
// backoff. `.bot action aoe off` is unaffected: it adds the separate
// "no aoe" veto strategy, which does not depend on the kit.
inline std::vector<std::string> ShamanTankCombatKit()
{
    return { "tank shaman", "tank assist", "pull", "pull back", "close", "tank face",
        "cure", "totems", "buff", "boost", "aoe" };
}

// Noncombat kit mirrors the druid tank branch: spec strategy + "tank assist"
// on top of the shared shaman noncombat pieces.
inline std::vector<std::string> ShamanTankNonCombatKit()
{
    return { "tank shaman", "tank assist", "cure", "totems", "buff", "boost" };
}

// True when the installed combat kit names play a tank slot: the spec plus
// every framework piece. The bare "tank" alias (LFT handoff) counts too.
inline bool ShamanTankKitCoversTank(std::vector<std::string> const& kit)
{
    for (std::string const& name : kit)
    {
        if (name == "tank shaman" || name == "tank assist" || name == "tank" ||
            name == "tank face" || name == "pull" || name == "pull back")
            return true;
    }
    return false;
}

// Premade spec identity for the 7.3 tank build: class 7, spec index 3,
// name "tank". Pool bots drawing this spec get a persisted forced role TANK.
inline constexpr uint8_t kShamanTankPremadeClass = 7;
inline constexpr int kShamanTankPremadeSpec = 3;

} // namespace TortoiseBots
