#pragma once

// Pure policy for alt-bots (Issue #872): a player's own alt that keeps playing
// on its own after it is kicked from the owner's party.
//
// Header-only and free of core/database types on purpose: the rules are
// exercised standalone in tools/test_alt_bot_policy.cpp. AltBotService owns
// the live state and feeds these functions plain values.

#include <cstdint>
#include <string>
#include <vector>

namespace TortoiseBots
{

// One state machine per owned character. Offline and Companion are what
// exists today (logged out / `.bot add` follower); Autonomous is the alt-bot.
enum class AltBotState : uint8_t
{
    Offline,
    Companion,
    Autonomous,
};

enum class AltBotStartRefusal : uint8_t
{
    None,
    FeatureOff,
    NotOwnedAlt,
    Hardcore,
    NotInWorld,
};

// May a companion that just left the owner's party become an alt-bot?
// ownedAlt: a character on a player account (not a pool account, not hired).
// Hardcore is refused: a bot death would end the player's challenge run.
inline AltBotStartRefusal CheckAltBotStart(bool featureEnabled, bool ownedAlt, bool hardcore, bool inWorld)
{
    if (!featureEnabled)
        return AltBotStartRefusal::FeatureOff;
    if (!ownedAlt)
        return AltBotStartRefusal::NotOwnedAlt;
    if (!inWorld)
        return AltBotStartRefusal::NotInWorld;
    if (hardcore)
        return AltBotStartRefusal::Hardcore;
    return AltBotStartRefusal::None;
}

// Why an alt-bot session ended; picks the summary wording.
enum class AltBotEnd : uint8_t
{
    Removed,      // `.bot remove`, GM stop or the owner logging out (LogoutWithOwner)
    TakenBack,    // `.bot add` on the autonomous alt: it is a companion again
    PlayerLogin,  // the player logged in on the alt itself
};

// In-memory session counters for the end-of-session summary (no DB).
struct AltBotSessionCounters
{
    uint32_t startLevel = 0;
    uint32_t endLevel = 0;
    uint32_t quests = 0;
    uint32_t deaths = 0;
    uint32_t startMoney = 0; // copper
    uint32_t endMoney = 0;
};

inline std::string FormatAltBotMoney(int64_t copper)
{
    bool const negative = copper < 0;
    uint64_t const value = static_cast<uint64_t>(negative ? -copper : copper);
    std::string out = negative ? "-" : "+";
    uint64_t const gold = value / 10000, silver = (value / 100) % 100, cop = value % 100;
    if (gold)
        out += std::to_string(gold) + "g ";
    if (gold || silver)
        out += std::to_string(silver) + "s ";
    out += std::to_string(cop) + "c";
    return out;
}

inline std::string FormatAltBotSummary(std::string const& name, AltBotSessionCounters const& c)
{
    std::string out = name + " played on its own: ";
    if (c.endLevel > c.startLevel)
        out += "level " + std::to_string(c.startLevel) + " -> " + std::to_string(c.endLevel);
    else
        out += "level " + std::to_string(c.endLevel);
    out += ", " + std::to_string(c.quests) + (c.quests == 1 ? " quest" : " quests");
    out += ", " + std::to_string(c.deaths) + (c.deaths == 1 ? " death" : " deaths");
    out += ", gold " + FormatAltBotMoney(static_cast<int64_t>(c.endMoney) - static_cast<int64_t>(c.startMoney)) + ".";
    return out;
}

// F37: each active alt displaces one pool bot. Only a bot that nobody is
// using can go: not grouped, not in combat, not in an instance, no master.
struct PoolDisplaceCandidate
{
    uint32_t guidLow = 0;
    uint32_t level = 0;
    bool grouped = false;
    bool inCombat = false;
    bool inInstance = false;
    bool hasMaster = false;
};

// Returns the guid of a random bot among the lowest-level eligible ones, or 0.
// `roll` is any random number; it picks among the lowest-level ties.
inline uint32_t PickPoolBotToDisplace(std::vector<PoolDisplaceCandidate> const& candidates, uint32_t roll)
{
    uint32_t lowest = UINT32_MAX;
    uint32_t ties = 0;
    for (PoolDisplaceCandidate const& c : candidates)
    {
        if (c.grouped || c.inCombat || c.inInstance || c.hasMaster || !c.guidLow)
            continue;
        if (c.level < lowest)
        {
            lowest = c.level;
            ties = 1;
        }
        else if (c.level == lowest)
            ++ties;
    }
    if (!ties)
        return 0;
    uint32_t pick = roll % ties;
    for (PoolDisplaceCandidate const& c : candidates)
    {
        if (c.grouped || c.inCombat || c.inInstance || c.hasMaster || !c.guidLow || c.level != lowest)
            continue;
        if (!pick--)
            return c.guidLow;
    }
    return 0;
}

// The pool's online target while alts are active: each alt takes one slot.
inline uint32_t PoolTargetWithAlts(uint32_t target, uint32_t activeAlts)
{
    return activeAlts >= target ? 0 : target - activeAlts;
}
// Location safety (Issue #872, instances worker): an alt that becomes
// autonomous inside an instance/BG or on a transport is out of place alone,
// and an autonomous alt left alone inside an instance should leave.
// The module can only teleport it out; the core owns graveyards and portals.
enum class AltBotExitTarget : uint8_t
{
    None,            // no relocation needed
    InstanceExit,    // dungeon/raid map portal out (or BG entry point): preferred
    Graveyard,       // BG or no usable exit portal: nearest graveyard
    LevelZone,       // ordinary map: the C19 level-fitting scatter
};

struct AltBotLocation
{
    bool inWorld = false;
    bool alive = false;
    bool inBattleGround = false;
    bool onTransport = false;   // boat/zeppelin: outside the world routing
    bool instanceableMap = false; // dungeon, raid or BG map
    bool dungeonMap = false;    // dungeon or raid only (has an exit portal)
    bool haveExit = false;      // GetGoBackTrigger(map) returns a portal out
    bool haveGrave = false;     // nearest graveyard lookup returned a row
    bool graveSameMap = false;
};

// Where should an autonomous alt start out? Ungrouped and misplaced (in an
// instance/BG or on a transport) it leaves via the exit portal when one
// exists, else falls to the nearest graveyard (BGs always do: the BG exit
// teleports nowhere useful mid-match). Ordinary maps take the C19 scatter
// (None: the caller still runs TeleportToLevelFittingZone, it already
// fail-closes). Grouped with someone else it stays put (F42, handled by
// TryStart's early return, not here).
inline AltBotExitTarget PickAltBotStartExit(AltBotLocation const& where, bool grouped)
{
    if (!where.inWorld || grouped)
        return AltBotExitTarget::None;
    if (!where.inBattleGround && !where.onTransport && !where.instanceableMap)
        return AltBotExitTarget::None;
    if (where.inBattleGround || !where.dungeonMap || !where.haveExit)
        return where.haveGrave ? AltBotExitTarget::Graveyard : AltBotExitTarget::None;
    return AltBotExitTarget::InstanceExit;
}

// Throttle guard for the alone-in-instance sweep (AltBotService::Update runs
// every world tick; scanning the alt sessions is cheap but pointless per
// tick). Elapsed `nowMs - lastMs` ms with 32-bit wraparound.
constexpr uint32_t kAltBotSweepIntervalMs = 5000;
inline bool AltBotSweepDue(uint32_t nowMs, uint32_t lastMs)
{
    return nowMs - lastMs >= kAltBotSweepIntervalMs;
}

// An autonomous alt in a dungeon/raid map with no other real players in its
// group on that map leaves for the entrance (Issue #872). Cheap inputs only:
// the caller walks its own session list, never scans the world.
inline bool ShouldAltBotLeaveInstance(bool autonomous, bool inWorld, bool alive, bool teleporting,
    bool dungeonMap, bool grouped, bool groupHasOthersOnMap)
{
    if (!autonomous || !inWorld || !alive || teleporting || !dungeonMap)
        return false;
    return !grouped || !groupHasOthersOnMap;
}

} // namespace TortoiseBots
