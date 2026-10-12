// pi-lens-ignore-file: clang:pp_file_not_found,clang:unknown_typename,clang:use_of_undeclared_identifier,clang:unknown_type_name,clang:undeclared_var_use,clang:incomplete_member_access
#include "HireUnlockKillAdapter.h"
#include "../runtime/HireUnlockPolicy.h"
#include "../runtime/HireUnlockSync.h"
#include "../runtime/HireUnlockKillStore.h"
#include "../runtime/BotManager.h"
#include "../ai/playerbot/playerbot.h"

#include "ScriptObjects.h"
#include "Player.h"
#include "Creature.h"
#include "QuestDef.h"
#include "ObjectMgr.h"
#include "Maps/Cell.h"
#include "Maps/CellImpl.h"
#include "Maps/GridNotifiers.h"
#include "Maps/GridNotifiersImpl.h"
#include "Database/DatabaseEnv.h"

namespace TortoiseBots
{

HireUnlockKillAdapter::HireUnlockKillAdapter()
    : PlayerScript("tortoisebots_hire_unlock_kills",
        { PLAYERHOOK_ON_CREATURE_KILL, PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LOGOUT, PLAYERHOOK_ON_DELETE })
{
}

// The last Horseman standing leaves three corpses of its fellows on the
// same map instance (JUST_DIED, not yet respawned/despawned). Any other
// Horseman death still has a living member, so no credit yet.
static bool IsLastHorsemanStanding(Creature* killed)
{
    if (!killed || !killed->GetMap())
        return false;
    std::vector<uint32_t> members(
        kHireUnlockFourHorsemen, kHireUnlockFourHorsemen + 4);
    std::list<Creature*> found;
    MaNGOS::AllCreaturesMatchingOneEntryInRange check(killed, members, 500.0f);
    MaNGOS::CreatureListSearcher<MaNGOS::AllCreaturesMatchingOneEntryInRange> searcher(found, check);
    Cell::VisitAllObjects(killed, searcher, 500.0f);
    for (Creature* member : found)
    {
        if (!member || member == killed)
            continue;
        if (member->GetEntry() == killed->GetEntry() && member->GetObjectGuid() == killed->GetObjectGuid())
            continue;
        if (member->IsAlive())
            return false;
    }
    return true;
}

void HireUnlockKillAdapter::OnCreatureKill(Player* killer, Creature* killed)
{
    if (!killer || !killed)
        return;
    if (!sPlayerbotAIConfig.hireGearTiersEnabled)
        return;
    HireUnlockBoss const* boss = nullptr;
    HireUnlockCounter const* counter = HireUnlockCounterForKill(killed->GetEntry(), &boss);
    if (!counter || !boss)
        return;
    uint32_t encounter = HireUnlockEncounterKey(killed->GetEntry());
    if (IsHireUnlockGroupedEncounter(encounter) && !IsLastHorsemanStanding(killed))
        return;
    // The counter knows its tier quest (slot = ReqCreatureOrGOId index).
    uint32_t questId = HireUnlockQuestForTier(counter->tier);
    if (questId == 0)
        return;
    if (killer->GetQuestStatus(questId) != QUEST_STATUS_INCOMPLETE)
        return;
    if (killer->GetQuestRewardStatus(questId))
        return;
    if (!HireUnlockKillStore::Instance().AddKill(killer->GetGUIDLow(), encounter))
        return; // counted once per character, ever
    // Async write: the kill path never blocks on the database. INSERT
    // IGNORE keeps a racing double kill (two members' hooks on the same
    // frame) from erroring.
    CharacterDatabase.PExecute(
        "INSERT IGNORE INTO `tortoise_bots_hire_unlock_kills` (`guid`, `boss_entry`) VALUES ('%u', '%u')",
        killer->GetGUIDLow(), encounter);
    // KilledMonsterCredit walks the quest log and bumps the counter slot
    // whose ReqCreatureOrGOId matches; dedup is per player (this killer).
    killer->KilledMonsterCredit(counter->creditEntry);
}

void HireUnlockKillAdapter::OnLogin(Player* player)
{
    if (!player)
        return;
    if (!sPlayerbotAIConfig.hireGearTiersEnabled)
    {
        HireUnlockKillStore::Instance().Drop(player->GetGUIDLow());
        return;
    }
    if (BotManager::Instance().IsBot(player->GetObjectGuid()))
        return;
    std::set<uint32_t> kills;
    std::unique_ptr<QueryResult> rows(CharacterDatabase.PQuery(
        "SELECT `boss_entry` FROM `tortoise_bots_hire_unlock_kills` WHERE `guid` = '%u'",
        player->GetGUIDLow()));
    if (rows)
        do
        {
            kills.insert(rows->Fetch()[0].GetUInt32());
        } while (rows->NextRow());
    HireUnlockKillStore::Instance().SetKills(player->GetGUIDLow(), std::move(kills));
    SyncCounters(player);
}

void HireUnlockKillAdapter::OnLogout(Player* player)
{
    if (!player)
        return;
    HireUnlockKillStore::Instance().Drop(player->GetGUIDLow());
}

void HireUnlockKillAdapter::OnDelete(ObjectGuid guid, uint32 /*accountId*/)
{
    if (!guid.IsPlayer())
        return;
    uint32_t guidLow = guid.GetCounter();
    HireUnlockKillStore::Instance().Drop(guidLow);
    CharacterDatabase.DirectPExecute(
        "DELETE FROM `tortoise_bots_hire_unlock_kills` WHERE `guid` = '%u'", guidLow);
}

void HireUnlockKillAdapter::SyncCounters(Player* player)
{
    if (!player)
        return;
    if (!sPlayerbotAIConfig.hireGearTiersEnabled)
        return;
    std::set<uint32_t> recorded = HireUnlockKillStore::Instance().KillsFor(player->GetGUIDLow());
    std::set<uint32_t> optionalMaps(
        sPlayerbotAIConfig.hireGearOptionalMaps.begin(), sPlayerbotAIConfig.hireGearOptionalMaps.end());
    for (HireUnlockCounter const& counter : kHireUnlockCounters)
    {
        uint32_t questId = HireUnlockQuestForTier(counter.tier);
        if (questId == 0)
            continue;
        if (player->GetQuestStatus(questId) != QUEST_STATUS_INCOMPLETE)
            continue;
        QuestStatusData const* status = player->GetQuestStatusData(questId);
        if (!status)
            continue;
        uint8_t current = static_cast<uint8_t>(status->m_creatureOrGOcount[counter.slot]);
        uint8_t target = HireUnlockCounterTarget(counter, recorded, optionalMaps, current);
        if (target <= current)
            continue;
        // KilledMonsterCredit adds one per call to the server-side counter
        // (m_creatureOrGOcount, saved), updates the client and completes the
        // quest when the last slot fills. SendQuestUpdateAddCreatureOrGo
        // alone only updates the client view, so a retaken quest stayed at 0.
        // Credit entries are unique per counter, so only this slot moves.
        for (uint8_t i = current; i < target; ++i)
            player->KilledMonsterCredit(counter.creditEntry);
    }
}

} // namespace TortoiseBots
