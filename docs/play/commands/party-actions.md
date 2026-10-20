---
id: play-commands-party-actions
title: Tactical Party Actions
category: play
summary: Tactical party actions, live combat toggles, and direct movement commands for player-led parties.
tags: [commands, tactics, party-actions]
relates_to:
  - play-commands
  - play-commands-commander
  - server-settings-party
---

# Tactical Party Actions

TortoiseBots uses an intent-driven control model for combat and movement. Instead of micromanaging individual abilities, you issue high-level tactical orders; the server evaluates party roles and dispatches actions to the most capable bots.

Tactical party actions require you to be alive and in the world, and you must control the bots (as owner, group master, claimed bot, or GameMaster). If an action cannot be parsed, the error names the first word typed (up to 32 bytes, UTF-8 safe, `|`/line-breaks → spaces, `unknown` if empty) with code `invalid`.

## Dungeon Movement Mechanics

Companion bots follow special navigation rules inside dungeons and raids to avoid pathing deadlocks:

- **Ranged Reach and Line of Sight (LoS):** Ranged damage dealers and healers walk the navigation mesh to a point that has line of sight to their target within cast range, provided the target is already in sight (they do not path around corners to gain LoS, nor do they run into melee range when casting spells).
- **Platform Step-Down:** When a bot is stuck on a raised ledge or platform with no ramp, it checks for walkable ground 2 to 8 yards lower and steps down directly without getting stuck.
- **Follow Catch-Up:** Outside of combat, companion bots trailing more than 3 yards behind their follow distance walk the navmesh back to their follow slot near the leader once a 2-second stand-still is seen. Healers only break follow to heal allies who fall below near-full health.

## Summary

