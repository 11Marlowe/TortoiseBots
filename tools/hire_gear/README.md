# Hire-gear offline data (issue #490, PR 1 step 1)

Static BIS snapshots + item tier classification. The final per-tier set
generator is a later step; this directory holds only source snapshots and
the ladder-tier table it will select from.

## Layout

- `sources/bisclassic/` — `parse.py` + `LICENSE.txt` (GPLv2) for
  [BestInSlotClassic](https://github.com/Warkdev/BestInSlotClassic) at pinned
  commit `302ab4ecca0c86b43441710dab840af5edd3ca3e` (fetched 2026-10-12).
  The addon's Lua files are not vendored; `parse.py <checkout>` reads
  `bis_data.lua`, `bis_ranking.lua`, `items.lua` and writes the parsed
  rankings to `data/bisclassic.json.gz` (gzip, reproducible bytes).
- `sources/wowisclassic/scrape.py` — polite scraper (1 req/s, UA string) for
  all class/spec/phase pages (pre-raid, 1..6) → `data/wowisclassic.json`.
- `sources/turtle_sheets/fetch.py` — Turtle class workbooks (xlsx export;
  gear hyperlinks are the Turtle item IDs) → `data/turtle_sheets.json`.
- Those two site snapshots are **not in the repository** (git-ignored since
  2026-10-15). Run `scrape.py` / `fetch.py` locally before `generate.py`,
  `classify_items.py` or `enchants/build.py`; without them those tools stop
  with a "run … first" error and write nothing. The applied SQL keeps only
  item ids and an origin label per row.
- `classify_items.py` — ladder tier 0..7 per item id from the world DB
  → `data/item_tiers.csv`.
- `turtle_sheets.json` (local): `{class, tab, list_name(Naxx/K40), slot, item_name,
  item_id, enchant}` — Naxx + Tower-of-Karazhan-40 lists only.
- `item_tiers.csv`: `item_id,name,tier(0..7),flags,evidence` — lowest tier
  wins; flags `rep`/`pvp`/`legendary`/`unobtainable`. Ladder: 0 base,
  1 pre-raid dungeons (230,429,229,289,329,800), 2 ZG/AQ20/Lower-Kara
  (309,509,532), 3 MC/Ony/Timbermaw-Hold + world bosses (409,249,819),
  4 BWL/ES (469,807), 5 AQ40 (531), 6 Naxx (533), 7 Tower of Karazhan (814).

## Re-running

Needs the world DB (read-only SELECTs). The generator itself takes DB
connection from env vars or CLI args — never hardcode hosts in committed
files.

```sh
git clone https://github.com/Warkdev/BestInSlotClassic /tmp/bisc
git -C /tmp/bisc checkout 302ab4ecca0c86b43441710dab840af5edd3ca3e
python3 sources/bisclassic/parse.py /tmp/bisc/data
python3 sources/wowisclassic/scrape.py        # ~3 min, 1 req/s
python3 -c "import openpyxl" || pip install openpyxl
python3 sources/turtle_sheets/fetch.py        # xlsx per workbook
HIRE_GEAR_DB_HOST=<db> HIRE_GEAR_DB_USER=<u> HIRE_GEAR_DB_PASS=<p> \
  python3 classify_items.py
# or: python3 classify_items.py --db-host <db> --db-user <u> --db-pass <p>
```

DB tables read: `creature`, `gameobject`, `creature_template`,
`creature_loot_template`, `gameobject_loot_template`,
`reference_loot_template`, `disenchant/fishing/mail/item_loot_template`,
`npc_vendor`, `quest_template`, `spell_template`, `item_template`,
`skill_line_ability` (unused — trainer-taught crafts default to base).

Logic mirrors `ai/playerbot/RandomItemMgr.cpp` `BuildSourceTierIndex` /
`ClassifySourceTier` (spawn-map tiers, loot index walk with reference
expansion, recipe reverse map, quest classification) on the separate 0..7
ladder. Reference: lowest-tier source wins; AQ/Naxx/ZG token turn-in quest
products inherit the token's source tier; summoned/event bosses without a
static spawn (Gandling, Urok, Ouro, …) fall back to their loot entry's
sibling tier.

## Sources & licences

- BestInSlotClassic data: GPLv2, (c) Warkdev + class-Discord authors; see
  `sources/bisclassic/LICENSE.txt`. Facts (rankings) parsed into JSON.
- wowisclassic.com pages: cross-check snapshot (item/enchant ids per slot);
  not vendored, generated locally by `scrape.py`.
- Turtle WoW class gear sheets (community theorycraft, "Created by Ash of
  Bad Apples …"): Naxx/K40 tier overlay; not vendored, generated locally by
  `fetch.py`.

## Audit (`audit.py`, issue #490)

Independent checker for the generated `data/hire_gear.csv`. Derives
everything itself from the world DB (read-only) + the CSV + the local
wowisclassic snapshot (if present). Deliberately does NOT read `generate.py`,
`classify_items.py` or `item_tiers.csv`.

```sh
# DB from env HIRE_GEAR_DB_HOST/_USER/_PASS/_NAME or --db-* flags;
# no infra names are committed anywhere.
python3 audit.py --report audit-report.md
```

Checks per row (failure list + summary counts): 1 equip (exists,
req_level ≤ 60, allowable_class, vanilla armor cap / weapon proficiency
from `playercreateinfo_spell` + trainer-taught table, slot vs
inventory_type with 15:{13,17,21} 16:{13,14,22,23} 17:{15,25,26,28},
2H-in-15 ⇒ 16 empty), 2 source tier (every loot/vendor/quest-choice/
crafted source tiered by max(recipe tier, reagent tiers, one crafted
level), map→tier ladder, summoned bosses via
`creature_template.script_name` → raid script dir; FAIL if item tier >
row tier, WARN if unverified), 3 forbidden (honor rank, BG rep
889/890/509/510/729/730, other rep incl. Ravenholdt 349, exact PvP rank
name prefixes, legendary), 4 ANY duplicate item id in one set FAILs
(stricter than core, which blocks only `ITEM_FLAG_UNIQUE_EQUIPPED`),
5 spec stat sanity (WARN; threat/DPS procs count for tanks, mana procs
for healers, trigger-spell text included), 6 progression (avg ilvl per
tier must not decrease; slot downgrades > 5 WARN; source-tier reverts
WARN as 6-revert), 7 wowisclassic cross-check (review-only agreement %; SKIP when the local
snapshot is missing),
8 completeness (every full set covers slots 0,1,2,4–17; single-row
racial overrides exempt), 9 new content (tiers 3/5/6/7 need ≥3 own-tier
items per set, WARN).

Hand fixes (`generate.py` `HAND_OVERRIDES`): a small commented table pins
named items where the ranked source provably misfires for the spec
(caster item ranked above a same-tier melee item, relic slots the source
has no rows for). Overrides validate like ranked picks and land with
`source bisclassic`, `flags` bit1 (hand-touched).
