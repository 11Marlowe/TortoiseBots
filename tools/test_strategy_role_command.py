"""Execute the actual strategy command handler with command-context/AI doubles."""
import os
import pathlib
import shutil
import subprocess
import tempfile
import unittest


def function(source, signature):
    start = source.index(signature)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


class StrategyRoleCommand(unittest.TestCase):
    @unittest.skipUnless(shutil.which('g++'), 'g++ unavailable')
    def test_handler(self):
        root = pathlib.Path(os.environ.get('TBOTS_TEST_SOURCE', pathlib.Path(__file__).resolve().parents[1]))
        source = (root/'commands/Party.cpp').read_text()
        code = r'''
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using uint32=unsigned;
namespace BotState{constexpr int BOT_STATE_ALL=4;}
struct PlayerbotAI;
struct Player {std::string name;bool tank=false,heal=false,ranged=false,grouped=true,owned=true;PlayerbotAI* ai=nullptr;void* GetGroup(){return grouped?this:nullptr;}};
struct Context {
 std::set<std::string> supported{"loot","passive","aoe fear","dps assist","tank assist","ranged"};
 void GetSupportedStrategies(std::set<std::string>& out){out.insert(supported.begin(),supported.end());}
};
struct PlayerbotAI {
 Context context;std::vector<std::string> changes;int saves=0;
 Context* GetAiObjectContext(){return &context;}
 // The fixture checks grouped role semantics, not a class-name approximation.
 bool IsTank(Player* b,bool group){return group && b->tank;}
 bool IsHeal(Player* b,bool group){return group && b->heal;}
 bool IsRanged(Player* b,bool){return b->ranged;}
 void ChangeStrategy(std::string s,int state){if(state!=BotState::BOT_STATE_ALL)std::abort();changes.push_back(s);}
};
struct PlayerbotAIStorage {static PlayerbotAIStorage& Instance(){static PlayerbotAIStorage s;return s;}PlayerbotAI* GetAI(Player* p){return p?p->ai:nullptr;}};
struct Store{void Save(PlayerbotAI* ai){++ai->saves;}} sPlayerbotDbStore;
struct ChatHandler {std::string reply;void PSendSysMessage(const char* fmt,...){char b[1024];va_list args;va_start(args,fmt);vsnprintf(b,sizeof(b),fmt,args);va_end(args);reply=b;}};
struct BotRecord{};
struct BotCommandContext{Player* selectedBot=nullptr;std::vector<Player*> partyBots;};
Player owner{"Owner"};Player* requester=&owner;BotCommandContext current;std::map<std::string,Player*> online;
Player* Requester(ChatHandler*){return requester;}
BotCommandContext BuildContext(Player*){return current;}
std::vector<Player*> ResolveDynamicScope(BotCommandContext const& c){return c.selectedBot?std::vector<Player*>{c.selectedBot}:c.partyBots;}
bool ResolveOwnedBot(ChatHandler*,char const* name,Player*& bot,BotRecord*&,std::string& normalized){normalized=name;auto i=online.find(name);bot=i==online.end()?nullptr:i->second;return bot && bot->owned && bot->ai;}
std::string Trim(std::string const& s){auto a=s.find_first_not_of(" \t");return a==s.npos?"":s.substr(a,s.find_last_not_of(" \t")-a+1);}
'''
        for sig in ['static bool StrategyRoleMatches(', 'static bool KnownStrategyChange(']:
            if sig in source:
                code += function(source,sig)
        code += function(source,'bool HandleStrategy(')
        code += r'''
int main(){
 Player tank{"Tank",true,false,false},dps{"Dps"},mage{"Mage",false,false,true},heal{"Heal",false,true,true},stranger{"Stranger",false,false,false,true,false};
 PlayerbotAI a,b,c,d,e;tank.ai=&a;dps.ai=&b;mage.ai=&c;heal.ai=&d;stranger.ai=&e;
 online={{"Tank",&tank},{"Dps",&dps},{"Mage",&mage},{"Heal",&heal},{"Stranger",&stranger},{"assist",&tank}};
 current.partyBots={&tank,&dps,&mage,&heal};ChatHandler h;int failures=0;
 auto clear=[&](){for(auto ai:{&a,&b,&c,&d,&e}){ai->changes.clear();ai->saves=0;}h.reply.clear();};
 auto check=[&](bool ok,const char* name){if(!ok){++failures;std::cerr<<"FAIL "<<name<<"\n";}};
 auto changed=[&](PlayerbotAI& ai,std::string const& change){return ai.changes==std::vector<std::string>{change} && ai.saves==1;};
 HandleStrategy(&h,"+aoe fear");check(changed(a,"+aoe fear")&&changed(b,"+aoe fear")&&changed(c,"+aoe fear")&&changed(d,"+aoe fear"),"multiword strategy");clear();
 current.selectedBot=&tank;HandleStrategy(&h,"+dps assist @dps");check(a.saves==0&&d.saves==0&&changed(b,"+dps assist")&&changed(c,"+dps assist"),"role scope overrides selected bot and excludes tanks/healers");clear();
 HandleStrategy(&h,"+tank assist @tank");check(changed(a,"+tank assist")&&b.saves==0&&c.saves==0&&d.saves==0,"tank scope");clear();
 HandleStrategy(&h,"-aoe fear @heal");check(changed(d,"-aoe fear")&&a.saves==0&&b.saves==0&&c.saves==0,"heal removal");clear();
 HandleStrategy(&h,"~aoe fear @melee");check(changed(a,"~aoe fear")&&changed(b,"~aoe fear")&&c.saves==0&&d.saves==0,"melee includes melee tanks");clear();
 HandleStrategy(&h,"+aoe fear @ranged");check(changed(c,"+aoe fear")&&changed(d,"+aoe fear")&&a.saves==0&&b.saves==0,"ranged includes ranged healers");clear();
 a.context.supported.insert("bear");
 current.partyBots={&tank,&dps};HandleStrategy(&h,"+bear @dps");check(b.saves==0&&h.reply.find("Unknown strategy change")!=h.reply.npos,"role scope validates against role-matching bots only");clear();
 current.partyBots={&tank,&dps,&mage,&heal};
 HandleStrategy(&h,"+loot");check(changed(a,"+loot")&&b.saves==0,"legacy selected bot");clear();
 HandleStrategy(&h,"-passive Mage");check(changed(c,"-passive")&&a.saves==0,"legacy named bot");clear();
 HandleStrategy(&h,"  +aoe fear   Mage \t");check(changed(c,"+aoe fear")&&a.saves==0,"multiword named bot and whitespace");clear();
 current.selectedBot=nullptr;HandleStrategy(&h,"+dps assist");check(a.saves==1&&b.saves==1&&c.saves==1&&d.saves==1,"complete strategy wins over bot named assist");clear();
 HandleStrategy(&h,"+loot,-passive");check(changed(a,"+loot,-passive")&&changed(b,"+loot,-passive"),"legacy comma changes");clear();
 HandleStrategy(&h,"+loot,-passive Mage");check(changed(c,"+loot,-passive")&&a.saves==0,"legacy named comma changes");clear();
 HandleStrategy(&h,"+aoe fear Stranger");check(e.saves==0&&a.saves==0&&b.saves==0&&c.saves==0&&d.saves==0,"foreign bot cannot expand scope");clear();
 for(const char* invalid:{"","+","aoe fear","+aoe fear @dsp","+aoe fear @dps @tank","+aoe fear Missing"}){
  HandleStrategy(&h,invalid);check(a.saves+b.saves+c.saves+d.saves+e.saves==0,"invalid or unknown suffix has no changes");clear();
 }
 current.partyBots={&dps,&mage};HandleStrategy(&h,"+tank assist @tank");check(a.saves+b.saves+c.saves+d.saves==0&&h.reply.find("No party bot matches role")!=h.reply.npos,"no role match says so");clear();
 requester=nullptr;HandleStrategy(&h,"+loot");check(a.saves+b.saves+c.saves+d.saves==0,"console denied");
 std::cout<<"Strategy command production handler: "<<failures<<" failures\n";return failures?1:0;
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            cpp=pathlib.Path(tmp)/'fixture.cpp';exe=pathlib.Path(tmp)/'fixture'
            cpp.write_text(code)
            subprocess.run(['g++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)


if __name__ == '__main__':
    unittest.main()
