---
id: server-settings-economy
title: Economy & auction house settings
category: server
summary: Configuration settings for the simulated auction house market, vendor trips, and bot economy.
tags: [config, economy, ah, auction-house, vendor]
relates_to:
  - world-economy
  - world-social
  - server-settings-world-population
---

# Economy & Auction House Settings

These settings configure the simulated bot economy, including the native Auction House market service, vendor selling thresholds, bot loot drop adjustments, and travel mount cost-benefit rules. Settings go in `aiplayerbot.conf`; see [How configuration works](../configuration.md).

For player trading permissions with open-world bots, see [`AiPlayerbot.PoolBotTradeMode`](world-population.md#aiplayerbotpoolbottrademode). Administrative auction house commands (`.bot ah` / `.ahbot`) are documented in [Admin commands](../admin-commands.md).

## Summary

| Setting | Default | What it does |
| :--- | :---: | :--- |
| [`AiPlayerbot.AhMarketEnabled`](#aiplayerbotahmarketenabled) | `0` | Enables bots posting surplus items and bidding on the Auction House. |
| [`AiPlayerbot.AhMarketInterval`](#aiplayerbotahmarketinterval) | `120` | Interval in seconds between market appraisal and posting passes. |
| [`AiPlayerbot.AhMarketBatchSize`](#aiplayerbotahmarketbatchsize) | `1` | Maximum bots (one listing each) attempted per seller tick (1..5). |
| [`AiPlayerbot.AhMarketSyntheticSupply`](#aiplayerbotahmarketsyntheticsupply) | `0` | Injects synthetic profession materials and gear to stock empty auction houses. |
| [`AiPlayerbot.AhMarketBuyer`](#aiplayerbotahmarketbuyer) | `0` | Enables synthetic bot buyer orders to purchase player auctions. |
| [`AiPlayerbot.AhMarketBudgetUs`](#aiplayerbotahmarketbudgetus) | `2000` | Microsecond time budget ceiling per world tick for auction processing. |
| [`AiPlayerbot.AhMarketMaxOperations`](#aiplayerbotahmarketmaxoperations) | `32` | Maximum auction buyer and seller operations processed per tick. |
| [`AiPlayerbot.AhMarketChanceSell`](#aiplayerbotahmarketchancesell) | `10` | Percentage probability that an eligible bot posts surplus items for sale. |
| [`AiPlayerbot.AhMarketChanceBuy`](#aiplayerbotahmarketchancebuy) | `10` | Percentage probability that an eligible bot bids on or buys an auction. |
| [`AiPlayerbot.AhMarketBuyValue`](#aiplayerbotahmarketbuyvalue) | `80` | Required perceived value percentage before a bot purchases an auction. |
| [`AiPlayerbot.AhMarketMaxSpendPerBot`](#aiplayerbotahmarketmaxspendperbot) | `0` | Maximum copper for a single auction purchase (0 = unlimited). |
| [`AiPlayerbot.AhMarketMaxQuality`](#aiplayerbotahmarketmaxquality) | `4` | Maximum item quality (0 poor to 6 artifact) for synthetic auctions. |
| [`AiPlayerbot.AhMarketMaxLevel`](#aiplayerbotahmarketmaxlevel) | `60` | Maximum item level generated for synthetic auction stock. |
| [`AiPlayerbot.AhMarketDynamicLevel`](#aiplayerbotahmarketdynamiclevel) | `0` | Dynamically adjusts synthetic auction item levels to match online players. |
| [`AiPlayerbot.AhMarketLevelRefresh`](#aiplayerbotahmarketlevelrefresh) | `600` | Seconds between recalculations of dynamic auction stock levels. |
| [`AiPlayerbot.AhMarketVariance`](#aiplayerbotahmarketvariance) | `10` | Percentage price variance applied to baseline market valuations. |
| [`AiPlayerbot.AhMarketBidMin` / `AhMarketBidMax`](#aiplayerbotahmarketbidmin--ahmarketbidmax) | `75` / `90` (built in) | Percentage range of buyout price used for opening auction bids. |
| [`AiPlayerbot.AhMarketTimeMin` / `AhMarketTimeMax`](#aiplayerbotahmarkettimemin--ahmarkettimemax) | `8` / `24` (built in) | Duration range in hours for posted synthetic auction listings. |
| [`AiPlayerbot.AuctionPriceRefreshInterval`](#aiplayerbotauctionpricerefreshinterval) | `60` | Seconds between market price evaluation sweeps. |
| [`AiPlayerbot.VendorBatchMinCount` / `VendorBatchMinBagSpace`](#aiplayerbotvendorbatchmincount--vendorbatchminbagspace) | `8` / `60` | Inventory thresholds that trigger vendor selling trips for level 5+ bots. |
| [`AiPlayerbot.LowLevelVendorBatchMinCount` / `LowLevelVendorBatchMinBagSpace`](#aiplayerbotlowlevelvendorbatchmincount--lowlevelvendorbatchminbagspace) | `3` / `25` | Inventory thresholds that trigger vendor selling trips for sub-level 5 bots. |
| [`AiPlayerbot.LowLevelVendorMaxDistance`](#aiplayerbotlowlevelvendormaxdistance) | `600` | Maximum yard radius sub-level 5 bots travel to reach camp vendors. |
| [`AiPlayerbot.BotLootRateUncommon` / `BotLootRateRare`](#aiplayerbotbotlootrateuncommon--botlootraterare) | `1.0` / `1.0` (built in) | Multipliers boosting green and blue item drop rates for pool bots. |
| [`AiPlayerbot.MountBreakEvenFactor`](#aiplayerbotmountbreakevenfactor) | `1.5` | Safety factor applied to distance calculations before bots mount up. |

## Settings

### `AiPlayerbot.AhMarketEnabled`
Enables the autonomous Auction House market service for pool bots.
**Default:** `0` · **Values:** `0` off, `1` on · **Restart:** needed

<details>
<summary>How it works in detail</summary>

When enabled, random pool bots with surplus items in their bags will post trade goods, crafting materials, and Bind-on-Equip gear to the Auction House using native auction listings. Bots also evaluate active auctions and purchase gear upgrades.

This system requires `AiPlayerbot.RandomBotAutologin = 1`. Failed auction attempts are rate-limited per bot to prevent spam.

</details>

### `AiPlayerbot.AhMarketInterval`
Interval in seconds between auction appraisal passes for individual bots.
**Default:** `120` · **Values:** seconds (5..3600) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Defines how frequently an eligible bot checks its inventory for auctionable surplus items or scans market listings for gear upgrades.

</details>

### `AiPlayerbot.AhMarketBatchSize`
Maximum bots (one listing each) attempted per seller tick.
**Default:** `1` · **Values:** integers (1..5) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Caps the number of eligible seller bots attempted per seller tick (clamped between 1 and 5), with each chosen bot posting one item listing, spreading auction load across server ticks.

</details>

### `AiPlayerbot.AhMarketSyntheticSupply`
Injects simulated auctions to populate the market on low-population servers.
**Default:** `0` · **Values:** `0` off, `1` on · **Restart:** needed

<details>
<summary>How it works in detail</summary>

On servers with small populations, natural bot drops alone may leave the Auction House sparsely stocked.

When enabled, the service periodically generates synthetic listings of crafting supplies, consumables, and gear appropriate to server progression. Synthetic items are tagged internally and isolated from real player inventories.

</details>

### `AiPlayerbot.AhMarketBuyer`
Enables synthetic buyer orders to purchase auctions posted by human players.
**Default:** `0` · **Values:** `0` off, `1` on · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Simulates economic demand by having bots purchase realistically priced player auctions. When a purchase occurs, legitimate gold is delivered to the seller's mailbox via native in-game mail.

Purchases are organic: bids and buyouts only execute through bots that physically travel to an auctioneer during their roleplay phase.

</details>

### `AiPlayerbot.AhMarketBudgetUs`
Microsecond execution budget allocated per world tick for auction house processing.
**Default:** `2000` · **Values:** microseconds (100..20000) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Caps the CPU time spent on auction appraisal and bidding within any single server tick. The service yields work when this budget is reached, preventing auction processing from causing world tick latency.

</details>

### `AiPlayerbot.AhMarketMaxOperations`
Maximum number of auction transactions processed during a single server tick.
**Default:** `32` · **Values:** operations (1..256) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Places a hard ceiling on the number of auction creations, bids, or cancellations handled per tick, even if time remains within `AhMarketBudgetUs`.

</details>

### `AiPlayerbot.AhMarketChanceSell`
Percentage probability that an eligible bot with surplus items creates an auction listing.
**Default:** `10` · **Values:** percentage (0..100) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Determines how likely an eligible bot is to list qualifying surplus inventory on the Auction House during its market pass.

</details>

### `AiPlayerbot.AhMarketChanceBuy`
Percentage probability that an eligible bot browses the Auction House to buy an upgrade.
**Default:** `10` · **Values:** percentage (0..100) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Controls the probability that a bot with spare gold searches the auction house for gear upgrades or consumables during its market cycle.

</details>

### `AiPlayerbot.AhMarketBuyValue`
Perceived value percentage required before a bot commits to buying an auction.
**Default:** `80` · **Values:** percentage (`0..200`; `0` = never buys) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Determines price sensitivity when bots consider purchasing items. A higher threshold makes bots more discerning, requiring listings to be priced favorably relative to baseline market values.

</details>

### `AiPlayerbot.AhMarketMaxSpendPerBot`
Maximum copper amount for a single auction purchase.
**Default:** `0` · **Values:** copper amount (0 = unlimited) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Refuses any single bid or buyout above this price; it does not track cumulative spend.

</details>

### `AiPlayerbot.AhMarketMaxQuality`
Maximum item quality generated for synthetic auction supply listings.
**Default:** `4` · **Values:** `0` poor, `1` common, `2` uncommon, `3` rare, `4` epic, `5` legendary, `6` artifact (clamped 0..6) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Restricts the quality tier of items generated by `AiPlayerbot.AhMarketSyntheticSupply`. A value of `3` restricts synthetic auctions to rare (blue) items and below, while `4` allows epic (purple) items.

</details>

### `AiPlayerbot.AhMarketMaxLevel`
Maximum item level generated for synthetic auction supply listings.
**Default:** `60` · **Values:** item level (1..60) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Sets an upper bound on character level requirements for synthetically generated auction goods.

</details>

### `AiPlayerbot.AhMarketDynamicLevel`
Dynamically adjusts synthetic auction stock levels to match currently online players.
**Default:** `0` · **Values:** `0` static, `1` dynamic · **Restart:** needed

<details>
<summary>How it works in detail</summary>

When enabled, the service samples the level distribution of online human players and biases synthetic auction inventory toward items usable by those players.

</details>

### `AiPlayerbot.AhMarketLevelRefresh`
Interval in seconds between recalculations of dynamic market levels.
**Default:** `600` · **Values:** seconds (60..86400) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Defines how often the dynamic level sampler evaluates online player demographics when `AhMarketDynamicLevel = 1`.

</details>

### `AiPlayerbot.AhMarketVariance`
Percentage price variance applied to baseline market valuations.
**Default:** `10` · **Values:** percentage (0..100) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Introduces randomized price fluctuations above and below standard item values, creating realistic market variation across different sellers.

</details>

### `AiPlayerbot.AhMarketBidMin` / `AhMarketBidMax`
Percentage range of the buyout price used when calculating an auction's opening bid.
**Default:** `75` / `90` (built in; the key is not in the template — add it to `aiplayerbot.conf` to change it) · **Values:** percentage (1..100) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

When a bot posts an auction with a buyout price, its opening starting bid is rolled between these two percentages of the buyout price.

</details>

### `AiPlayerbot.AhMarketTimeMin` / `AhMarketTimeMax`
Duration range in hours for posted synthetic auction listings.
**Default:** `8` / `24` (built in; the key is not in the template — add it to `aiplayerbot.conf` to change it) · **Values:** hours (1..72) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Sets the minimum and maximum duration in hours for posted synthetic auctions before they expire if unpurchased.

</details>

### `AiPlayerbot.AuctionPriceRefreshInterval`
Interval in seconds between background auction price and market valuation updates.
**Default:** `60` · **Values:** seconds (5..3600) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Controls how frequently the Auction House pricing engine sweeps active listings to refresh market price averages and item valuations.

</details>

### `AiPlayerbot.VendorBatchMinCount` / `VendorBatchMinBagSpace`
Inventory thresholds that trigger vendor selling trips for bots at level 5 or higher.
**Default:** `8` / `60` · **Values:** item count (untuned upper bound) / bag-space percent · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Bots at level 5 or higher travel to merchant NPCs to sell unwanted items once their inventory holds at least `VendorBatchMinCount` vendor-trash items and total bag capacity is at least `VendorBatchMinBagSpace` percent full.

Bots also initiate vendor trips early if selling their inventory would cover the gold required for their next trainable class ability rank.

</details>

### `AiPlayerbot.LowLevelVendorBatchMinCount` / `LowLevelVendorBatchMinBagSpace`
Inventory thresholds that trigger vendor selling trips for beginner bots below level 5.
**Default:** `3` / `25` · **Values:** item count (untuned upper bound) / bag-space percent · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Beginner bots carry small starter bags and collect only a few grey items before running out of space.

These lowered thresholds ensure low-level characters sell their loot and afford early class spells instead of hoarding items indefinitely.

</details>

### `AiPlayerbot.LowLevelVendorMaxDistance`
Maximum yard distance sub-level 5 bots may travel to reach a vendor.
**Default:** `600` · **Values:** yards (`0` disables beginner vendor trips) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

General roleplay travel is restricted below level 5 to prevent fragile characters from wandering into higher-level hostile creatures. Setting this to `0` disables beginner vendor trips.

This exception allows low-level bots to navigate to local vendors within their immediate starting camp, capped to this yard radius and restricted to starter area level bands.

</details>

### `AiPlayerbot.BotLootRateUncommon` / `BotLootRateRare`
Drop rate multipliers for green (uncommon) and blue (rare) items dropped for pool bots.
**Default:** `1.0` / `1.0` (built in; the key is not in the template — add it to `aiplayerbot.conf` to change it) · **Values:** float multiplier (1.0..N) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Provides an optional loot boost exclusively for masterless random pool bots to stimulate their equipment progression and supply the Auction House.

When a pool bot kills a creature, uncommon and rare loot table entries roll an additional check scaled by this factor. Quest items, quest starters, and unique items at `max_count` are skipped. Other quality entries remain untouched. Boost events are logged to `bot_events.csv` as `BotLootBonus`. Real players, player-owned bots, and mixed player parties always use standard core drop rates.

A value of `1.0` disables the boost.

</details>

### `AiPlayerbot.MountBreakEvenFactor`
Safety factor applied to distance calculations before bots mount up for travel.
**Default:** `1.5` · **Values:** float multiplier · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Bots calculate whether mounting up saves time based on travel distance, mount cast time, and movement speed increase.

This multiplier adds a safety buffer to the calculated break-even distance. With the default factor of `1.5`, bots mount up when travel exceeds approximately 350 yards on a turtle mount (9% speed), 50 yards on a standard mount (60% speed), and 30 yards on an epic mount (100% speed).

Setting this to `0` removes the distance threshold, causing bots to mount for any movement.

</details>
