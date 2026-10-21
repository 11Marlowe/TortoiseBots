---
id: classes-warrior
title: Warrior Bot Guide
category: classes
summary: Complete guide to Warrior bot AI, stance dancing, tanking rotations, melee DPS, shouts, and Turtle WoW custom abilities.
tags: [classes, warrior, tank, dps, melee]
relates_to:
  - classes-overview
---

# Warrior Bot Guide

Warriors serve as primary dungeon tanks or powerful melee DPS, managing dynamic stance switches, rage expenditure, interrupt timing, and defensive cooldowns.

## Specs & roles

| Spec | Role | Primary Focus |
| :--- | :--- | :--- |
| **Protection** | Tank | Defensive Stance holding, Sunder Armor stacking, Shield Slam, and peel taunts. |
| **Arms** | Melee DPS | Battle Stance two-handed strikes, Mortal Strike, Overpower procs, and cleaves. |
| **Fury** | Melee DPS | Berserker Stance dual-wielding, Bloodthirst, Whirlwind, and Execute bursts. |

---

## What the bot does in combat

### Protection (Tank)
- **Pull & Engagement:** Opens with *Charge* on distant targets before swapping to Defensive Stance. Ranged pulls using bows, guns, or thrown weapons fire when line of sight or distance requires it.
- **Threat Generation:**
  - Maintains *Shield Block* on cooldown to activate *Revenge*.
  - Stacks *Sunder Armor* to 5 stacks, refreshing before expiry.
  - Casts *Shield Slam* on cooldown (or procced by Improved Shield Slam).
  - Uses *Thunder Clap* (usable in Defensive Stance on Turtle) against 2 or more melee attackers.
- **Mitigation & Taunts:**
  - Activates *Shield Wall* early at low health (20-50%) and holds *Last Stand* for critical health emergencies (<20%).
  - Taunts with *Taunt* immediately if any attacker peels onto non-tank party members.
  - Intervenes to intercept physical attacks aimed at focused group members.

### Arms & Fury (Melee DPS)
- **Arms:**
  - Fights in Battle Stance, maintaining *Rend*, casting *Mortal Strike* on cooldown and *Overpower* on dodges.
  - Cleaves packs with *Sweeping Strikes* and *Whirlwind* (dancing to Berserker Stance for Whirlwind).
  - Dumps rage with *Master Strike* and uses *Heroic Strike* once rage reaches 40.
- **Fury:**
  - Fights dual-wielding in Berserker Stance, centering rage spend on *Bloodthirst* and *Whirlwind*.
  - Activates *Death Wish*, *Recklessness*, and *Retaliation* during encounter burn phases.
- **Execute Phase:** When targets drop below 20% health, *Execute* becomes top priority, consuming all available rage.
- **Finishing Runners:** Warriors in Berserker Stance with *Intercept* off cooldown charge after fleeing enemies within 25 yards per [shared runner rules](overview.md#finishing-runners).

---

## Turtle WoW custom spells

- **Master Strike (Spell ID 54023):** Weapon-dispatched burst strike (30s cooldown, 20 rage, per game data) wired into Arms and Fury rotations as a high-damage physical nuke.
- **Defensive Tactics (Spell ID 51606):** Passive Protection talent granting threat aura benefits while wielding a shield in Battle or Berserker Stance.

---

## Utility & interrupts

- **Interrupts:**
  - *Protection:* Relies strictly on *Shield Bash* to remain in Defensive Stance.
  - *Fury:* Uses *Pummel* (in Berserker Stance).
  - *Arms:* Uses *Pummel* first, falling back to *Shield Bash* without stance dancing.
  - Interrupts only fire on active cast bars or channels; delayed instants never trigger interrupts.
- **Shouts:** Maintains *Battle Shout* on party members (skipping if an equal or stronger *Blessing of Might* is active). Protection maintains *Demoralizing Shout*. *Intimidating Shout* is restricted to open-world emergencies and never used in dungeons.
- **Rage hygiene:** The *Disarm*, *Bloodrage* and *Battle Shout* rows were gated on 2026-10-21 after a trace audit found Protection tanks under 20 rage in 81 % of combat samples while these rows were tried, and failed as impossible, on almost every tick: *Disarm* now needs 20 rage, a ready spell and a target with a main-hand weapon, *Bloodrage* needs its cooldown to be over as well as low rage and medium health, and *Battle Shout* needs 10 rage (rules in `WarriorRagePolicy.h`).
- **Hold Ground Opt-In:** When [PoolMeleeHoldGround](../server/settings/performance.md#aiplayerbotpoolmeleeholdground) is enabled, pool warriors fight through low-health emergencies using *Shield Wall* and *Last Stand* rather than fleeing.

---

## Talent builds

TortoiseBots provides pre-configured talent progressions at 5-level checkpoints (levels 10–60) tailored for Turtle WoW 1.18.1:

| Spec Name | Config ID | Role | 60 Allocation | Key Signatures & Synergies |
| :--- | :--- | :--- | :--- | :--- |
| **Fury** | `1.0` | Melee DPS | `17 / 34 / 0` | Death Wish, Flurry, Bloodthirst, Ravager (-3s Whirlwind CD), 17 Arms dip for Deep Wounds + Impale. |
| **Protection** | `1.1` | Tank | `7 / 0 / 44` | Shield Slam, Concussion Blow, Gag Order, Defensive Tactics, Tactical Mastery dip. |
| **Arms** | `1.2` | Melee DPS | `40 / 11 / 0` | Sweeping Strikes, Mortal Strike, Master Strike, Boundless Anger, 11 Fury dip. |

<details>
<summary>Leveling milestones and progression rationale</summary>

- **Fury (`1.0`):**
  - *Levels 10–30:* Takes *Cruelty* (5/5), *Dual Wield Specialization* (5/5), *Unbridled Wrath* (5/5), *Piercing Howl* (1/1), and *Death Wish* (30).
  - *Levels 30–40:* Takes *Flurry* (5/5) and *Bloodthirst* (40).
  - *Levels 40–60:* Takes *Ravager* (3/3) and *Improved Execute* (2/2), then dips into Arms for *Tactical Mastery* (5/5), *Improved Overpower* (2/2), *Deep Wounds* (3/3), and *Impale* (2/2).
- **Protection (`1.1`):**
  - *Levels 10–30:* Takes *Shield Specialization* (5/5), *Anticipation* (3/3), *Toughness* (5/5), *Last Stand* (20), *Improved Taunt* (2/2), and *Improved Revenge* (3/3).
  - *Levels 30–50:* Takes *Defiance* (5/5), *Gag Order* (2/2), *Shield Slam* (40), *Improved Shield Slam* (2/2), *Reprisal* (2/2), and *Concussion Blow* (50).
  - *Levels 50–60:* Takes *Defensive Tactics* (3/3) and 7 points in Arms (*Tactical Mastery* + *Improved Heroic Strike*).
- **Arms (`1.2`):**
  - *Levels 10–40:* Takes *Deflection* (5/5), *Deep Wounds* (3/3), *Two-Handed Weapon Specialization* (3/3), *Impale* (2/2), *Sweeping Strikes* (30), and *Mortal Strike* (40).
  - *Levels 40–60:* Takes *Boundless Anger* (3/3) and *Master of Arms* (5/5), followed by an 11-point Fury dip into *Cruelty* (5/5) and *Piercing Howl* (1/1).

</details>

---

## Class settings

There are no class-specific configuration flags for Warriors. General combat, tanking, and pull settings are configured in [Combat settings](../server/settings/combat.md).
