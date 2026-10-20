---
id: world-living-world
title: Living world & autonomous bots
category: world
summary: How autonomous bots roam the world, level up, travel, gather, and manage their gear.
tags: [world, living-world, leveling, bots]
relates_to:
  - world-social
  - world-queues
  - world-economy
  - server-settings-world-population
---

# Living world & autonomous bots

This page explains how autonomous bots inhabit the realm, progress through levels, explore zones, and handle equipment and sustenance. Settings controlling pool size, leveling, and wandering are configured in `aiplayerbot.conf`; see [World population & immersion settings](../server/settings/world-population.md).

## Overview

TortoiseBots populates the game world with autonomous characters on managed pool accounts. These bots travel through zones, complete quests, grind hostile creatures, gather crafting materials, trade on the Auction House, and form guilds. The world feels active and populated without requiring human players to fill every zone.

A managed pool account is an account recorded in the character-database table `tortoise_bots_pool_account`. The module creates these accounts automatically during auto-creation or hiring, or an administrator can register them manually at the server console. An account whose username begins with the configured account prefix is not a pool account unless it is registered in that table. For information on rebuilding pool characters, see [Resetting the managed bot pool](../server/bot-pool.md).

Hired mercenary companions are temporary guests created on managed pool accounts. When a player hires a mercenary, a new character is created for that contract. When the player dismisses the companion, leaves the party, disbands the group, or logs out past the disconnect grace period, the hired character is permanently deleted. Dismissed companions never enter the roaming pool, ensuring that player-geared characters do not alter the organic leveling world. Kicking an owned alt-bot from your party behaves differently: the alt roams using pool behavior but retains owner protection. For details, see [Alt-bots](../play/commands/roster-and-hiring.md#alt-bots-autonomous-alts) and [Alt-bots configuration](../server/settings/party.md#aiplayerbotaltbots).

## Autonomous bot lifecycle

Autonomous bots follow a continuous decision loop from login to rest and maintenance:

```mermaid
flowchart TD
    Login["Login & Spawn at Inn / Bindpoint"] --> GearCheck["Check Gear Durability & Bag Space"]
    GearCheck -->|"Need Repair / Empty Bags"| Town["Visit Blacksmith & Vendor Junk"]
    GearCheck -->|"Ready to Venture"| GoalChoice{"Choose Activity Intent"}
    Town --> GoalChoice
    GoalChoice -->|Questing| Quest["Accept & Track Local Quests"]
    GoalChoice -->|Grinding| Grind["Path to Level-Appropriate Mob Camp"]
    GoalChoice -->|Gathering| Gather["Path to Herb & Ore Resource Nodes"]
    Quest --> Travel["Travel via Foot / Flight Master / Zeppelin"]
    Grind --> Travel
    Gather --> Travel
    Travel --> Combat["Engage Mobs with Class Rotations"]
    Combat --> Loot["Loot Mobs, Quest Items & Resources"]
    Loot --> Rest["Rest: Consume Food / Water"]
    Rest --> CheckBags{"Bags Full or Quest Done?"}
    CheckBags -->|Yes| Hearth["Hearthstone / Return to Town"]
    CheckBags -->|No| Combat
    Hearth --> TurnIn["Turn In Quests & Learn Spell Ranks"]
    TurnIn --> AH["Post Gathered Items on Auction House"]
```

Bots check their gear and bag state upon logging in. If bags are full or durability is broken, the bot visits town vendors before venturing out. Otherwise, the bot rolls an activity purpose, travels to its destination, engages targets, loots corpses, and rests before continuing or returning to town.

## Open-world activities

The table below summarizes autonomous activities in the open world:

| Activity | How it works |
| :--- | :--- |
| **Zone travel** | Bots travel on foot, take flight paths, ride zeppelins and boats, and use hearthstones. |
| **Grinding & combat** | Bots engage level-appropriate mobs, pull with ranged abilities, and execute class rotations. |
| **Grind-spot ladder** | Bots move to new camps and zones as they outgrow lower-level creatures. |
| **Beginner grinding** | Fresh bots (levels 1-4) hunt starter beasts and ignore no-XP critters. |
| **Questing** | Bots accept level-appropriate quests, complete objectives, and turn them in for rewards. |
| **Leisure mixer** | Pool bots pick leisure journeys from a weighted distribution of quests, grinding, camps, and exploration. |
| **Quest-log upkeep** | Bots abandon obsolete or failed quests and prioritize turn-ins over grinding. |
| **Gathering** | Bots gather nearby herbs and mining nodes, and skin beasts they kill. |
| **Fishing** | Idle bots with fishing poles fish in nearby open water or visit fishing locations. |
| **Town services** | Bots visit nearby trainers, repair gear at blacksmiths, and sell vendor trash. |

<details>
<summary>Travel & flight paths in detail</summary>

Autonomous pool bots travel without teleporting. When a bot selects a distant destination, it plans a route using flight masters, zeppelins, boats, or roads.

Flight paths are selected from known taxi nodes to the node nearest the destination. The hop must be level-appropriate (the destination area can be at most five levels above the bot), and the bot must have enough gold to cover the flight fare above its class trainer savings reserve. The bot walks to the flight master, boards the taxi, and pays standard fare. Cross-zone travel such as leaving starting islands occurs naturally by air or boat rather than artificial movement.

</details>

<details>
<summary>Grinding & combat rules in detail</summary>

A bot picks grind targets within its sight distance, configured by [`AiPlayerbot.SightDistance`](../server/settings/combat.md#aiplayerbotsightdistance) (default 60 yards). Melee bots close into melee range, while ranged bots position at spell distance.

Autonomous bots never initiate combat against grey (zero-XP) creatures. If attacked by a grey creature, the bot defends itself, and ordered or quest kills remain permitted. If a bot cannot reach its selected target within 15 seconds due to terrain obstruction, the target is abandoned and blacklisted for five minutes. If a second give-up occurs within three minutes, the bot leaves the area and selects a different camp.

</details>

<details>
<summary>Grind-spot ladder (levels 1–60) in detail</summary>

Bots progress across a level ladder to avoid lingering in outgrown areas. Grind destinations are valid only when creatures sit between two levels below and one level above the bot (widening to two levels above starting at level 10).

When local creatures stop providing adequate experience, the current travel target expires. The bot then walks, takes a flight master, or boards a boat to the nearest camp matching its level band. Travel destinations allow an area rating up to three levels above the bot's level (or five levels above for starter levels 1-4). Camps occupied by too many active bots (one-third or more of the spawn count) are skipped in favor of quieter spots across the zone.

</details>

<details>
<summary>Beginner grinding (levels 1–4) in detail</summary>

Fresh bots in starting valleys (Valley of Trials, Camp Narache, Northshire, Coldridge, Shadowglen) target creatures up to their own level, including coinless starter beasts like boars, scorpids, and plainstriders.

True non-experience critters like rabbits are skipped, while wild critter-type beasts that yield experience remain valid targets. Below level 10, grind orders restrict targets to at most one level above the bot, preventing fresh characters from attempting difficult encounters.

</details>

<details>
<summary>Questing & quest-log maintenance in detail</summary>

Masterless pool bots pick up quests appropriate for their level. Below level 10, bots only accept quests rated at most one level above themselves and avoid cross-zone deliveries until reaching the required level.

Pool bots periodically clean their quest logs:
- Finished quests that cannot be rewarded are abandoned.
- Grey delivery quests whose required items are lost are dropped.
- Grey quests are ignored unless the reward provides an equipment upgrade.
- Unfinishable solo quests (such as elite, raid, or out-of-zone objectives) are abandoned when fewer than two log slots remain.
- If a bot makes no progress on a quest objective for five minutes, the quest is parked for 30 minutes, allowing the bot to work on other tasks.
- A completed quest turn-in journey takes precedence over grinding, and the bot paths directly to the quest giver without initiating unnecessary fights along the way.

</details>

<details>
<summary>Leisure mixer in detail</summary>

Masterless pool bots select their next leisure journey from a weighted table:
- **Questing:** Weight 60 (requires open quest log slots and unparked quests).
- **Grinding:** Weight 15 (requires unparked grind objectives).
- **Camp hunting:** Weight 10 (available at level 5 and above).
- **Exploration:** Weight 5 (available when the exploration strategy is active).

If a chosen purpose becomes invalid, the bot re-rolls immediately rather than idling. Service errands (visiting vendors, trainers, mailboxes, or the Auction House) always bypass the leisure roll and take immediate priority.

</details>

<details>
<summary>Gathering & professions in detail</summary>

Bots gather resource nodes they encounter while roaming. Herb and ore nodes within sight and range are gathered, and skinnable beasts are skinned immediately after looting.

Starting at level 10, bots with Herbalism or Mining will travel directly to nearby resource nodes. If a bot reaches a resource spawn point and finds it depleted, that location is placed on a 15-minute cooldown. Nodes require the appropriate gathering tools (Mining Pick, Skinning Knife) in the bot's inventory; without the tool, gathering interactions fail.

</details>

<details>
<summary>Fishing in detail</summary>

Masterless pool bots carrying a fishing pole engage in fishing as a casual side activity. Fishing occurs only when the bot is completely idle—with no pending travel, no completed quests to turn in, no vendor or repair needs, and bags below 90% capacity.

Side-activity fishing lasts at most five minutes (or five casts) once per hour, targeting visible fish schools or nearby open shores within 40 yards. Entering combat ends fishing immediately and restores the bot's weapon. Additionally, bots can undertake dedicated travel errands to stored world fishing locations. Owned and hired companion bots never fish autonomously.

</details>

<details>
<summary>Town life & service visits in detail</summary>

When idle in town, bots interact with nearby service NPCs within 50 yards before requesting travel errands. They turn in completed quests, accept available quests, train affordable new spell ranks, and vendor accumulated junk.

A vendor trip requires a valid reason: items that vendors buy (grey items or surplus gear), broken equipment needing repair, or an empty food bag that the bot has coin to refill. If a vendor visit yields no successful sales, vendor errands are paused for ten minutes. Autonomous travel announcements and vendor confirmations remain muted unless a player explicitly queries the bot.

</details>

## Level seeding & starter progression

On realms where bots progress organically, new pool bots are initialized with sensible starting attributes:

### Level seed & distribution
A newly created pool bot receives an initial level randomly selected between [`AiPlayerbot.RandomBotStartLevelMin`](../server/settings/world-population.md#aiplayerbotrandombotstartlevelmin--randombotstartlevelmax) and `Max` (default 1 to 60). This distributes characters across all level bands rather than bunching the entire population at level 1. With [`AiPlayerbot.LevelLadder`](../server/settings/world-population.md#aiplayerbotlevelladder) enabled, the online population is distributed proportionally across level brackets.

New bots are distributed evenly across the starting zones via [`AiPlayerbot.RandomBotEvenStartZones`](../server/settings/world-population.md#aiplayerbotrandombotevenstartzones). Auto-creation monitors starting zone populations and assigns newly created bots to less populated regions. Custom race starting areas relocate naturally to core racial hubs (such as Goblins to Valley of Trials and High Elves to Northshire).

Hunters observe standard class progression: hunters do not own pets below level 10. Upon reaching level 10, a pool hunter acquires a level-appropriate pet.

### Initial skills & professions
When persistent leveling is enabled ([`AiPlayerbot.DisableRandomLevels = 1`](../server/settings/world-population.md#aiplayerbotdisablerandomlevels)):
- **Weapon skills:** Fresh bots receive full class-compatible weapon skills maxed to their current level cap (for example, 5/5 at level 1) to ensure attacks connect reliably.
- **Ranged abilities:** Bots automatically learn the shooting ability matching their weapon proficiencies (*Shoot Bow*, *Shoot Gun*, *Shoot Crossbow*, *Throw*, or wand *Shoot*).
- **Dual Wield:** Class level thresholds gate dual wielding: warrior level 10, rogue level 10, and hunter level 20. Bots below these thresholds cannot learn Dual Wield, and any premature grant is removed on login. Shamans cannot dual wield.
- **Defensive passives:** Passives such as *Block* and *Parry* are restored on login.
- **Professions:** Bots receive First Aid, Cooking, and Fishing at level 1. At level 5, bots roll two class-matched primary professions. Each roll pairs one gathering skill with its corresponding craft (Mining with Blacksmithing or Engineering; Skinning with Leatherworking; Herbalism with Alchemy) or pairs two gathering skills. One in three cloth casters rolls Tailoring with Enchanting. Primary profession tools are provided automatically.

## Field kits & equipment upkeep

Every seeded pool bot and newly hired companion receives a complete starting field kit:

| Item category | Contents |
| :--- | :--- |
| **Bags** | Three 14-slot Journeyman's Backpacks; one container slot remains reserved for quivers or ammo pouches. |
| **Soul bags** | Warlocks receive a 12-slot Small Soul Pouch, upgraded automatically as larger bags drop. |
| **Ammo containers** | Hunters receive a quiver or ammo pouch matching their equipped ranged weapon, updated by level. |
| **Ranged weapons** | Warriors and rogues carry a level-appropriate thrown weapon for pulling. |
| **Profession tools** | Mining picks, skinning knives, blacksmith hammers, arclight spanners, and fishing poles matching professions. |
| **Mounts** | Swift Riding Turtle at level 18 ([`AiPlayerbot.TurtleMountAtLevel`](../server/settings/world-population.md#aiplayerbotturtlemountatlevel)), racial mounts at level 40 (60% speed) and level 60 (100% speed). |
| **Currencies & consumables** | Starting coin scaled to initial level, and a stack of First Aid bandages. |

Hired mercenary companions receive an hourly restock of class reagents, rations, potions, and bandages.

### Gear evaluation & upgrades
When [`AiPlayerbot.RandomGearUpgradeEnabled = 1`](../server/settings/world-population.md#aiplayerbotrandomgearupgradeenabled), bots evaluate looted and vendor gear using class and spec stat weight scales:
- **Stat weighting:** Equipment is scored by primary spec stats (Strength and Agility for melee; Spell Power and Intellect for casters).
- **Weapon rules:** Spec-forbidden weapons are replaced when an upgrade drops (such as an Assassination rogue replacing a mace with a dagger).
- **Fury warriors below level 20:** Fury warriors can equip two-handed weapons until *Dual Wield* is trained, at which point one-handed weapons take priority if they out-score the two-hander.
- **Protection shields:** Tanking specs retain their highest-scoring shield in bags even while holding a two-handed weapon, swapping the shield into the off-hand immediately if a one-handed weapon is equipped.
- **Armor classes:** Bots prioritize their native armor proficiencies, but will wear a lower armor class if an item fills an empty slot or beats the current item without sacrificing total armor and block.
- **Vendor weapon purchases:** Idle pool bots with surplus gold above their trainer reserve will purchase single weapon upgrades from vendors. Profession tools are excluded from weapon evaluation.

### Bag management & cleanup order
Bots manage their inventory dynamically to prevent stalls:
- Soul bags and quivers take dedicated bag slots; plain bag upgrades only replace plain bags.
- When bag capacity reaches 90% and the bot cannot reach a vendor, a cleanup routine destroys items in strict order: useless items, quest items (only if broke and unable to sell), cheapest grey trash, profession materials, and lastly consumables.
- Class trainer spell funds take priority: a bot short on coins for a trainer rank will travel to a vendor if its stock covers the required gold or if eight items have accumulated at 60% bag fullness.

## Sustenance & resting

Bots manage health and mana between fights to maintain continuous activity:

- **Simultaneous eating and drinking:** Out of combat, bots consume food and water at the same time in a single resting cycle.
- **Health and mana thresholds:** Bots utilizing rations eat until reaching 90% health and drink until nearly full mana. Bots without ration cheats rest until reaching 50% health and 85% mana.
- **Combat response:** A resting bot stands up immediately and engages if an enemy attacks or if a nearby party member within 60 yards enters combat.
- **Crowd control discipline:** A bot assigned a crowd control mark (such as Moon) will not start a non-essential meal if its health is at or above 70% while the target is alive and in sight.
- **Ration restocking:** Pool bots receive level-appropriate vendor food and water stacks automatically. Owned and hired bots restock their provisions through vendors or player trades.
- **Mage conjuration:** Mage bots out of combat conjure food and water and trade them to mana-dependent group members who are low on supplies.
