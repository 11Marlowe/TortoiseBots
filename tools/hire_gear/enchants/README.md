# Hire-gear enchants (issue #490)

Generator: `build.py` (offline data, no server code) reads
`../data/wowisclassic.json` + `../data/turtle_sheets.json` and writes
`../data/enchants_by_tier.json`: for each of the 28 `<class>_<spec>` keys
(matching `runtime/HireSpecPolicy.h` gossip order) x tiers 1..7 x equip slot
(0,1,2,4,5,6,7,8,9,10,11,14,15,16,17) -> permanent-enchant **spell id**.

## Id space

The `spell` values are **enchant spell ids** — the same space as the
server's `ai_playerbot_enchant_candidates.spellid` and the argument
`PlayerbotAI::EnchantItemT(spellid, slot, item)` expects. Verified in code:
`EnchantItemT` (`ai/playerbot/PlayerbotAI.cpp`) looks up the spell, takes
`spellInfo->EffectMiscValue[0]` as the `SpellItemEnchantment` id, and writes
it to the item's PERM slot. NOT SpellItemEnchantment ids.

Consumable-item idents in wowisclassic (`1164x` arcanums, `1978x` ZG class
enchants, `2007x` signets, `2354x` Scourge, `18283` Biznicks, `10548` Sniper
Scope, `18182` Chromatic Mantle, `18251` Core Armor Kit) are mapped to their
`item_template.spellid_1` spell ids (all verified live).

## Tier rules (Appendix C, cumulative)

1 pre-raid, 2 ZG+AQ20+LowerKara, 3 MC+Ony+TimbermawHold(819),
4 BWL+ES, 5 AQ40, 6 Naxx, 7 Tower of Karazhan.
An enchant unlocks at the tier of the content that provides it
(ZG arcanums/signets t2, Sapphiron shoulders t6, ...); rep-gated enchants
(AD/Cenarion/Timbermaw/ZG) are ALLOWED. Each tier's pick is the newest source
listing it (wowisclassic phase choice when its content is unlocked, Turtle
Naxx/K40 tab choice at 6/7), else best by role; nothing downgrades except
where the Turtle sheet itself lists a weaker pick (flagged in the report).

Weapon rules: 2H-only rows (mask 1378: Superior Impact, Major Spirit/Intellect,
Timbermaw Agility, Nature/Shadow Damage) never on offhand; 2H-primary specs
(arms/ret/enhance/feral-staff/hunter) still carry a mirrored-OH row for the
K40-Bear-Mit 1H+branch exception — harmless when the slot is empty
(`EnchantItemT` no-ops on no item). Shield specs take Greater Stamina on 16;
scopes (17) for hunters only (Sniper Scope t1-2, Biznicks t3+).

## Gemstone / buckle choices per role (tiers 1-7)

Gemstones (neck 1 + rings 10/11) and belt buckles (5) are available from
tier 1; the pick below is used at every tier unless the Turtle Naxx/K40 tab
for that spec lists a different one (per-tab overrides: mage frost
Glittering Sapphire, fire Radiant Ember, arcane Pure Shining Moonstone;
boomkin/elemental Enchanted Emerald; cat/ret/enhance Stunning Imperial at 6;
rogue Brilliant Opal at 6/7).

| Role | Specs | Gemstone (neck+rings) | Belt buckle |
|---|---|---|---|
| Tank (stamina/defense) | warrior_protection, paladin_protection, druid_bear | 51045 Stunning Imperial (+3 all stats) | 57173 Mithril (+6 agi) |
| Healer (healing/mp5) | paladin_holy, priest_holy/discipline, shaman_restoration, druid_restoration | 51038 Beautiful Diamond (+12 healing) | 57182 Dreamsteel (+10 int) |
| Caster (spell power/hit) | mage_frost/fire/arcane, warlock_x3, priest_shadow, druid_balance, shaman_elemental | 51043 Arcane Emerald (+5 spell dmg) | 57182 Dreamsteel (+10 int) |
| Melee (agi/str/AP/hit) | warrior_arms/fury, paladin_retribution, rogue_x3, shaman_enhancement, druid_cat | 51042 Azerothian Ruby (+6 str); hunters/rogues 51035 Brilliant Opal (+6 agi) at 6/7 per sheet | 73 Obsidian (+10 str); tanks/hunters keep Mithril |
| Hunter (agi/scope) | hunter_x3 | 51035 Brilliant Opal (+6 agi) | 57173 Mithril (+6 agi) |

Why these: DBC-verified stats — Stunning Imperial is the only tank-line gem
(+3 all incl. stam); Beautiful Diamond the largest heal gem (+12);
Arcane Emerald the generic spell-damage gem (+5, school-neutral vs
Topaz/Frost-specific); Azerothian Ruby the largest str gem (+6);
Brilliant Opal the largest agi gem (+6). Buckles: Obsidian (+10 str) tops
melee, Dreamsteel (+10 int) tops casters/healers, Mithril (+6 agi) tops
hunters and Mit-tanks per the Turtle sheets (Threat sheets use Obsidian).

## Re-running

Needs the world DB only for the one-off `work/invtypes.tsv` dump
(gear item -> inventory_type, used for page-position-independent slot
resolution). The generator itself is offline:

```sh
python3 build.py   # -> ../data/enchants_by_tier.json
```

Work notes kept in `work/` (not committed): `gear_ids.txt`,
`invtypes.tsv`, `pairs.tsv`, `pool_pairs.tsv`.
