---
id: server-settings-dungeons-raids
title: Dungeon & Raid Settings
category: server
summary: Configuration flags for encounter mechanics, hazard avoidance, raid tactics, and creature auras.
tags: [config, dungeons, raids, tactics, bosses]
relates_to:
  - play-dungeons-and-raids
  - play-dungeon-reference
---

# Dungeon & Raid Settings

These settings configure boss encounter mechanics, environmental hazard evasion, raid positioning, and creature aura avoidance. Settings go in `aiplayerbot.conf`; see [How configuration works](../configuration.md).

## Summary

| Setting | Default | What it does |
| :--- | :---: | :--- |
| [`AiPlayerbot.AvoidCreatureAuras`](#aiplayerbotavoidcreatureauras) | `1` | Steers bots away from permanent damaging creature auras and hazardous monsters. |
| [`AiPlayerbot.BombRunoutDistance`](#aiplayerbotbombrunoutdistance) | `30.0` | Distance in yards bomb debuff carriers run away from the raid center. |
| [`AiPlayerbot.EnableCustomRaidTactics`](#aiplayerbotenablecustomraidtactics) | `1` | Gates scripted encounter tactics and transition strategies for custom raids. |
| [`AiPlayerbot.HazardEvasionDistance`](#aiplayerbothazardevasiondistance) | `12.0` | Lateral evasion distance in yards used to step clear of ground hazards. |
| [`AiPlayerbot.OnyxiaWhelpsAndEggs`](#aiplayerbotonyxiawhelpsandeggs) | `1` | Prioritizes whelp clearing and egg avoidance during Onyxia phase two. |

## Settings

### `AiPlayerbot.AvoidCreatureAuras`
Directs bots to maintain safe distances from dangerous creatures emitting lethal proximity auras or explosions.
**Default:** `1` · **Values:** `0` off, `1` on · **Restart:** needed

<details>
<summary>How it works in detail</summary>

When enabled, bots actively steer clear of hazardous creatures that damage or kill players standing near them:
- **Maraudon Creeping Sludge (12222):** Emits a permanent poison aura. Bots step out sideways when within 8 yards, and pathfinding routes stay at least 9 yards away. Non-tank bots engage from a ~9.5-yard stand-off hold ring. If a bot cannot attack effectively for 3 seconds, it sets the target aside for 30 seconds to focus on other mobs. Group tanks are exempt from the sludge ring during combat so they can establish threat and reposition the mob.
- **Naxxramas Living Poison (16027):** Lethal floating ooze that detonates for massive damage within 4 yards. Because detonation is fatal, all bots (including tanks) step out 10 yards, path 11 yards away, and hold at an 11.5-yard stand-off ring. Living Poison cannot be selected or attacked; bots focus entirely on evasion.

Gap-closer abilities such as Charge, Intercept, and Feral Charge verify that the destination landing point is outside the hazard ring before executing. An explicit attack order on that target is never avoided, gap-closer-vetoed, or set aside. Positions are tracked via local proximity scans and periodic grid snapshots.

When set to `0`, bots treat hazardous creatures like standard mobs, chasing into melee range without avoiding dangerous proximity auras.

</details>

### `AiPlayerbot.BombRunoutDistance`
Specifies how far players carrying bomb debuffs must run away from the raid center before stopping.
**Default:** `30.0` · **Values:** positive float · **Restart:** needed

<details>
<summary>How it works in detail</summary>

In raid encounters featuring bomb or explosive dispersal mechanics (such as Baron Geddon Living Bomb), bots afflicted with the debuff disengage from combat.

The afflicted bot flees straight away from the center of the raid group until no party or raid member stands within this distance, then holds position until the debuff expires or detonates harmlessly.

</details>

### `AiPlayerbot.EnableCustomRaidTactics`
Master toggle for scripted encounter behaviors and phase transitions across custom raid instances.
**Default:** `1` · **Values:** `0` off, `1` on · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Enables automated raid tactical handling across custom raid encounters, including Emerald Sanctum (map 807), Lower Karazhan Halls (map 532), and Karazhan Crypt (map 800).

When enabled, bots recognize boss phase transitions, target priorities, and custom encounter scripts. When disabled, custom-raid tactics stay off; vanilla raid tactics are unaffected.

</details>

### `AiPlayerbot.HazardEvasionDistance`
Distance in yards bots move laterally to step out of stationary or ground-targeted environmental hazards.
**Default:** `12.0` · **Values:** positive float · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Step length when spreading/flanking: just past the trigger radius (`radius+1`), capped by this setting (default 12 yd).

Defines the spatial clearance used during raid spread and flank positioning when stepping clear of ground fire, poisonous clouds, or localized hazard fields.

</details>

### `AiPlayerbot.OnyxiaWhelpsAndEggs`
Enforces whelp target prioritization and egg cluster avoidance during the airborne phase of the Onyxia encounter.
**Default:** `1` · **Values:** `0` off, `1` on · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Active on map 249 (Onyxia's Lair) only while the `onyxia` fight strategy is active and the boss is airborne during phase two:
- Ranged damage bots (mages, warlocks, hunters, balance druids, and elemental shamans) automatically switch target to the nearest Onyxian Whelp within 40 yards and kill it before returning to the boss. Whelps located within 10 yards of unbroken eggs are skipped to prevent collateral hatching.
- All bots in the encounter back away toward the nearest cave entrance if they find themselves within 12 yards of unbroken Onyxia eggs.

An explicit attack order always takes precedence over automated whelp targeting. When set to `0`, bots use standard target selection and ignore egg proximity.

</details>
