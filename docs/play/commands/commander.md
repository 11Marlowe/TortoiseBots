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
  - server-settings-performance
---

# Remote Commander Commands

The Remote Commander interface allows a player or GameMaster to lead a bot-only group of up to five bots without being present in the group. You can issue tactical orders, navigate dungeons, open objects, and view a live perception feed from anywhere in the game world.

Remote Commander is disabled by default and requires [`AiPlayerbot.RemoteCommander`](../../server/settings/performance.md#aiplayerbotremotecommander) to be enabled (`1`).

## Commander Rules and Architecture

- **Binding Authority:** A GM, or anyone who controls every bot in the group (owner account, master, or claim). Even GMs cannot bind parties containing real players, raid groups, or another player's hires.
- **Party Leadership:** The group is led by a designated lead bot. Follower bots follow the lead bot and maintain threat backoff behind the tank (yielding on mobs hitting healer or damage bots; threat backoff is never stored with the bot).
- **Suppressed Behaviors:** Bound bots have open-world behaviors stripped: `grind`, `travel`, `travel once`, `tfish`, `rpg` (including craft, player, guild, and jump), and `explore`. Autonomous `wander` is explicitly not stripped. Autonomous alt-bots are never moved by `.bot cmd` (they are stripped from tactical orders; order them via whisper or party chat instead).
- **Hired Companions:** You can bind up to 5 of your own hired mercenary companions. While bound, the requirement that hires share a group with their hirer is suspended. Unbinding returns the hires to your group if you are online. Disbanding ends the binding and returns hires. Hirer logout initiates normal logout grace and regrouping. Server restarts end active hires.
- **Instance Preservation:** Instance progress is preserved when your party is kept (bot-only, all bots named); a rebuilt group starts with no instance bind until `cmd enter`.

## Summary

| Command | Permission | What It Does |
| :--- | :--- | :--- |
| [`.bot commander bind`](#bot-commander-bind) | Control All / GM | Binds a bot-only group to your remote control. |
| [`.bot commander unbind`](#bot-commander-unbind) | Bound Commander | Releases your remote control over the bound party. |
| [`.bot commander status`](#bot-commander-status) | Bound Commander | Displays members (dead bots marked) and lead bot. |
| [`.bot commander view`](#bot-commander-view) | Bound Commander | Toggles real-time perception feed stream. |
| [`.bot cmd attack`](#bot-cmd-attack) | Bound Commander | Orders party bots to attack a target GUID. |
| [`.bot cmd pull`](#bot-cmd-pull) | Bound Commander | Directs the tank to pull a target to the lead bot. |
| [`.bot cmd pullback`](#bot-cmd-pullback) | Bound Commander | Tank shoots target and retreats to lead bot anchor. |
| [`.bot cmd cc`](#bot-cmd-cc) | Bound Commander | Assigns crowd control on a target GUID. |
| [`.bot cmd focus`](#bot-cmd-focus) | Bound Commander | Marks Skull and commands all bots to focus GUID. |
| [`.bot cmd move`](#bot-cmd-move) | Bound Commander | Navigates the lead bot and party to 3D coordinates. |
| [`.bot cmd stay`](#bot-cmd-stay) | Bound Commander | Halts all bots in the commanded party in place. |
| [`.bot cmd follow`](#bot-cmd-follow) | Bound Commander | Resumes follower movement behind the lead bot. |
| [`.bot cmd rest`](#bot-cmd-rest) | Bound Commander | Orders all bots in the commanded group to rest. |
| [`.bot cmd revive`](#bot-cmd-revive) | Owner or GM | Releases a dead bot's spirit to run to corpse. |
| [`.bot cmd lootmethod`](#bot-cmd-lootmethod) | Bound Commander | Sets the loot method and threshold for the party. |
| [`.bot cmd summon`](#bot-cmd-summon) | Owner or GM | Teleports a bound bot directly to the lead bot. |
| [`.bot cmd enter`](#bot-cmd-enter) | Owner or GM | Moves party through a dungeon entrance trigger. |
| [`.bot cmd loot`](#bot-cmd-loot) | Owner or GM | Orders a bot to loot a corpse or container. |
| [`.bot cmd open`](#bot-cmd-open) | Owner or GM | Commands a bot to interact with or unlock an object. |
| [`.bot cmd use`](#bot-cmd-use) | Owner or GM | Commands a bot to use an item from its bags. |
| [`.bot cmd ritual`](#bot-cmd-ritual) | Owner or GM | Directs 2-5 bots to activate summoning rituals. |
| [`.bot cmd state`](#bot-cmd-state) | Bound Commander | Queries the current state of a world game object. |

---

## Commander Management Commands

### `.bot commander bind`

Binds a bot-only party under your remote leadership.

```text
.bot commander bind <LeadBot> [<Bot> ...]
```

The specified lead bot becomes party leader. If you specify additional bots, they are invited to the group. A groupless lead bot requires at least one named follower (refused with `no-members` if none given). An existing bot-only group is adopted as-is.

<details>
<summary>How binding works and refusal codes</summary>

- **Kept-Party Rule:** If the lead bot and named bots are already in your party, the party is kept intact: the lead bot takes leadership, you leave the group, and dungeon instances remain preserved.
- **Hired Companions:** You can bind up to 5 of your own hired companions. Hires belonging to other players cannot be bound.
- **Refusal Codes:** `disabled`, `already-bound`, `no-bot`, `group-has-player`, `group-raid`, `group-too-large`, `group-bound`, `no-members`, `member-grouped`, `bot-bound`, `party-unnamed`, `lead-bound`, `hired`, `denied`, `invite-failed`.

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

Lists each member's name (dead bots marked with `(dead)`) and sends `TBM:ACTION_ACK|commander status|party|<count>|<leadName>`.

### `.bot commander view`

Enables or disables the live perception feed.

```text
.bot commander view on|off
```

When enabled, the server streams structured perception frames (`TBM:VIEW`) detailing party vitals, nearby hostiles, and active threat tables. A new binding starts with the view off by default. Update frequency is governed by [`AiPlayerbot.RemoteCommanderViewMs`](../../server/settings/performance.md#aiplayerbotremotecommanderviewms).

---

## Tactical Remote Orders (`.bot cmd`)

Tactical orders use the prefix `.bot cmd`. Tactical targets are identified strictly by their 64-bit object GUID (in hex `0x...` or decimal). Object entry IDs (`entry:<id>`) are accepted only by `loot`, `open`, `use`, `ritual`, and `state`.

### `.bot cmd attack`

Orders bots in the commanded party to attack the target GUID.

```text
.bot cmd attack <guid> [bot]
```

Specifying a bot name directs only that bot to engage. Healers keep healing unless explicitly named in the command. If a tank is holding another mob, ACK returns `tank-stays:<name>` or `held:<name>` (held attacks can hold up to 20 seconds).

### `.bot cmd pull`

Directs the group tank to pull the target GUID, while other bots wait at the lead bot's coordinates.

```text
.bot cmd pull <guid> [seconds] [ranged]
```

Accepts an optional delay in seconds (defaults to 10 seconds, range 0 to 60) and an optional `ranged` flag (accepted in either order). Pull maintains target hold and non-tanks engage after the delay expires.

### `.bot cmd pullback`

Directs the tank to pull the target from range and immediately retreat to the lead bot's position.

```text
.bot cmd pullback <guid> [seconds] [ranged]
```

Defaults to a 5-second pull delay (accepted in either order with `ranged`). Non-tank bots hold at the lead anchor and engage only after the tank returns to the anchor spot and the delay expires.

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

Coordinates must be within `AiPlayerbot.ReactDistance` (default 150 yards) of the lead bot on walkable ground. Coordinates accept commas or semicolons as separators (without spaces). Fails with `too-far`, `unreachable`, or `off-mesh` (>5 yards off navmesh). After combat ends, bots return to this stay spot.

<details>
<summary>Direct movement mode</summary>

Adding the `direct` flag (e.g. `.bot cmd move <x> <y> <z> direct`) commands the party to walk a straight line instead of using navigation mesh paths (useful for unlinked ramps or ledges). Preconditions: horizontal distance within 25 yards, vertical within 8 yards (no more than 3 yards below or 0.5 yards above ground), and clear line of sight. Followers gather first, and nothing moves unless the lead bot and every follower pass all checks.

</details>

### `.bot cmd stay`

Commands all bots in the party, including the lead bot, to hold their current positions.

```text
.bot cmd stay
```

### `.bot cmd follow`

Orders followers to resume follow formation behind the lead bot. The lead bot keeps its current position (even if mid-movement).

```text
.bot cmd follow
```

### `.bot cmd rest`

Orders all bots in the commanded party to sit, eat food, and drink water until fully restored. Bots that are already full accept the order cleanly.

```text
.bot cmd rest
```

### `.bot cmd revive`

Releases the spirit of a dead companion bot so it can run back to its corpse and resurrect.

```text
.bot cmd revive <bot>
```

Requires Owner or GM permissions. Live bots are refused (`not-dead`). Dead bots are released, run back to their corpse, and are revived by their own dead strategy.

### `.bot cmd lootmethod`

Configures the loot distribution method and threshold for the commanded party.

```text
.bot cmd lootmethod <ffa|group|rr|master|nbg> [threshold]
```

Supported methods: `ffa`, `group`, `rr`, `master` (lead bot is master looter), `nbg`. Threshold is kept when omitted; aliases include `common`/`white`, `uncommon`/`green`, `rare`/`blue`, `epic`/`purple`, `legendary`/`orange` (`0` is rejected). The acknowledgment reflects the method now in force (`failed` if unchanged).

### `.bot cmd summon`

Teleports a named bound bot directly to the lead bot's current coordinates.

```text
.bot cmd summon <bot>
```

Requires Owner or GM permissions. Takes 3 seconds, allowed out of combat only. Rebinds master and restores follow movement upon arrival.

### `.bot cmd enter`

Moves the commanded party through a dungeon entrance trigger.

```text
.bot cmd enter <area trigger id|near> [bot]
```

Requires Owner or GM permissions. Passing `near` picks the nearest entrance trigger within `ReactDistance` (lowest ID breaks ties). Bots must be on the trigger map and within `ReactDistance`. Checks instance requirements and lockouts (`requirement`, `instance-limit`; GM bypasses, raid maps refused). Lead bot enters first (30-second budget), followed by followers (30-second budget). Replies with `(n of m got in)`.

---

## Object and Inventory Orders

These commands require Owner or GM permissions (except `.bot cmd state`, which is available to any bound commander).

### Mechanics: The Walk and The Act

- **The Walk:** The assigned bot walks the navmesh to within 1.5 yards of the target object (bounded by `ReactDistance`). Followers follow the actor bot if it is the lead bot. Stance and previous orders are restored upon completion.
- **The Act:** The bot stands still, its stay anchor updates, active spells drop, and it performs the interaction. Engaging in combat interrupts the act; in-place `use` continues if already started.

### `.bot cmd loot`

Directs a bot to walk to a corpse or game object container and loot its contents.

```text
.bot cmd loot <guid|entry:<id>[@x,y,z]> <bot> [itemId]
```

Corpse preference is applied when selecting targets. Without `itemId`, loots all items and money; with `itemId`, loots only that item (money is ignored). Prechecks locks; fails with `locked` if locked.

### `.bot cmd open`

Commands a bot to walk to a game object (chest, door, lever, altar) and interact with it.

```text
.bot cmd open <guid|entry:<id>[@x,y,z]> <bot>
```

Opening chests uses the bot's Opening spell (lockpicking or key, 5-second cast); reports `spell:<id>` in acknowledgment.

### `.bot cmd use`

Commands a bot to use an item from its inventory bags, optionally targeting a unit or object.

```text
.bot cmd use <itemId> <bot> [target-guid|entry:<id>[@x,y,z]]
```

### `.bot cmd ritual`

Coordinates between 2 and 5 bots to interact simultaneously with a summoning ritual object.

```text
.bot cmd ritual <guid|entry:<id>[@x,y,z]> <bot1> <bot2> [<bot3>...]
```

Checks required participant count vs named bots (`not-enough` if insufficient). The first named bot initiates the channel, and subsequent bots assist.

### `.bot cmd state`

Queries the current animation and interaction state of a world game object. Available to any bound commander.

```text
.bot cmd state <guid|entry:<id>[@x,y,z]>
```

Reports three fields:
- `state:<active|ready|alternative>`
- `loot:<not-ready|ready|activated|just-deactivated>`
- `flags:<none|in-use|locked|interact-cond|no-interact>`

---

## Target by Entry and Timing

Objects can be targeted by entry ID: `entry:<id>` (nearest within `ReactDistance`, default 150 yd) or `entry:<id>@<x>,<y>,<z>` (nearest within 30 yd of coordinates). Format: decimal integer $\le$ 16777215, coordinates comma- or semicolon-separated without spaces. Emits a preceding `TBM:ACTION_TARGET` line.

### Asynchronous Execution Protocol

Commands that require walking or casting emit asynchronous lifecycle records:
1. `TBM:ACTION_PENDING|cmd <verb>|bot:<Name>|<budget seconds>|<guid>` (or `party` for rituals and enters). Budgets: walk is 10 s + distance / 4 yd/s (capped at 60 s); loot 8 s; open 4 s (8 s for chests); use 15 s. Immediate actions complete without PENDING.
2. `TBM:ACTION_ACK|cmd <verb>|bot:<Name>|<count>|<detail>`.
3. `TBM:ACTION_ERR|cmd <verb>|<code>|<message>`.

Catalog of error codes: `no-bot`, `denied`, `too-far`, `no-item`, `locked`, `busy`, `in-combat`, `invalid-target`, `not-found`, `unavailable`, `cancelled`, `timeout`, `invalid`, `failed`, `dead-target`, `friendly`, `no-target`, `unreachable`, `off-mesh`, `no-los`, `no-ground`, `not-enough`, `interrupted`, `requirement`, `instance-limit`, `not-bound`, `lead-unavailable`, `no-ai`.

---

## Perception Feed (`TBM:VIEW`)

When enabled via `.bot commander view on`, the server transmits periodic perception frames (version `v1`) approximately once per second (`AiPlayerbot.RemoteCommanderViewMs`).

- **Transport:** Frames use lines capped at 250 bytes, max 8 lines per world tick, skipping frames when behind. Sequence numbers start at 1, coordinates use 1- or 2-decimal precision, and frames are constructed entirely from cached memory values.

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
