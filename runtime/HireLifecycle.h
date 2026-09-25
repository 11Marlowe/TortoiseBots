#pragma once

// pi-lens-ignore: clang:pp_file_not_found
#include "ObjectGuid.h"

#include <cstdint>
#include <ctime>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Player;
class Group;

namespace TortoiseBots
{

// Issue #192: hired-companion lifecycle. Tracks which Headless bots are hired
// companions (vs roaming pool), runs the master-disconnect grace timer, and
// dismisses companions when their group goes away. Driven from BotManager's
// world tick and the module GroupScript adapter; never blocks the tick.
class HireLifecycle
{
public:
    static HireLifecycle& Instance();

    // Mark a freshly hired bot. Called once per Hire().
    void Claim(ObjectGuid botGuid, ObjectGuid masterGuid, uint32_t ownerAccountId);
    void Release(ObjectGuid botGuid);

    bool IsHired(ObjectGuid botGuid) const;
    ObjectGuid GetMaster(ObjectGuid botGuid) const;

    // Group hooks (called from the module GroupScript adapter).
    void OnGroupMemberRemoved(Group* group, ObjectGuid guid);
    void OnGroupDisband(Group* group);

    // Human master login/logout (called from the module PlayerScript adapter).
    void OnMasterLogin(Player* master);
    void OnMasterLogout(Player* master);

    void Update(uint32_t diff);

private:
    HireLifecycle() = default;

    struct HiredRecord
    {
        ObjectGuid botGuid;
        ObjectGuid masterGuid;
        uint32_t ownerAccountId = 0;
        // Master offline: bot guards in place until grace expires, then
        // dismisses. Zero = master online (or never seen offline).
        time_t masterOfflineSince = 0;
        bool greeted = false;
    };

    void Dismiss(HiredRecord const& record, char const* reason, bool removeFromGroup = true);
    bool MasterOnline(HiredRecord const& record) const;
    void Reunite(HiredRecord& record, Player* master);
    void SweepGracePeriod(time_t now);

    std::unordered_map<uint32_t, HiredRecord> m_hired;
    std::unordered_set<uint32_t> m_dismissing;
    uint32_t m_updateElapsedMs = 0;
    uint32_t m_restockElapsedMs = 0;
};

} // namespace TortoiseBots
