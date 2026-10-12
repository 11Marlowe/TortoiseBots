// pi-lens-ignore: clang:pp_file_not_found
#include "ScriptObjects.h"

class Player;
class Creature;

namespace TortoiseBots
{

// Issue #490 PR 2 (kills): boss-kill credit for the hire-unlock quests
// (90100-90106). Core already fans Player::KilledMonster out to every group
// member in reward distance, so OnCreatureKill credits only its own killer:
// no group walk here (dedup is per player). The Horsemen encounter credits
// once, when the last of the four dies. Quest-accept sync lives on the
// recruiter adapter (the CreatureScript these NPCs already route to); login
// sync lives here. Module-only: zero core changes.
class HireUnlockKillAdapter final : public PlayerScript
{
public:
    HireUnlockKillAdapter();

    void OnCreatureKill(Player* killer, Creature* killed) override;
    void OnLogin(Player* player) override;
    void OnLogout(Player* player) override;
    void OnDelete(ObjectGuid guid, uint32 accountId) override;

    // Raise every in-progress unlock quest's counters to (recorded
    // encounters of that counter + optional-map bosses), capped at the
    // counter size. Never lowers. Public so the recruiter's contracts page
    // refreshes before it renders.
    static void SyncCounters(Player* player);
};

} // namespace TortoiseBots
