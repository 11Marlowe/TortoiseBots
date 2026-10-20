#pragma once

// Pure policy for random-bot shop mounts vs racial mounts (no core types, so
// tools/test_mount_shop_policy.cpp builds it standalone).
//
// Masks follow item_template AllowableRace / AllowableClass: 0 or -1 means no
// restriction, anything else is the set of allowed races or classes.
//
// Shop mount: a collection item every race and class can use. Racial fit: the
// item's race and class masks include the bot. Spell-only mounts (class mounts,
// forms, donor spells) have no item and are not classified here. The Swift
// Riding Turtle (spell 30174) is the level-18 quest reward and is never a shop
// mount.
//
// Random pool bots get only racial-fit mounts unless AiPlayerbot.RandomBotShopMounts
// is on, which restores today's any-mount behaviour. Player-owned alts are never
// gated (decided by the caller).

#include <cstddef>
#include <cstdint>

namespace MountShopPolicy
{
    constexpr uint32_t TurtleMountSpell = 30174;
    constexpr uint32_t TurtleMountItem = 23720;

    // mask 0 or every bit of `all` set = unrestricted.
    inline bool CoversAll(int32_t mask, uint32_t all)
    {
        return mask == 0 || (static_cast<uint32_t>(mask) & all) == all;
    }

    inline bool AllowsBit(int32_t mask, uint32_t bit)
    {
        return mask == 0 || (static_cast<uint32_t>(mask) & bit) != 0;
    }

    // allRaceMask / allClassMask are the core RACEMASK_ALL_PLAYABLE and
    // CLASSMASK_ALL_PLAYABLE, passed in so this header stays free of core
    // includes. spellId 0 is not a mount.
    inline bool IsShopMount(uint32_t spellId, int32_t allowableRace, int32_t allowableClass,
        uint32_t allRaceMask, uint32_t allClassMask)
    {
        if (spellId == 0 || spellId == TurtleMountSpell)
            return false;
        return CoversAll(allowableRace, allRaceMask) && CoversAll(allowableClass, allClassMask);
    }

    // Racial fit: the item's race and class masks include this bot.
    inline bool FitsBot(int32_t allowableRace, int32_t allowableClass, uint32_t raceBit, uint32_t classBit)
    {
        return AllowsBit(allowableRace, raceBit) && AllowsBit(allowableClass, classBit);
    }

    // Flag on = any mount. Flag off = no shop mounts, and only mounts that fit the bot.
    inline bool IsMountAllowed(bool shopMountsEnabled, bool isShopMount, bool fitsBot)
    {
        return shopMountsEnabled || (!isShopMount && fitsBot);
    }

    // -------------------------------------------------------------------------
    // Strict own-race racial vendor mounts
    // -------------------------------------------------------------------------
    // Derived from Turtle WoW world database dumps:
    //   tw_world_npc_vendor.sql, tw_world_item_template.sql, tw_world_collection_mount.sql.
    //
    // Vendors:
    //   Human / High Elf: Katie Hunter (384), Unger Statforth (1460),
    //                     Merideth Carlson (2357), Gregor MacVince (4885)
    //   Orc:              Ogunaro Wolfrunner (3362)
    //   Dwarf:            Veron Amberstill (1261)
    //   Night Elf:        Lelanai (4730)
    //   Undead:           Zachariah Post (4731)
    //   Tauren:           Harb Clawhoof (3685)
    //   Gnome:            Milli Featherwhistle (7955)
    //   Troll:            Zjolnir (7952)
    //   Goblin:           Dizzle Fuelbopper (80137) & Vanessa Clang (80961)
    //
    // Speed on Turtle scales purely with riding skill:
    //   skill 75 (level 40) -> 60%, skill 150 (level 60) -> 100%.
    // So normal = required level 40, epic = required level 60.

    // Human (1) & High Elf (10): horses
    constexpr uint32_t kHumanNormalMounts[] = { 2411, 2414, 5655, 5656, 12353, 12354 };
    constexpr uint32_t kHumanEpicMounts[]   = { 18776, 18777, 18778 };

    // Orc (2): wolves
    constexpr uint32_t kOrcNormalMounts[]   = { 1132, 5665, 5668 };
    constexpr uint32_t kOrcEpicMounts[]     = { 18796, 18797, 18798 };

    // Dwarf (3): rams
    constexpr uint32_t kDwarfNormalMounts[] = { 5864, 5872, 5873 };
    constexpr uint32_t kDwarfEpicMounts[]   = { 18785, 18786, 18787 };

    // Night Elf (4): sabers
    constexpr uint32_t kNightElfNormalMounts[] = { 8629, 8631, 8632 };
    constexpr uint32_t kNightElfEpicMounts[]   = { 18766, 18767, 18902, 80446 };

    // Undead (5): skeletal horses
    constexpr uint32_t kUndeadNormalMounts[] = { 13331, 13332, 13333 };
    constexpr uint32_t kUndeadEpicMounts[]   = { 13334, 18791 };

    // Tauren (6): kodos
    constexpr uint32_t kTaurenNormalMounts[] = { 15277, 15290 };
    constexpr uint32_t kTaurenEpicMounts[]   = { 18793, 18794, 18795 };

    // Gnome (7): mechanostriders
    constexpr uint32_t kGnomeNormalMounts[] = { 8563, 8595, 13321, 13322 };
    constexpr uint32_t kGnomeEpicMounts[]   = { 18772, 18773, 18774 };

