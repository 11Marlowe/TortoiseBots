#pragma once

// Pure policy for alt-bots (Issue #872): a player's own alt that keeps playing
// on its own after it is kicked from the owner's party.
//
// Header-only and free of core/database types on purpose: the rules are
// exercised standalone in tools/test_alt_bot_policy.cpp. AltBotService owns
// the live state and feeds these functions plain values.

#include <cctype>
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

// B8/F41/G43: towards other players an autonomous alt behaves like a pool
// bot: strangers (anyone not on the owner's account and not a GM) may give
// non-destructive, combat-related orders. Only the owner has full control.
// The gate below keys on the chat command's first word, exactly like
// PlayerbotAI::AuthTierForCommand; HandleCommand and the `.bot` commander
// paths feed it plain values so the rule stays unit-testable (Issue #872).
enum class AltBotStrangerVerdict : uint8_t
{
    Allowed,    // combat movement or a read-only/info command
    OwnerOnly,  // economy, inventory, guild, AI configuration, removal
};

// First word of a chat command, lowercased, leading blanks skipped.
inline std::string AltBotFirstCommandWord(std::string const& command)
{
    size_t begin = command.find_first_not_of(" \t");
    if (begin == std::string::npos)
        return {};
    size_t end = command.find_first_of(" \t", begin);
    std::string word = command.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
    for (char& c : word)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return word;
}

// May a stranger (not the owner's account, not a GM) give this chat command
// to an autonomous alt-bot? Allowed: follow/stay/guard/free/wander, combat
// orders (attack/pull/pullback/tank/rti/formation/stance), movement queries
// and read-only info (who/where/wts/stats/c/items/spells), emotes and the
// ready check. Everything else is owner-only: trade, selling, destroying,
// equipping, talents, quest abandon, logout, guild commands, hearth bind
// (home), hire, strategy/AI edits (co/nc/de/all/react/save/max/possible/
// attackers/position/queue/los/cast/pet/buff), and any manipulation outside
// combat. `grind` stays owner-only even though a pool bot takes it from
// anyone: on an alt it rewrites the whole leisure mixer (B7), not one
// fight. Unknown commands fail closed to OwnerOnly.
inline AltBotStrangerVerdict CheckAltBotStrangerCommand(std::string const& command)
{
    std::string const word = AltBotFirstCommandWord(command);
    // Follow/stay/guard/free/wander/flee/runaway double as the first words
    // of the CombatStrategy-side two-word movement shortcuts
    // (`stay ...`, `flee ...`), so the first-word key covers both.
    if (word == "follow" || word == "stay" || word == "guard" || word == "free" ||
        word == "wander" || word == "flee" || word == "runaway" ||
        word == "attack" || word == "pull" || word == "pullback" ||
        word == "tank" || word == "rti" ||
        word == "formation" || word == "stance" ||
        word == "ready" || word == "rebuff" ||
        word == "who" || word == "where" || word == "wts" || word == "stats" ||
        word == "c" || word == "items" || word == "inv" || word == "inventory" ||
        word == "spells" || word == "emote" || word == "help" ||
        word == "wait" || word == "jump")
        return AltBotStrangerVerdict::Allowed;
    return AltBotStrangerVerdict::OwnerOnly;
}

// F40: the alt accepts a guild invite only from the owner's account (or a
// GM, who may do anything). A bot already in a guild never joins another.
// `sameAccount`: inviter's account == the alt's owner account. The guild-id
// and hired checks stay at the call site; this is only the account gate.
inline bool AltBotMayAcceptGuildInvite(bool sameAccount, bool isGm)
{
    return sameAccount || isGm;
}
// Group invites need no alt gate: a stranger invites the autonomous alt like
// a pool bot (AcceptInvitationAction). An ungrouped bot with no master
// passes the INVITE security level for anyone, and joining the stranger's
// group does not end the session: AltBotService::OnMasterBound ends it only
// for the owner's account. Verified, not wired (Issue #872).
inline bool AltBotMayAcceptGroupInvite() { return true; }

// B8: strangers invite the alt like a pool bot and give it the combat orders
// above. `.bot remove`/logout-style removal stays owner-or-GM only
// (CanControlBot already enforces that); this mirrors the chat gate for the
// `.bot` commander paths, whose tactical verbs match the chat list.
inline AltBotStrangerVerdict CheckAltBotStrangerOrder(std::string const& verb)
{
    return CheckAltBotStrangerCommand(verb);
}

// G49: the pool reset can never touch an owned alt. Targets come only from
// the managed-account registry (SnapshotTargets selects characters whose
// account is registered in tortoise_bots_pool_account; a player account is
// never registered there), and deletion revalidates the snapshotted account
// (DeleteNextTarget refuses a character that moved accounts). An alt is a
// character on a player account (TryStart requires a non-registered
// account), so it can never be a target: the account sets are disjoint by
// construction. Policy-level form of that disjointness (Issue #872).
inline bool AltBotExcludedFromPoolReset(bool accountIsRegisteredPool) { return !accountIsRegisteredPool; }

// G50: "party leave deletes hired" can never touch an owned alt. Deletion
// needs a tortoise_bots_hire ledger row written once by Hire() for a
// module-created character (DecideHireDeletion refuses without one), and the
// group hooks only dismiss entries already in the hire registry
// (OnGroupMemberRemoved/OnGroupDisband iterate m_hired). An alt never has a
// row and never enters the registry, so it never reaches deletion.
inline bool AltBotExcludedFromHireDeletion(bool hasHireLedgerRow) { return !hasHireLedgerRow; }

} // namespace TortoiseBots
