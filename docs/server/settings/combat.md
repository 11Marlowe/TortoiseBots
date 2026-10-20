---
id: server-settings-combat
title: Combat & Reaction Thresholds
category: server
summary: Configuration thresholds for health, mana, engagement distances, casting pacing, pull rules, and strategy rotation.
tags: [config, combat, thresholds, health, mana, distance]
relates_to:
  - classes-overview
  - play-commands-party-actions
---

# Combat & Reaction Thresholds

These settings configure health and mana trigger percentages, engagement ranges, casting delays, and tank pull mechanics. Settings go in `aiplayerbot.conf`; see [How configuration works](../configuration.md).

## Summary

| Setting | Default | What it does |
| :--- | :---: | :--- |
| [`AiPlayerbot.AlmostFullHealth`](#aiplayerbotalmostfullhealth) | `90` | Health percentage ceiling above which bots stop casting heals to conserve mana. |
| [`AiPlayerbot.CriticalHealth`](#aiplayerbotcriticalhealth) | `20` | Health percentage considered an emergency, triggering survival cooldowns. |
| [`AiPlayerbot.LowHealth`](#aiplayerbotlowhealth) | `50` | Health percentage triggering prioritized heavy healing spells. |
| [`AiPlayerbot.MediumHealth`](#aiplayerbotmediumhealth) | `70` | Health percentage triggering maintenance heals and damage-over-time effects. |
| [`AiPlayerbot.LowMana`](#aiplayerbotlowmana) | `15` | Mana floor percentage where casters conserve mana or switch to wanding. |
| [`AiPlayerbot.MediumMana`](#aiplayerbotmediummana) | `40` | Mana percentage where bots transition toward conservative ability rotations. |
| [`AiPlayerbot.FollowDistance`](#aiplayerbotfollowdistance) | `1.5` | Standard following distance in yards between a follower bot and leader on foot. |
| [`AiPlayerbot.RaidFollowDistance`](#aiplayerbotraidfollowdistance) | `5.0` | Expanded following distance in yards used inside dungeons and raids. |
| [`AiPlayerbot.SightDistance`](#aiplayerbotsightdistance) | `60.0` | Perception distance in yards used to detect monsters and define guard leashes. |
| [`AiPlayerbot.SpellDistance`](#aiplayerbotspelldistance) | `26.0` | Maximum range in yards used for initiating ranged spell attacks. |
| [`AiPlayerbot.ReactDistance`](#aiplayerbotreactdistance) | `150.0` | Maximum distance in yards bots remain responsive to combat and leader calls. |
| [`AiPlayerbot.LootDistance`](#aiplayerbotlootdistance) | `25.0` | Maximum distance in yards bots roam from their anchor to loot corpses. |
| [`AiPlayerbot.GlobalCooldown`](#aiplayerbotglobalcooldown) | `1500` | Pacing delay in milliseconds enforced between rapid spell casts. |
| [`AiPlayerbot.RangedPull`](#aiplayerbotrangedpull) | `1` | Enables automatic ranged openings for commanded tank pulls. |
| [`AiPlayerbot.RangedPullMeleeApproach`](#aiplayerbotrangedpullmeleeapproach) | `20` | Walk-in approach threshold in yards before tank pulls open from range. |
| [`AiPlayerbot.MinRandomBotChangeStrategyTime`](#aiplayerbotminrandombotchangestrategytime) | `1800` | Minimum seconds a pool bot retains its strategy set before periodic refresh. |
| [`AiPlayerbot.MaxRandomBotChangeStrategyTime`](#aiplayerbotmaxrandombotchangestrategytime) | `7200` | Maximum seconds a pool bot retains its strategy set before periodic refresh. |

## Settings

### `AiPlayerbot.AlmostFullHealth`
Health percentage ceiling above which healer bots stop casting heals to conserve party mana.
**Default:** `90` · **Values:** `1` to `100` percent · **Restart:** needed

<details>
<summary>How it works in detail</summary>

When a party member health exceeds this percentage, healer bots conserve their mana by canceling or holding back further healing casts.

Damage-over-time maintenance heals (such as Renew or Rejuvenation) are not applied when targets are already above this line.

</details>

### `AiPlayerbot.CriticalHealth`
Health percentage considered an extreme emergency, triggering high-impact defensive cooldowns.
**Default:** `20` · **Values:** `1` to `100` percent · **Restart:** needed

<details>
<summary>How it works in detail</summary>

When a bot or party member drops below this health percentage, bots immediately deploy emergency survival abilities:
- Paladins cast *Lay on Hands* or *Divine Shield*.
- Warriors activate *Last Stand* or *Shield Wall*.
- Healers bypass standard rotations to land emergency direct heals.

Random pool bots without player masters also evaluate fleeing mechanics when reaching critical health unless hold-ground settings are configured.

</details>

### `AiPlayerbot.LowHealth`
Health percentage triggering prioritized heavy healing spells.
**Default:** `50` · **Values:** `1` to `100` percent · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Defines the trigger point where healers interrupt low-priority casting to channel heavy heals (*Greater Heal*, *Healing Wave*, *Healing Touch*).

For safer dungeon runs or high-damage raid encounters, raising this value to `65` provides earlier healing intervention.

</details>

### `AiPlayerbot.MediumHealth`
Health percentage triggering routine maintenance healing and small direct heals.
**Default:** `70` · **Values:** `1` to `100` percent · **Restart:** needed

<details>
<summary>How it works in detail</summary>

When group members fall below this line, healers apply maintenance heals (*Flash Heal*, *Flash of Light*, *Renew*, *Rejuvenation*).

Utility actions such as Paladin Judgement upkeep are suspended while any group member remains below this threshold.

</details>

### `AiPlayerbot.LowMana`
Mana floor percentage where casters conserve mana or switch to wanding.
**Default:** `15` · **Values:** `1` to `100` percent · **Restart:** needed

<details>
<summary>How it works in detail</summary>

When caster mana falls below this percentage, bots enter mana conservation mode:
- Priests, mages, and warlocks switch to shooting wands rather than casting high-cost nukes.
- Healers reserve remaining mana strictly for critical emergency heals.
- Paladins hold back costly offensive judgements and consecrations.

</details>

### `AiPlayerbot.MediumMana`
Threshold percentage where bots begin balancing damage output against mana longevity.
**Default:** `40` · **Values:** `1` to `100` percent · **Restart:** needed

<details>
<summary>How it works in detail</summary>

At this mana level, casters and hybrid classes begin pacing their spell usage during prolonged encounters.

Paladins require at least this mana percentage to maintain offensive seals and judgements in groups.

</details>

### `AiPlayerbot.FollowDistance`
Standard following distance in yards between a follower bot and the leader on foot.
**Default:** `1.5` (yards) · **Values:** positive float · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Sets the tight following distance used when travelling on foot in the open world.

When mounted out of combat and outside instances, bots automatically add 3 yards to this radius to accommodate larger mount collision models and avoid clipping.

</details>

### `AiPlayerbot.RaidFollowDistance`
Expanded following distance in yards used inside dungeons and raids.
**Default:** `5.0` (yards) · **Values:** positive float · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Replaces standard follow distance inside instances to ensure party members do not cluster tightly on top of the leader.

This wider spacing prevents party-wide damage from area-of-effect monster spells and frontal cleaves.

</details>

### `AiPlayerbot.SightDistance`
Perception distance in yards used to detect monsters and define guard leashes.
**Default:** `60.0` (yards) · **Values:** positive float · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Defines how far bots scan their surroundings for visible hostile targets and party members.

This value also sets the outer leash boundary when a bot is placed into guard stance (`guard` strategy).

</details>

### `AiPlayerbot.SpellDistance`
Maximum distance in yards used for initiating ranged spell attacks.
**Default:** `26.0` (yards) · **Values:** positive float · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Establishes the default engagement distance for ranged caster damage spells and long-range utility debuffs.

Bots path toward targets until within this distance before beginning cast sequences.

</details>

### `AiPlayerbot.ReactDistance`
Maximum distance in yards at which bots remain responsive to combat and leader calls.
**Default:** `150.0` (yards) · **Values:** positive float · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Sets the outer awareness envelope around the leader.

If a follower falls farther behind than this distance, it triggers `AiPlayerbot.BoostFollow` mount checks to catch up.

</details>

### `AiPlayerbot.LootDistance`
Maximum distance in yards bots will roam from their anchor point to loot corpses.
**Default:** `25.0` (yards) · **Values:** positive float · **Restart:** needed

<details>
<summary>How it works in detail</summary>

When combat concludes and dead monsters drop loot, bots widen their movement leash to this radius to retrieve items before resuming their follow positions.

</details>

### `AiPlayerbot.GlobalCooldown`
Pacing delay in milliseconds enforced between rapid spell casts.
**Default:** `1500` (milliseconds) · **Values:** positive integer · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Controls the delay between consecutive spell casts, emulating the standard vanilla global cooldown.

Prevents bot AI from queuing multiple instant abilities in the same engine tick.

</details>

### `AiPlayerbot.RangedPull`
Enables automatic ranged weapon or spell opening for commanded tank pulls.
**Default:** `1` · **Values:** `0` off, `1` on · **Restart:** needed

<details>
<summary>How it works in detail</summary>

During commanded pulls (`.bot action pull` or `.bot action pullback`), tanks open from range using a bow, gun, crossbow, thrown weapon, or pull spell if the mob is in shot range and either unreachable by direct pathing (cliffs, ledges) or farther than `AiPlayerbot.RangedPullMeleeApproach`.

Setting this to `0` disables automatic ranged pulls for plain pull commands (the tank walks into melee range instead). Commanded pullback orders and explicit `ranged` arguments always open from range regardless of this toggle. Tank autonomous dungeon pulls are unaffected.

</details>

### `AiPlayerbot.RangedPullMeleeApproach`
Walk-in approach threshold in yards before commanded pulls open from range.
**Default:** `20` (yards) · **Values:** positive float · **Restart:** needed

<details>
<summary>How it works in detail</summary>

If the intended target of a commanded pull is farther away than this distance, the tank shoots from range rather than walking in, avoiding accidental body-pulls of nearby monster packs.

Requires `AiPlayerbot.RangedPull = 1`. Lower this distance to trigger ranged pulling more aggressively; raise it to encourage tanks to walk closer before opening.

</details>

### `AiPlayerbot.MinRandomBotChangeStrategyTime`
Minimum duration in seconds a pool bot retains its active strategy configuration before refreshing.
**Default:** `1800` (30 minutes; built in; the key is not in the template — add it to `aiplayerbot.conf` to change it) · **Values:** seconds · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Sets the minimum interval before a random pool bot recalculates its combat and non-combat strategies.

Prevents rapid strategy churning while ensuring bots rotate through diverse gameplay styles over time.

</details>

### `AiPlayerbot.MaxRandomBotChangeStrategyTime`
Maximum duration in seconds a pool bot retains its active strategy configuration before refreshing.
**Default:** `7200` (2 hours; built in; the key is not in the template — add it to `aiplayerbot.conf` to change it) · **Values:** seconds · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Sets the upper bound for periodic pool bot strategy updates.

Together with the minimum timer, bots choose a randomized refresh time between these two bounds.

</details>
