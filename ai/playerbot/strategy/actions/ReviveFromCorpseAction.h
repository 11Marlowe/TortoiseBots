#pragma once
#include "playerbot/PlayerbotAI.h"
#include "MovementActions.h"

struct AreaTriggerTeleport;

namespace ai
{
    // The door a managed bot released inside a dungeon revives at, nearest its
    // corpse (CorpseEntrancePolicy.h RepopEntranceRules). False: no answer.
    bool PickRepopEntrance(Player* bot, uint32 dungeonMapId, float corpseX, float corpseY, float corpseZ,
                           AreaTriggerTeleport const*& outAt);

	class ReviveFromCorpseAction : public MovementAction
    {
	public:
		ReviveFromCorpseAction(PlayerbotAI* ai) : MovementAction(ai, "revive from corpse") {}
        virtual bool Execute(Event& event) override;
    };

    class FindCorpseAction : public MovementAction
    {
    public:
        FindCorpseAction(PlayerbotAI* ai) : MovementAction(ai, "find corpse") {}
        virtual bool Execute(Event& event) override;
        virtual bool isUseful() override;
    };

	class SpiritHealerAction : public MovementAction
    {
	public:
	    SpiritHealerAction(PlayerbotAI* ai, std::string name = "spirit healer") : MovementAction(ai,name) {}
        virtual bool Execute(Event& event) override;
        virtual bool isUseful() override;
    private:
        bool ShouldApplyResurrectionSickness();
    };
}
