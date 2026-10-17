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

// ---- Protection rules (Issue #872, B9-B16 + footprint) ----
//
// An autonomous alt keeps the pool's leisure strategy set (AiFactory gates on
// IsFreeAltBot, which is true for it), so every protection below is an extra
// gate at the action/value site. The rules stay core-free: callers feed plain
// values, and each rule documents the gate it pairs with.
//
// Record random=false on purpose (pool identity paths - pool reset, gear
// randomization, AH, hire deletion - must never see an alt), so gates keyed
// on sRandomBotFacade.IsRandomBot(bot) never fire for alts. Gates below key
// on IsAutonomous instead: it is the only identity an alt-bot has.

// B9: no AH, no mail, no trade, no guild bank. The command choke point
// (AuthTierForCommand OwnerOrGm + IsBotOwnerOrGm) already forces every one of
// these through the owner's account: a stranger's `.bot` order is denied.
// Autonomous economy trips additionally refuse an alt at the value/trigger
// site (RpgAHSell/RpgAHBuy/RpgGetMail/RpgTradeUseful refuse when autonomous;
// GuildBankAction::MoveFromCharToBank is a no-op for everyone).
inline bool AltBotMayUseEconomy(bool isAutonomous)
{
    return !isAutonomous;
}

// B10: vendoring sells only greys (poor) and unneeded whites (common), NEVER
// soulbound or equipped items. Gate in SellAction: the parse resolves to a
// quality-filtered list and Sell() vetoes soulbound/equipped pieces.
// Qualities: 0 poor, 1 common, 2 uncommon, 3 rare, 4 epic, 5 legendary.
inline bool AltBotMayVendorItem(uint32_t quality, bool isSoulbound, bool isEquipped)
{
    if (isSoulbound || isEquipped)
        return false;
    return quality <= 1;
}

// B10 second half: repairs from its own gold. The repair path is
// RepairAllAction with no cheat: alts carry only Bank cheats repair/breath
// (BotCheats), so HasCheat(gold) is false and the gold is really spent.
inline bool AltBotRepairIsPaid(bool hasGoldCheat)
{
    return !hasGoldCheat;
}

// B11: gold goes on trainer spells, repair and flight paths only. Never
// mount, bags, respec or other vendor buys. The vendor buy loop's per-usage
// budgets (BuyAction) already key off these flags: mount/bag/respec usages
// refuse for autonomous alts while spells/repair/travel pass.
enum class AltBotBuyNeed : uint8_t
{
    TrainerSpell,
    Repair,
    FlightPath,
    Mount,
    Bag,
    Respec,
    OtherVendorBuy,
};
inline bool AltBotMaySpendGoldOn(AltBotBuyNeed need)
{
    switch (need)
    {
        case AltBotBuyNeed::TrainerSpell:
        case AltBotBuyNeed::Repair:
        case AltBotBuyNeed::FlightPath:
            return true;
        default:
            return false;
    }
}

// B12: no auto talent spending. The level-up packet path (SMSG_LEVELUP_INFO ->
// AutoSetTalentsAction) and the AI-build hook (PlayerbotAI init) both refuse
// when IsAutonomous: points stay unspent for the player. Explicit `.bot
// talents` orders from the owner account still work (they are commands, not
// automation).
inline bool AltBotMayAutoSpendTalents(bool isAutonomous)
{
    return !isAutonomous;
}

// B13: quest reward choice is the best item for the class (item score). The
// auto path (TalkToQuestGiverAction::RewardMultipleItem) already scores usage
// EQUIP-first with a live-stat-weight tiebreak; this documents the pick rule
// for the standalone test: the highest score wins, earliest choice on ties.
// `scores` holds (choice index, score) pairs.
inline uint32_t AltBotQuestRewardChoice(std::vector<std::pair<uint32_t, uint32_t>> const& scores)
{
    uint32_t best = 0;
    bool have = false;
    uint32_t bestScore = 0;
    for (auto const& scored : scores)
    {
        if (!have || scored.second > bestScore)
        {
            have = true;
            best = scored.first;
            bestScore = scored.second;
        }
    }
    return best;
}

// B14: auto-equip upgrades only if the old piece fits in the bags. The
// EquipItem path moves the displaced piece to the bags via core SwapItem;
// this gate runs first: with no free slot the equip is skipped and the bot
// keeps playing. Companion `.bot add` with zero free slots warns the owner
// instead (TryStart FreeBagSlots check).
inline bool AltBotMayAutoEquip(bool oldPieceGoesSomewhere)
{
    return oldPieceGoesSomewhere;
}

// B16: no drop boost. ApplyBotLootBonus already refuses non-random bots and
// alts carry record.random=false, so no gate change is needed; this pins it.
inline bool AltBotMayGetDropBoost(bool isRandomBot)
{
    return isRandomBot;
}

// C (group loot): an alt in any group rolls Greed or Pass, never Need.
// Gate in RollAction::CalculateRollVote: NEED demotes to GREED for alts.
// 0 = Pass, 1 = Greed, 2 = Need (matches ROLL_PASS/GREED/NEED order).
inline uint32_t AltBotGroupLootVote(uint32_t vote, bool isAutonomous)
{
    if (isAutonomous && vote == 2)
        return 1;
    return vote;
}

// B15: the item cheat (ammo/reagents) applies as for the pool. The cheat
// mask is never set per-bot, so an alt reads HasCheat(item) from the Bank
// list (BotCheats) while the pool reads the random list (RndBotCheats).
// Enabled at the HasCheat choke point, not by editing the lists: the Bank
// list serves owned alts too, which must never get the item cheat.
inline bool AltBotItemCheatCovered(bool altHasItemCheat)
{
    return altHasItemCheat;
}

// Footprint: never rebinds its hearthstone. RpgHomeBindTrigger refuses when
// autonomous; owned non-autonomous alts keep today's behaviour.
inline bool AltBotMayRebindHearth(bool isAutonomous)
{
    return !isAutonomous;
}

// Footprint: quests. An autonomous alt only abandons quests it took itself
// (AltBotService::IsTakenQuest, recorded at accept, cleared on session end)
// and keeps free quest-log slots for the player. MAX_QUEST_LOG_SIZE is 25 on
// this core. Accepts stay allowed until only the reserve is left.
constexpr uint32_t kAltBotFreeQuestSlots = 10;
constexpr uint32_t kMaxQuestLogSize = 25;
inline bool AltBotMayAcceptQuest(uint32_t questsHeld, uint32_t freeSlotsReserved = kAltBotFreeQuestSlots)
{
    uint32_t capacity = kMaxQuestLogSize > freeSlotsReserved ? kMaxQuestLogSize - freeSlotsReserved : 0;
    return questsHeld < capacity;
}
inline bool AltBotMayAbandonQuest(bool isAutonomous, bool takenByAlt)
{
    if (!isAutonomous)
        return true;
    return takenByAlt;
}

// Footprint: never PICKS targets that lower reputation on its own. Killing
// `entry` lowers some standing when the core holds a ReputationOnKill row
// for it with a negative value. Rep lost inside a group fight is accepted
// (minor, documented).
inline bool AltBotMayPickTarget(bool killLowersRep)
{
    return !killLowersRep;
}

} // namespace TortoiseBots
