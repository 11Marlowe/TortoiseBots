---
id: play-commands-commander
title: Remote Commander Commands
category: play
summary: Remote commander mode and intent commands for directing bot-only parties from anywhere.
tags: [commands, commander, remote-control]
relates_to:
  - play-commands
  - play-commands-party-actions
  - play-commands-addon
---

# Remote Commander Commands

The Remote Commander interface allows a player or GameMaster to lead a bot-only group of up to five bots without being present in the group. You can issue tactical orders, navigate dungeons, open objects, and view a live perception feed from anywhere in the game world.

Remote Commander is disabled by default and requires [AiPlayerbot.RemoteCommander](../../server/settings/performance.md#aiplayerbotremotecommander) to be enabled (1).

## Commander Rules and Architecture

- **Binding Authority:** A player may bind a bot group if they own all member bots (same account) or have hired them. GameMasters can bind any bot party.
- **Party Leadership:** The group is led by a designated lead bot. Other bots follow the lead bot and maintain threat backoff behind the tank.
- **Suppressed Behaviors:** Bound bots have open-world autonomous wandering, grinding, gathering, and roleplay actions temporarily disabled so they remain under direct control.
- **Hired Companions:** You can bind your own hired mercenary companions into a commanded party. While bound, the normal requirement that hires must share a group with their hirer is suspended. Unbinding returns the hires to your group.
- **Instance Preservation:** Binding a group preserves existing dungeon instance locks. When entering instances, bots take over the instance map without resetting progress.

## Summary

| Command | What It Does |
| :--- | :--- |
| [`.bot commander bind`](#bot-commander-bind) | Binds a bot-only group to your remote control. |
| [`.bot commander unbind`](#bot-commander-unbind) | Releases your remote control over the bound party. |
| [`.bot commander status`](#bot-commander-status) | Displays the status and members of your bound party. |
| [`.bot commander view`](#bot-commander-view) | Toggles the real-time perception data stream. |
| [`.bot cmd attack`](#bot-cmd-attack) | Orders party bots to attack a target GUID. |
| [`.bot cmd pull`](#bot-cmd-pull) | Directs the tank to pull a target to the lead bot. |
| [`.bot cmd pullback`](#bot-cmd-pullback) | Tank shoots target and retreats to lead bot anchor. |
| [`.bot cmd cc`](#bot-cmd-cc) | Assigns crowd control on a target GUID. |
| [`.bot cmd focus`](#bot-cmd-focus) | Marks Skull and commands all bots to focus GUID. |
| [`.bot cmd move`](#bot-cmd-move) | Navigates the lead bot and party to 3D coordinates. |
| [`.bot cmd stay`](#bot-cmd-stay) | Halts all bots in the commanded party in place. |
| [`.bot cmd follow`](#bot-cmd-follow) | Resumes follower movement behind the lead bot. |
| [`.bot cmd rest`](#bot-cmd-rest) | Orders all bots in the commanded group to rest. |
| [`.bot cmd revive`](#bot-cmd-revive) | Releases a dead bot's spirit to run to corpse. |
| [`.bot cmd lootmethod`](#bot-cmd-lootmethod) | Sets the loot method and threshold for the party. |
| [`.bot cmd summon`](#bot-cmd-summon) | Teleports a bound bot directly to the lead bot. |
| [`.bot cmd enter`](#bot-cmd-enter) | Moves party through a dungeon entrance trigger. |
| [`.bot cmd loot`](#bot-cmd-loot) | Orders a bot to loot a corpse or container. |
| [`.bot cmd open`](#bot-cmd-open) | Commands a bot to interact with or unlock an object. |
| [`.bot cmd use`](#bot-cmd-use) | Commands a bot to use an item from its bags. |
| [`.bot cmd ritual`](#bot-cmd-ritual) | Directs 2-5 bots to activate summoning rituals. |
| [`.bot cmd state`](#bot-cmd-state) | Queries the current state of a world game object. |

---

## Commander Management Commands

### `.bot commander bind`

Binds a bot-only party under your remote leadership.

```text
.bot commander bind <LeadBot> [<Bot> ...]
```

The specified lead bot becomes party leader. If you specify additional bots, they are invited to the group.

<details>
<summary>How binding works in detail</summary>

- **Kept-Party Rule:** If the lead bot and named bots are already in your party, the party is kept intact: the lead bot takes leadership, you leave the group, and dungeon instances remain preserved.
- **Hired Companions:** You can bind up to 5 of your own hired companions. Hires belonging to other players cannot be bound.
- **Refusal Reasons:** Refused if disabled (`disabled`), already bound (`already-bound`), party contains real players (`group-has-player`), party has more than 5 members (`group-too-large`), or a bot is controlled by another player (`hired`).

</details>

### `.bot commander unbind`

Ends your active remote commander session.

```text
.bot commander unbind
```

Releases the binding. Followers restore their standard follow targets. Hired companions automatically rejoin your party if you are online.

### `.bot commander status`

Prints a summary of the currently bound party in chat.

```text
.bot commander status
```

Reports each member's name, role, living status, and distance from the lead bot.

### `.bot commander view`

Enables or disables the live perception feed.

```text
.bot commander view on|off
```

When enabled, the server streams structured perception frames (`TBM:VIEW`) detailing party vitals, nearby hostiles, and active threat tables. A new binding starts with the view off by default.

---

## Tactical Remote Orders (`.bot cmd`)

Remote tactical commands use the prefix `.bot cmd`. Targets are identified by their 64-bit object GUID (in hex `0x...` or decimal) or by entry ID.

### `.bot cmd attack`

Orders all bots in the commanded party to attack the target GUID.

```text
.bot cmd attack <guid> [bot]
```

Specifying a bot name directs only that bot to engage. If a tank is already holding another mob, it remains on its current target unless explicitly commanded.

### `.bot cmd pull`

Directs the group tank to pull the target GUID, while other bots wait at the lead bot's coordinates.

```text
.bot cmd pull <guid> [seconds] [ranged]
```

Accepts an optional delay in seconds (0 to 60) and an optional `ranged` flag to force a ranged pull.

### `.bot cmd pullback`

Directs the tank to pull the target from range and immediately retreat to the lead bot's position.

```text
.bot cmd pullback <guid> [seconds] [ranged]
```

The lead bot's location acts as the anchor spot. Non-tank bots hold at the anchor and engage after the delay expires.

### `.bot cmd cc`

Assigns a crowd control mark and target to a capable party bot.

```text
.bot cmd cc <mark> <guid> [bot]
```

Places the specified raid target icon on the target GUID and assigns the crowd control duty to a bot.

### `.bot cmd focus`

Applies the Skull raid mark to the specified target GUID and commands all party bots to focus damage.

```text
.bot cmd focus <guid>
```

### `.bot cmd move`

Directs the lead bot to pathfind to specific 3D coordinates; followers follow the lead bot.

```text
.bot cmd move <x> <y> <z> [direct]
```

Coordinates must be within 150 yards of the lead bot on walkable ground.

<details>
<summary>Direct movement mode</summary>

Adding the `direct` flag (e.g. `.bot cmd move <x> <y> <z> direct`) commands the party to walk a straight line instead of using navigation mesh paths. Use this mode for ramps or ledges not linked in the mesh (such as Uldaman's scorpid pit ramp). The target point must be within 25 yards horizontally and have line of sight from the lead bot.

</details>

### `.bot cmd stay`

Commands all bots in the party, including the lead bot, to hold their current positions.

```text
.bot cmd stay
```

### `.bot cmd follow`

Orders followers to resume follow formation behind the lead bot.

```text
.bot cmd follow
```

### `.bot cmd rest`

Orders all bots in the commanded party to sit, eat food, and drink water until fully restored.

```text
.bot cmd rest
```

### `.bot cmd revive`

Releases the spirit of a dead companion bot so it can run back to its corpse and resurrect.

```text
.bot cmd revive <bot>
```

Requires Owner or GM permissions. Live bots or bots that have not released spirit will reject the command.

### `.bot cmd lootmethod`

Configures the loot distribution method and threshold for the commanded party.

```text
.bot cmd lootmethod <ffa|group|rr|master|nbg> [threshold]
```

Supported methods:
- `ffa`: Free-for-all
- `group`: Group loot
- `rr`: Round robin
- `master`: Master looter (lead bot acts as master)
- `nbg`: Need before greed

Threshold values: `1` to `5`, or `common`, `uncommon`, `rare`, `epic`, `legendary`.

### `.bot cmd summon`

Teleports a named bound bot directly to the lead bot's current coordinates.

```text
.bot cmd summon <bot>
```

Requires Owner or GM permissions. The summon takes 3 seconds and is only allowed out of combat.

### `.bot cmd enter`

Moves the commanded party through a dungeon entrance trigger.

```text
.bot cmd enter <area trigger id|near> [bot]
```

Passing `near` selects the closest instance entrance trigger within 150 yards. The lead bot enters first to initialize the dungeon instance; followers follow immediately after.

---

## Object and Inventory Orders

These commands allow remote commanders to interact with world objects, chests, and items.

### `.bot cmd loot`

Directs a bot to walk to a corpse or game object container and loot its contents.

```text
.bot cmd loot <guid|entry:<id>[@x,y,z]> <bot> [itemId]
```

Without an `itemId`, the bot loots all accessible items and money. Specifying an `itemId` loots only that specific item.

### `.bot cmd open`

Commands a bot to walk to a game object (chest, door, lever, or altar) and interact with it.

```text
.bot cmd open <guid|entry:<id>[@x,y,z]> <bot>
```

Locked chests are opened using the bot's Opening spell (lockpicking or keys). The command waits for the interaction to complete.

### `.bot cmd use`

Commands a bot to use an item from its inventory bags, optionally targeting a unit or object.

```text
.bot cmd use <itemId> <bot> [target-guid|entry:<id>[@x,y,z]]
```

Used for potions, bandages, quest item combinations, or key activations.

### `.bot cmd ritual`

Coordinates between 2 and 5 bots to interact simultaneously with a summoning ritual object.

```text
.bot cmd ritual <guid|entry:<id>[@x,y,z]> <bot1> <bot2> [<bot3>...]
```

Used for group summoning rituals, such as the Altar of the Keepers and Altar of Archaedas in Uldaman. All participating bots channel the ritual together.

### `.bot cmd state`

Queries the current animation and interaction state of a world game object.

```text
.bot cmd state <guid|entry:<id>[@x,y,z]>
```

Returns state flags (`active`, `ready`, `in-use`, `locked`) without requiring bots to move.

---

## Target by Entry and Timing

Objects can be targeted by their database entry ID when GUIDs are unknown:
- `entry:<id>`: Selects the nearest spawned object matching the ID within 150 yards.
- `entry:<id>@<x>,<y>,<z>`: Selects the nearest spawned object within 30 yards of the specified coordinates.

### Asynchronous Execution Protocol

Commands that require movement or casting run asynchronously:
1. `TBM:ACTION_PENDING|cmd <verb>|bot:<Name>|<budget seconds>|<guid>`: Emitted when the action begins.
2. `TBM:ACTION_ACK|cmd <verb>|bot:<Name>|<count>|<detail>`: Emitted upon successful completion.
3. `TBM:ACTION_ERR|cmd <verb>|<code>|<message>`: Emitted if the action fails or is interrupted.

Common error codes: `no-bot`, `denied`, `too-far`, `no-item`, `locked`, `busy`, `in-combat`, `invalid-target`, `not-found`, `unavailable`, `cancelled`, `timeout`.

---

## Perception Feed (`TBM:VIEW`)

When enabled via `.bot commander view on`, the server transmits periodic perception frames (version `v1`) approximately once per second (`AiPlayerbot.RemoteCommanderViewMs`).

```text
TBM:VIEW|v1|<seq>|B|<lead>,<party>,<hostiles>,<threats>,<truncated>
TBM:VIEW|v1|<seq>|P|<party record>[;<party record>...]
TBM:VIEW|v1|<seq>|H|<hostile record>[;<hostile record>...]
TBM:VIEW|v1|<seq>|T|<threat record>[;<threat record>...]
TBM:VIEW|v1|<seq>|E|<lines>
```

- **B (Begin):** Sequence number, lead bot GUID, counts of party/hostile/threat records, and truncation flag (`0` or `1`).
- **P (Party):** `guid,name,hp,maxhp,power,maxpower,powertype,x,y,z,o,map,instance,target,spell,dead,state,action,result,reason`.
- **H (Hostile):** `guid,entry,x,y,z,hp%,target,spell,flags,reaction,kind`. Capped at 24 units.
- **T (Threat):** `hostileGuid,botGuid:pct[,botGuid:pct...]`. Top three threat holders per enemy.
- **E (End):** Total count of lines in the frame.

### Example Frame

```text
TBM:VIEW|v1|12|B|0x2A,1,1,1,0
TBM:VIEW|v1|12|P|0x2A,Tanky,4000,5000,35,100,1,-8913.2,554.5,93.1,1.50,0,0,0xF130000C5E0012AB,0,0,c,heroic strike,O,-
TBM:VIEW|v1|12|H|0xF130000C5E0012AB,3874,-8900.0,560.0,94.0,64,0x2A,0,-,h,-
TBM:VIEW|v1|12|T|0xF130000C5E0012AB,0x2A:100,0x2B:41
TBM:VIEW|v1|12|E|5
```
