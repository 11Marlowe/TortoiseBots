#include "playerbot/playerbot.h"
#include "UnstuckAction.h"
#include "playerbot/TravelMgr.h"

using namespace ai;

// A random bot's homebind is usually still its racial starting inn: hearthing a level 40
// bot "unstuck" parked it in Elwynn for good (2042 hearths in 4.7 h on a live realm).
// Hearth only while home is in an area the bot has not outgrown; otherwise repop, which
// brings it to the nearest graveyard.
static bool HearthLeadsSomewhereUseful(PlayerbotAI* ai, Player* bot)
{
    if (!sPlayerbotAIConfig.unstuckHearthLevelFit || ai->HasRealPlayerMaster())
        return true;

    int32 homeLevel = 0;
    if (!sTravelMgr.TryGetValidatedAreaLevel(bot->GetHomeBindAreaId(), homeLevel))
        return false;

    return homeLevel + 10 >= (int32)bot->GetLevel();
}

bool UnstuckAction::Execute(Event& event)
{
    std::string source = event.GetSource();
    Player* bot = ai->GetBot();
    Player* master = ai->GetMaster();

    // Default action if no specific source is matched
    if (source.empty())
    {
        ai->TellDebug(master, "Unstuck: No specific source, resetting.", "debug unstuck");
        return ai->DoSpecificAction("reset", event, true);
    }

    // Handle move stuck scenarios
    if (source.find("move stuck") != std::string::npos)
    {
        ai->TellDebug(master, "Unstuck: Move stuck detected, resetting.", "debug unstuck");
        return ai->DoSpecificAction("reset", event, true);
    }

    // Handle long move stuck scenarios
    if (source.find("move long stuck") != std::string::npos)
    {
        ai->TellDebug(master, "Unstuck: Long move stuck detected, attempting hearthstone or repop.", "debug unstuck");
        if (AI_VALUE2(bool, "action useful", "hearthstone") && bot->IsAlive() && HearthLeadsSomewhereUseful(ai, bot))
        {
            return ai->DoSpecificAction("hearthstone", event, true);
        }
        else
        {
            return ai->DoSpecificAction("repop", event, true);
        }
    }

    // Handle combat stuck scenarios
    if (source.find("combat stuck") != std::string::npos)
    {
        ai->TellDebug(master, "Unstuck: Combat stuck detected, resetting position.", "debug unstuck");
        return ai->DoSpecificAction("reset", event, true);
    }

    // Handle long combat stuck scenarios
    if (source.find("combat long stuck") != std::string::npos)
    {
        ai->TellDebug(master, "Unstuck: Long combat stuck detected, attempting hearthstone or repop.", "debug unstuck");
        if (AI_VALUE2(bool, "action useful", "hearthstone") && bot->IsAlive() && HearthLeadsSomewhereUseful(ai, bot))
        {
            return ai->DoSpecificAction("hearthstone", event, true);
        }
        else
        {
            return ai->DoSpecificAction("repop", event, true);
        }
    }

    // Fallback to reset if no specific condition is met
    ai->TellDebug(master, "Unstuck: Fallback to reset action.", "debug unstuck");
    return ai->DoSpecificAction("reset", event, true);
}