    // Troll (8): raptors
    constexpr uint32_t kTrollNormalMounts[] = { 8588, 8591, 8592 };
    constexpr uint32_t kTrollEpicMounts[]   = { 18788, 18789, 18790 };

    // Goblin (9): Rocket Cars (40) and Shredders (60)
    constexpr uint32_t kGoblinNormalMounts[] = { 80460, 80461, 80462 };
    constexpr uint32_t kGoblinEpicMounts[]   = { 81190, 81191 };

    enum class MountTier : uint8_t
    {
        Normal = 0,
        Epic = 1
    };

    struct MountItemSpan
    {
        uint32_t const* data;
        size_t size;

        uint32_t const* begin() const { return data; }
        uint32_t const* end() const { return data + size; }
        bool empty() const { return size == 0; }
        bool contains(uint32_t id) const
        {
            for (size_t i = 0; i < size; ++i)
                if (data[i] == id)
                    return true;
            return false;
        }
    };

    inline MountItemSpan GetRacialMounts(uint8_t race, MountTier tier)
    {
        switch (race)
        {
        case 1:  // Human
        case 10: // High Elf (same as Human)
            return tier == MountTier::Normal
                ? MountItemSpan{ kHumanNormalMounts, sizeof(kHumanNormalMounts)/sizeof(uint32_t) }
                : MountItemSpan{ kHumanEpicMounts, sizeof(kHumanEpicMounts)/sizeof(uint32_t) };
        case 2:  // Orc
            return tier == MountTier::Normal
                ? MountItemSpan{ kOrcNormalMounts, sizeof(kOrcNormalMounts)/sizeof(uint32_t) }
                : MountItemSpan{ kOrcEpicMounts, sizeof(kOrcEpicMounts)/sizeof(uint32_t) };
        case 3:  // Dwarf
            return tier == MountTier::Normal
                ? MountItemSpan{ kDwarfNormalMounts, sizeof(kDwarfNormalMounts)/sizeof(uint32_t) }
                : MountItemSpan{ kDwarfEpicMounts, sizeof(kDwarfEpicMounts)/sizeof(uint32_t) };
        case 4:  // Night Elf
            return tier == MountTier::Normal
                ? MountItemSpan{ kNightElfNormalMounts, sizeof(kNightElfNormalMounts)/sizeof(uint32_t) }
                : MountItemSpan{ kNightElfEpicMounts, sizeof(kNightElfEpicMounts)/sizeof(uint32_t) };
        case 5:  // Undead
            return tier == MountTier::Normal
                ? MountItemSpan{ kUndeadNormalMounts, sizeof(kUndeadNormalMounts)/sizeof(uint32_t) }
                : MountItemSpan{ kUndeadEpicMounts, sizeof(kUndeadEpicMounts)/sizeof(uint32_t) };
        case 6:  // Tauren
            return tier == MountTier::Normal
                ? MountItemSpan{ kTaurenNormalMounts, sizeof(kTaurenNormalMounts)/sizeof(uint32_t) }
                : MountItemSpan{ kTaurenEpicMounts, sizeof(kTaurenEpicMounts)/sizeof(uint32_t) };
        case 7:  // Gnome
            return tier == MountTier::Normal
                ? MountItemSpan{ kGnomeNormalMounts, sizeof(kGnomeNormalMounts)/sizeof(uint32_t) }
                : MountItemSpan{ kGnomeEpicMounts, sizeof(kGnomeEpicMounts)/sizeof(uint32_t) };
        case 8:  // Troll
            return tier == MountTier::Normal
                ? MountItemSpan{ kTrollNormalMounts, sizeof(kTrollNormalMounts)/sizeof(uint32_t) }
                : MountItemSpan{ kTrollEpicMounts, sizeof(kTrollEpicMounts)/sizeof(uint32_t) };
        case 9:  // Goblin
            return tier == MountTier::Normal
                ? MountItemSpan{ kGoblinNormalMounts, sizeof(kGoblinNormalMounts)/sizeof(uint32_t) }
                : MountItemSpan{ kGoblinEpicMounts, sizeof(kGoblinEpicMounts)/sizeof(uint32_t) };
        default:
            return MountItemSpan{ nullptr, 0 };
        }
    }

    inline bool IsRacialMountForRace(uint8_t race, uint32_t itemId)
    {
        return GetRacialMounts(race, MountTier::Normal).contains(itemId) ||
               GetRacialMounts(race, MountTier::Epic).contains(itemId);
    }

    // By level: from 40 normal (req 40); from 60 epic (req 60).
    // Level < 40 allows none from the racial list.
    // Level 40..59 allows only normal racial mounts of that race.
    // Level 60 allows normal and epic racial mounts of that race.
    inline bool IsRacialMountAllowed(uint8_t race, uint8_t level, uint32_t itemId)
    {
        if (level < 40)
            return false;
        if (GetRacialMounts(race, MountTier::Normal).contains(itemId))
            return true;
        if (level >= 60 && GetRacialMounts(race, MountTier::Epic).contains(itemId))
            return true;
        return false;
    }

    // Sorting preference when riding:
    // 2 = Epic racial mount (level >= 60)
    // 1 = Normal racial mount (level >= 40)
    // 0 = Turtle mount (30174) or non-racial fallback
    inline int GetMountPreference(uint8_t race, uint8_t level, uint32_t itemId, uint32_t spellId)
    {
        (void)spellId;
        if (level >= 60 && GetRacialMounts(race, MountTier::Epic).contains(itemId))
            return 2;
        if (level >= 40 && GetRacialMounts(race, MountTier::Normal).contains(itemId))
            return 1;
        return 0;
    }
}
