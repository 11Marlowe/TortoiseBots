---
id: play-commands-whispers
title: Whisper Commands
category: play
summary: Direct bot delegation and command reference via chat whispers and the CLI proxy.
tags: [commands, whispers, delegation]
relates_to:
  - play-commands
  - play-commands-party-actions
  - play-commands-bot-panel
---

# Whisper Commands

In addition to group-wide party actions, you can issue commands directly to individual companion bots using in-game whispers (`/w <BotName> <command>`) or the command-line proxy `.bot command <BotName> <command>`. Whispers allow precise control over inventory, vendor transactions, quest handling, and talent allocation.

## Command Delegation Proxy

Any whisper command can be sent through the `.bot command` proxy, making it easy to create macros without opening whisper channels:

```text
.bot command <BotName> <command>
# Example: .bot command Tanky trainer
```

Permission checks apply to whispers the same as chat commands: commands that modify gear, spend gold, or change talents require the sender to be the bot's account owner or a GameMaster.

---

## Situational Cheat-Sheet

### Inventory & Items

| Command / Whisper | Parameters | What It Does |
| :--- | :--- | :--- |
| `c` / `items` / `inv` | None | Lists equipped gear and inventory items with counts in chat. |
| `e` / `equip` | `<item link\|name>` | Equips the specified item from bags into matching gear slot. |
| `ue` | `<slot>` | Unequips gear from the specified equipment slot into bags. |
| `u` | `<item link\|name>` | Uses an item from inventory (potions, food, bandages). |
| `destroy` | `<item link\|name>` | Destroys an inventory item to free up bag space. |
| `drop` | `<quest link\|name>` | Abandons the specified quest from the bot's quest log. |
| `loot` | `[object link]` | Loots nearby corpses and containers immediately. |

### Vendors, Repairs & Economy

| Command / Whisper | Parameters | What It Does |
| :--- | :--- | :--- |
| `s` | `[gray\|all\|<item>]` | Sells junk items or specific items when vendor is open. |
| `b` | `<item link\|name>` | Buys the specified item from the active vendor window. |
| `bb` | `<item link\|name>` | Buys back an accidentally sold item from the vendor. |
| `repair` | None | Repairs damaged equipment at an armorer NPC. |
| `t` | None | Initiates or accepts a direct trade window with the master. |

### Quests & NPC Interaction

| Command / Whisper | Parameters | What It Does |
| :--- | :--- | :--- |
| `q` / `quests` | None | Lists active quests and objective progress in chat. |
| `accept` | None | Accepts an offered quest from a targeted NPC. |
| `talk` | None | Interacts and gossips with the targeted NPC. |
| `r` | `[reward index]` | Selects quest reward and turns in completed quest. |
| `share` | None | Shares eligible quests with other party members. |

### Training, Spells & Talents

| Command / Whisper | Parameters | What It Does |
| :--- | :--- | :--- |
| `trainer` | None | Trains all available class spells and ability ranks. |
| `talents` | None | Prints spent talent points and tree distribution. |
| `talents shift` | `<talent link>` | Allocates remaining talent points toward linked build. |
| `spells` | None | Lists known spells and highest learned spell ranks. |

### Travel, Rest & Recovery

| Command / Whisper | Parameters | What It Does |
| :--- | :--- | :--- |
| `home` | None | Binds the bot's Hearthstone at the current innkeeper. |
| `taxi` | `<destination>` | Purchases a flight path to the specified destination. |
| `release` | None | Releases spirit to the graveyard after dying. |
| `corpse run` | None | Paths back to corpse location in spirit form. |
| `revive` | None | Resurrects at Spirit Healer with resurrection sickness. |

### Combat Overrides

| Command / Whisper | Parameters | What It Does |
| :--- | :--- | :--- |
| `attack` | None | Attacks your current target and breaks active stay. |
| `tank attack` | None | Orders the party tank to taunt and engage your target. |
| `flee` / `runaway` | None | Retreats toward master in passive mode. |
| `end pull` | None | Aborts an active pull maneuver; tank returns to follow. |
| `rebuff` | None | Refreshes expiring party buffs out of combat. |
| `buff` | None | Re-casts missing class buffs on party members. |
| `pet` | `[follow\|stay\|attack]` | Directs pet movement and combat behavior. |
| `grind` | None | Toggles solo autonomous grinding mode for the bot. |
| `reset` | None | Flushes AI action queues and resets combat strategies. |

---

