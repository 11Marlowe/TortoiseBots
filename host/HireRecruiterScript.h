#pragma once

class Player;
class Creature;
class Quest;

namespace TortoiseBots
{

// Issue #192: <Mercenary Hire> recruiter gossip. Module-only CreatureScript
// bound by script_name in the world SQL migration (entries 95000+). Walks the
// player through class -> race -> gender -> spec/role -> confirm, then calls
// the shared HireProvisionService (same checks, same costs as `.bot hire`).
class HireRecruiterScript
{
public:
    static bool OnHello(Player* player, Creature* creature);
    static bool OnSelect(Player* player, Creature* creature, uint32_t sender, uint32_t action);
    // Issue #490 PR 2 (kills): quest-accept sync for the unlock quests.
    // Returning false lets the core finish the accept normally.
    static bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest);
    // Writes the configured world-buff unlock fees into the quest rows at
    // startup. Core has already loaded the quests, so a change is live from
    // the next restart.
    static void SyncUnlockFees();
    // Issue #490 PR 2: writes the unlock-chain PrevQuestId into the hire
    // unlock quests (90100-90106): the chain when the flag is off, 0 for all
    // when HireGearAllQuestsAvailable. Same pattern and log line as
    // SyncUnlockFees; called from the same place.
    static void SyncHireUnlockChain();
};

} // namespace TortoiseBots
