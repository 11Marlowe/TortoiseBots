#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <set>

// Issue #490 PR 2 (kills): per-character hire-unlock boss kills. Rows live
// in tortoise_bots_hire_unlock_kills (guid, boss_entry = encounter key);
// the cache is filled at login from the character DB, credited on boss
// kills, dropped at logout. Map updates run on several threads, so every
// access takes the mutex. DB writes are asynchronous (PExecute): the kill
// path never blocks on the database.
// Sync math (recorded + optional maps -> quest-log counter) lives in
// runtime/HireUnlockSync.h so the adapter and the gossip page share it.

namespace TortoiseBots
{

class HireUnlockKillStore
{
public:
    static HireUnlockKillStore& Instance();

    // Replace this character's set (login load). Empty set = no kills.
    void SetKills(uint32_t guidLow, std::set<uint32_t> kills);
    // Forget the character (logout, delete).
    void Drop(uint32_t guidLow);
    // True when the encounter is already recorded for the character.
    bool HasKill(uint32_t guidLow, uint32_t encounterKey);
    // Record the encounter; false when already present (no double insert).
    bool AddKill(uint32_t guidLow, uint32_t encounterKey);
    // Copy of the recorded encounters (sync path).
    std::set<uint32_t> KillsFor(uint32_t guidLow);

private:
    HireUnlockKillStore() = default;

    std::mutex m_mutex;
    std::map<uint32_t, std::set<uint32_t>> m_kills;
};

} // namespace TortoiseBots
