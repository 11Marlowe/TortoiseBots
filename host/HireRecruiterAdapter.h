#pragma once

// pi-lens-ignore: clang:pp_file_not_found
#include "ScriptObjects.h"
class Player;
class Creature;
class Quest;

namespace TortoiseBots
{

// Module-owned CreatureScript adapter binding the <Mercenary Hire> recruiter
// NPCs (script_name tortoise_mercenary_hire) to the gossip wizard. Lives in
// the native module: zero core changes, core builds with bots off.
class HireRecruiterAdapter final : public CreatureScript
{
public:
    HireRecruiterAdapter();
    bool OnGossipHello(Player* player, Creature* creature) override;
    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override;
    uint32 GetDialogStatus(Player* player, Creature* creature) override;
    // Issue #490 PR 2 (kills): sync unlock counters on quest accept
    // (restores progress after abandon + retake, credits optional-map
    // bosses). Fires for every quest taken at these NPCs; non-unlock
    // quests are ignored.
    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override;
    // Hides the recruiters while hiring is off: the NPCs get no AI and are
    // removed from the world, so they never show, at any grid load or respawn.
    CreatureAI* GetAI(Creature* creature) const override;
};

} // namespace TortoiseBots
