
#include "playerbot/playerbot.h"
#include "SetHomeAction.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "playerbot/TravelMgr.h"

using namespace ai;

// Binding at an inn the bot has outgrown re-anchors its hearth to a low-level
// zone (same +10 rule as UnstuckAction's HearthLeadsSomewhereUseful). Refuse
// such binds while the flag is on; unknown inn levels fail closed (allow).
static bool InnLeadsSomewhereUseful(PlayerbotAI* ai, Player* bot, uint32 innAreaId)
{
    if (!sPlayerbotAIConfig.leaveOutgrownZones || ai->HasRealPlayerMaster())
        return true;

    int32 innLevel = 0;
    if (!sTravelMgr.TryGetValidatedAreaLevel(innAreaId, innLevel))
        return true;

    return innLevel + 10 >= (int32)bot->GetLevel();
}

bool SetHomeAction::Execute(Event& event)
{
    Player* requester = event.GetOwner() ? event.GetOwner() : GetMaster();
    ObjectGuid selection = bot->GetSelectionGuid();
    bool isRpgAction = AI_VALUE(GuidPosition, "rpg target") == selection;

    if (!isRpgAction)
    {
        if (requester)
        {
            selection = requester->GetSelectionGuid();
        }
        else
        {
            return false;
        }
    }

    if (selection)
    {
        Unit* unit = ai->GetUnit(selection);
        if (unit && unit->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_INNKEEPER))
        {
            Creature* creature = ai->GetCreature(selection);
            if (!creature)
                return false;
            if (!InnLeadsSomewhereUseful(ai, bot, sServerFacade.GetAreaId(creature)))
            {
                ai->TellPlayer(requester, "This inn is in a zone I have outgrown; keeping my current home");
                return false;
            }

            bot->GetSession()->SendBindPoint(creature);
            ai->TellPlayer(requester, "This inn is my new home", PlayerbotSecurityLevel::PLAYERBOT_SECURITY_ALLOW_ALL, false);
            RESET_AI_VALUE(WorldPosition, "home bind");
            return true;
        }
    }

    std::list<ObjectGuid> npcs = AI_VALUE(std::list<ObjectGuid>, "nearest npcs");
    for (std::list<ObjectGuid>::iterator i = npcs.begin(); i != npcs.end(); i++)
    {
        Creature *unit = bot->GetNPCIfCanInteractWith(*i, UNIT_NPC_FLAG_INNKEEPER);
        if (!unit)
            continue;

        if (!InnLeadsSomewhereUseful(ai, bot, sServerFacade.GetAreaId(unit)))
            continue;

        bot->GetSession()->SendBindPoint(unit);
        ai->TellPlayer(requester, "This inn is my new home", PlayerbotSecurityLevel::PLAYERBOT_SECURITY_ALLOW_ALL, false);
        RESET_AI_VALUE(WorldPosition, "home bind");
        return true;
    }

    ai->TellPlayer(requester, "Can't find any innkeeper around");
    return false;
}
