"""Compile the instance guard and candidate/dispatch prefixes from production.

World stubs isolate selection. This does not simulate pathfinding or the engine.
Pass a checkout as argv[1] to reproduce the assertions against unpatched code.
"""
import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(sys.argv.pop(1)).resolve() if __name__ == '__main__' and len(sys.argv) > 1 and not sys.argv[1].startswith('-') else pathlib.Path(__file__).resolve().parents[1]


def function(src, sig):
    start = src.index(sig)
    opening = src.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (src[end] == '{') - (src[end] == '}')
        end += 1
    return src[start:end]


class InstanceGrindTargetTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which('g++'), 'g++ unavailable')
    def test_selection_and_cached_dispatch(self):
        values = ROOT / 'ai/playerbot/strategy/values'
        actions = ROOT / 'ai/playerbot/strategy/actions'
        grind = (values / 'GrindTargetValue.cpp').read_text()
        choose = (actions / 'ChooseTargetActions.cpp').read_text()
        guard = (function(grind, 'bool GrindTargetValue::IsAllowedInstanceTarget(')
                 if 'bool GrindTargetValue::IsAllowedInstanceTarget(' in grind else
                 'bool GrindTargetValue::IsAllowedInstanceTarget(PlayerbotAI*,Unit*) { return true; }')
        first = grind[grind.index('    std::list<ObjectGuid> attackers ='):grind.index('    std::list<ObjectGuid> targets =')]
        fresh_start = grind.index('    for (std::list<ObjectGuid>::iterator tIter = targets.begin();')
        fresh = grind[fresh_start:grind.index('        if (abs(bot->getPositionZ()', fresh_start)]
        idle_start = grind.index('    for (Unit* unit : units)', grind.index('Unit* GrindTargetValue::FindIdleFallbackTarget'))
        idle = grind[idle_start:grind.index('        Creature* creature =', idle_start)]
        execute = function(choose, 'bool ai::AttackAnythingAction::Execute(')
        execute = execute[:execute.index('    if (result)')] + '    return result;\n}'
        fixture = r'''
#include <cassert>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <type_traits>
#include <vector>
using ObjectGuid = unsigned;
struct Unit;
struct Threat {
    std::map<Unit*,float> threat;
    float getThreat(Unit* u) { return threat[u]; }
};
struct Map { bool dungeon=true; bool IsDungeon() { return dungeon; } };
struct Unit {
    virtual ~Unit()=default;
    unsigned guid=2, entry=11262, instance=1;
    bool alive=true, combat=false, world=true;
    Unit* victim=nullptr;
    Threat threat;
    bool IsAlive() { return alive; }
    bool IsInCombat() { return combat; }
    bool IsInMap(Unit* u) { return instance==u->instance; }
    ObjectGuid GetObjectGuid() { return guid; }
    Unit* GetVictim() { return victim; }
    Threat& GetThreatManager() { return threat; }
    bool IsCreature() { return entry!=0; }
};
struct Creature: Unit {
    bool evade=false, unreachable=false;
    bool IsInEvadeMode() { return evade; }
    bool IsEvadeBecauseTargetNotReachable() { return unreachable; }
};
struct Player;
struct Group {
    std::vector<Player*> members;
    ObjectGuid icon=0;
    ObjectGuid GetTargetIcon(int) { return icon; }
};
std::vector<Player*> LiveGroupMembers(Group* g) { return g->members; }
struct Player: Unit {
    Group* group=nullptr;
    Unit* pet=nullptr;
    Map map;
    bool IsInWorld() { return world; }
    bool InBattleGround() { return false; }
    Group* GetGroup() { return group; }
    Map* GetMap() { return &map; }
    Unit* GetPet() { return pet; }
};
template<class T> struct Value { T value; T Get() { return value; } };
struct AiObjectContext {
    Value<std::list<ObjectGuid>> candidates;
    ObjectGuid explicitTarget=0;
    std::string rti;
    template<class T> Value<T>* GetValue(std::string) { return &candidates; }
    template<class T> T ValueOf(std::string) {
        if constexpr(std::is_same_v<T,ObjectGuid>) return explicitTarget;
        else return rti;
    }
};
#define AI_VALUE(type,name) context->ValueOf<type>(name)
struct PlayerbotAI {
    Player* bot;
    Player* master;
    bool human=true;
    AiObjectContext context;
    std::map<ObjectGuid,Unit*> units;
    Player* GetBot() { return bot; }
    Player* GetMaster() { return master; }
    bool HasRealPlayerMaster() { return human; }
    AiObjectContext* GetAiObjectContext() { return &context; }
    Unit* GetUnit(ObjectGuid g) { return units[g]; }
};
struct RtiTargetValue { static int GetRtiIndex(std::string s) { return s=="skull"?7:-1; } };
struct { bool IsAlive(Unit* u) { return u->IsAlive(); } } sServerFacade;
struct GuidPosition { GuidPosition(Unit*) {} };
struct CanFreeMoveValue { static bool CanFreeTarget(PlayerbotAI*,GuidPosition) { return true; } };
struct Event {};
namespace ai {
struct GrindTargetValue { static bool IsAllowedInstanceTarget(PlayerbotAI*,Unit*); };
struct AttackAction {
    int attacks=0;
    bool Execute(Event&) { ++attacks; return true; }
};
struct AttackAnythingAction: AttackAction {
    PlayerbotAI* ai;
    Unit* target;
    Unit* GetTarget() { return target; }
    bool Execute(Event&);
};
}
using namespace ai;
'''
        fixture += guard + '\n' + execute
        pre = 'auto IsAllowedInstanceTarget=GrindTargetValue::IsAllowedInstanceTarget;auto* bot=ai->GetBot();auto* context=ai->GetAiObjectContext();auto logGrind=[](Unit*,std::string){};\n'
        fixture += '\nUnit* First(PlayerbotAI* ai) {' + pre + first + 'return nullptr;}\n'
        fixture += 'Unit* Fresh(PlayerbotAI* ai) {' + pre + 'auto targets=context->candidates.Get();\n' + fresh + 'return unit;}return nullptr;}\n'
        fixture += 'Unit* Idle(PlayerbotAI* ai) {' + pre + 'std::list<Unit*> units;for(auto g:context->candidates.Get())units.push_back(ai->GetUnit(g));\n' + idle + 'return unit;}return nullptr;}\n'
        fixture += r'''
int main() {
    Player bot, master, healer, outsider;
    Creature cave, pet;
    Group group;group.members={&bot,&master,&healer};
    bot.group=master.group=healer.group=&group;
    PlayerbotAI ai{&bot,&master};ai.units[2]=&cave;ai.context.candidates.value={2};
    auto expect=[&](bool allowed) {
        for(auto selector:{Fresh,First,Idle}) assert((selector(&ai)==&cave)==allowed);
        Event event; AttackAnythingAction action;action.ai=&ai;action.target=&cave;
        assert(action.Execute(event)==allowed);
        assert(action.attacks==(allowed?1:0));
    };
    // Phase-one cave candidate, including an unengaged cached attacker.
    expect(false);
    cave.combat=true; expect(false);
    cave.threat.threat[&master]=0; expect(false);
    cave.victim=&outsider; expect(false);
    cave.victim=&healer; expect(true);
    cave.victim=nullptr;cave.threat.threat[&master]=1;expect(true);
    cave.threat.threat.clear();healer.pet=&pet;cave.victim=&pet;expect(true);
    cave.victim=nullptr;bot.victim=&cave;expect(true);bot.victim=nullptr;
    cave.victim=&healer;healer.instance=2;expect(false);healer.instance=1;
    cave.combat=false;cave.victim=nullptr;
    ai.context.explicitTarget=2;expect(true);ai.context.explicitTarget=0;
    ai.context.rti="skull";group.icon=2;expect(true);group.icon=3;expect(false);
    ai.context.rti.clear();group.icon=0;
    bot.map.dungeon=false;expect(true);bot.map.dungeon=true;
    ai.human=false;expect(true);ai.human=true;
    bot.group=nullptr;expect(true);bot.group=&group;
    master.group=nullptr;expect(true);master.group=&group;
    std::cout<<"PASS: three candidate paths and pre-attack dispatch; fresh/foreign rejected, group/pet defense and orders allowed; world/solo unchanged\n";
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            cpp=pathlib.Path(tmp)/'fixture.cpp';exe=pathlib.Path(tmp)/'fixture'
            cpp.write_text(fixture)
            subprocess.run(['g++','-std=c++17','-O0',str(cpp),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)


if __name__ == '__main__':
    unittest.main()
