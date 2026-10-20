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

## Dungeon Movement Mechanics

Companion bots follow special navigation rules inside dungeons and raids to avoid pathing deadlocks:

- **Ranged Reach and Line of Sight (LoS):** Ranged damage dealers and healers walk the navigation mesh to a point that has line of sight to their target within cast range. They do not run into melee range when casting spells.
- **Platform Step-Down:** When a bot is stuck on a raised ledge or platform with no ramp, it checks for walkable ground 2 to 8 yards lower and steps down directly without getting stuck.
- **Follow Catch-Up:** Outside of combat, companion bots trailing more than 3 yards behind their follow distance navigate directly toward the leader. Healers only break follow to heal allies who fall below near-full health.

## Summary

| Command | Target Required | What It Does |
| :--- | :--- | :--- |
| [`.bot action attack`](#bot-action-attack) | Hostile Target | Orders party damage and tank bots to engage target. |
| [`.bot action interrupt`](#bot-action-interrupt) | Casting Hostile | Orders the first bot with a ready interrupt to cast. |
| [`.bot action stop`](#bot-action-stop) | None | Clears combat queues and stops current attacks. |
| [`.bot action flee`](#bot-action-flee) | None | Orders bots to break combat and follow passively. |
| [`.bot action pull`](#bot-action-pull) | Hostile Target | Sends tank to pull while party waits to engage. |
| [`.bot action pullback`](#bot-action-pullback) | Hostile Target | Tank pulls and retreats to your anchor position. |
| [`.bot action come`](#bot-action-come) | None | Commands all bots to sprint to your exact location. |
| [`.bot action stay`](#bot-action-stay) | None | Halts bots at their current positions. |
| [`.bot action follow`](#bot-action-follow) | None | Resumes tight follow formation behind the leader. |
| [`.bot action focus skull`](#bot-action-focus-skull) | Hostile Target | Sets Skull raid mark and orders focus fire. |
| [`.bot action cc`](#bot-action-cc) | Target / None | Assigns crowd control raid mark to a capable bot. |
| [`.bot action cc clear`](#bot-action-cc-clear) | Bot / None | Clears crowd control mark assignments. |
| [`.bot action auto cc`](#bot-action-auto-cc) | None | Toggles autonomous crowd control on loose adds. |
| [`.bot action sap`](#bot-action-sap) | Rogue / None | Toggles pre-pull Sap behavior on CC marks. |
| [`.bot action aoe`](#bot-action-aoe) | None | Toggles area-of-effect spells across party bots. |
| [`.bot action loot`](#bot-action-loot) | None | Toggles corpse looting on or off. |
| [`.bot action repair`](#bot-action-repair) | None | Orders bots to repair gear at a nearby vendor. |
| [`.bot action sell`](#bot-action-sell) | None | Orders bots to sell gray junk items to vendor. |
| [`.bot action rest`](#bot-action-rest) | None | Orders bots to sit and consume food and drink. |
| [`.bot action release`](#bot-action-release) | None | Commands dead companion bots to release spirit. |
| [`.bot action corpse run`](#bot-action-corpse-run) | None | Commands ghost bots to run back to their corpse. |
| [`.bot action learn`](#bot-action-learn) | None | Orders bots to learn spells from nearby trainers. |
| [`.bot action trade`](#bot-action-trade) | Companion Bot | Opens a direct trade window with targeted bot. |
| [`.bot action ready`](#bot-action-ready) | None | Initiates a group readiness check across all bots. |
| [`.bot action raid status`](#bot-action-raid-status) | None | Displays active raid strategies for party bots. |
| [`.bot action raid tankface`](#bot-action-raid-tankface) | Dragon Target | Directs tank to face dragon boss away from group. |
| [`.bot action raid douse`](#bot-action-raid-douse) | None | Orders bots to douse nearby Molten Core runes. |
| [`.bot action raid custom status`](#bot-action-raid-custom-status) | None | Reports custom Turtle raid strategy status. |
| [`.bot action raid custom on`](#bot-action-raid-custom-on) | None | Enables custom Turtle raid transition strategies. |
| [`.bot action raid custom off`](#bot-action-raid-custom-off) | None | Disables custom Turtle raid transition strategies. |
| [`.bot follow`](#bot-follow) | Bot / None | Orders one bot or the party to follow master. |
| [`.bot stay`](#bot-stay) | Bot / None | Orders one bot or the party to hold position. |
| [`.bot guard`](#bot-guard) | Bot | Orders one bot to defend its current ground. |
| [`.bot free`](#bot-free) | Bot | Releases a bot to autonomous free movement. |
| [`.bot ready`](#bot-ready) | Bot / None | Checks readiness for one bot or the entire group. |
| [`.bot attack`](#bot-attack) | Bot | Orders one specific bot to attack your target. |
| [`.bot interrupt`](#bot-interrupt) | Bot | Orders one specific bot to interrupt a cast. |
| [`.bot pullback`](#bot-pullback) | Bot / None | Dispatches pullback maneuver on designated tank. |
| [`.bot formation`](#bot-formation) | None / Name | Sets party follow formation spacing and pattern. |
| [`.bot strategy`](#bot-strategy) | Strategy Name | Adds or removes combat and non-combat strategies. |
| [`.bot loot`](#bot-loot) | None / on / off | Toggles corpse looting on targeted bot or party. |
| [`.bot repair`](#bot-repair) | Bot / None | Orders targeted bot or party to repair gear. |
| [`.bot sell`](#bot-sell) | Bot / None | Orders targeted bot or party to sell gray junk. |
| [`.bot rest`](#bot-rest) | Bot / None | Orders bots to sit and consume food and drink. |

---

## Tactical Party Actions

### `.bot action attack`

Orders all controllable party tanks and DPS bots to engage your current hostile target.

```text
.bot action attack
```

Bots accept targets out to 150 yards and path directly toward them. Dedicated healers continue healing allies rather than dealing damage.

<details>
<summary>How it works in detail</summary>

- **Evade Handling:** Creatures in evade mode are refused with `is evading` until they return to their spawn and reset.
- **Unreachable Targets:** The command lifts any temporary unreachable flags on that mob family. If the target remains physically unreachable after 15 seconds, bots drop the target.
- **Target Validation:** Dead targets are rejected with `dead-target`. Friendly targets or invalid targets are refused immediately.
- **Stay Release:** Issuing `.bot action attack` ends any active `.bot action stay` on all dispatched bots.

</details>

### `.bot action interrupt`

Directs the first available party bot with a ready interrupt ability to interrupt your target's spell cast.

```text
.bot action interrupt
```

The system checks for abilities like *Kick*, *Pummel*, *Earth Shock*, or *Counterspell*. If the chosen bot is out of casting distance, it closes range before executing the interrupt.

### `.bot action stop`

Clears active combat queues and stops ongoing attacks across all companion bots.

```text
.bot action stop
```

Bots clear current spell casts, cease auto-attacks, and hold position until another order is given.

### `.bot action flee`

Orders companion bots to disengage from combat immediately and follow the leader passively without attacking.

```text
.bot action flee
```

Bots drop their combat targets and pull holds, following you closely. Any subsequent combat or movement order (`attack`, `pull`, `pullback`, `follow`, `stay`) ends the passive hold.

### `.bot action pull`

Directs the party tank to pull your hostile target while other bots hold back at your position.

```text
.bot action pull [seconds] [ranged]
```

By default, other bots hold for 10 seconds (configurable from 0 to 60 seconds) after the tank engages before joining combat. Healers continue healing but deal no damage during the wait.

<details>
<summary>How it works in detail</summary>

- **Melee and Ranged Pulling:** The tank walks into melee range unless it has an equipped ranged weapon and ammo (or a ranged spell). If the target is unreachable on foot or farther than the approach distance configured in [AiPlayerbot.RangedPullMeleeApproach](../../server/settings/combat.md#aiplayerbotrangedpullmeleeapproach) (default 20 yards), the tank pulls from range.
- **Forced Ranged:** Adding the `ranged` argument (e.g. `.bot action pull 10 ranged`) forces a ranged pull if the tank can shoot. If the tank lacks ammo or line of sight, it falls back to a melee pull and whispers the reason.
- **Early Release:** The hold ends early if you issue `.bot action attack` or `.bot action stop`.
- **Refusal Codes:** Refused if target is dead (`dead-target`), friendly (`friendly`), beyond 450 yards (`too-far`), or invalid (`invalid-target`). If no tank is available, returns `no-tank`, or `tank-busy` if the tank is already in combat.

</details>

### `.bot action pullback`

Orders the tank to pull the target from range and immediately retreat to your current position, breaking line of sight around corners.

```text
.bot action pullback [seconds] [ranged]
```

The tank approaches only close enough to fire, shoots, and returns to hold at your coordinates. Other bots wait at your position for `seconds` (default 5, range 0 to 60) before attacking.

<details>
<summary>How it works in detail</summary>

- **Corner Pulling:** Use this maneuver to pull enemy packs around corners or through doorways to break enemy caster line of sight.
- **Anchor Hold:** The tank holds ground at the pullback spot for that fight only. Once the pulled enemies are defeated, the tank resumes normal following.
- **Fallback Melee:** If the tank has no working ranged weapon, it walks in, strikes the target once in melee, and sprints back to your position.

</details>

### `.bot action come`

Commands all companion bots to sprint directly to your current coordinates and hold position.

```text
.bot action come
```

Aliases include `.bot action hold` and `.bot action comestay`. Bots arrive at your exact spot and switch their movement state to `stay`. Issue `.bot action follow` to bring them back into formation.

### `.bot action stay`

Halts all companion bots at their current positions and makes them hold ground.

```text
.bot action stay
```

Bots stop moving immediately. Subsequent combat orders (`attack`, `pull`, `pullback`, `focus skull`, or `cc`) automatically release dispatched bots from stay so they engage and follow once combat finishes.

### `.bot action follow`

Breaks current movements and orders companion bots to resume follow formation behind the player.

```text
.bot action follow
```

If [AiPlayerbot.DungeonFollowHoldBeforePacks](../../server/settings/party.md#aiplayerbotdungeonfollowholdbeforepacks) is enabled (1), bots trailing more than 30 yards behind in a dungeon stop outside unpulled enemy packs and whisper you a warning instead of pulling accidental adds.

### `.bot action focus skull`

Applies the **Skull** raid target icon to your current hostile target and commands all damage bots to focus fire on it.

```text
.bot action focus skull
```

Tank bots establish threat while DPS bots direct their highest-priority single-target abilities at the Skull target.

### `.bot action cc`

Assigns a crowd control (CC) raid target icon to a capable companion bot.

```text
.bot action cc <mark> [bot]
```

Supported marks include `moon`, `star`, `circle`, `diamond`, `triangle`, `square`, and `cross`.

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

Specifying a bot name clears only that bot's assigned mark; omitting the name clears marks from all companion bots in the party.

### `.bot action auto cc`

Toggles smart autonomous crowd control on loose hostile adds.

```text
.bot action auto cc [on|off]
```

This feature is **off by default**. When enabled, capable bots automatically crowd-control unassigned loose enemies attacking healers or casters, provided no group member is currently damaging that enemy.

### `.bot action sap`

Controls whether rogues sneak up and apply *Sap* to their crowd control mark before a pull.

```text
.bot action sap [on|off] [bot]
```

Pre-pull Sap is **on by default**. Turning it `off` adds the `no sap` non-combat strategy. Running `.bot action sap` without arguments toggles the state.

### `.bot action aoe`

Toggles area-of-effect (AoE) abilities across all companion bots.

```text
.bot action aoe [on|off]
```

AoE is **on by default**. Running `.bot action aoe off` applies the `no aoe` strategy, preventing bots from casting multi-target spells and abilities that might break CC or pull extra packs.

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

Corpse looting is **on by default**. Omitting the argument flips the current state and returns an acknowledgment with the resulting setting.

### `.bot action repair`

Orders companion bots to repair all damaged equipment at a nearby repair merchant.

```text
.bot action repair
```

Bots interact with the nearest blacksmith or armorer NPC within interaction distance.

### `.bot action sell`

Orders companion bots to sell all gray-quality junk items in their inventory to a nearby merchant.

```text
.bot action sell
```

Bots interact with an open vendor window and vend trash items to free up bag space.

### `.bot action rest`

Orders companion bots to sit, eat food, and drink water until their health and mana are fully restored.

```text
.bot action rest
```

Aliases include `.bot action drink` and `.bot action eat`. If bots cannot rest, the system responds with specific refusal reasons (e.g. `in combat`, `no drink`, `no food`, `leader is moving`).

### `.bot action release`

Commands dead companion bots to release their spirit to the nearest graveyard.

```text
.bot action release
```

### `.bot action corpse run`

Commands companion bots in spirit form to path back to their corpse or instance portal.

```text
.bot action corpse run
```

### `.bot action learn`

Commands companion bots near class trainers to purchase and train all available spells and ranks.

```text
.bot action learn
```

Bots must stand within interaction range of their respective class trainers.

### `.bot action trade`

Opens a direct player-to-bot trade window with your targeted companion bot.

```text
.bot action trade
```

### `.bot action ready`

Initiates a party-wide readiness check across all companion bots.

```text
.bot action ready
```

Bots evaluate their health, mana, and expiring party buffs. If any bot is drinking, dead, or low on resources, the check reports the unready status in party chat.

### `.bot action raid status`

Displays the active raid encounter strategy currently configured for party bots.

```text
.bot action raid status
```

Reports raid states such as `molten core`, `onyxia`, `four horseman`, or `outdoor`.

### `.bot action raid tankface`

Orders tank bots to position dragon bosses facing directly away from the raid group.

```text
.bot action raid tankface
```

This prevents frontal cleaves and breath attacks from hitting healers and damage dealers.

### `.bot action raid douse`

Commands companion bots with Aqual or Eternal Quintessence to douse nearby Molten Core boss runes.

```text
.bot action raid douse
```

### `.bot action raid custom status`

Reports the active state of custom Turtle WoW raid encounter strategies.

```text
.bot action raid custom status
```

Outputs states for Emerald Sanctum, Lower Karazhan Halls, Karazhan Crypt, or `off`.

### `.bot action raid custom on`

Enables custom raid encounter tactics for Turtle WoW raid instances.

```text
.bot action raid custom on
```

Requires [AiPlayerbot.EnableCustomRaidTactics](../../server/settings/dungeons-raids.md#aiplayerbotenablecustomraidtactics) to be enabled on the server.

### `.bot action raid custom off`

Disables automated custom raid tactics, returning bots to standard manual control.

```text
.bot action raid custom off
```

---

## Direct Movement and Party Commands

In addition to `.bot action`, direct shorthand commands can be issued to specific bots or the entire party.

### `.bot follow`

Orders an owned companion bot to follow you, or resets party-wide follow when used without a name.

```text
.bot follow [Name]
```

Follow is the default fallback movement mode when strategies rebuild or after combat finishes outside battlegrounds.

### `.bot stay`

Orders a specific bot or the whole group to halt and stay at their current coordinates.

```text
.bot stay [Name]
```

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

Runs a readiness check on an individual named bot or the entire group.

```text
.bot ready [Name]
```

Verifies that health and mana are above combat thresholds and key class buffs are active.

### `.bot attack`

Orders a specific companion bot to attack your current hostile target.

```text
.bot attack <Name>
```

Like party-wide attacks, this command ends any active `stay` on the targeted bot.

### `.bot interrupt`

Orders a specific companion bot to interrupt your current target's spell cast.

```text
.bot interrupt <Name>
```

### `.bot pullback`

Dispatches a pullback maneuver on a named bot or your party tank.

```text
.bot pullback [Name]
```

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

Mounted companion bots automatically receive 3 extra yards of clearance outside instances so they do not crowd the player.

### `.bot strategy`

Applies or removes specific combat (`co`), non-combat (`nc`), or dead (`de`) strategies on a bot or the whole party.

```text
.bot strategy <+|-|~strategy> [Name]
```

Useful strategy toggles:
- **Single-Target Burn (`co +focus` / `co -focus`):** Off by default. While active, bots suppress area attacks, shouts, fears, and multi-dotting so crowd control is not broken.
- **AoE Fear Toggle (`co +aoe fear` / `co -aoe fear`):** Suppresses Priest *Psychic Scream*, Warlock *Howl of Terror*, and Warrior *Intimidating Shout* inside instances to avoid accidental pack pulls.

### `.bot loot`

Toggles corpse looting on your targeted bot or all companion bots in the party.

```text
.bot loot [on|off]
```

Corpse looting is on by default. With multiple bots in scope, the server confirms with `Loot strategy toggled for N bot(s).` without naming the new state; check the new state via `TBM:BOTSTATE` or use `.bot action loot`, whose acknowledgment includes the resulting state.

### `.bot repair`

Orders your targeted companion bot or all party bots to repair damaged equipment at a nearby vendor NPC.

```text
.bot repair
```

Also available as `.bot action repair`.

### `.bot sell`

Orders your targeted companion bot or all party bots to sell gray vendor junk items to a nearby merchant.

```text
.bot sell
```

Also available as `.bot action sell`.

### `.bot rest` <a id="bot-rest"></a>

Orders scoped companion bots to sit and consume food and drink until health and mana reach full.

```text
.bot rest
```

Aliases: `.bot drink`, `.bot eat`. Hostile combat or master movement breaks rest immediately. Also available as `.bot action rest`.
