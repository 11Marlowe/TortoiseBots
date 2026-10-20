---
id: play-known-limitations
title: Known Limitations & Quirks
category: play
summary: Authoritative list of current engine boundaries, blocked features, explicit non-goals, and gameplay quirks.
tags: [limitations, non-goals, quirks, engine, roadmap]
relates_to:
  - play-getting-started
  - play-dungeons-and-raids
  - server-bot-pool
---

# Known Limitations & Quirks

TortoiseBots strives to make companion bots behave like human players. However, certain technical boundaries, design trade-offs, and behavioral quirks exist. This page provides an authoritative summary of what bots cannot do yet, deliberate non-goals, and edge cases you may notice while playing.

## Blocked & Incomplete Features

- **Self-Botting (Playing Your Own Character as a Bot):** You cannot convert your currently logged-in character into an automated bot. The server architecture does not allow attaching native bot AI to an active client network session. Bots must be secondary characters logged in as headless sessions from your account or hired mercenaries.
- **Synthetic Auction Market:** While bots list and purchase items from their actual inventories, fully synthetic auction supply and synthetic buyers are optional features disabled by default. When enabled, synthetic items remain isolated from real player inventories.
- **Pre-Movement Loot Admission:** The server engine currently lacks an upfront query for game object loot rights. A bot may occasionally walk toward a gathered herb or mined vein before determining that another player is currently opening it or that it is empty.

## Explicit Non-Goals

TortoiseBots deliberately excludes certain features to maintain immersion, stability, and vanilla fidelity:

- **Autonomous Dungeon Clearing:** Autonomous bot pathfinding that clears entire dungeons without a human leader is an explicit non-goal. 5-player dungeons and raids are designed for a human player directing companion bots.
- **Expansion Content (TBC / WotLK):** No Death Knights, glyphs, vehicles, Outland/Northrend zones, or Arena ratings. Classes and mechanics strictly adhere to Vanilla 1.12 with Turtle WoW 1.18.1 additions.
- **Synchronous LLM in Combat or Movement:** Combat decisions, healing triage, interrupts, and movement never wait on external language model responses. The combat and reaction engine is entirely deterministic C++.
- **Multi-Tab Guild Vaults:** Turtle WoW 1.18.1 does not support WotLK-style multi-tab guild banks. Bot interactions with unsupported guild bank actions fail closed safely.

## Unstarted Roadmap Ideas

These features are recognized design goals on the project roadmap but have not yet been implemented:

- **Advanced PvP and Duel AI:** Bot combat logic specifically optimized for 1v1 duels and open-world player versus player skirmishes.
- **Alternative Core Porting:** Porting the TortoiseBots native module to other vanilla or classic expansion server cores.

## Tactical & Targeting Rules for Players

When adventuring with bots, understanding their targeting rules helps avoid unexpected pulls:

- **Pull Level Caps:** Autonomous pool bots below level 10 will only pull mobs up to 1 level above their own level. From level 10 upward, they can engage enemies up to 4 levels higher.
- **Pull Health and Mana Thresholds:** Masterless pool bots will not initiate a new pull when their health or mana is below medium thresholds ([`AiPlayerbot.MediumHealth`](../server/settings/combat.md#aiplayerbotmediumhealth), default 70%). However, if an enemy aggros on them first, they fight back immediately regardless of health.
- **Runner Finishing Policy:** When a humanoid enemy flees below 30% health, ranged DPS bots prioritize finishing the runner within spell range. Melee warriors will use *Intercept* to finish runners if in Berserker Stance; otherwise melee stay on nearby targets to avoid pulling adjacent packs.
- **Auto-Attack Engagement:** Bots automatically arm melee auto-attacks when entering melee range, even if their primary spec is ranged, ensuring they do not stand idle when out of mana or within melee distance.

## Known Quirks and Workarounds

| Quirk / Situation | What Happens | What You Should Do |
| :--- | :--- | :--- |
| **Line-of-Sight on Stairs** | On steep or multi-tiered stairs (e.g. Blackrock Depths), pathfinding can occasionally desync. | Use [`.bot summon <Name>`](commands/roster-and-hiring.md#bot-summon) to snap bots to your floor. |
| **Warlock Life Tap Risk** | Warlocks stop tapping below 50% health, but incoming burst damage can catch a low bot before heals land. | Set healer reaction urgency higher ([`AiPlayerbot.LowHealth`](../server/settings/combat.md#aiplayerbotlowhealth) = 65). |
| **Shaman Totem Churn** | Shamans may leave totems behind when moving through large rooms and re-drop them at new fight locations. | Normal behavior; Shaman mana recovers during out-of-combat drinking. |
| **Upkeep Buff Mana Floor** | Bots will not recast upkeep buffs out of combat if their mana is below 40% (or 70% for charged shields). | Let bots drink to full; buffing resumes automatically once mana recovers. |
| **Greater Group Buffs** | Group buffs are cast only when at least 3 party members lack both the group and single-target versions. | Normal behavior; single-target buffs cover solo missing party members. |
| **Target Out of Line of Sight** | Bots buff line-of-sight party members first before walking toward members hidden behind pillars. | Step into open line of sight so bots can see you to cast buffs. |
| **Duplicate Buff Protection** | When multiple bots can cast the same buff, they coordinate a short claim window to prevent double-casting. | Normal behavior; each member is covered once without wasting mana. |
| **Hidden GM Group Members** | A party member who is a hidden GM is intentionally skipped by bot buff and heal searches. | Run `.gm visible on` if testing companion bot spells on a GM character. |
| **Loot Timing vs Next Pull** | Bots check live combat state before opening corpses to avoid kneeling down next to incoming adds. | Clear the pack; bots will loot remaining corpses once combat ends. |
| **Loot Near Unpulled Packs** | Inside dungeons, bots skip looting corpses that lie within sight range of unpulled hostile packs. | Clear surrounding packs; skipped corpses remain queued for up to 3 minutes. |
| **Evading Enemies** | When an enemy enters evade mode and runs home, bots drop it from targeting immediately. | Wait for the mob to reach its spawn point and stop moving before re-engaging. |
| **Ghost Corpse Run Stalls** | If a ghost bot encounters blocked terrain on a corpse run, it hands off to the spirit healer. | If a companion ghost is stuck outside an instance, zone in and summon it. |
| **Gather Cast Interruption** | Gathering nodes requires standing still; bots pause follow movement while gathering ore or herbs. | Stand next to the gathering node until the bot finishes harvesting. |

<details>
<summary>How companion travel and instance safety are isolated</summary>

Companion bots under your command have their autonomous travel errands suppressed while grouped. They will not wander off to visit vendors, class trainers, or distant quest givers while you are leading them through an instance.

Inside dungeons and raids, companion bots also suppress autonomous gathering scans: they will not break formation to run across a room to mine an ore vein or pick a flower near unpulled packs. They only gather nodes when standing immediately beside them out of combat.

</details>
