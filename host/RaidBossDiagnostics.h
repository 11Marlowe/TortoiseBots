#pragma once
#include "ScriptObjects.h"
#include "../runtime/policy/infra/RaidBossLogPolicy.h"
#include <map>
#include <mutex>
#include <tuple>

class WorldSession;
class WorldPacket;
namespace TortoiseBots {
class RaidBossDiagnostics final : public UnitScript
{
public:
    RaidBossDiagnostics() : UnitScript("tortoisebots_raid_boss_diagnostics",
        { UNITHOOK_ON_UNIT_UPDATE, UNITHOOK_ON_UNIT_EXIT_COMBAT, UNITHOOK_ON_UNIT_DEATH }) {}
    void OnUnitUpdate(Unit* unit, uint32 diff) override;
    void OnUnitExitCombat(Unit* unit) override;
    void OnUnitDeath(Unit* unit, Unit*) override { OnUnitExitCombat(unit); }
    static void ObservePacket(WorldSession* session, WorldPacket const& packet);
private:
    using Key = std::tuple<uint32, uint32, uint64>;
    struct Clock { RaidBossLog::Throttle throttle; uint32 seen = 0; };
    std::map<Key, Clock> clocks;
    std::mutex mutex; // Different raid maps can update on different workers.
    static Key Identity(Unit* unit);
};
}