| Command | Who | Target Required | What It Does |
| :--- | :--- | :--- | :--- |
| [`.bot action attack`](#bot-action-attack) | Group Master / Owner | Hostile Target | Orders scoped damage and tank bots to engage target. |
| [`.bot action interrupt`](#bot-action-interrupt) | Group Master / Owner | Casting Hostile | Orders the first bot with a ready interrupt to cast. |
| [`.bot action stop`](#bot-action-stop) | Group Master / Owner | None | Clears combat queues and stops current attacks. |
| [`.bot action flee`](#bot-action-flee) | Group Master / Owner | None | Orders bots to break combat and follow passively. |
| [`.bot action pull`](#bot-action-pull) | Group Master / Owner | Hostile Target | Sends tank to pull while party waits to engage. |
| [`.bot action pullback`](#bot-action-pullback) | Group Master / Owner | Hostile Target | Tank pulls and retreats to your anchor position. |
| [`.bot action come`](#bot-action-come) | Group Master / Owner | None | Commands scoped bots to sprint to your location. |
| [`.bot action stay`](#bot-action-stay) | Group Master / Owner | None | Halts scoped bots at their current positions. |
| [`.bot action follow`](#bot-action-follow) | Group Master / Owner | None | Resumes tight follow formation behind the leader. |
| [`.bot action focus skull`](#bot-action-focus-skull) | Group Master / Owner | Hostile Target / None | Sets Skull mark (or uses existing) and orders attack. |
| [`.bot action cc`](#bot-action-cc) | Group Master / Owner | Target / None | Assigns crowd control raid mark to a capable bot. |
| [`.bot action cc clear`](#bot-action-cc-clear) | Group Master / Owner | Bot / None | Clears crowd control mark assignments. |
| [`.bot action auto cc`](#bot-action-auto-cc) | Group Master / Owner | None | Toggles autonomous crowd control on loose adds. |
| [`.bot action sap`](#bot-action-sap) | Group Master / Owner | Rogue / None | Toggles pre-pull Sap behavior on CC marks. |
| [`.bot action aoe`](#bot-action-aoe) | Group Master / Owner | None | Toggles area-of-effect spells across party bots. |
| [`.bot action loot`](#bot-action-loot) | Group Master / Owner | None | Toggles corpse looting on or off. |
| [`.bot action repair`](#bot-action-repair) | Group Master / Owner | None | Orders bots to repair gear at a nearby vendor. |
| [`.bot action sell`](#bot-action-sell) | Group Master / Owner | None | Orders bots to sell gray junk items to vendor. |
| [`.bot action rest`](#bot-action-rest) | Group Master / Owner | None | Orders bots to sit and consume food and drink. |
| [`.bot action release`](#bot-action-release) | Group Master / Owner | None | Commands dead companion bots to release spirit. |
| [`.bot action corpse run`](#bot-action-corpse-run) | Group Master / Owner | None | Commands ghost bots to run back to their corpse. |
| [`.bot action learn`](#bot-action-learn) | Group Master / Owner | None | Orders bots to learn spells from nearby trainers. |
| [`.bot action trade`](#bot-action-trade) | Group Master / Owner | Companion Bot | Opens a direct trade window with targeted bot. |
| [`.bot action ready`](#bot-action-ready) | Group Master / Owner | None | Initiates a group readiness check across all bots. |
| [`.bot action raid status`](#bot-action-raid-status) | Group Master / Owner | None | Displays active raid strategies for party bots. |
| [`.bot action raid tankface`](#bot-action-raid-tankface) | Group Master / Owner | Dragon Target | Directs tank to face dragon boss away from group. |
| [`.bot action raid douse`](#bot-action-raid-douse) | Group Master / Owner | None | Orders bots to douse nearby Molten Core runes. |
| [`.bot action raid custom status`](#bot-action-raid-custom-status) | Group Master / Owner | None | Reports custom Turtle raid strategy status. |
| [`.bot action raid custom on`](#bot-action-raid-custom-on) | Group Master / Owner | None | Enables custom Turtle raid transition strategies. |
| [`.bot action raid custom off`](#bot-action-raid-custom-off) | Group Master / Owner | None | Disables custom Turtle raid transition strategies. |
| [`.bot follow`](#bot-follow) | Owner / GM | Bot (Required) | Orders a specific bot to follow you. |
| [`.bot stay`](#bot-stay) | Owner / GM | Bot (Required) | Orders a specific bot to hold position. |
| [`.bot guard`](#bot-guard) | Owner / GM | Bot (Required) | Orders one bot to defend its current ground. |
| [`.bot free`](#bot-free) | Owner / GM | Bot (Required) | Releases a bot to autonomous free movement. |
| [`.bot ready`](#bot-ready) | Owner / GM | Bot (Required) | Checks readiness for one specific bot. |
| [`.bot attack`](#bot-attack) | Owner / GM | Bot (Required) | Orders one specific bot to attack your target. |
| [`.bot interrupt`](#bot-interrupt) | Owner / GM | Bot (Required) | Orders one specific bot to interrupt a cast. |
| [`.bot pullback`](#bot-pullback) | Owner / GM | Live Target | Dispatches pullback maneuver on designated tank. |
| [`.bot formation`](#bot-formation) | Owner / GM | None / Name | Sets party follow formation spacing and pattern. |
| [`.bot strategy`](#bot-strategy) | Owner / GM | Strategy Name | Adds or removes combat and non-combat strategies. |
| [`.bot loot`](#bot-loot) | Owner / GM | None / on / off | Toggles corpse looting on targeted bot or party. |
| [`.bot repair`](#bot-repair) | Owner / GM | Bot / None | Orders targeted bot or party to repair gear. |
| [`.bot sell`](#bot-sell) | Owner / GM | Bot / None | Orders targeted bot or party to sell gray junk. |
| [`.bot rest`](#bot-rest) | Owner / GM | Bot / None | Orders bots to sit and consume food and drink. |

---

## Action Reference

### `.bot action attack`

Orders scoped tanks and DPS bots (selected bot if targeted, else party) to engage your current hostile target.

```text
.bot action attack
```

Bots accept targets out to `AiPlayerbot.ReactDistance` (150 yards by default, follows config) and path directly toward them. Dedicated healers continue healing allies rather than dealing damage.

<details>
<summary>How it works in detail</summary>

- **Evade Handling:** Creatures in evade mode cause bots to whisper `"<name> is evading"`.
- **Unreachable Targets:** The command lifts any temporary unreachable flags on that mob family. If the target remains physically unreachable after 15 seconds, bots drop the target and ignore that mob family for five minutes.
- **Target Validation:** Dead targets are rejected with `dead-target`. Friendly targets or invalid targets are refused immediately (`no-target`, `failed`).
- **Stay Release:** Issuing `.bot action attack` ends any active `.bot action stay` on all dispatched bots.

</details>

### `.bot action interrupt`

Directs the first available party bot with a ready interrupt ability to interrupt your target's spell cast.

```text
.bot action interrupt
```

The system checks for abilities like *Kick*, *Pummel*, *Earth Shock*, or *Counterspell*. If the chosen bot is out of casting distance, it closes range before executing the interrupt. Refusal codes: `no-target`, `dead-target`, `not-casting`, and `no-interrupt`.

### `.bot action stop`

Clears active combat queues and stops ongoing attacks across all companion bots.

```text
.bot action stop
```

Bots stop attacking and moving, but keep normal follow movement. Spell casts are interrupted.

### `.bot action flee`

Orders companion bots to disengage from combat immediately and follow the leader passively without attacking.

```text
.bot action flee
```

Bots drop their combat targets and pull holds, following you closely. Any subsequent combat or movement order (`attack`, `pull`, `pullback`, `focus skull`, `interrupt`, `cc` with a target, `come`, `follow`, `stay`) ends the passive hold.

### `.bot action pull`

Directs the party tank to pull your hostile target while other bots hold back at your position.

```text
.bot action pull [seconds] [ranged]
```

Parameters `[seconds]` (0 to 60, default 10) and `[ranged]` can be passed in either order. Other bots hold for the specified delay after the tank engages before joining combat. The tank keeps the pulled mob as its target and runs its normal rotation without needing a second order. Wait ends on time even if the fight finishes, a bot dies, or a movement order is given.

<details>
<summary>How it works in detail</summary>

- **Melee and Ranged Pulling:** The tank walks into melee range unless it has an equipped ranged weapon and ammo (or a ranged spell). If the target is unreachable on foot or farther than the approach distance configured in [AiPlayerbot.RangedPullMeleeApproach](../../server/settings/combat.md#aiplayerbotrangedpullmeleeapproach) (default 20 yards), the tank pulls from range.
- **Forced Ranged:** Adding `ranged` always requests a ranged opening — the tank walks to a firing position (the `RangedPull`/sight/range preconditions apply only to the automatic opening). If the tank cannot shoot, the blockers below apply and it body-pulls instead. Blockers are `no-ranged-weapon`, `weapon-untrained`, `shoot-spell-missing`, `no-ammo`, and `too-close` (whispered only if ordered ranged and blocker is not `NoWeapon` or `TooClose`). Line of sight alone triggers a quiet body-pull.
- **Early Release:** The pull hold ends early if you issue `.bot action attack` or `.bot action flee` (issuing `stop` does not release the DPS join window).
- **Refusal Codes:** Refused if target is dead (`dead-target`), friendly (`friendly`), beyond `ReactDistance * 3` (450 yards at default settings, returning `too-far`), unable to start walking to a firing position (`unreachable`), or invalid (`invalid-target`). If no tank is available, returns `no-tank`, or `tank-busy` if the tank is already in combat.

</details>

### `.bot action pullback`

Orders the tank to pull the target from range and immediately retreat to your current position, breaking line of sight around corners.

```text
.bot action pullback [seconds] [ranged]
```

Accepts `[seconds]` (0 to 60, default 5) and `[ranged]` in either order. The tank approaches only close enough to fire, shoots, and returns to your position. Other bots attack `seconds` after the tank is back at your position.

<details>
<summary>How it works in detail</summary>

- **Corner Pulling:** Use this maneuver to pull enemy packs around corners or through doorways to break enemy caster line of sight.
- **Anchor Hold:** The tank holds ground at the pullback spot for that fight only. Once the pulled enemies are defeated, the tank resumes normal following.
- **Fallback Melee:** If the tank has no working ranged weapon, it walks in, strikes the target once in melee, and sprints back to your position.

</details>

### `.bot action come`

Commands scoped bots (selected bot if targeted, else party) to sprint directly to your current coordinates and hold position.

```text
.bot action come
```

Aliases include `.bot action hold` and `.bot action comestay`. While `come` records a come order (allowing pending summon follow-restore to run), `hold` and `comestay` record a hold order. Bots arrive at your exact spot and switch their movement state to `stay`.

### `.bot action stay`

Halts scoped bots (selected bot if targeted, else party) at their current positions and makes them hold ground.

```text
.bot action stay
```

Bots stop moving immediately. Subsequent combat orders (`attack`, `pull`, `pullback`, `focus skull`, `interrupt`, or `cc` with a target) automatically release dispatched bots from stay so they engage and follow once combat finishes.

### `.bot action follow`

Breaks current movements and orders scoped bots (selected bot if targeted, else party) to resume follow formation behind the player.

```text
.bot action follow
```

If [`AiPlayerbot.DungeonFollowHoldBeforePacks`](../../server/settings/party.md#aiplayerbotdungeonfollowholdbeforepacks) is enabled, bots in a dungeon or raid trailing while nobody is in combat stop outside unpulled packs and whisper you a warning instead of pulling accidental adds (resuming when the pack dies or the leader returns within 25 yards).

### `.bot action focus skull`

Applies the **Skull** raid target icon to your current hostile target (or targets the already Skull-marked mob if you have no target) and commands all party bots in scope to attack it.

```text
.bot action focus skull
```

Orders all party bots in scope to attack the Skull mark using their normal rotations (healers are not exempted here).

### `.bot action cc`

Assigns a crowd control (CC) raid target icon to a capable companion bot.

```text
.bot action cc <mark> [bot]
```

Supported marks include `moon`, `star`, `circle`, `diamond`, `triangle`, `square`, and `cross` (`skull` is technically accepted but reserved for focus fire).

<details>
<summary>How it works in detail</summary>

- **Role and Spell Matching:** Matches marks to class abilities: Mage (*Polymorph*), Rogue (*Sap*), Warlock (*Banish* / *Fear*), Priest (*Shackle Undead*), Druid (*Hibernate* / *Entangling Roots*), Hunter (*Freezing Trap* / *Scare Beast*), and Paladin (*Turn Undead*).
- **Exclusive Ownership:** Only one bot owns a specific mark at a time. Assigning a mark to another bot clears it from the previous owner.
- **AoE Protection:** Party bots avoid using area-of-effect spells or multi-target attacks near crowd-controlled targets to avoid breaking the control.
- **Pre-Pull Sap:** If a rogue is assigned a mark on a humanoid target, it stealths and saps the target out of combat before the pull by default.

</details>

### `.bot action cc clear`

Clears crowd control mark assignments from party bots.

```text
.bot action cc clear [bot]
```

Specifying a bot name clears only that bot's assigned mark; targeting an owned bot clears that bot; omitting both clears marks from all companion bots in the party.

### `.bot action auto cc`

Toggles smart autonomous crowd control on loose hostile adds.

```text
.bot action auto cc [on|off]
```

With no argument, it toggles the setting. When enabled:
- Activates in group combat against 2 or more enemies.
- Target must be a non-tank healer or ranged hostile, never the tank's victim.
- Target must have no DoT aura active, must not be the last mob alive, must not be Skull-marked, and must not be the tank's target.
- One bot per mob, no re-sheeping active CC, and explicit CC marks always take precedence.

### `.bot action sap`

Controls whether rogues sneak up and apply *Sap* to their crowd control mark before a pull.

```text
.bot action sap [on|off] [bot]
```

Pre-pull Sap is on by default. Specifying a bot name (or targeting a bot) scopes to that bot, else all party rogues; non-rogues are skipped. If no rogues are in scope, returns `no-rogue`. The server acknowledges with `on`, `off`, or `mixed`.

### `.bot action aoe`

Toggles area-of-effect (AoE) abilities across all companion bots.

```text
.bot action aoe [on|off]
```

Bare `aoe` toggles the switch. AoE is on by default. Turning it off applies the `no aoe` strategy (shared with `.bot behavior <bot> aoe` and persisted across sessions), preventing bots from casting multi-target spells and abilities that might break CC. Non-AoE cluster pickers (such as `dps aoe`) continue targeting clusters without area damage.

<details>
<summary>Spells affected by AoE toggle</summary>

- **Mage:** Arcane Explosion, Blast Wave, Blizzard, Cone of Cold, Flamestrike, Frost Nova.
- **Warlock:** Rain of Fire, Howl of Terror.
- **Priest:** Holy Nova, Psychic Scream.
- **Druid:** Hurricane, Swipe, Demoralizing Roar, Challenging Roar.
- **Hunter:** Multi-Shot, Volley, Carve, Explosive Trap, Frost Trap.
- **Paladin:** Consecration, Holy Wrath.
- **Shaman:** Chain Lightning, Magma Totem, Fire Nova, Earthquake.
- **Warrior:** Cleave, Whirlwind, Sweeping Strikes, Thunder Clap, Demoralizing Shout, Challenging Shout, Intimidating Shout, Piercing Howl.
- **Rogue:** Blade Flurry.
- **Items & Racials:** War Stomp, Goblin Sapper Charge, Grenades, Oil of Immolation.

</details>

### `.bot action loot`

Toggles corpse looting on or off across companion bots.

```text
.bot action loot [on|off]
```

Corpse looting is on by default. Omitting the argument flips the current state and returns an acknowledgment with the resulting setting.

### `.bot action repair`

Orders companion bots to repair all damaged equipment at a nearby repair merchant.

```text
.bot action repair
```

Bots interact with the nearest NPC with the repair flag (`UNIT_NPC_FLAG_REPAIR`).

### `.bot action sell`

Orders companion bots to sell all gray-quality junk items in their inventory to a nearby merchant.

```text
.bot action sell
```

Bots find a nearby stocked vendor NPC themselves (`UNIT_NPC_FLAG_VENDOR`) and sell trash items to free up bag space.

### `.bot action rest`

Orders companion bots to sit, eat food, and drink water until their health and mana are fully restored.

```text
.bot action rest
```

Aliases include `.bot action drink` and `.bot action eat` (mages in the party trade conjured water and food to resting bots; item-cheat bots conjure instead of consuming). If bots cannot rest, the system responds with grouped reasons (`No bot rested: <reason>: <bots>...`, capped at 240 bytes with `+K more`):
- `dead`
- `in combat`
- `party still fighting`
- `nothing to rest (mana X% >= Y%, health A% >= B%)`
- `no food or drink` (or `no drink`, `no food`)
- `leader is moving`
- `other: not useful now`

### `.bot action release`

Commands dead companion bots to release their spirit to the nearest graveyard.

```text
.bot action release
```

See [Roster and Hiring Commands: .bot release](roster-and-hiring.md#bot-release) for full details.

### `.bot action corpse run`

Commands companion bots in spirit form to path back to their corpse or instance portal.

```text
.bot action corpse run
```

Alias: `.bot action corpserun`. See [Roster and Hiring Commands: .bot corpse run](roster-and-hiring.md#bot-corpse-run).

### `.bot action learn`

Commands companion bots near class trainers to purchase and train all available spells and ranks.

```text
.bot action learn
```

Target a trainer you have selected. See [Roster and Hiring Commands: .bot learn](roster-and-hiring.md#bot-learn).

### `.bot action trade`

Opens a direct player-to-bot trade window with your targeted companion bot.

```text
.bot action trade
```

See [Bot Panel: .bot item trade](bot-panel.md#bot-item-trade).

### `.bot action ready`

Initiates a party-wide readiness check across all companion bots.

```text
.bot action ready
```

Bots evaluate health (above 90%), mana (above 70%), distance (within `AiPlayerbot.SightDistance`, 60 yards by default), hunter ammo, pet happiness, and consumable counts. Each bot whispers its readiness status directly to the requester. Buff topping off occurs only on real raid checks when [`AiPlayerbot.ForceRebuffOnReadyCheck`](../../server/settings/world-population.md#aiplayerbotforcerebuffonreadycheck) is enabled (default 0, disabled).

### `.bot action raid status`

Displays the active raid encounter strategy currently configured for party bots.

```text
.bot action raid status
```

Reports active encounter strategies (including `molten core`, `onyxia`, `four horseman`, `solnius`, `emerald sanctum`, `lower karazhan`, `karazhan crypt`) or falls back to `instance map <id>, no raid strategy` (or `outdoor`).

### `.bot action raid tankface`

Orders tank bots to position dragon bosses facing directly away from the raid group.

```text
.bot action raid tankface
```

This prevents frontal cleaves and breath attacks from hitting healers and damage dealers.

### `.bot action raid douse`

Commands companion bots with Eternal Quintessence (or Aqual Quintessence) to douse nearby Molten Core boss runes. Eternal Quintessence is prioritized.

```text
.bot action raid douse
```

### `.bot action raid custom status`

Reports the active state of custom Turtle WoW raid encounter strategies.

```text
.bot action raid custom status
```

Outputs exact lowercase states: `emerald sanctum`, `lower karazhan`, `karazhan crypt`, or `off`.

### `.bot action raid custom on`

Enables custom raid encounter tactics for Turtle WoW raid instances.

```text
.bot action raid custom on
```

The manual command works regardless of configuration; [`AiPlayerbot.EnableCustomRaidTactics`](../../server/settings/dungeons-raids.md#aiplayerbotenablecustomraidtactics) controls automatic enablement upon zone-in.

### `.bot action raid custom off`

Disables automated custom raid tactics, returning bots to standard manual control.

```text
.bot action raid custom off
```

---

## Direct Movement and Party Commands

Direct commands require a specific named bot target for individual orders, while party-wide actions use `.bot action`.

### `.bot follow`

Orders an owned companion bot to follow you.

```text
.bot follow <Name>
```

A bot name is required (without a name, the server returns usage). For party-wide follow, use [`.bot action follow`](#bot-action-follow).

### `.bot stay`

Orders a specific companion bot to halt and stay at its current coordinates.

```text
.bot stay <Name>
```

A bot name is required. For party-wide stay, use [`.bot action stay`](#bot-action-stay).

### `.bot guard`

Orders a specific bot to hold ground at its current position and defend against hostile enemies entering its aggro radius.

```text
.bot guard <Name>
```

### `.bot free`

Releases an owned bot from `stay` or `guard` back to autonomous movement.

```text
.bot free <Name>
```

### `.bot ready`

Runs a readiness check on an individual named bot.

```text
.bot ready <Name>
```

A bot name is required. Checks health (>90%), mana (>70%), distance, and supplies, whispering the result to the requester. For party-wide readiness, use [`.bot action ready`](#bot-action-ready).

### `.bot attack`

Orders a specific companion bot to attack your current hostile target.

```text
.bot attack <Name>
```

Ends any active `stay` on the targeted bot; the bot engages the target and then resumes following you when combat ends.

### `.bot interrupt`

Orders a specific companion bot to interrupt your current target's spell cast.

```text
.bot interrupt <Name>
```

### `.bot pullback`

Dispatches a pullback maneuver on your designated tank (or selected bot).

```text
.bot pullback
```

Does not accept a name argument. Requires a live non-bot hostile target. The tank pulls from range and retreats to your position.

### `.bot formation`

Configures the geometric follow formation used by companion bots while following the party leader.

```text
.bot formation [botName] <formation>
```

Supported formations:
- `chaos` (default, resets with `default`)
- `melee`
- `queue`
- `circle`
- `line`
- `shield`
- `arrow`
- `near`
- `far`

Mounted companion bots automatically receive 3 extra yards of clearance outside instances out of combat so they do not crowd the player.

### `.bot strategy`

Applies or removes specific combat (`co`), non-combat (`nc`), or dead (`de`) strategies on a bot or the whole party.

```text
.bot strategy <+|-|~strategy> [Name]
```

Arguments must begin with `+`, `-`, or `~`. (The prefix `co` is for whisper commands, e.g. `/w <Bot> co +focus`).

- **Single-Target Burn (`+focus` / `-focus`):** Off by default. Suppresses multi-target attacks, shouts, and fears while keeping traps and totems intact (use `ss` / skip spell to exclude specific spells).
- **AoE Fear Toggle (`+aoe fear` / `-aoe fear`):** Suppresses Priest *Psychic Scream*, Warlock *Howl of Terror*, and Warrior *Intimidating Shout* inside dungeons and raids, and in the open world while grouped with you.

### `.bot loot`

Shorthand toggle for corpse looting. See [`.bot action loot`](#bot-action-loot).

```text
.bot loot [on|off]
```

### `.bot repair`

Shorthand repair command. See [`.bot action repair`](#bot-action-repair).

```text
.bot repair
```

### `.bot sell`

Shorthand sell command. See [`.bot action sell`](#bot-action-sell).

```text
.bot sell
```

### `.bot rest` <a id="bot-rest"></a>

Shorthand rest command. See [`.bot action rest`](#bot-action-rest).

```text
.bot rest
```

Aliased as `.bot drink` and `.bot eat`.

> [!NOTE]
> **Pet Stance, Autocast, Rebuff, and Boost Notes:**
> - There are no direct `.bot pet aggressive` or `.bot pet autocast` chat commands. Pet commands are issued via whisper (`/w <Bot> pet aggressive`, `/w <Bot> pet autocast <spell>`) or the proxy (`.bot command <Bot> pet aggressive`, `.bot command <Bot> pet autocast <spell>`).
> - There is no direct `.bot rebuff` chat command. Send `/w <Bot> rebuff` or `.bot command <Bot> rebuff` to top off expiring party buffs out of combat.
> - There is no direct `.bot boost` chat command. Configure boost targets via `/w <Bot> boost target +<Name>` or `.bot command <Bot> boost target +<Name>`.
