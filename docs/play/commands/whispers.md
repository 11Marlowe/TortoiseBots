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

### Inventory, Items & Banking

| Command / Whisper | Parameters | What It Does |
| :--- | :--- | :--- |
| `c` / `items` / `inv` | None | Lists equipped gear and inventory items with counts in chat. |
| `e` / `equip` | `<item link\|name>` | Equips the specified item from bags into matching gear slot. |
| `ue` | `<slot>` | Unequips gear from the specified equipment slot into bags. |
| `u` | `<item link\|name>` | Uses an item from inventory (potions, food, bandages). |
| `destroy` | `<item link\|name>` | Destroys an inventory item (owner-only on alt-bots). |
| `drop` | `<quest link\|name>` | Abandons the specified quest (owner-only on alt-bots). |
| `loot` | `[object link]` | Loots nearby corpses and containers immediately. |
| `bank` | None | Interacts with a nearby banker NPC to manage personal storage. |
| `gbank` / `gb` | None | Interacts with a nearby guild banker NPC. |
| `mail` / `sendmail` | `[recipient] [subject]` | Interacts with a nearby mailbox to check or send mail. |
| `craft` | `<item link>` | Crafts an item using the bot's learned trade skills. |
| `outfit <name>` | `<name>` | Equips a previously saved equipment outfit set. |
| `outfit save <name>` | `<name>` | Saves the bot's currently equipped items under a profile name. |

### Vendors, Repairs & Economy

| Command / Whisper | Parameters | What It Does |
| :--- | :--- | :--- |
| `s` | `[gray\|all\|<item>]` | Sells junk items or specific items when vendor is open. |
| `b` | `<item link\|name>` | Buys the specified item from the active vendor window. |
| `bb` | `<item link\|name>` | Buys back an accidentally sold item from the vendor. |
| `repair` | None | Repairs damaged equipment at an armorer NPC. |
| `t` | None | Initiates or accepts a direct trade window with the master. |
| `wts` | None | Advertises items for sale or checks trade availability. |

### Quests & NPC Interaction

| Command / Whisper | Parameters | What It Does |
| :--- | :--- | :--- |
| `q` / `quests` | None | Lists active quests and objective progress in chat. |
| `accept` | None | Accepts an offered quest from a targeted NPC. |
| `talk` | None | Interacts and gossips with the targeted NPC. |
| `r` | `[reward index]` | Selects quest reward and turns in completed quest. |
| `share` | None | Shares eligible quests with other party members. |

### Training, Spells & Strategy Profiles

| Command / Whisper | Parameters | What It Does |
| :--- | :--- | :--- |
| `trainer` | None | Trains all available class spells and ability ranks. |
| `talents` | None | Prints spent talent points and tree distribution. |
| `talents shift` | `<talent link>` | Allocates remaining talent points toward linked build. |
| `spells` | None | Lists known spells and highest learned spell ranks. |
| `cast <spell>` | `<spell link\|name>` | Commands the bot to cast a known spell in combat. |
| `castnc <spell>` | `<spell link\|name>` | Commands the bot to cast a known spell out of combat. |
| `save ai <name>` | `<name>` | Saves current strategy configuration to DB as a profile. |
| `load ai <name>` | `<name>` | Loads a previously saved strategy profile from DB. |
| `list ai` | None | Lists all saved strategy configuration profiles for this bot. |
| `reset ai` | None | Resets the bot's strategy configuration to default AI. |

### Travel, Rest & Recovery

| Command / Whisper | Parameters | What It Does |
| :--- | :--- | :--- |
| `home` | None | Binds the bot's Hearthstone at the current innkeeper. |
| `taxi` | `<destination>` | Purchases a flight path to the specified destination. |
| `release` | None | Releases spirit to the graveyard after dying. |
| `corpse run` | None | Paths back to corpse location in spirit form. |
| `revive` | None | Resurrects at Spirit Healer with resurrection sickness. |

### Tactical & Combat Overrides

| Command / Whisper | Parameters | What It Does |
| :--- | :--- | :--- |
| `attack` | None | Attacks your current target and breaks active stay. |
| `tank attack` | None | Orders the party tank to taunt and engage your target. |
| `flee` / `runaway` | None | Retreats toward master in passive mode. |
| `end pull` | None | Aborts an active pull maneuver; tank returns to follow. |
| `rebuff` | None | Refreshes expiring party buffs out of combat. |
| `buff` | None | Re-casts missing class buffs on party members. |
| `pet` | `[follow\|stay\|attack]` | Directs pet movement and combat behavior. |
| `rti <mark>` | `<mark>` | Assigns raid target icon focus or CC mark. |
| `attack rti <mark>` | `<mark>` | Orders bot to attack enemy marked with raid target icon. |
| `pull rti <mark>` | `<mark>` | Orders tank bot to pull enemy marked with raid icon. |
| `stance` | `[stance name]` | Queries or sets warrior/druid combat stance or form. |
| `save mana` | `[on\|off]` | Toggles conservative mana-saving ability rotations. |
| `max dps` | None | Temporarily prioritizes maximum damage output. |
| `possible attack targets`| None | Queries list of hostile targets in sensory range. |
| `attackers` | None | Prints count and names of enemies currently attacking. |
| `position` | None | Reports the bot's current coordinates and orientation. |
| `who` | None | Prints character summary, spec, and status in chat. |
| `where` | None | Reports current map, zone, and location coordinates. |
| `los` | None | Reports line-of-sight status to current target. |
| `wait for attack time` | None | Holds attacks until main-hand swing timer synchronizes. |
| `jump` | None | Triggers an immediate jump action. |
| `warning` | None | Prints active combat or environmental warnings. |
| `grind` | None | Toggles solo autonomous grinding mode for the bot. |
| `reset` | None | Flushes AI action queues and resets combat strategies. |

