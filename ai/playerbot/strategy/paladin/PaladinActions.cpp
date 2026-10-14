
#include "playerbot/playerbot.h"
#include "PaladinActions.h"
#include "playerbot/ServerFacade.h"
#include "playerbot/GroupMembers.h"

using namespace ai;

bool CastJudgementAction::isUseful()
{
    if (!ai->HasAnyAuraOf(bot, "seal of justice", "seal of command", "seal of righteousness", "seal of light", "seal of wisdom", NULL))
        return false;

    return AI_VALUE2(uint8, "mana", "self target") >= sPlayerbotAIConfig.mediumMana;
}

bool CastConsecrationAction::isUseful()
{
    if (!CastSpellAction::isUseful())
        return false;

    uint8 const mana = AI_VALUE2(uint8, "mana", "self target");
    if (mana < sPlayerbotAIConfig.mediumMana)
        return false;

    // Enemies that would actually stand in it (8yd radius around the paladin).
    uint32 inReach = 0;
    for (ObjectGuid const& guid : AI_VALUE(std::list<ObjectGuid>, "attackers"))
    {
        Unit* const attacker = ai->GetUnit(guid);
        if (attacker && attacker->IsAlive() && bot->GetDistance(attacker) <= 8.0f)
            ++inReach;
    }

    if (inReach >= 3)
        return true;

    return inReach == 2 && mana >= 70;
}

// "no paladin aura" queues this fallback next to the spec aura, and a queued
// basket outlives the tick (ExpireActionTime). Once the spec aura has landed,
// a stale fallback would cast the first aura the bot does not have and swap
// it out. Only act while the bot has none of its own auras up.
bool CastPaladinAuraAction::isUseful()
{
    static const char* const kAuras[] =
    {
        "devotion aura", "retribution aura", "concentration aura", "sanctity aura",
        "shadow resistance aura", "fire resistance aura", "frost resistance aura", "crusader aura"
    };

    for (const char* aura : kAuras)
    {
        if (AI_VALUE2(uint32, "spell id", aura) && ai->HasMyAura(aura, bot))
            return false;
    }

    return true;
}

bool CastPaladinAuraAction::Execute(Event& event)
{
    std::vector<std::string> altAuras;
    std::vector<std::string> haveAuras;
    altAuras.push_back("devotion aura");
    altAuras.push_back("retribution aura");
    altAuras.push_back("concentration aura");
    altAuras.push_back("sanctity aura");
    altAuras.push_back("shadow resistance aura");
    altAuras.push_back("fire resistance aura");
    altAuras.push_back("frost resistance aura");
    altAuras.push_back("crusader aura");

    for (auto aura : altAuras)
    {
        if (AI_VALUE2(uint32, "spell id", aura))
            haveAuras.push_back(aura);
    }

    if (haveAuras.empty())
    {
        return false;
    }

    for (auto aura : haveAuras)
    {
        if (!ai->HasAura(aura, bot))
        {
            uint32 spellDuration = sPlayerbotAIConfig.globalCoolDown;
            bool executed = ai->CastSpell(aura, bot, nullptr, false);
            if (executed)
            {
                SetDuration(1.0f);
            }

            return executed;
        }
    }

    return false;
}

Unit* CastBlessingAction::GetTarget()
{
    return bot;
}

bool CastBlessingAction::isPossible()
{
    Unit* target = GetTarget();
    if (target)
    {
        std::string blessing = GetBlessingForTarget(target);
        if (!blessing.empty())
        {
            SetSpellName(blessing);
            return CastSpellAction::isPossible();
        }
    }

    return false;
}

