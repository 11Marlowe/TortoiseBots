#pragma once

// Whether a bot opens a locked door with a plain use (CMSG_GAMEOBJ_USE, a
// player's right-click), as the core lets a player do: GameObject::PlayerCanUse
// (tortoise-wow src/game/Objects/GameObject.cpp:2247-2273) checks only a key
// item for a door, and GameObject::Use opens it (UseDoorOrButton, :1453).
//
// Uldaman's Temple Doors 124368 and 141869 carry lock 86 (Lock.dbc: a skill
// entry of lock type 10, quick open, no skill): the bot's own door path looked
// for a spell of its own that opens the lock, found none and refused ("could
// not use it"), while the door opens on a plain use (2026-10-16 live run: the
// agent's use_object opened both).
//
// A plain use is taken only for a lock that needs nothing a player could lack:
// no key item and no profession skill (lockpicking, herbalism, mining, fishing,
// survival: SkillByLockType). A door that needs a key keeps the key path, and a
// lockpicking lock is never opened by a use.
//
// Standalone (no core types): types[i] is the lock's Type[i] (0 none, 1 key
// item, 2 skill), index[i] its Index[i] (the item, or the lock type),
// skillNeeded[i] whether SkillByLockType(index[i]) is a real skill.

#include <cstdint>

namespace TortoiseBots
{
namespace DoorLock
{

constexpr std::uint32_t kLockKeyNone = 0;
constexpr std::uint32_t kLockKeyItem = 1;
constexpr std::uint32_t kLockKeySkill = 2;
constexpr int kLockSlots = 8;

inline bool OpensOnPlainUse(const std::uint32_t (&types)[kLockSlots], const std::uint32_t (&index)[kLockSlots],
    const bool (&skillNeeded)[kLockSlots])
{
    for (int i = 0; i < kLockSlots; ++i)
    {
        if (types[i] == kLockKeyItem && index[i])
            return false;
        if (types[i] == kLockKeySkill && skillNeeded[i])
            return false;
        if (types[i] != kLockKeyNone && types[i] != kLockKeyItem && types[i] != kLockKeySkill)
            return false; // a kind of lock this rule does not know
    }
    return true;
}

} // namespace DoorLock
} // namespace TortoiseBots
