
#include "playerbot/playerbot.h"
#include "GuildTriggers.h"
#include "Guild/GuildMgr.h"
#include "../../../runtime/AltBotService.h"

using namespace ai;

bool LeaveLargeGuildTrigger::IsActive()
{
	if (!bot->GetGuildId())
		return false;

	if (ai->HasActivePlayerMaster())
		return false;

	if (ai->IsAlt())
		return false;

	// Issue #872 (F40): an autonomous alt stays in its guild; it never
	// leaves on its own. (It also never auto-joins: GuildAcceptAction and
	// PetitionSignAction only take the owner's invite.)
	if (TortoiseBots::AltBotService::Instance().IsAutonomous(bot->GetObjectGuid()))
		return false;

	Guild* guild = sGuildMgr.GetGuildById(bot->GetGuildId());

	if (guild->GetMemberSize() >= 1000) //Try to prevent guild overflow.
		return true;

	if (!sPlayerbotAIConfig.randomBotGuildNearby)
		return false;

	if (ai->IsInRealGuild())
		return false;

	Player* leader = sObjectMgr.GetPlayer(guild->GetLeaderGuid());

	//Only leave the guild if we know the leader is not a real player.
	if (!leader || !PlayerbotAIStorage::Instance().GetAI(leader) || PlayerbotAIStorage::Instance().GetAI(leader)->IsRealPlayer())
		return false;

	uint32 members = guild->GetMemberSize();
	uint32 maxMembers = ai->GetMaxPreferedGuildSize();

	return members > maxMembers;
}