std::string CastBlessingAction::GetBlessingForTarget(Unit* target)
{
    // Same one-slot rule as the on-party choice below: a second blessing
    // from the same paladin evicts the first (core SPELL_BLESSING
    // per-target-per-caster), so never overwrite a live blessing — own or
    // another caster's — with a "better" pick for self.
    std::string chosenBlessing = "";
    if (target)
    {
        static char const* const kBlessings[] =
        {
            "blessing of might", "blessing of wisdom", "blessing of kings",
            "blessing of sanctuary", "blessing of salvation", "blessing of light",
        };
        for (char const* base : kBlessings)
        {
            std::string const single(base);
            if (ai->HasAura(single, target) || ai->HasAura("greater " + single, target))
                return std::string();
        }
        std::vector<std::string> possibleBlessings = GetPossibleBlessingsForTarget(target);
        for (const std::string& blessing : possibleBlessings)
        {
            const std::string greaterBlessing = "greater " + blessing;
            if ((greater && ai->CanCastSpell(greaterBlessing, target, 0, nullptr, true)) ||
                (!greater && ai->CanCastSpell(blessing, target, 0, nullptr, true)))
            {
                chosenBlessing = greater ? greaterBlessing : blessing;
                break;
            }
        }
    }

    return chosenBlessing;
}

std::vector<std::string> CastPveBlessingAction::GetPossibleBlessingsForTarget(Unit* target) const
{
    std::vector<std::string> blessings;
    if (target && target->IsPlayer())
    {
        Player* player = (Player*)target;
        if (ai->IsTank(player))
        {
            if (player->GetClass() == CLASS_PALADIN)
            {
                blessings = { "blessing of wisdom", "blessing of kings", "blessing of might", "blessing of sanctuary", "blessing of light" };
            }
            else
            {
                blessings = { "blessing of kings", "blessing of might", "blessing of sanctuary", "blessing of light", "blessing of wisdom" };
            }
        }
        else if (ai->IsHeal(player))
        {
            blessings = { "blessing of wisdom", "blessing of kings", "blessing of light", "blessing of sanctuary", "blessing of might" };
        }
        else
        {
            if (player->GetClass() == CLASS_PALADIN)
            {
                blessings = { "blessing of wisdom", "blessing of might", "blessing of kings", "blessing of light", "blessing of sanctuary" };
            }
            else
            {
                blessings = { "blessing of might", "blessing of kings", "blessing of light", "blessing of wisdom", "blessing of sanctuary" };
            }
        }
    }

    return blessings;
}

std::vector<std::string> CastPvpBlessingAction::GetPossibleBlessingsForTarget(Unit* target) const
{
    std::vector<std::string> blessings;
    if (target && target->IsPlayer())
    {
        Player* player = (Player*)target;
        if (ai->IsTank(player))
        {
            if (player->GetClass() == CLASS_PALADIN)
            {
                blessings = { "blessing of wisdom", "blessing of kings", "blessing of might", "blessing of sanctuary", "blessing of light" };
            }
            else
            {
                blessings = { "blessing of kings", "blessing of might", "blessing of sanctuary", "blessing of light", "blessing of wisdom" };
            }
        }
        else if (ai->IsHeal(player))
        {
            blessings = { "blessing of wisdom", "blessing of kings", "blessing of light", "blessing of sanctuary", "blessing of might" };
        }
        else
        {
            if (player->GetClass() == CLASS_PALADIN)
            {
                blessings = { "blessing of wisdom", "blessing of might", "blessing of kings", "blessing of light", "blessing of sanctuary" };
            }
            else
            {
                blessings = { "blessing of might", "blessing of kings", "blessing of light", "blessing of wisdom", "blessing of sanctuary" };
            }
        }
    }

    return blessings;
}

std::vector<std::string> CastRaidBlessingAction::GetPossibleBlessingsForTarget(Unit* target) const
{
    std::vector<std::string> blessings;
    if (target && target->IsPlayer())
    {
        Player* player = (Player*)target;
        if (ai->IsTank(player))
        {
            if (player->GetClass() == CLASS_PALADIN)
            {
                blessings = { "blessing of wisdom", "blessing of kings", "blessing of might", "blessing of sanctuary", "blessing of light" };
            }
            else
            {
                blessings = { "blessing of kings", "blessing of might", "blessing of sanctuary", "blessing of light", "blessing of wisdom" };
            }
        }
        else if (ai->IsHeal(player))
        {
            blessings = { "blessing of wisdom", "blessing of kings", "blessing of light", "blessing of sanctuary", "blessing of might" };
        }
        else
        {
            if (player->GetClass() == CLASS_PALADIN)
            {
                blessings = { "blessing of wisdom", "blessing of might", "blessing of kings", "blessing of light", "blessing of sanctuary" };
            }
            else
            {
                blessings = { "blessing of might", "blessing of kings", "blessing of light", "blessing of wisdom", "blessing of sanctuary" };
            }
        }
    }

    return blessings;
}

