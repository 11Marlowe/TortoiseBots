---
id: world-queues
title: Dungeon & battleground queues
category: world
summary: How bots participate in Looking-For-Trouble dungeon matchmaking and PvP battlegrounds.
tags: [world, queues, lft, battlegrounds, pvp, dungeons]
relates_to:
  - world-living-world
  - play-dungeons-and-raids
  - server-settings-queues
---

# Dungeon & battleground queues

This page explains how bots fill dungeon parties through Looking-For-Trouble (LFT) and participate in player-versus-player battlegrounds. Queue settings and intervals are configured in `aiplayerbot.conf`; see [Queue & matchmaking settings](../server/settings/queues.md).

## Looking-For-Trouble (LFT) dungeon autofill

The Looking-For-Trouble autofill service monitors dungeon queue requests and backfills missing roles with eligible open-world bots:

| Feature | Behavior |
| :--- | :--- |
| **Human demand gating** | Random bots only queue when real players are waiting in LFT; bots never queue autonomously. |
| **Role composition** | Autofill provides the exact missing roles (Tank, Healer, DPS) needed to form a complete 5-player party. |
| **Strict spec matching** | Tanks must equip a shield (or Bear Form for Druids); Healers must know core healing spells. |
| **Party travel** | Formed groups walk to the dungeon portal together; there is no automatic teleport or summon. |

<details>
<summary>Demand accounting & role ratios in detail</summary>

LFT autofill calculates missing roles based on the waiting party configuration:
- A solo **Tank** receives a Healer and three DPS bots.
- A solo **Healer** receives a Tank and three DPS bots.
- A solo **DPS** receives a Tank, a Healer, and two DPS bots.
- Groups of 2 to 4 players receive only the specific roles missing from their party.

Player role signups match standard core logic: multi-role selections take the first available vacant slot in order (Tank, Healer, DPS). If a queued player party fits no valid slot combination, the fill service pauses until the queue state resolves rather than stranding bots.

</details>

<details>
<summary>Strict role validation & spec borrowing in detail</summary>

Role matching is fail-closed based on each bot's talent spec and active equipment:
- A **Tank** slot requires a tanking specialization with a shield equipped. The bot equips the best shield from its bags before queueing. Druid tanks must have Bear Form.
- A **Healer** slot requires a healing specialization with core healing spells trained.
- If an eligible bot cannot be found, the slot remains empty with a logged reason rather than assigning an unsuitable spec.

Administrators can optionally allow bots to respec into missing roles using [`AiPlayerbot.RandomBotLftAllowRoleBorrow`](../server/settings/queues.md#aiplayerbotrandombotlftallowroleborrow) (default 0). When a bot whose default kit differs from its queued role is adopted (such as a Feral Druid queued as a Tank), the engine assigns the appropriate role kit (Bear kit instead of Cat kit).

</details>

<details>
<summary>Dungeon portal travel & entry in detail</summary>

Once a group forms, bots accept the dungeon invite and adopt the party master. Bots path toward the dungeon portal on foot with their party.

There is no automatic dungeon teleportation or summoning. Players must guide their group to the instance portal or use [`.bot summon`](../play/commands/roster-and-hiring.md#bot-summon) to bring straggling bots directly to the entrance.

</details>

### LFT with your own party bots
When you queue for LFT with owned alts or hired companions in your party:
- The core automatically answers the role check for your party bots based on your explicit [`.bot role`](../play/commands/roster-and-hiring.md#bot-role) setting, active combat strategies, or automatic spec and gear evaluation.
- Your party bots automatically accept dungeon offers on the fill service tick cadence, preventing the 90-second offer timeout from expiring.
- Party auto-accept operates whether [`AiPlayerbot.RandomBotLftEnabled`](../server/settings/queues.md#aiplayerbotrandombotlftenabled) is enabled or disabled.

## Battleground auto-queue (human demand)

The battleground service populates PvP matches so players can experience active battlegrounds:

- **Supported battlegrounds:** Warsong Gulch (WSG), Arathi Basin (AB), and Alterac Valley (AV).
- **Demand-driven backfill:** When human players enter a battleground queue, the module queues level-appropriate random bots on both factions to reach match minimums and launch the game.
- **Match launch threshold:** The system fills each faction to the match minimum plus one. This launches matches promptly without accumulating surplus bots that could trigger a separate, unwanted bot-only match.
- **Prompt exit:** When a battleground match concludes, bots leave the instance within approximately 10 seconds rather than waiting for the server instance shutdown timer.
- **Enabled by default:** Controlled by [`AiPlayerbot.RandomBotBgEnabled`](../server/settings/queues.md#aiplayerbotrandombotbgenabled) (default 1).

<details>
<summary>Battleground objectives & map mechanics in detail</summary>

Bots run dedicated objective strategies inside battlegrounds:
- **Warsong Gulch:** Bots carry the enemy flag, escort friendly flag carriers, and defend base capture points.
- **Arathi Basin:** Bots capture neutral or enemy banners within reach, ignoring friendly banners. Banner interactions are rate-limited to once every 4 seconds.
- **Alterac Valley:** Bots navigate toward 17 fixed map waypoints (`AV_*` locations), coordinating attacks on graveyards, towers, bunkers, and commanders.
- **Healer preservation:** Healers (Holy/Discipline Priests, Restoration Shamans/Druids, Holy Paladins) retain their healing talent specs and priorities in battlegrounds. General DPS strategy directives cannot override healer combat roles in PvP.

</details>

## Autonomous bot-only matches

To keep battlegrounds lively when no human players are queued, pool bots can organize self-contained matches:

- **Autonomous seeding:** When queue demand from real players is zero, pool bots seed their own Warsong Gulch matches in the most populated level bracket.
- **Player priority:** Human queue demand takes absolute priority. Autonomous seeding pauses instantly whenever a real player queues for any battleground.
- **Instance concurrency:** Autonomous matches are restricted to solo pool bots, accumulating in single-bot batches until 10v10 is reached. Concurrency is capped by default to one active match per bracket.
- **Settings:** Governed by [`AiPlayerbot.RandomBotBgAutonomous`](../server/settings/queues.md#aiplayerbotrandombotbgautonomous) (default 1) and [`AiPlayerbot.RandomBotBgAutonomousMaxInstances`](../server/settings/queues.md#aiplayerbotrandombotbgautonomousmaxinstances) (default 1). Requires `RandomBotBgEnabled = 1`.

## Battlegrounds with your own party bots

Players can queue for battlegrounds alongside their owned alts and hired companions:

- **Join as Group:** At the battlemaster, select **Join as Group**. Party bots pass group qualification checks (matching faction, present in world, correct level bracket, no Deserter debuff) and enter the queue with you.
- **Supported maps:** Group queueing functions for Warsong Gulch and Arathi Basin. Because the core server rejects group queues for Alterac Valley, queue AV solo alongside your bots.
- **Auto-accepting invites:** Party bots automatically accept the battleground entry invite, clear follow holds, and engage map-specific PvP strategies for the duration of the match.
- **Queue isolation:** The automated queue service never cancels or modifies party bot queue entries.
