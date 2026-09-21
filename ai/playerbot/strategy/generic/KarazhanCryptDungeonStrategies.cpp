
#include "playerbot/playerbot.h"
#include "KarazhanCryptDungeonStrategies.h"

using namespace ai;

void KarazhanCryptDungeonStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Guard Captain Gort cleaves: flank discipline comes from the shared
    // dragon/dungeon triggers; crypt-specific stomp timers need live
    // measurement before hard-coding (#237).
    (void)triggers;
}

void KarazhanCryptDungeonStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Narrow tunnels clip wide follow formations through walls and grates.
    // Formation state is manual (.bot formation near, see crypt guide);
    // NextAction carries no event param so a blind re-queue cannot pick
    // "near". No auto-queue here by design.
    (void)triggers;
}