### Guild Management

| Command / Whisper | Parameters | What It Does |
| :--- | :--- | :--- |
| `guild invite` | `<player>` | Invites the named character to the bot's guild. |
| `guild join` | None | Directs the bot to accept an outstanding guild invite. |
| `guild leave` | None | Orders the bot to leave its current guild. |
| `guild promote` | `<player>` | Promotes a guild member to the next higher rank. |
| `guild demote` | `<player>` | Demotes a guild member to the next lower rank. |
| `guild leader` | `<player>` | Transfers guild leadership to the named member. |
| `guild remove` | `<player>` | Kicks the named member from the guild. |

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

On autonomous alt-bots, this command is strictly restricted to the alt's owner account; orders from other players are refused.

### `/w <BotName> drop`

Abandons the specified quest from the bot's quest log.

```text
/w <BotName> drop <quest link|name>
```

On autonomous alt-bots, quest abandonment is strictly restricted to the character's owner account.

### `/w <BotName> bank` / `gbank`

Interacts with a nearby banker or guild banker NPC.

```text
/w <BotName> bank
/w <BotName> gbank
/w <BotName> gb
```

Requires the bot to stand within interaction range of a bank or guild bank NPC.

### `/w <BotName> mail` / `sendmail`

Interacts with a nearby mailbox to check received mail or compose outgoing mail.

```text
/w <BotName> mail
/w <BotName> sendmail [recipient] [subject]
```

Requires the bot to stand within interaction range of a mailbox.

### `/w <BotName> craft`

Orders the bot to craft an item using its learned trade skills.

```text
/w <BotName> craft <item link>
```

The bot checks whether it possesses the required profession skill, tools, and reagents before crafting.

### `/w <BotName> outfit`

Saves or equips complete gear sets.

```text
/w <BotName> outfit save <name>
/w <BotName> outfit <name>
```

`outfit save` records all currently equipped items under the specified profile name. Whispering `outfit <name>` equips the saved outfit from the bot's bags.

### `/w <BotName> cast` / `castnc`

Commands the bot to cast a specific known spell.

```text
/w <BotName> cast <spell link|name>
/w <BotName> castnc <spell link|name>
```

`cast` executes the spell in combat; `castnc` executes the spell out of combat.

### `/w <BotName> save ai` / `load ai`

Saves and loads strategy configurations from the database under named profiles.

```text
/w <BotName> save ai <name>
/w <BotName> load ai <name>
/w <BotName> list ai
/w <BotName> reset ai
```

- `save ai <name>`: Persists the bot's active combat and non-combat strategies to the database.
- `load ai <name>`: Restores a previously saved profile.
- `list ai`: Lists all saved profiles belonging to this bot.
- `reset ai`: Resets strategies and action values to defaults.

### `/w <BotName> rti`

Assigns or orders attacks using Raid Target Icons.

```text
/w <BotName> rti <mark>
/w <BotName> attack rti <mark>
/w <BotName> pull rti <mark>
```

Supported marks include `skull`, `cross`, `square`, `moon`, `triangle`, `diamond`, `circle`, and `star`.

### Tactical & Informational Whispers

Informational and tactical status queries for group members:

```text
/w <BotName> stance [stance name]
/w <BotName> save mana [on|off]
/w <BotName> max dps
/w <BotName> possible attack targets
/w <BotName> attackers
/w <BotName> position
/w <BotName> who
/w <BotName> where
/w <BotName> wts
/w <BotName> los
/w <BotName> wait for attack time
/w <BotName> jump
/w <BotName> warning
```

- `stance`: Queries or sets warrior/druid stance or form.
- `save mana`: Toggles mana conservation rotation (`on` / `off`).
- `max dps`: Pushes offensive rotations without conservation.
- `possible attack targets`: Lists perceived hostile creatures in sensory range.
- `attackers`: Lists enemies currently targeting or damaging the bot.
- `position`: Reports map coordinates and heading.
- `who`: Outputs level, race, class, and active spec.
- `where`: Reports current zone and area name.
- `wts`: Advertises trade items or checks merchant status.
- `los`: Reports line-of-sight status to current combat target.
- `wait for attack time`: Pauses auto-attack until swing timer synchronizes.
- `jump`: Makes the bot jump immediately.
- `warning`: Reports active environment or combat warnings.

<a id="guild-management"></a>
### Guild Management Whispers

Commands for managing bot guilds through chat whispers:

```text
/w <BotName> guild invite <player>
/w <BotName> guild join
/w <BotName> guild leave
/w <BotName> guild promote <player>
/w <BotName> guild demote <player>
/w <BotName> guild leader <player>
/w <BotName> guild remove <player>
```

Allows human guild members with proper permissions to direct bot guild officers and leaders to manage guild rosters, invite recruits, promote members, or transfer guild leadership.

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
