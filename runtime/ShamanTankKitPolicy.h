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
// name "tank". Pool bots drawing this spec get the forced role TANK from the
// factory (and keep tanking from the build read once it is gone).
inline constexpr uint8_t kShamanTankPremadeClass = 7;
inline constexpr int kShamanTankPremadeSpec = 3;

// ai::BOT_ROLE_TANK (BotRoles) — the bit a forced role carries for a tank.
inline constexpr uint8_t kShamanTankRoleBit = 0x01;

// Does this shaman bot play the tank kit? An explicit forced role (`.bot
// role`, hire, dungeon finder) decides outright: an ordered tank plays tank
// even without the build, an ordered DPS/healer keeps its own kit. Only with
// no forced role does the bot's own build decide — that is the durable half,
// since talents live in the character DB and the forced role does not
// (issue #1125: a pool shaman that rolled 7.3 came back from a restart and
// after every group leave with its tank talents and the enhancement DPS kit).
inline bool ShamanPlaysTankKit(uint8_t forcedRole, bool tankBuild)
{
    if (forcedRole)
        return (forcedRole & kShamanTankRoleBit) != 0;
    return tankBuild;
}

// Roll weight of a premade path for a random pool bot. The 7.3 shaman tank
// build weighs 0 while AiPlayerbot.ShamanTankEnabled is off; every other path
// keeps its configured probability (issue #1125). Negative weights count as 0.
inline uint32_t PremadePathRollWeight(uint8_t cls, int pathId, int probability, bool shamanTankEnabled)
{
    if (!shamanTankEnabled && cls == kShamanTankPremadeClass && pathId == kShamanTankPremadeSpec)
        return 0;
    return probability > 0 ? static_cast<uint32_t>(probability) : 0;
}

} // namespace TortoiseBots
