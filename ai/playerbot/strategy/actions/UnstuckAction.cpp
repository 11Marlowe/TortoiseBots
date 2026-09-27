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

        // The reset below nulls the travel target (PlayerbotAI::Reset(true)).
        // A sticky need - e.g. an unvisited class trainer - then re-requests the
        // same destination within seconds while the bot never moves: live bots
        // re-picked 'trainer class' every ~5 s (this trigger's poll interval)
        // instead of walking there. mod-playerbots' stuck reset never touches
        // travel, so keep an active target across the reset; the move-retry
        // cooldown still retires it if the spot is truly unreachable.
        TravelTarget* travelTarget = AI_VALUE(TravelTarget*, "travel target");
        bool const keepTravel = travelTarget && travelTarget->IsActive() &&
            travelTarget->GetDestination() && travelTarget->getPosition();
        TravelDestination* dest = keepTravel ? travelTarget->GetDestination() : nullptr;
        WorldPosition* pos = keepTravel ? travelTarget->getPosition() : nullptr;
        TravelStatus status = keepTravel ? travelTarget->GetStatus() : TravelStatus::TRAVEL_STATUS_NONE;
        std::vector<std::string> conditions = keepTravel ? travelTarget->GetConditions() : std::vector<std::string>();
        bool const forced = keepTravel && travelTarget->IsForced();
        uint32 const moveRetry = keepTravel ? travelTarget->GetRetryCount(true) : 0;
        uint32 const extendRetry = keepTravel ? travelTarget->GetRetryCount(false) : 0;
        int32 const timeLeft = keepTravel ? travelTarget->GetTimeLeft() : 0;
        GuidPosition groupCopy = keepTravel ? travelTarget->GetGroupmember() : GuidPosition();

        bool const reset = ai->DoSpecificAction("reset", event, true);

        if (keepTravel)
        {
            travelTarget->SetTarget(dest, pos);
            travelTarget->SetStatus(status);
            travelTarget->SetConditions(conditions);
            travelTarget->SetForced(forced);
            travelTarget->SetRetry(true, moveRetry);
            travelTarget->SetRetry(false, extendRetry);
            if (groupCopy)
                travelTarget->SetGroupCopy(groupCopy);
            travelTarget->SetExpireIn(timeLeft > 0 ? (uint32)timeLeft : 1000);
        }

        return reset;
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
