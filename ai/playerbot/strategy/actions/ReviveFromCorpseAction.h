#pragma once
#include "playerbot/PlayerbotAI.h"
#include "MovementActions.h"

namespace ai
{
    // The area trigger id of the door a managed bot released inside a dungeon
    // revives at, nearest its corpse (CorpseEntrancePolicy.h RepopEntranceRules,
    // validated maps only). 0: no answer.
    uint32 PickRepopEntrance(Player* bot, uint32 dungeonMapId, float corpseX, float corpseY, float corpseZ);

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
