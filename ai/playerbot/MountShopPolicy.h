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

#include <cstdint>

namespace MountShopPolicy
{
    constexpr uint32_t TurtleMountSpell = 30174;

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
}
