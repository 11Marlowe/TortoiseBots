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
// Every recruiter runs this AI. Startup creatures are added before the bot
// config is read, so GetAI cannot trust the hire flag. The first update, which
// runs after startup, checks it once: with hiring off the recruiter is removed
// from the map for this session. A restart with hiring on brings them back.

class RecruiterAI : public CreatureAI
{
public:
    explicit RecruiterAI(Creature* creature) : CreatureAI(creature) {}
    void UpdateAI(uint32 const /*diff*/) override
    {
        if (m_checked)
            return;
        m_checked = true;
        if (sPlayerbotAIConfig.hireEnabled)
            return;
        // Removed from the map, not killed: a game master would still see a
        // respawned invisible creature, and a death would save a respawn time.
        m_creature->AddObjectToRemoveList();
    }

private:
    bool m_checked = false;
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
    if (!creature)
        return nullptr;
    return new RecruiterAI(creature);
}

} // namespace TortoiseBots
