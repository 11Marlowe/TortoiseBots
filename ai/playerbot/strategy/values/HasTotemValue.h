#pragma once
#include "playerbot/PlayerbotAI.h"
#include "playerbot/strategy/AiObjectContext.h"
#include "playerbot/strategy/Value.h"
#include "TargetValue.h"
#include "playerbot/LootObjectStack.h"
#include "playerbot/ServerFacade.h"

namespace ai
{
    class HasTotemValue : public BoolCalculatedValue, public Qualified
	{
	public:
        HasTotemValue(PlayerbotAI* ai, std::string name = "has totem") : BoolCalculatedValue(ai, name), Qualified() {}

        bool Calculate() override
        {
            std::list<ObjectGuid> units = *context->GetValue<std::list<ObjectGuid> >("nearest npcs");
            for (std::list<ObjectGuid>::iterator i = units.begin(); i != units.end(); i++)
            {
                Unit* unit = ai->GetUnit(*i);
                if (!unit)
                    continue;

                Creature* totem = dynamic_cast<Creature*>(unit);
                if (!totem || !totem->IsTotem())
                    continue;

                const bool totemIsInRange = strstri(totem->GetName(), qualifier.c_str()) &&
                    sServerFacade.GetDistance2d(bot, totem) <= ai->GetRange("spell");

                if (!totemIsInRange) continue;

                Unit* totemOwner = totem->GetOwner();
                if (!totemOwner)
                    continue;

                // Owner-scoped: only OUR totem quiets our own slot. Totems
                // only buff the caster's own party, so another subgroup's
                // totem must never read as "slot full" for us (live raid
                // 2026-10-14: 6 of 7 ordered shamans never dropped after
                // `drop` because a mate's same-slot totem counted).
                // Same-subgroup cover (not re-dropping a mate's live totem
                // that already covers us) stays in the slot triggers, which
                // keep their group-aware aura scan.
                if (totemOwner == bot) return true;
            }

            return false;
        }
    };
}
