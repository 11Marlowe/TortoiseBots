// pi-lens-ignore: clang:pp_file_not_found
#include "ScriptObjects.h"

class Player;
class Creature;

namespace TortoiseBots
{

// Issue #490 PR 2 (kills) + PR 3 (credit): boss-kill credit for the
// hire-unlock quests (90100-90106). OnCreatureKill fans out to every
// non-bot group member in reward distance (dedup is per player): a bot or
// pet landing the blow must not starve the party's real players, since the
// members' own hooks may never fire. Bots never count. The Horsemen
// encounter credits once, when the last of the four dies; Thekal's real
// death (entry 14509, P1 is a fake) aliases to 14599. Quest-accept sync
// lives on the recruiter adapter (the CreatureScript these NPCs already
// route to); login sync lives here. Module-only: zero core changes.
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
