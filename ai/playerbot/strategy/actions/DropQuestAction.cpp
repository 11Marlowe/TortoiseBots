
#include "playerbot/playerbot.h"
#include "DropQuestAction.h"

using namespace ai;

bool DropQuestAction::Execute(Event& event)
{
    Player* requester = event.GetOwner() ? event.GetOwner() : GetMaster();
    std::string link = event.GetParam();
    if (!GetMaster())
        return false;

    PlayerbotChatHandler handler(GetMaster());
    uint32 entry = handler.extractQuestId(link);
    std::vector<uint32> questIds;

    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = GetQuestSlotIdCompat(bot, slot);
        Quest const* quest = sObjectMgr.GetQuestTemplate(questId);
        if (!quest)
            continue;

        if (questId == entry || link.find(quest->GetTitle()) != std::string::npos || link == "all")
        {
            questIds.push_back(questId);
            if (link != "all")
                break;
        }
    }

    for (uint32 questId : questIds)
    {
        ai->DropQuest(questId);
        ai->TellPlayer(requester, BOT_TEXT("quest_remove"), PlayerbotSecurityLevel::PLAYERBOT_SECURITY_ALLOW_ALL, false);
    }

    return !questIds.empty();
}

bool CleanQuestLogAction::Execute(Event& event)
{
    Player* requester = event.GetOwner() ? event.GetOwner() : GetMaster();
    if (ai->HasActivePlayerMaster())
        return false;

    // The INCOMPLETE grey rule below is the old behaviour and always runs.
    // The COMPLETE branch is quest-log upkeep for masterless random bots.
    bool upkeep = sPlayerbotAIConfig.botQuestLogUpkeep;

    bool dropped = false;
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE;)
    {
        uint32 questId = GetQuestSlotIdCompat(bot, slot);
        QuestStatus status = questId ? bot->GetQuestStatus(questId) : QUEST_STATUS_NONE;
        if (!questId || (status != QUEST_STATUS_INCOMPLETE && status != QUEST_STATUS_COMPLETE))
        {
            ++slot;
            continue;
        }

        Quest const* quest = sObjectMgr.GetQuestTemplate(questId);
        // Class quests are never dropped.
        if (!quest || quest->GetRequiredClasses())
        {
            ++slot;
            continue;
        }

        bool drop = false;
        if (status == QUEST_STATUS_INCOMPLETE)
        {
            drop = bot->GetLevel() >= quest->GetQuestLevel() + 8 &&
                !quest->HasSpecialFlag(QUEST_SPECIAL_FLAG_DELIVER);
        }
        else if (upkeep && !bot->CanRewardQuest(quest, false))
        {
            // Finished but no longer rewardable (e.g. delivered items were
            // sold): no taker destination will ever be built for it, so it
            // would pin a log slot forever.
            drop = true;
        }

        if (!drop)
        {
            ++slot;
            continue;
        }

        ai->DropQuest(questId);
        ai->TellPlayer(requester, BOT_TEXT("quest_remove") + " " + chat->formatQuest(quest),
                       PlayerbotSecurityLevel::PLAYERBOT_SECURITY_ALLOW_ALL, false);
        dropped = true;
    }

    return dropped;
}
