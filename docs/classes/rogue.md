---
id: classes-rogue
title: Rogue Bot Guide
category: classes
summary: Complete guide to Rogue bot AI, stealth openers, combo point spenders, poison application, interrupts, and dungeon threat drops.
tags: [classes, rogue, dps, melee, stealth]
relates_to:
  - classes-overview
---

# Rogue Bot Guide

Rogues deliver exceptional single-target physical melee DPS, pre-combat crowd control (*Sap*), rapid interrupts (*Kick*), and defensive threat dumps (*Feint*, *Vanish*).

## Specs & roles

| Spec | Role | Primary Focus |
| :--- | :--- | :--- |
| **Combat** | Melee DPS | Sustained sword/mace DPS, Slice and Dice uptime, and Blade Flurry cleave. |
| **Assassination** | Melee DPS | Dagger strikes, poison synergy, Cold Blood burst, and high critical damage. |
| **Subtlety** | Melee DPS | High mobility, bleed scaling, Preparation resets, and opening burst. |

---

## What the bot does in combat

### Stealth & Openers
- Automatically enters *Stealth* out of combat.
- Manoeuvres behind targets to open with *Ambush* (with main-hand daggers) or *Garrote* (swords/maces).
- Uses *Cheap Shot* as a fallback opener or when lockdown stuns are required.
- **Pre-pull Sap:** When assigned a crowd control mark on an unpulled humanoid, a rogue following a human master stealths forward and casts *Sap* before combat begins. Can be toggled with [`.bot action sap [on|off] [bot]`](../play/commands/party-actions.md#bot-action-sap).

### Combo Points & Finisher Priority
- **Builders:** Uses *Sinister Strike* (Combat), *Backstab* (Assassination), or *Hemorrhage* (Subtlety).
- **Slice and Dice:** Combat prioritizes *Slice and Dice* above *Eviscerate*; Assassination and Subtlety prioritize *Eviscerate*.
- **Spenders:** At 4–5 combo points, casts *Eviscerate* or *Rupture*. Dumps banked combo points (1+) into *Eviscerate* if the target falls below 25% health.
- *Expose Armor* is withheld if the target already carries *Sunder Armor*.

### Cooldowns & Defensive Play
- **Combat:** Activates *Adrenaline Rush* and *Blade Flurry* during difficult fights. *Blade Flurry* is automatically cancelled if a crowd-controlled mob comes within 8 yards.
- **Assassination:** Holds *Cold Blood* until 4 or more combo points are banked to guarantee a critical finisher strike.
- **Subtlety:** Activates *Preparation* to reset cooldowns during drawn-out battles.
- **Defensives:** Triggers *Evasion* at low health, *Blind* to peel attackers, and *Vanish* at critical health (<20%) to drop threat. *Feint* is cast only when a living party tank or pet is present to hold aggro.

<details>
<summary>Dungeon threat drops in detail</summary>

When enabled via configuration, rogues in 5-man dungeons use the same threat-drop discipline as in raids:
- Casts *Feint* upon reaching medium threat (relative threat ≥ 60%).
- Casts *Vanish* upon reaching high threat (relative threat ≥ 80%), provided the tank is holding the target.
- Requires an active, living tank to prevent accidental resets in solo play.

</details>

---

## Turtle WoW custom spells

- **Surprise Attack (Spell ID 52511):** Unblockable combo-building strike integrated into the Combat rotation.
- **Noxious Assault (Spell ID 52714):** Assassination talent providing attack power and instant poison delivery.
- **Envenom (Spell ID 52531):** Finisher consuming Deadly Poison stacks for instant Nature damage and an attack speed bonus.
- **Shadow of Death & Mark for Death (Spell IDs 52710 / 52538):** Subtlety talents enhancing banked burst detonation and party attack power.

---

## Utility & interrupts

- **Poisons:** Automatically applies poisons based on context: Raid and generic combat use Instant Poison (main hand) and Deadly Poison (off hand); general PvE uses Instant Poison on both hands; PvP uses Mind-Numbing Poison (main hand) and Crippling Poison (off hand). Respects level requirements (Instant at level 20, Deadly at level 30).
- **Interrupts:** Casts *Kick* immediately to lock out enemy spell casts. Uses *Kidney Shot* as a stun fallback.
- **Self-Defense CC:** Uses *Gouge* at medium health and *Blind* at critical health to disorient attackers.
- **Hold Ground Opt-In:** When [PoolMeleeHoldGround](../server/settings/performance.md#aiplayerbotpoolmeleeholdground) is enabled, pool rogues stand their ground using *Evasion* and *Vanish* rather than fleeing when low on health.

---

## Talent builds

TortoiseBots provides pre-configured talent progressions at 5-level checkpoints (levels 10–60) tailored for Turtle WoW 1.18.1:

| Spec Name | Config ID | Role | 60 Allocation | Key Signatures & Synergies |
| :--- | :--- | :--- | :--- | :--- |
| **Combat** | `4.0` | Melee DPS | `20 / 31 / 0` | Dual Wield Spec, Blade Rush, Adrenaline Rush, Hack and Slash, Lethality dip. |
| **Assassination** | `4.1` | Melee DPS | `40 / 11 / 0` | Cold Blood, Seal Fate, Noxious Assault, Envenom, Vigor, Precision dip. |
| **Subtlety** | `4.2` | Melee DPS | `10 / 0 / 41` | Hemorrhage, Preparation, Mark for Death, Honor Among Thieves, Assassination dip. |

<details>
<summary>Leveling milestones and progression rationale</summary>

- **Combat (`4.0`):**
  - *Levels 10–30:* Takes *Opportunity* (5/5), *Precision* (5/5), *Improved Backstab* (3/3), *Improved Sprint* (2/2), and *Dual Wield Specialization* (30).
  - *Levels 30–40:* Takes *Surprise Attack* (1/1), *Hack and Slash* (2/2), *Blade Rush* (35), and *Adrenaline Rush* (40).
  - *Levels 40–60:* Dips into Assassination for *Malice* (5/5), *Ruthlessness* (3/3), *Relentless Strikes* (1/1), and *Lethality* (5/5).
- **Assassination (`4.1`):**
  - *Levels 10–35:* Takes *Malice* (5/5), *Ruthlessness* (3/3), *Murder* (2/2), *Relentless Strikes* (1/1), *Lethality* (5/5), and *Cold Blood* (35).
  - *Levels 35–50:* Takes *Efficient Poisons* (3/3), *Envenom* (40), *Seal Fate* (45), and *Noxious Assault* (50).
  - *Levels 50–60:* Dips into Combat (*Opportunity* 5/5 + *Precision* 3/5).
- **Subtlety (`4.2`):**
  - *Levels 10–30:* Takes *Camouflage* (5/5), *Serrated Blades* (3/3), *Initiative* (3/3), *Ghostly Strike* (3/3), and *Hemorrhage* (30).
  - *Levels 30–45:* Takes *Preparation* (35), *Shadow of Death* (1/1), *Bloody Mess* (2/2), and *Mark for Death* (45).
  - *Levels 45–60:* Takes *Honor Among Thieves* (2/2) and dips 10 points into Assassination (*Malice* 5/5 + *Ruthlessness* 3/3 + *Murder* 2/2).

</details>

---

## Class settings

- [`AiPlayerbot.RogueDungeonThreatDrops`](../server/settings/class-behaviour.md#aiplayerbotroguedungeonthreatdrops): Enables Feint on medium threat and Vanish on high threat in 5-man dungeons.