## Detailed Whisper Reference

### `/w <BotName> trainer`

Directs the bot to interact with a currently open class trainer NPC and learn all affordable spells, abilities, and ranks in one click.

```text
/w <BotName> trainer
```

Requires the bot to stand near a trainer of its matching class.

### `/w <BotName> talents`

Prints the bot's current talent tree distribution and unspent talent points to chat.

```text
/w <BotName> talents
/w <BotName> talents shift <talent link>
```

Passing `shift <link>` allocates available talent points toward the talent build encoded in the link.

### `/w <BotName> rebuff`

Prompts the bot to refresh all expiring party buffs out of combat.

```text
/w <BotName> rebuff
```

Performs a 2-minute expiration check and applies missing buffs. Healing actions temporarily yield while rebuffing runs.

### `/w <BotName> end pull`

Aborts a pull maneuver currently underway.

```text
/w <BotName> end pull
```

Whisper this to the tank bot if an enemy evades, paths abnormally, or gets stuck. The tank immediately drops the pull target and returns to follow formation instead of waiting for the 15-second timeout.

### `/w <BotName> attack`

Orders the bot to attack your current hostile target.

```text
/w <BotName> attack
```

Like party-wide attacks, this ends any active stay hold on the bot. If you have no target, the bot replies that you have no target and maintains its position.

### `/w <BotName> flee`

Orders the bot to drop combat and retreat toward you in passive mode.

```text
/w <BotName> flee
```

Alias: `runaway`. Any subsequent attack or movement command restores active combat.

### `/w <BotName> loot`

Orders the bot to loot nearby corpses, mineral nodes, herbs, or chests immediately.

```text
/w <BotName> loot [object link]
```

Passing an object link instructs the bot to interact specifically with that linked object.

### `/w <BotName> s`

Sells gray junk items or specific bag items when standing near an open merchant window.

```text
/w <BotName> s [gray|all|<item link>]
```

`s gray` sells poor-quality vendor trash; `s all` sells all non-essential items; linking an item sells only that item.

### `/w <BotName> repair`

Instructs the bot to repair all damaged equipment at a nearby repair merchant NPC.

```text
/w <BotName> repair
```

### `/w <BotName> c`

Prints a summary of equipped gear and inventory items carried by the bot in chat.

```text
/w <BotName> c
```

Aliases include `items` and `inv`.

### `/w <BotName> e`

Equips the specified item link or item name from the bot's inventory bags into its appropriate equipment slot.

```text
/w <BotName> e <item link|name>
```

Alias: `equip`.

### `/w <BotName> ue`

Unequips gear from the specified equipment slot into the bot's inventory bags.

```text
/w <BotName> ue <slot>
```

Fails if the bot has no free bag space.

### `/w <BotName> u`

Commands the bot to use an item from its inventory bags (e.g. potions, bandages, quest items).

```text
/w <BotName> u <item link|name>
```

### `/w <BotName> destroy`

Permanently destroys the specified item from the bot's bags to free up inventory space.

```text
/w <BotName> destroy <item link|name>
```

### `/w <BotName> q`

Prints the bot's active quest list and current objective completion status in chat.

```text
/w <BotName> q
```

Alias: `quests`.

### `/w <BotName> home`

Binds the bot's Hearthstone to the local innkeeper.

```text
/w <BotName> home
```

Requires the bot to stand within interaction range of an innkeeper NPC.

### `/w <BotName> taxi`

Purchases flight taxi transport to the specified destination from a flight master NPC.

```text
/w <BotName> taxi <destination>
```

### `/w <BotName> release`

Orders a dead companion bot to release spirit to the graveyard.

```text
/w <BotName> release
```

### `/w <BotName> corpse run`

Commands a companion bot in ghost form to path back to its corpse and resurrect.

```text
/w <BotName> corpse run
```

### `/w <BotName> revive`

Orders a companion bot in ghost form to resurrect immediately at the Spirit Healer.

```text
/w <BotName> revive
```

Applies resurrection sickness as normal.

### `/w <BotName> co` / `nc` / `strategy`

Inspects or modifies combat (`co`), non-combat (`nc`), or dead (`de`) strategies for that bot.

```text
/w <BotName> co <+|-|~strategy>
/w <BotName> nc <+|-|~strategy>
/w <BotName> co ?
```

Typing `co ?` prints the bot's currently active combat strategies. Prefixing a strategy name with `+` enables it, `-` disables it, and `~` toggles it.
