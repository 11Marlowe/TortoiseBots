---
id: classes-shaman
title: Shaman Bot Guide
category: classes
summary: Complete guide to Shaman bot AI, totem sets, weapon imbues, chain heals, shocks, and Turtle WoW custom abilities.
tags: [classes, shaman, healer, dps, totems, melee]
relates_to:
  - classes-overview
---

# Shaman Bot Guide

Shamans provide unmatched group utility through versatile totem sets, elemental shocks, weapon imbues, and potent chain spells across Melee DPS, Caster DPS, and Healing roles.

## Specs & roles

| Spec | Role | Primary Focus |
| :--- | :--- | :--- |
| **Restoration** | Healer | Chain Heal throughput, single-target wave downranking, and totem support. |
| **Enhancement** | Melee DPS | Front-line melee damage, Windfury burst, Stormstrike, and totem utility. |
| **Elemental** | Ranged DPS | High-damage Lightning Bolt and Chain Lightning spells with instant crits. |

---

## What the bot does in combat

### Restoration (Healer)
- Casts *Chain Heal* when several nearby allies are injured.
- Casts *Healing Wave* and *Lesser Healing Wave*, downranking direct heals to match incoming wounds per [shared downranking rules](overview.md#spell-downranking).
- Employs *Ancestral Swiftness* to instantly land emergency *Healing Wave* casts on dying targets.
- Deals off-spec damage (*Flame Shock*, *Lightning Bolt*, *Chain Lightning* into packs) per [healer off-DPS rules](overview.md#healer-off-dps-offdps).

### Enhancement (Melee DPS)
- Fights in melee using dual-wield or two-handed weapons, keeping weapon imbues active.
- Casts *Stormstrike* before *Lightning Strike* to amplify nature burst.
- Cycles *Flame Shock* for periodic damage and *Earth Shock* for burst damage.
- Triggers *Bloodlust* (custom Turtle ability): self frenzy; party melee crits spread the haste across the group.

### Elemental (Ranged DPS)
- Attacks from range with *Lightning Bolt* and casts *Chain Lightning* against enemy packs.
- Keeps *Water Shield* active for mana regeneration (*Lightning Shield* before training).
- Activates *Elemental Mastery* for guaranteed critical strikes.
- Saves *Earth Shock* as a finishing execute below 25% health and below 1500 absolute HP while reserving interrupts for casting targets.
- Respects [party threat back-off](overview.md#party-threat-back-off) on *Chain Lightning* and *Earthquake* when holding ≥50% tank threat.

<details>
<summary>Totem orchestration details</summary>

Shamans maintain 4-element totem sets tailored to group composition:
- **Earth Totem:** Drops *Strength of Earth Totem* if melee allies are present; otherwise leaves the slot open (*Stoneskin Totem* is the fallback when Strength is unknown). Reacts to fears with *Tremor Totem* and to runners with *Earthbind Totem*.
- **Fire Totem:** Drops *Searing Totem* on single targets; drops *Magma Totem* or *Fire Nova Totem* on packs (only when in detonation range).
- **Water Totem:** Drops *Mana Spring Totem* if mana users are present; drops *Healing Stream Totem* otherwise. Reacts to poison and disease with cleansing totems.
- **Air Totem:** Drops *Windfury Totem* for melee groups; drops *Tranquil Air Totem* or *Grace of Air Totem* for casters. Reacts to incoming enemy spells with *Grounding Totem*.
- **Out of Combat:** Totems can be pre-dropped before pulls and reclaimed using *Totemic Recall* to refund mana.

</details>

---

## Turtle WoW custom spells

- **Earthquake (Spell ID 48306):** Elemental AoE spell causing Nature damage and knockdown pulses.
- **Lightning Strike (Spell ID 51387):** Instant melee strike releasing active shields for heavy Nature damage.
- **Spirit Link (Spell ID 51363):** Restoration talent linking party health pools to distribute spike tank damage evenly.
- **Ancestral Swiftness (Spell ID 16188):** Emergency instant-cast cooldown paired with *Healing Wave*.
- **Bloodlust (Spell ID 45509):** Enhancement haste ability whose attack speed buff spreads through melee crits.

---

## Utility & interrupts

- **Interrupts:** Casts *Earth Shock* to interrupt dangerous enemy spellcasters and healing mobs.
- **Weapon Imbues:** Maintains *Windfury Weapon* and *Flametongue Weapon* (Enhancement) or *Flametongue Weapon* (Elemental/Resto), falling back to *Rockbiter Weapon* or *Frostbrand Weapon* when those are not known.
- **Dispels:** Strips enemy buffs with *Purge* and cleanses allies using *Cure Poison* and *Cure Disease*.
- **Earth Shield:** Maintains *Earth Shield* on the group tank for reactive healing.
- **Resurrection:** Revives dead allies out of combat with *Ancestral Spirit*.
- **Ghost Wolf:** Shifts to *Ghost Wolf* when carrying a flag in PvP.
- **Player Totem Controls:** Players can customize totem sets via [`.bot totem`](../play/commands/bot-panel.md#bot-totem) or manage them with `drop` and `status`.

---

## Talent builds

TortoiseBots provides pre-configured talent progressions at 5-level checkpoints (levels 10–60) tailored for Turtle WoW 1.18.1:

| Spec Name | Config ID | Role | 60 Allocation | Key Signatures & Synergies |
| :--- | :--- | :--- | :--- | :--- |
| **Restoration** | `7.0` | Healer | `13 / 0 / 38` | Ancestral Swiftness, Spirit Link, Improved Chain Heal, Water Shield, Elemental Focus dip. |
| **Enhancement** | `7.1` | Melee DPS | `17 / 34 / 0` | Stormstrike, Bloodlust, Elemental Weapons (+40% Windfury AP), Flurry, Elemental Devastation. |
| **Elemental** | `7.2` | Ranged DPS | `42 / 0 / 9` | Elemental Mastery, Earthquake, Call of Thunder, Lightning Mastery, Resto efficiency dip. |

<details>
<summary>Leveling milestones and progression rationale</summary>

- **Restoration (`7.0`):**
  - *Levels 10–35:* Takes *Improved Healing Wave* (5/5), *Tidal Focus* (5/5), *Ancestral Healing* (3/3), *Healing Way* (3/3), and *Restorative Totems* (5/5).
  - *Levels 35–50:* Takes *Improved Water Shield* (3/3), *Ancestral Swiftness* (40), *Tidal Surge* (2/2), and *Spirit Link* (50).
  - *Levels 50–60:* Dips into Elemental for *Convection* (5/5), *Earth's Grasp* (2/2), and *Elemental Focus* (1/1).
- **Enhancement (`7.1`):**
  - *Levels 10–35:* Takes *Ancestral Knowledge* (5/5), *Thundering Strikes* (5/5), *Stable Shields* (3/3), *Lightning Strike* (1/1), and *Flurry* (30).
  - *Levels 35–50:* Takes *Elemental Weapons* (3/3), *Enhancing Totems* (2/2), *Stormstrike* (40), and *Bloodlust* (50).
  - *Levels 50–60:* Dips into Elemental for *Convection* (5/5), *Concussion* (5/5), *Elemental Focus* (1/1), and *Elemental Devastation* (3/3).
- **Elemental (`7.2`):**
  - *Levels 10–40:* Takes *Convection* (5/5), *Concussion* (5/5), *Elemental Focus* (1/1), *Call of Thunder* (5/5), *Call of Flame* (3/3), *Storm Reach* (2/2), and *Elemental Mastery* (40).
  - *Levels 40–50:* Takes *Lightning Mastery* (5/5) and *Earthquake* (45).
  - *Levels 50–60:* Dips into Restoration for *Improved Healing Wave* (5/5) and *Tidal Focus* (4/5).

</details>

---

## Class settings

There are no class-specific configuration flags for Shamans. General combat and resting settings are configured in [Combat settings](../server/settings/combat.md).