Unit* CastBlessingOnPartyAction::GetTarget()
{
    // NOTE: PaladinTriggers.cpp FindBlessingCandidate duplicates this scan
    // for the trigger side (different base class, no shared helper). Keep
    // the two in sync: same order, same holds-nothing rule, self included.
    static char const* const kBlessings[] =
    {
        "blessing of might", "blessing of wisdom", "blessing of kings",
        "blessing of sanctuary", "blessing of salvation", "blessing of light",
    };

    bool knowsAny = false;
    for (char const* blessing : kBlessings)
    {
        if (AI_VALUE2(uint32, "spell id", blessing))
        {
            knowsAny = true;
            break;
        }
    }

    if (!knowsAny)
    {
        return nullptr;
    }

    Group* group = bot ? bot->GetGroup() : nullptr;
    if (!group)
    {
        return nullptr;
    }

    // Same member order as PartyMemberValue (healers, tanks, others) so the
    // first gap found is the most urgent one; unlike that search the bot
    // itself is included — its own class bucket still needs the Greater
    // cast (the ret that uniquely knows Kings must cover its own class).
    std::list<Player*> healers, tanks, others;
    for (Player* member : LiveGroupMembers(group))
    {
        if (!member || member->GetMapId() != bot->GetMapId())
            continue;
        // Hidden GM masters (VISIBILITY_OFF) can never land a spell: the
        // core drops them in Spell::CheckTarget after the cast starts, so
        // they read as "bare" forever and wedge the scan (PartyMemberValue::
        // Check skips them the same way; paladin loop 2026-10-11).
        if (member != bot && member->GetVisibility() == VISIBILITY_OFF)
            continue;
        if (member == bot)
        {
            others.push_back(member);
            continue;
        }
        if (ai->IsHeal(member))
            healers.push_back(member);
        else if (ai->IsTank(member))
            tanks.push_back(member);
        else
            others.push_back(member);
    }

    std::list<Player*> order;
    order.insert(order.end(), healers.begin(), healers.end());
    order.insert(order.end(), tanks.begin(), tanks.end());
    order.insert(order.end(), others.begin(), others.end());

    // One blessing slot per member (core: one SPELL_BLESSING per target per
    // caster): only a member holding no blessing at all is a candidate.
    // Anything already blessed has its slot spoken for by some paladin, and
    // casting over it evicts (same caster) or churns against (other caster)
    // the live aura — the Light/Kings alternation seen live 2026-10-14.
    for (Player* member : order)
    {
        if (sServerFacade.getDistance2d(member, bot) > 30.0f)
            continue;
        bool holdsAny = false;
        for (char const* blessing : kBlessings)
        {
            std::string const single(blessing);
            if (ai->HasAura(single, member) || ai->HasAura("greater " + single, member))
            {
                holdsAny = true;
                break;
            }
        }
        if (!holdsAny && sServerFacade.IsWithinLOSInMap(bot, member))
            return member;
    }

    return nullptr;
}

bool CastBlessingOnPartyAction::isUseful()
{
    // Two paladins pick through the same shared "party member without my aura"
    // value: neither sees the other's in-flight cast, so both can resolve the
    // same member the same tick and overwrite each other's blessing. Stand
    // down while another bot holds a live claim on this member for the
    // blessing this bot would cast (BuffClaimRegistry, 4 s TTL).
    Unit* target = GetTarget();
    if (!target)
        return false;
    std::string const blessing = GetBlessingForTarget(target);
    if (!blessing.empty() && BuffClaimRegistry::IsTargetClaimedByOther(bot, target, blessing))
        return false;
    return CastSpellAction::isUseful();
}

