#pragma once
#include "playerbot/PlayerbotAI.h"
#include "playerbot/strategy/AiObjectContext.h"
#include "playerbot/strategy/Value.h"
#include "NearestUnitsValue.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "playerbot/AvoidAoePolicy.h"
#include "playerbot/CreatureAuraKeepoutPolicy.h"

namespace ai
{
    // List of hostile and neutral targets in a range around the bot
    class PossibleTargetsValue : public NearestUnitsValue, public Qualified
	{
	public:
        PossibleTargetsValue(PlayerbotAI* ai, std::string name = "possible targets", float range = sPlayerbotAIConfig.sightDistance, bool ignoreLos = false) :
          NearestUnitsValue(ai, name, range, ignoreLos), Qualified() { this->keepLegacyRefresh = true; }

        std::list<ObjectGuid> Calculate() override;
        static bool IsValid(Unit* target, Player* player, bool ignoreLos = false);

    protected:
        virtual void FindUnits(std::list<Unit*> &targets) override;
        virtual bool AcceptUnit(Unit* unit) override;

        static void FindPossibleTargets(Player* player, std::list<Unit*>& targets, float range);
        static bool IsFriendly(Unit* target, Player* player);
        static bool IsAttackable(Unit* target, Player* player);
	};

    class AllTargetsValue : public PossibleTargetsValue
	{
	public:
        AllTargetsValue(PlayerbotAI* ai, float range = sPlayerbotAIConfig.sightDistance) :
        PossibleTargetsValue(ai, "all targets", range, true) {}
	};

    // Proactive AoE sensor (donor PossibleTriggersValue shape): hostile,
    // not-selectable units within 15yd whose periodic-trigger aura fires a
    // school-damage spell (e.g. invisible void-zone trigger NPCs). LoS is
    // ignored — the trigger sits inside the zone it marks.
    // With AiPlayerbot.AvoidCreatureAuras on it also accepts a selectable
    // creature listed in CreatureAuraKeepoutPolicy.h that carries its aura
    // (Maraudon's Creeping Sludge), so the same sensor sees it.
    class PossibleTriggersValue : public NearestUnitsValue
	{
	public:
        PossibleTriggersValue(PlayerbotAI* ai) :
            NearestUnitsValue(ai, "possible triggers", kMaxAoeAvoidRadiusYd, true) { this->keepLegacyRefresh = true; }

        // The keep-out row for a live listed creature carrying its aura while
        // the toggle is on; nullptr otherwise. The explicit-order exemption is
        // per bot and stays with the callers.
        static CreatureAuraKeepout const* KeepoutFor(Unit* unit);

    protected:
        void FindUnits(std::list<Unit*> &targets) override;
        bool AcceptUnit(Unit* unit) override;
	};
}
