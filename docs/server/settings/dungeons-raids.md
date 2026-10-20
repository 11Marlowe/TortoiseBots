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
| [`AiPlayerbot.BombRunoutDistance`](#aiplayerbotbombrunoutdistance) | `30.0` | Distance in yards bomb debuff carriers run clear of the living group. |
| [`AiPlayerbot.BotAreaTriggers`](#aiplayerbotbotareatriggers) | *(empty)* | Audited allowlist of scripted area triggers a bot party may fire (off when empty; opt in with `962`). |
| [`AiPlayerbot.BotGossipNpcs`](#aiplayerbotbotgossipnpcs) | *(empty)* | Creature entries `.bot cmd gossip` may talk to (empty: every gossip order is refused). |
| [`AiPlayerbot.DungeonRunMaxMinutes`](#aiplayerbotdungeonrunmaxminutes) | `180` | Time limit of a run leader run (`0` = none). |
| [`AiPlayerbot.DungeonRunRezHold`](#aiplayerbotdungeonrunrezhold) | `0` | The run leader's regroup waits for a living resurrector to raise the fallen (and for its mana) before it revives the party at the spirit healer. |
| [`AiPlayerbot.DungeonSelfClear`](#aiplayerbotdungeonselfclear) | `0` | Lets a bound remote commander start the opt-in run leader (`.bot run`). |
| [`AiPlayerbot.DungeonTactics`](#aiplayerbotdungeontactics) | `0` | Applies the per-boss tactics table (`ai_boss_tactic`) to every bot, not only commanded parties. |
| [`AiPlayerbot.EnableCustomRaidTactics`](#aiplayerbotenablecustomraidtactics) | `1` | Gates scripted encounter tactics and transition strategies for custom raids. |
| [`AiPlayerbot.HazardEvasionDistance`](#aiplayerbothazardevasiondistance) | `12.0` | Cap in yards on the step bots take when spreading for spacing. |
| [`AiPlayerbot.OnyxiaWhelpsAndEggs`](#aiplayerbotonyxiawhelpsandeggs) | `1` | Prioritizes whelp clearing and egg avoidance during Onyxia phase two. |

## Settings

### `AiPlayerbot.AvoidCreatureAuras`
Directs bots to maintain safe distances from dangerous creatures emitting lethal proximity auras or explosions.
**Default:** `1` · **Values:** `0` off, `1` on · **Restart:** needed

<details>
<summary>How it works in detail</summary>

When enabled, bots actively steer clear of hazardous creatures that damage or kill players standing near them:
- **Maraudon Creeping Sludge (12222):** Emits a permanent poison aura. Bots step out sideways when within 8 yards, and pathfinding routes stay at least 9 yards away. Non-tank bots engage from a ~9.5-yard stand-off hold ring. If a bot cannot attack effectively for 3 seconds, it sets the target aside for 30 seconds to focus on other mobs. Group tanks are exempt from the sludge ring during combat so they can hold mobs inside it.
- **Naxxramas Living Poison (16027):** Lethal floating ooze that detonates for massive damage within 5 yards. Because detonation is fatal, all bots (including tanks) step out 10 yards, path 11 yards away, and hold at an 11.5-yard stand-off ring. Living Poison cannot be selected or attacked; bots focus entirely on evasion.

Gap-closer abilities such as Charge, Intercept, and Feral Charge verify that the destination landing point is outside the hazard ring before executing. An explicit attack order on that target is never avoided, gap-closer-vetoed, or set aside. Positions are tracked via local proximity scans and periodic grid snapshots.

When set to `0`, bots treat hazardous creatures like standard mobs, chasing into melee range without avoiding dangerous proximity auras.

</details>

### `AiPlayerbot.BombRunoutDistance`
Specifies how far bots carrying bomb debuffs must run from the center of the living group before stopping.
**Default:** `30.0` · **Values:** positive float · **Restart:** needed

<details>
<summary>How it works in detail</summary>

In raid encounters featuring bomb or explosive dispersal mechanics (such as Baron Geddon Living Bomb), bots carrying the debuff run clear of the group.

The afflicted bot runs straight away from the center of the living group until no party or raid member stands within this distance, then holds position until the debuff ends.

</details>

### `AiPlayerbot.BotAreaTriggers`
Audited allowlist of scripted area triggers that a bot in a commanded or player-led party runs when it walks into them.
**Default:** *(empty)* · **Values:** comma-separated area trigger ids; empty = off (the default); recommended `962` · **Restart:** needed

<details>
<summary>How it works in detail</summary>

**Scripted area triggers for bots (audited allowlist, off by default):** a bot has no game client, so it never reports walking into an area trigger and a trigger script never ran for it (Zul'Farrak's Witch Doctor Zum'rah stayed friendly before an all-bot party). A living bot in a group inside a dungeon runs the script of a listed trigger it walks into, once per entry, as a player's client would, but only when its party is bound to a remote commander (every run-leader run is) or has a human member within 40 yd, so a follower running ahead of its player cannot start an event alone. Raids are left out until bots are tested there. Instance entrances and exits, taverns and battleground entrances never fire. Each bot tests its own map's listed triggers twice a second, no world scan. Comma-separated ids; empty (the default) turns it off, as new automation ships off. Opt in with `AiPlayerbot.BotAreaTriggers = 962`. Audited ids: **962** (Zul'Farrak, `at_zumrah`): if Witch Doctor Zum'rah is alive within 30 yd he turns hostile, yells and his encounter is marked in progress; nothing else. Not every trigger script is safe to start from a bot (`at_ring_of_law` starts the Blackrock Depths arena, the UBRS stadium trigger starts that event, `at_shadowforge_bridge` spawns guards): read a trigger's script and document what it does here before adding its id.

</details>

### `AiPlayerbot.BotGossipNpcs`
Allowlist of creature entries the `.bot cmd gossip` order may talk to (issue #1012).
**Default:** *(empty)* · **Values:** comma-separated creature entries; empty = every gossip order refused · **Restart:** needed

<details>
<summary>How it works in detail</summary>

**Bot gossip allowlist (issue #1012):** comma-separated creature entries that `.bot cmd gossip` may talk to. A gossip option can do anything its script says (take money, give items, teleport), so list only NPCs whose scripts you have read. Empty: every gossip order is refused with `not-allowed` before anything is sent to the NPC. Audited: `7604` Sergeant Bly (starts his fight after the Zul'Farrak pyramid event) and `7607` Weegli Blastfuse (blows the End Door after it), so `7607,7604` opens both. See [`.bot cmd gossip`](../../play/commands/commander.md#bot-cmd-gossip).

</details>

### `AiPlayerbot.DungeonRunMaxMinutes`
How long a run leader run (`.bot run`) may last before it ends by itself.
**Default:** `180` · **Values:** minutes; `0` = no limit · **Restart:** needed

<details>
<summary>How it works in detail</summary>

**Run leader time limit:** a `.bot run` that has lasted this many minutes since `run start` (a pause counts) ends with the status note `time limit reached (3h)`, the bots staying bound. `0` = no limit. Only read while `AiPlayerbot.DungeonSelfClear = 1`. See [Dungeon Run Leader](../../play/dungeon-run-leader.md).

</details>

### `AiPlayerbot.DungeonRunRezHold`
Makes the run leader's regroup wait for a living priest, paladin or shaman to resurrect the fallen where they lie, instead of a fixed 15 s.
**Default:** `0` · **Values:** `0` the old fixed wait, `1` the rez hold · **Restart:** needed

<details>
<summary>How it works in detail</summary>

**Rez hold:** only read while `AiPlayerbot.DungeonSelfClear = 1`. With `0` the regroup behaves as before: when a priest, paladin or shaman is out of combat it waits 15 s for a resurrection, then sends `cmd revive party`. With `1` the run asks the rez hold (`runtime/RezHoldPolicy.h`) every second while someone is dead. It only **holds**: it sends the resurrector no order (the bots' own AI decides who raises whom, and a resurrector short of mana drinks by itself). It **holds** up to 60 s from the moment a resurrector could act (plus 12 s for each body beyond the resurrectors, at most 120 s, and a resurrection being cast may finish: 12 s more, once), **waits for mana** while the only resurrectors are short of their spell's cost (90 s in all), and sends `cmd revive party` at once when no living bot can resurrect, when every body lies more than 80 yd from the party, or when the time is up. The 240 s the dead get to be up after `revive party` starts when the hold gives up. A druid's Rebirth counts only when no priest, paladin or shaman is alive on the map within sight, and only off cooldown with its seed in the bags. Everyone dead is a wipe: the old wipe recovery takes it. See [Dungeon Run Leader](../../play/dungeon-run-leader.md). Derived from jrad7/mod-dungeon-clear (AGPL-3.0-or-later).

</details>

### `AiPlayerbot.DungeonSelfClear`
Allows the opt-in run leader: a bound remote commander's party clears a stored dungeon plan by itself.
**Default:** `0` · **Values:** `0` off (every `.bot run` refused), `1` on (needs `AiPlayerbot.RemoteCommander = 1`) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

**Dungeon self-clear (run leader):** with `1` (and `AiPlayerbot.RemoteCommander = 1`) a bound remote commander may start `.bot run start [plan]`: its party walks a dungeon plan from the world tables `ai_dungeon_plan`/`ai_dungeon_leg`/`ai_dungeon_step` and pulls, fights, rests, loots and opens doors by itself, using the commander's own `.bot cmd` orders. With 3 or more dead it regroups by itself once the fight is over (the safety rules are in the run leader page); it pauses on any player order and at every wait's ceiling. Bots then clear without a player giving each order, so it is off by default; with `0` every `.bot run` is refused. See [Dungeon Run Leader](../../play/dungeon-run-leader.md).

</details>

### `AiPlayerbot.DungeonTactics`
Applies the per-boss rows of the world table `ai_boss_tactic` (interrupt, kill first, stay behind) while bots fight that boss.
**Default:** `0` · **Values:** `0` only parties a remote commander has bound, `1` every bot on a map with rows · **Restart:** needed

<details>
<summary>How it works in detail</summary>

**Dungeon boss tactics:** applies the per-boss rows of the world table `ai_boss_tactic` while bots fight that boss: interrupt a listed spell (*Pummel*, *Shield Bash*, *Kick*, *Counterspell*, *Earth Shock*), kill a listed add before anything else and in a listed order (DPS only, never tanks or healers), and stay out of the front of a boss with a frontal attack (everyone but the bot the boss attacks). Uldaman up to Ironaya and the Obsidian Sentinel so far; see [Boss Tactics](../../play/boss-tactics.md). It changes how bots play, so it is off by default: with `0` only bots in a group a remote commander has bound get it, with `1` every bot on a map that has rows does. An explicit `attack` order from a player always wins.

</details>

### `AiPlayerbot.EnableCustomRaidTactics`
Master toggle for scripted encounter tactics across custom raid instances.
**Default:** `1` · **Values:** `0` off, `1` on · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Enables automated raid tactical handling across custom raid encounters, including Emerald Sanctum (map 807), Lower Karazhan (map 532), and Karazhan Crypt (map 800).

When enabled, entering one of those maps swaps in its strategy with that raid's encounter scripts. When disabled, custom-raid tactics stay off; vanilla raid tactics are unaffected.

</details>

### `AiPlayerbot.HazardEvasionDistance`
Cap in yards on the step bots take when spreading for spacing.
**Default:** `12.0` · **Values:** positive float · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Step length when spreading: just past the trigger radius (`radius+1`), capped by this setting (default 12 yd).

It caps the spread step used for raid spacing; a longer step would overshoot out of melee and ping-pong back.

</details>

### `AiPlayerbot.OnyxiaWhelpsAndEggs`
Enforces whelp target prioritization and egg cluster avoidance during the airborne phase of the Onyxia encounter.
**Default:** `1` · **Values:** `0` off, `1` on · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Active on map 249 (Onyxia's Lair) only while the `onyxia` fight strategy is active and the boss is airborne during phase two:
- Ranged damage bots (mages, warlocks, hunters, and ranged shaman and druid specs; never tanks or healers) automatically switch target to the nearest Onyxian Whelp within 40 yards and kill it before returning to the boss. Whelps located within 10 yards of unbroken eggs are skipped to prevent collateral hatching.
- All bots in the encounter back away toward the nearest cave mouth (`-10,-180` north, `-16,-250` south) if they find themselves within 12 yards of unbroken Onyxia eggs.

An explicit attack order always takes precedence over automated whelp targeting. When set to `0`, bots use standard target selection and ignore egg proximity.

</details>