bool CastBlessingOnPartyAction::isPossible()
{
    Unit* target = GetTarget();
    if (target)
    {
        std::string blessing = GetBlessingForTarget(target);
        if (!blessing.empty())
        {
            SetSpellName(blessing);
            return CastSpellAction::isPossible();
        }
    }

    return false;
}

bool CastBlessingOnPartyAction::Execute(Event& event)
{
    Unit* const target = GetTarget();
    if (!CastSpellAction::Execute(event))
        return false;
    // Claim only on a cast that actually started: a whiff (range/LOS at cast
    // time) must not stand the other paladin down for the TTL.
    if (target)
        BuffClaimRegistry::Claim(bot->GetObjectGuid(), target->GetObjectGuid(), GetSpellName());
    return true;
}

std::string CastBlessingOnPartyAction::GetBlessingForTarget(Unit* target)
{
    // Core aura rule (tortoise-wow Unit::RemoveNoStackAurasDueToAuraHolder,
    // live 2026-10-14: every blessing classifies SPELL_BLESSING, one per
    // target per caster): a second blessing from the SAME paladin always
    // evicts the first, whatever the family; different casters coexist. So
    // each paladin owns exactly one blessing slot per member: skip a member
    // that holds ANY blessing already (donor mod-playerbots single-path
    // HasBlessingAura guard), whatever the caster — the old same-family
    // check kept "topping up" over the other paladin's blessing and evicting
    // its own (observed Light/Kings alternation on the holy paladin, and
    // Might-then-Wisdom churn on the priest).
    if (!target)
        return std::string();

    static char const* const kBlessings[] =
    {
        "blessing of might", "blessing of wisdom", "blessing of kings",
        "blessing of sanctuary", "blessing of salvation", "blessing of light",
    };
    for (char const* base : kBlessings)
    {
        std::string const single(base);
        if (ai->HasAura(single, target) || ai->HasAura("greater " + single, target))
            return std::string();
    }

    std::string chosenBlessing = "";
    std::vector<std::string> possibleBlessings = GetPossibleBlessingsForTarget(target);
    for (const std::string& blessing : possibleBlessings)
    {
        // Don't cast greater salvation on possible tank classes
        if (greater && blessing == "blessing of salvation" && target->IsPlayer())
        {
            const uint8 playerClass = ((Player*)target)->GetClass();
            if (playerClass == CLASS_PALADIN || playerClass == CLASS_WARRIOR || playerClass == CLASS_DRUID)
            {
                break;
            }
        }

        const std::string greaterBlessing = "greater " + blessing;
        if ((greater && ai->CanCastSpell(greaterBlessing, target, 0, nullptr, true)) ||
            (!greater && ai->CanCastSpell(blessing, target, 0, nullptr, true)))
        {
            chosenBlessing = greater ? greaterBlessing : blessing;
            break;
        }
    }

    return chosenBlessing;
}

std::vector<std::string> CastPveBlessingOnPartyAction::GetPossibleBlessingsForTarget(Unit* target) const
{
    std::vector<std::string> blessings;
    if (target && target->IsPlayer())
    {
        Player* player = (Player*)target;
        if (ai->IsTank(player))
        {
            if (player->GetClass() == CLASS_PALADIN)
            {
                blessings = { "blessing of wisdom", "blessing of kings", "blessing of might", "blessing of sanctuary", "blessing of light" };
            }
            else
            {
                blessings = { "blessing of kings", "blessing of might", "blessing of sanctuary", "blessing of light", "blessing of wisdom" };
            }
        }
        else if (ai->IsHeal(player))
        {
            blessings = { "blessing of wisdom", "blessing of kings", "blessing of light", "blessing of sanctuary", "blessing of might" };
        }
        else if (ai->IsRanged(player))
        {
            if (player->GetClass() == CLASS_HUNTER)
            {
                blessings = { "blessing of wisdom", "blessing of kings", "blessing of might", "blessing of light", "blessing of sanctuary" };
            }
            else
            {
                blessings = { "blessing of kings", "blessing of wisdom", "blessing of light", "blessing of sanctuary", "blessing of might" };
            }
        }
        else
        {
            if (player->GetClass() == CLASS_PALADIN)
            {
                blessings = { "blessing of wisdom", "blessing of might", "blessing of kings", "blessing of light", "blessing of sanctuary" };
            }
            else
            {
                blessings = { "blessing of might", "blessing of kings", "blessing of light", "blessing of wisdom", "blessing of sanctuary" };
            }
        }
    }
    else
    {
        // Blessings for pets
        blessings = { "blessing of might", "blessing of kings", "blessing of light", "blessing of sanctuary" };
    }

    return blessings;
}

