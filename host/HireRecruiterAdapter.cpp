// pi-lens-ignore-file: clang:pp_file_not_found,clang:unknown_typename,clang:use_of_undeclared_identifier,clang:unknown_type_name,clang:undeclared_var_use,clang:incomplete_member_access
#include "HireRecruiterAdapter.h"
#include "HireRecruiterScript.h"
#include "../ai/playerbot/PlayerbotAIConfig.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "QuestDef.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "Creature.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "AI/CreatureAI.h"
class Player;

namespace TortoiseBots
{

namespace
{
// Null AI the hidden recruiters run on: the same no-op core vendors idle on.
class HiddenRecruiterAI : public CreatureAI
{
public:
    explicit HiddenRecruiterAI(Creature* creature) : CreatureAI(creature) {}
    void UpdateAI(uint32 const /*diff*/) override {}
};

} // namespace

HireRecruiterAdapter::HireRecruiterAdapter() : CreatureScript("tortoise_mercenary_hire")
{
}

bool HireRecruiterAdapter::OnGossipHello(Player* player, Creature* creature)
{
    return HireRecruiterScript::OnHello(player, creature);
}

bool HireRecruiterAdapter::OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    return HireRecruiterScript::OnSelect(player, creature, sender, action);
}

bool HireRecruiterAdapter::OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
{
    return HireRecruiterScript::OnQuestAccept(player, creature, quest);
}

// The base CreatureScript answers 0 (no icon), which hides the quest marks of
// the capital recruiters' world-buff quests (issue #492). UNDEFINED hands the
// "!"/"?" decision back to the core's normal quest-giver status.
uint32 HireRecruiterAdapter::GetDialogStatus(Player* /*player*/, Creature* /*creature*/)
{
    return DIALOG_STATUS_UNDEFINED;
}

CreatureAI* HireRecruiterAdapter::GetAI(Creature* creature) const
{
    if (!creature || sPlayerbotAIConfig.hireEnabled)
        return nullptr;
    creature->SetVisibility(VISIBILITY_OFF);
    creature->RemoveFromWorld();
    creature->SetRespawnTime(0);
    return new HiddenRecruiterAI(creature);
}

} // namespace TortoiseBots
