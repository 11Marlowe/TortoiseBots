---
id: class-shaman
title: Shaman Bot AI & Specs
category: classes
summary: Deep dive into Shaman totem orchestration, weapon imbues, chain heals, interrupts, and custom Turtle spells.
tags: [class, shaman, healer, dps, totems, melee]
relates_to:
  - class-overview
  - guide-player-controls
---

# Shaman Bot AI & Specs

Shamans bring unparalleled group utility through totem sets, elemental shocks, weapon imbues, and potent chain spells across Melee DPS, Caster DPS, and Healing.

## Supported Specs & Roles

- **Restoration (Healer):** Premier multi-target healer utilizing *Chain Heal*, *Healing Wave*, *Lesser Healing Wave*, and *Mana Tide Totem*. *Chain Heal* fires on headcount (5+ members within 30 yards, 3 hurt in a 5-man scaling to 15 in a raid). When nobody needs healing and mana is above the priest's fight-scaled reserve (85% on a hard fight, 70% otherwise), casts *Flame Shock* / *Lightning Bolt* (*Chain Lightning* into packs) at the lowest relevance so every heal wins first — the same healer-dps pattern priests use. In a group that damage only goes on a mob already fighting the party (another member or the shaman), never on a fresh one, unless no other member is within `SightDistance` or the mob is the shaman's explicit attack target.
- **Restoration (Healer):** Premier multi-target healer utilizing *Chain Heal*, *Healing Wave*, and *Lesser Healing Wave*. *Chain Heal* fires on headcount (5+ members within 30 yards, 3 hurt in a 5-man scaling to 15 in a raid). When nobody needs healing and mana is above the priest's fight-scaled reserve (85% on a hard fight, 70% otherwise), casts *Flame Shock* / *Lightning Bolt* (*Chain Lightning* into packs) at the lowest relevance so every heal wins first — the same healer-dps pattern priests use.
- **Enhancement (Melee DPS):** Dual-wielding or two-handed melee powerhouse utilizing *Windfury*, *Stormstrike*, and shocks.
- **Elemental (Ranged DPS):** Nature and fire caster driving high burst through *Lightning Bolt*, *Chain Lightning*, and *Elemental Mastery*. *Earth Shock* is saved as an execute (target below 25% health and 1500 hp) so the shared shock cooldown and boss debuff slots are not wasted on healthy targets; interrupts still fire anytime.
- **Elemental (Ranged DPS):** Nature and fire caster driving high burst through *Lightning Bolt*, *Chain Lightning*, and *Elemental Mastery*. Keeps *Water Shield* up once trained (*Lightning Shield* before that) and fires *Chain Lightning* on cooldown into ranged packs (never single-target). *Chain Lightning* counts as an area spell for the party threat back-off (it waits while the shaman holds 50% or more of the tank's threat on any mob it is fighting).

---

## Totem Orchestration

Shamans automatically drop and maintain fixed per-spec 4-element totem sets (other totems are selectable via manual `totem ...` strategies):

- **Earth Totem:** picked by subgroup — melee in our own party (warriors/rogues/hunters/feral/melee, pets excluded) mean *Strength of Earth Totem*; caster/healer groups keep the slot empty instead of buffing nobody. Solo bots at low health drop *Stoneclaw Totem* as a panic button instead (taunts attackers off the bot); grouped bots keep the spec totem unless ordered `totem earth stoneclaw`. *Tremor Totem* is manual-only.
- **Earth Totem:** picked by subgroup (see above). Reacts to fear/charm with *Tremor Totem* (any party member feared or charmed) and to runners with *Earthbind Totem* (fleeing target or snared party member) — only while the earth slot is empty.
- **Fire Totem:** *Searing Totem* (single target), *Magma Totem* / *Fire Nova Totem* (AoE packs). Restoration drops *Searing Totem* only while `healer should attack` holds (nobody needs healing, mana comfortable) — no 170-mana chip damage mid heal fight.
- **Water Totem:** picked by subgroup — mana users in our own party mean *Mana Spring Totem*, else *Healing Stream Totem*. A restoration shaman in a caster/healer group therefore springs instead of trickle-healing. Reacts to party poison/disease with *Poison/Disease Cleansing Totem* (one trigger per debuff type) — only while the water slot is empty.
- **Air Totem:** picked by subgroup — melee in our own party mean *Windfury Totem*, casters/healers get *Tranquil Air Totem* (Turtle 25908, threat reduction) or *Grace of Air Totem*. Reacts to enemy casts aimed at the party with *Grounding Totem* — only while the air slot is empty.
- **Mana Tide Totem (not available on Turtle 1.18.1: no talent, no trainer; the rows stay for cores that have it):** fires on our own low mana OR a dry healer/caster in our own subgroup (it restores the whole subgroup). It is a cooldown battery, not upkeep: it drops while *Mana Spring* / *Healing Stream* is up (the water cover scan does not veto it). Restoration spends mana on damage (*Earth Shock*, *Searing Totem*, offdps) only while `healer should attack` holds — interrupts stay exempt; in raids off-spec damage additionally needs 95 % mana.
- **Pre-pull:** out of combat the same subgroup-picked air/water/earth totems drop at below-combat relevance, so the pull starts with *Mana Spring* etc. already up. Slot quiet is owner-scoped (only the bot's own totem counts — another subgroup's totem never reads as slot-full); totem cover is matched by aura id (*Windfury Totem Effect*, not the cast name). *Totemic Recall* never fires from the combat engine, so mid-fight drops are not recalled away.
- **Player control (`.bot totem`):** per-slot choice (`<earth|fire|water|air> <totem|auto|none>`, persisted per bot; `auto` is the subgroup pick above; short names accepted, `mana spring` = `mana spring totem`), drop mode (`mode <auto|prepull|manual>` — `manual` only drops on order), on-demand `drop` (the addon button; queues the set now — `auto` slots resolve the subgroup pick at drop time — walks into range), and `status` (`TBM:TOTEM` lines). Explicit orders beat the census; manual `totem <slot>` strategies beat both.

Reactive swaps never fight an explicit order: any manual `totem <slot> <which>` strategy for that slot disables the matching swap, and a dropped totem quiets its trigger (no flapping).
- **Earth Totem:** *Strength of Earth Totem* (automatic; *Stoneskin Totem* as fallback). *Tremor Totem* is manual-only.
- **Fire Totem:** *Searing Totem* (single target), *Magma Totem* / *Fire Nova Totem* (AoE packs). *Fire Nova Totem* is only dropped with the target in detonation range of the drop point — no more wasted totems on distant fights.
- **Water Totem:** *Mana Spring Totem* or *Healing Stream Totem* (automatic per spec). *Poison Cleansing Totem* is manual-only.
- **Air Totem:** *Windfury Totem* (automatic; *Grace of Air Totem* for Enhancement), *Grounding Totem* as fallback.

---

## Turtle WoW 1.18.1 Custom Content

- **Earthquake (Spell ID 48306):**
  - Custom Turtle WoW Elemental AoE spell causing Nature damage and aftershocks. Integrated into Elemental AoE rotations when AoE is enabled.
- **Lightning Strike (Spell ID 51387):**
  - Custom Enhancement talent that releases the active shield for an instant Nature burst.
- **Spirit Link (Spell ID 51363):**
  - Restoration talent linking party members to distribute incoming tank damage evenly across the group, mitigating lethal spike damage.
- **Ancestral Swiftness (Spell ID 16188):**
  - Instant cast trigger paired with *Healing Wave* for instantaneous emergency tank saves.
- **Bloodlust (Spell ID 45509):**
  - Custom Turtle WoW enhancement ability: self frenzy whose haste spreads through party melee crits.

---

## Utility & Interrupts

- **Shock rotation:** Elemental and Enhancement keep *Flame Shock* up first, then spend the shared shock cooldown on *Earth Shock*; *Earth Shock* keeps its separate interrupt duty on casting targets (own target and off-target enemy healers).
- **Strike order (Enhancement):** *Stormstrike* fires before the shield-consuming *Lightning Strike*, so the nature-vulnerability debuff amplifies everything after it.
- **Snares:** Casts *Frost Shock* to snare targets.
- **Weapon Imbues:** Automatically maintains weapon imbues (*Windfury Weapon* / *Flametongue Weapon* for Enhancement, *Flametongue Weapon* otherwise, with *Rockbiter Weapon* / *Frostbrand Weapon* as fallbacks). The imbue is cast with the weapon as its item target (it used to be passed as the cast item, so the server refused every imbue and the upkeep repeated forever).
- **Dispels & Cleansing:** Uses *Purge* to strip enemy buffs (shields, HoTs) and *Cure Poison* / *Cure Disease* on party members (no curse cure exists for shamans in 1.12; curses stay a druid/mage job).
- **Mana sense:** Never skips a target at or below the low-health line for mana reasons; above the medium line it refuses oversized or average-efficiency heals, and below the medium-mana line it refuses mana-hungry heals (*Lesser Healing Wave*) in favor of *Healing Wave*. Tanks count the expected heal at two-thirds.
- **Downranking:** *Healing Wave* / *Lesser Healing Wave* are downranked to the wound like every healer's direct heals (cheapest rank covering missing health minus incoming heals; top rank at or below the low-health line) — self casts included (the self rows share the party swap). Turtle's *Healing Wave* rows use the chain-heal target id, so the gate accepts it for jump-less lines (*Chain Heal* itself stays top rank). Rules in `HealDownrankPolicy.h`.
- **Resurrection:** Resurrects fallen party members with *Ancestral Spirit* (no self-Reincarnation).
- **Defensive & Utility:** Keeps *Earth Shield* on the party tank, recalls standing totems out of combat with *Totemic Recall* for mana (never the *Mana Tide* cooldown totem, never while anyone fights), and shifts to *Ghost Wolf* when carrying a PvP flag.
- **Heals instead of fleeing when outnumbered:** A solo bot (no group) with mana at or above `AiPlayerbot.LowMana` (default 15%) is exempt from the `outnumbered` flee, like a solo priest, and heals itself through the fight with *Healing Wave*. Grouped or out of mana it still breaks off; the `panic` and `critical health no master` escapes are unchanged.

---

## Premade Talent Specs & Progression (1.18.1)

TortoiseBots provides verified 5-level talent checkpoints (levels 10–60) tailored for Turtle WoW 1.18.1:

| Spec Name | Config ID | Role | 60 Allocation | Key Signatures & Synergies |
| :--- | :--- | :--- | :--- | :--- |
| **Restoration** | `7.0` | Healer | `13 / 0 / 38` | Ancestral Swiftness (40), Spirit Link (50), Improved Chain Heal, Improved Water Shield, Elemental Focus dip. |
| **Enhancement** | `7.1` | Melee DPS | `17 / 34 / 0` | Stormstrike (40), Bloodlust (50), Elemental Weapons (40% Windfury AP), Elemental Devastation melee crit synergy. |
| **Elemental** | `7.2` | Ranged DPS | `42 / 0 / 9` | Elemental Mastery (40), Earthquake (45), Call of Thunder, Lightning Mastery, Resto efficiency dip. |

### Leveling Milestones & Progression Rationale

- **Restoration (`7.0`):**
  - *Levels 10–35:* Healing throughput foundation: *Improved Healing Wave* (5/5 cast time reduction), *Tidal Focus* (5/5 cost reduction), *Ancestral Healing* (3/3 armor buff on crit heal), *Healing Way* (3/3), and *Restorative Totems* (5/5).
  - *Levels 35–50:* *Improved Water Shield* (3/3 mana sustain), *Ancestral Swiftness* (40 emergency instant heal), *Tidal Surge* (2/2), and *Spirit Link* (50 capstone damage distribution).
  - *Levels 50–60:* Transitions into Elemental for *Convection* (5/5 shock/spell cost reduction), *Earth's Grasp* (2/2), and *Elemental Focus* (Clearcasting).
- **Enhancement (`7.1`):**
  - *Levels 10–35:* Melee burst foundation: *Ancestral Knowledge* (5/5), *Thundering Strikes* (5/5 crit), *Stable Shields* (3/3), *Lightning Strike* (1/1 instant melee strike), and *Flurry* (30 +30% attack speed on crit).
  - *Levels 35–50:* *Elemental Weapons* (3/3 +40% Windfury AP bonus), *Enhancing Totems* (2/2), *Stormstrike* (40 extra attack), and *Bloodlust* (50 capstone).
  - *Levels 50–60:* Elemental synergy dip into *Convection* (5/5), *Concussion* (5/5 shock damage), *Elemental Focus* (1/1), and *Elemental Devastation* (3/3 granting +9% melee crit on spell crits).
- **Elemental (`7.2`):**
  - *Levels 10–40:* Caster nuking path: *Convection* (5/5), *Concussion* (5/5), *Elemental Focus* (1/1 Clearcasting), *Call of Thunder* (5/5 Lightning crit), *Call of Flame* (3/3), *Storm Reach* (2/2 range), and *Elemental Mastery* (40 guaranteed crit).
  - *Levels 40–50:* *Lightning Mastery* (5/5 -1s Lightning Bolt cast time) and *Earthquake* (45 custom AoE capstone).
  - *Levels 50–60:* Restoration dip into *Improved Healing Wave* (5/5) and *Tidal Focus* (4/5) to conserve mana during long dungeon encounters.