std::vector<std::string> CastPvpBlessingOnPartyAction::GetPossibleBlessingsForTarget(Unit* target) const
{
    std::vector<std::string> blessings;
    if (target && target->IsPlayer())
    {
        Player* player = (Player*)target;
        if (ai->IsTank(player))
        {
            if (player->GetClass() == CLASS_PALADIN)
            {
                blessings = { "blessing of wisdom", "blessing of kings", "blessing of might", "blessing of sanctuary", "blessing of light" };
            }
            else
            {
                blessings = { "blessing of kings", "blessing of might", "blessing of sanctuary", "blessing of light", "blessing of wisdom" };
            }
        }
        else if (ai->IsHeal(player))
        {
            blessings = { "blessing of wisdom", "blessing of kings", "blessing of light", "blessing of sanctuary", "blessing of might" };
        }
        else if (ai->IsRanged(player))
        {
            if (player->GetClass() == CLASS_HUNTER)
            {
                blessings = { "blessing of wisdom", "blessing of kings", "blessing of might", "blessing of light", "blessing of sanctuary" };
            }
            else
            {
                blessings = { "blessing of kings", "blessing of wisdom", "blessing of light", "blessing of sanctuary", "blessing of might" };
            }
        }
        else
        {
            if (player->GetClass() == CLASS_PALADIN)
            {
                blessings = { "blessing of wisdom", "blessing of might", "blessing of kings", "blessing of light", "blessing of sanctuary" };
            }
            else
            {
                blessings = { "blessing of might", "blessing of kings", "blessing of light", "blessing of wisdom", "blessing of sanctuary" };
            }
        }
    }
    else
    {
        // Blessings for pets
        blessings = { "blessing of might", "blessing of kings", "blessing of light", "blessing of sanctuary" };
    }

    return blessings;
}

std::vector<std::string> CastRaidBlessingOnPartyAction::GetPossibleBlessingsForTarget(Unit* target) const
{
    std::vector<std::string> blessings;
    if (target && target->IsPlayer())
    {
        Player* player = (Player*)target;
        if (ai->IsTank(player))
        {
            if (player->GetClass() == CLASS_PALADIN)
            {
                blessings = { "blessing of wisdom", "blessing of kings", "blessing of might", "blessing of sanctuary", "blessing of light" };
            }
            else
            {
                blessings = { "blessing of kings", "blessing of might", "blessing of sanctuary", "blessing of light", "blessing of wisdom" };
            }
        }
        else if (ai->IsHeal(player))
        {
            blessings = { "blessing of wisdom", "blessing of kings", "blessing of sanctuary", "blessing of light", "blessing of might" };
        }
        else if (ai->IsRanged(player))
        {
            if (player->GetClass() == CLASS_HUNTER)
            {
                blessings = { "blessing of wisdom", "blessing of kings", "blessing of might", "blessing of light", "blessing of sanctuary" };
            }
            else
            {
                blessings = { "blessing of kings", "blessing of wisdom", "blessing of light", "blessing of sanctuary", "blessing of might" };
            }
        }
        else
        {
            if (player->GetClass() == CLASS_PALADIN)
            {
                blessings = { "blessing of wisdom", "blessing of might", "blessing of kings", "blessing of light", "blessing of sanctuary" };
            }
            else
            {
                blessings = { "blessing of might", "blessing of kings", "blessing of light", "blessing of wisdom", "blessing of sanctuary" };
            }
        }
    }
    else
    {
        // Blessings for pets
        blessings = { "blessing of might", "blessing of kings", "blessing of sanctuary", "blessing of light" };
    }

    return blessings;
}