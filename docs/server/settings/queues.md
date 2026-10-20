---
id: server-settings-queues
title: Queue settings (LFT & battlegrounds)
category: server
summary: Configuration settings for Looking-For-Trouble (LFT) dungeon autofill and battleground auto-queue systems.
tags: [config, queues, lft, dungeon-finder, battlegrounds, pvp]
relates_to:
  - world-queues
  - server-settings-performance
---

# Queue Settings (LFT & Battlegrounds)

These settings configure automated Looking-For-Trouble (LFT) dungeon finder autofill and player-versus-player (PvP) battleground queue services. Settings go in `aiplayerbot.conf`; see [How configuration works](../configuration.md).

For general matchmaking behavior, dungeon portal pathing, and battleground mechanics, see [Queues & Matchmaking](../../world/queues.md).

## Summary

| Setting | Default | What it does |
| :--- | :---: | :--- |
| [`AiPlayerbot.RandomBotLftEnabled`](#aiplayerbotrandombotlftenabled) | `1` | Enables random bots filling vacant slots in human LFT dungeon queues. |
| [`AiPlayerbot.RandomBotLftAllowRoleBorrow`](#aiplayerbotrandombotlftallowroleborrow) | `0` | Allows LFT to borrow and respec bots into roles they do not naturally fit. |
| [`AiPlayerbot.RandomBotLftUpdateInterval`](#aiplayerbotrandombotlftupdateinterval) | `15000` | Milliseconds between evaluation passes of the LFT autofill service. |
| [`AiPlayerbot.RandomBotLftMaxFillsPerInterval`](#aiplayerbotrandombotlftmaxfillsperinterval) | `1` | Maximum dungeon group formations processed per autofill interval pass. |
| [`AiPlayerbot.RandomBotBgEnabled`](#aiplayerbotrandombotbgenabled) | `1` | Injects bots into Warsong Gulch, Arathi Basin, and Alterac Valley when players queue. |
| [`AiPlayerbot.RandomBotBgAutonomous`](#aiplayerbotrandombotbgautonomous) | `1` | Seeds bot-only Warsong Gulch matches when no human players are in queue. |
| [`AiPlayerbot.RandomBotBgAutonomousMaxInstances`](#aiplayerbotrandombotbgautonomousmaxinstances) | `1` | Maximum concurrent bot-only battleground instances allowed per level bracket. |
| [`AiPlayerbot.RandomBotBgQueueInterval`](#aiplayerbotrandombotbgqueueinterval) | `30000` | Milliseconds between autonomous battleground seeder queue evaluation passes. |
| [`AiPlayerbot.RandomBotBgMaxQueuePerInterval`](#aiplayerbotrandombotbgmaxqueueperinterval) | `1` | Number of bots queued into autonomous battlegrounds per seeder check. |

## Settings

### `AiPlayerbot.RandomBotLftEnabled`
Enables automatic bot backfill for players queued in the Looking-For-Trouble (LFT) tool.
**Default:** `1` · **Values:** `0` off, `1` on · **Restart:** needed

<details>
<summary>How it works in detail</summary>

When human players queue for dungeons in the LFT tool and wait for missing roles (such as a Tank or Healer), eligible idle random bots in the world are selected to fill the empty party slots.

Fill is strictly demand-driven: bots never initiate dungeon finder queues on their own without a waiting human player. When a group forms, bots automatically accept the dungeon invite.

There is no automatic teleport into the dungeon. The formed party walks to the instance portal together, so group leaders can summon stragglers using `.bot summon` if needed. Your own party bots always queue with you and auto-accept dungeon invitations regardless of this setting.

</details>

### `AiPlayerbot.RandomBotLftAllowRoleBorrow`
Allows the LFT autofill service to borrow and respec bots into missing group roles.
**Default:** `0` · **Values:** `0` strict spec matching, `1` allow borrowing · **Restart:** needed

<details>
<summary>How it works in detail</summary>

By default (`0`), role assignment is fail-closed based on each bot's natural talent specialization and equipment. A tank slot requires a dedicated tank with a shield equipped, a druid tank requires bear form capabilities, and a healer requires healing talents and spells. If no matching bot is available, the slot remains open.

Setting this to `1` allows the matcher to select a bot and reconfigure (respec) it into the missing role via forced role + talent rebuild.

</details>

### `AiPlayerbot.RandomBotLftUpdateInterval`
Interval in milliseconds between LFT queue evaluation passes.
**Default:** `15000` · **Values:** milliseconds · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Defines how often the LFT autofill service scans the matchmaking queue for waiting players needing bot companions.

</details>

### `AiPlayerbot.RandomBotLftMaxFillsPerInterval`
Maximum number of dungeon groups formed per autofill service tick.
**Default:** `1` · **Values:** integer count · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Limits how many group proposals are processed within a single update interval, spreading character logins and strategy reconfigurations smoothly across time.

</details>

### `AiPlayerbot.RandomBotBgEnabled`
Enables automatic bot matchmaking for battleground PvP queues when players queue.
**Default:** `1` · **Values:** `0` off, `1` on · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Monitors PvP queues for Warsong Gulch (WSG), Arathi Basin (AB), and Alterac Valley (AV). When real human players queue, random bots of the matching level bracket queue on both factions to meet the core's minimum player requirement plus one.

This launches battleground matches within seconds even on low-population private servers. When a match concludes, bots leave within approximately 10 seconds rather than waiting for instance shutdown timers.

Your own party bots can queue alongside you via "Join as Group" at battlemasters (supported for WSG and AB; AV requires solo queues in the core) and will auto-accept match invites regardless of this toggle.

</details>

### `AiPlayerbot.RandomBotBgAutonomous`
Allows pool bots to initiate bot-only Warsong Gulch matches when no human players are queued.
**Default:** `1` · **Values:** `0` off, `1` on · **Restart:** needed

<details>
<summary>How it works in detail</summary>

When no human players are waiting in any PvP queue, the system seeds 10v10 Warsong Gulch matches in the most populated level bracket so bots have competitive matches of their own.

Human player demand always takes precedence: the autonomous seeder halts immediately whenever a real human queues for any battleground. This feature requires `AiPlayerbot.RandomBotBgEnabled = 1` to operate.

</details>

### `AiPlayerbot.RandomBotBgAutonomousMaxInstances`
Limits the number of concurrent autonomous bot-only battlegrounds active per level bracket.
**Default:** `1` · **Values:** integers (0..2) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Caps the number of simultaneous bot-only matches running per level bracket. The default setting of 1 limits each bracket to a single active 10v10 match, which is optimal for average hardware. Raise to 2 only on high-performance dedicated servers.

</details>

### `AiPlayerbot.RandomBotBgQueueInterval`
Interval in milliseconds between queue checks for the autonomous battleground seeder.
**Default:** `30000` · **Values:** milliseconds · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Defines how frequently the autonomous seeder checks whether an additional bot can be queued into an unfilled bot-only match.

Human demand is checked every 5 s (up to 10 bots per check); the interval and budget pace the autonomous seeder; setting `AiPlayerbot.RandomBotBgMaxQueuePerInterval = 0` disables both paths.

</details>

### `AiPlayerbot.RandomBotBgMaxQueuePerInterval`
Number of bots queued per interval check by the autonomous battleground seeder.
**Default:** `1` · **Values:** integer count (0 disables both autonomous and human demand queues) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Controls the pace at which bots are queued into forming autonomous battlegrounds, preventing sudden server-tick spikes from batch character transfers.

Setting this to `0` disables both autonomous seeding and human demand backfill.

</details>
