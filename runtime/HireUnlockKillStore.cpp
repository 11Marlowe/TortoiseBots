#include "HireUnlockKillStore.h"

namespace TortoiseBots
{

HireUnlockKillStore& HireUnlockKillStore::Instance()
{
    static HireUnlockKillStore instance;
    return instance;
}

void HireUnlockKillStore::SetKills(uint32_t guidLow, std::set<uint32_t> kills)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_kills[guidLow] = std::move(kills);
}

void HireUnlockKillStore::Drop(uint32_t guidLow)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_kills.erase(guidLow);
}

bool HireUnlockKillStore::HasKill(uint32_t guidLow, uint32_t encounterKey)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    auto it = m_kills.find(guidLow);
    return it != m_kills.end() && it->second.count(encounterKey) != 0;
}

bool HireUnlockKillStore::AddKill(uint32_t guidLow, uint32_t encounterKey)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_kills[guidLow].insert(encounterKey).second;
}

std::set<uint32_t> HireUnlockKillStore::KillsFor(uint32_t guidLow)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    auto it = m_kills.find(guidLow);
    return it != m_kills.end() ? it->second : std::set<uint32_t>();
}

} // namespace TortoiseBots
