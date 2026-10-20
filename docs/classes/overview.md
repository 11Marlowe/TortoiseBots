---
id: classes-overview
title: Class AI & Roles Overview
category: classes
summary: Summary of the nine Vanilla classes, spec detection, combat roles, and shared behaviors in TortoiseBots.
tags: [classes, overview, roles, specs, combat]
relates_to:
  - classes-warrior
  - classes-paladin
  - classes-hunter
  - classes-rogue
  - classes-priest
  - classes-shaman
  - classes-mage
  - classes-warlock
  - classes-druid
---

# Class AI & Roles Overview

TortoiseBots implements combat and utility AI for all nine Vanilla classes (Warrior through Druid). Bots detect their active talent spec dynamically, execute class-appropriate rotations, and support Turtle WoW 1.18.1 custom spells and talents.

## Supported Roles & Classes

| Class | Tank | Healer | Melee DPS | Ranged DPS | Crowd Control (CC) | Interrupts |
| :--- | :---: | :---: | :---: | :---: | :--- | :--- |
| **[Warrior](warrior.md)** | Prot | - | Arms, Fury | - | Intimidating Shout, Concussion Blow | Shield Bash, Pummel |
| **[Paladin](paladin.md)** | Prot | Holy | Retribution | - | Hammer of Justice, Repentance | Hammer of Justice, Repentance |
| **[Hunter](hunter.md)** | - | - | Survival | BM, MM | Freezing Trap, Scare Beast | None (Scatter Shot snares) |
| **[Rogue](rogue.md)** | - | - | All Specs | - | Sap, Blind, Gouge | Kick |
| **[Priest](priest.md)** | - | Holy, Disc | - | Shadow | Shackle Undead, Psychic Scream, Chastise | Silence (Shadow) |
| **[Shaman](shaman.md)** | - | Resto | Enhancement | Elemental | None | Earth Shock |
| **[Mage](mage.md)** | - | - | - | All Specs | Polymorph | Counterspell |
| **[Warlock](warlock.md)** | - | - | - | All Specs | Fear, Banish | Spell Lock (Felhunter) |
| **[Druid](druid.md)** | Bear | Resto | Cat | Balance | Entangling Roots, Hibernate | Bash |

---

## Spec Detection & Strategy Selection

```mermaid
flowchart TD
    Talents["Spent Talent Points (Talent.dbc)"] --> Tally["AiFactory::GetPlayerSpecTab (Count Tab Points)"]
    Tally --> Tree{"Dominant Talent Tree?"}
    Tree -->|Tab 1 Highest| Spec1["Primary Spec Strategy (e.g. Arms, Holy, Assassination)"]
    Tree -->|Tab 2 Highest| Spec2["Secondary Spec Strategy (e.g. Fury, Protection, Combat)"]
    Tree -->|Tab 3 Highest| Spec3["Tertiary Spec Strategy (e.g. Prot, Retribution, Subtlety)"]
    Spec1 --> Modifiers["Apply Contextual Role Modifiers (Tank / Heal / DPS)"]
    Spec2 --> Modifiers
    Spec3 --> Modifiers
    Modifiers --> ActiveEngine["Active Class Strategy Engine Attached"]
```

The bot determines its combat strategy dynamically by inspecting spent talent points:

- **Dominant Talent Tree:** The bot tallies spent talent points across the three class tabs. Whichever tree has the highest allocation activates that combat strategy.
- **Hybrid Support:** Hybrid classes adapt forms and stances based on context and assigned group roles.
- **Battlegrounds:** Healing specs retain healing priorities in battlegrounds, layering battleground objectives and PvP strategies on top.
- **Dynamic Role Assignment:** In parties, players can set bot roles to adjust aggro generation, healing urgency, and positioning.

Players can use [`.bot role self`](../play/commands/roster-and-hiring.md#bot-role) to override runtime role inference. Without an override, role detection uses party composition, stance or form, and tanking equipment. If another tank already occupies the role, temporary stance or form changes do not promote a second tank.

---

## Shared Bot Behaviors

| Subsystem | Trigger Condition | Bot Action / Behavior | Configuration / Control |
| :--- | :--- | :--- | :--- |
| **Resting & Recovery** | Out-of-combat health or mana below recovery thresholds. | Sits down and consumes food and drink simultaneously. | See [Combat settings](../server/settings/combat.md). |
| **Buffs & Imbues** | Buff missing on self or party member. | Recasts class buffs, seals, auras, and weapon poisons. | Automatic out-of-combat maintenance. |
| **Targeting & Assist** | Tank or party leader engages hostile enemies. | Assists leader focus target and respects crowd control marks. | See [Party actions](../play/commands/party-actions.md). |
| **Finishing Runners** | Enemy mob flees below 30% health. | Ranged DPS finish within spell range; eligible melee close distance. | See detailed runner policy below. |
| **Healer Positioning** | Party member out of heal range or line of sight. | Closes distance toward the tank or injured party member. | Automatic positioning routine. |
| **Looting & Gathering** | Corpse or nearby gathering node available. | Approaches corpse, loots quest items, and rolls on items. | See [Party actions](../play/commands/party-actions.md). |

<details>
<summary>Runner finishing and healer positioning details</summary>

### Finishing Runners
When an enemy mob flees below 30% health (either flee state or flee-for-assistance):
- Ranged DPS finish the target within spell range.
- Warriors finish the runner if *Intercept* is available (in combat, Berserker Stance, rage available, off cooldown) within 25 yards.
- Other melee only attack runners already in melee reach.
- During `stay` holds, melee bots hold their position while ranged bots shoot within spell range.
- Healers and tanks do not chase runners.
- The active runner is never dropped for a fresh mob, and crowd-controlled targets are never chased.

### Healer Positioning
When a party member is out of heal range or line of sight:
- The healer moves toward that member. The default follow anchor is the tank when no member is injured.
- If another living healer already has that member in range and sight, the healer does not path to them unless the member is at or below critical health.
- Silenced, pacified, crowd-controlled, or out-of-mana healers do not satisfy the coverage check.
- Out of combat, healers only path to members below 90% health.

</details>
