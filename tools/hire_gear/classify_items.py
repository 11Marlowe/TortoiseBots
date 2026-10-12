#!/usr/bin/env python3
"""Classify every source item id into the hire-gear LADDER tier 0..7.

Ladder (issue #490):
  0 = base (world drop / vendor / quest / crafted outside the maps below)
  1 = maps 230,429,229,289,329,800 (pre-raid dungeons)
  2 = maps 309,509,532 (ZG, AQ20, Lower Kara)
  3 = maps 409,249,819 + world bosses (MC, Ony, Timbermaw Hold, Azuregos 6109,
      Kazzak 12397, Emeriss 14889, Lethon 14888, Taerar 14890, Ysondre 14887)
  4 = maps 469,807 (BWL, Emerald Sanctum)
  5 = map 531 (AQ40)
  6 = map 533 (Naxx)
  7 = map 814 (Tower of Karazhan)
Lowest tier wins. Reference loot templates are followed, and AQ/Naxx/ZG
token turn-in quest products inherit the token's source tier.

Flags: rep (item rep requirement, conditioned vendor, or rep-gated turn-in
quest), pvp (ITEM_FLAG_NO_DISENCHANT or honor rank), legendary (quality 5),
unobtainable (no source found at all).

DB: env HIRE_GEAR_DB_* (host/user/pass/db, defaults localhost/mangos/mangos/
tw_world) or CLI --db-host/--db-user/--db-pass/--db-name. Read-only SELECTs.

Logic mirrors ai/playerbot/RandomItemMgr.cpp BuildSourceTierIndex /
ClassifySourceTier (read-only reference): spawn-map tiers, loot index walk,
recipe reverse map, quest classification; but on the separate 0..7 ladder.

Output: data/item_tiers.csv (item_id,name,tier,flags,evidence)
"""
import argparse
import csv
import gzip
import json
import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
DATA = HERE / "data"
OUT = DATA / "item_tiers.csv"

MAP_TIER = {230: 1, 429: 1, 229: 1, 289: 1, 329: 1, 800: 1,
            309: 2, 509: 2, 532: 2,
            409: 3, 249: 3, 819: 3,  # 819 = Timbermaw Hold (Turtle 20-man)
            469: 4, 807: 4,
            531: 5, 533: 6, 814: 7}
WORLD_BOSSES = {6109, 12397, 14889, 14888, 14890, 14887,
                 # Turtle world-boss variants (same encounters, tier 3)
                 63107, 63108, 63109, 63110, 63111, 63112}
# Raid-faction rep gear allowed from its tier when RAID_REP_ALLOWED is ON
# (owner decision): faction id -> ladder tier.
RAID_REP_TIER = {270: 2,   # Zandalar Tribe (ZG)
                 609: 2,   # Cenarion Circle (AQ20 rewards)
                 910: 5,   # Brood of Nozdormu (AQ40)
                 529: 6}   # Argent Dawn, Naxx T3 turn-ins only
# PvP BG factions (never allowed as gear).
BG_FACTIONS = {889, 890, 509, 510, 730, 729}
# PvP name families (prefix/substring match, case-insensitive).
PVP_NAMES = ("knight-", "lieutenant", "champion's", "legionnaire's",
             "marshal's", "general's", "warlord's", "veteran's",
             "executor's", "sergeant's", "insignia",
             "medallion of the horde", "medallion of the alliance",
             "highlander's", "defiler's", "battlemaster's",
             "frostwolf", "stormpike", "outrider's", "outrunner's",
             "executioner's", "field medic's")
# Alterac Valley rewards this DB has no vendor/rep rows for: Stormpike/
# Frostwolf rep gear (19084-19104), AV quest rewards (19105-19113) and the
# exalted tomes/weapons/rings (19308-19325, incl. two test daggers).
AV_REWARDS = set(range(19084, 19114)) | set(range(19308, 19326))


def connect(args):
    import pymysql
    kw = {"host": args.db_host, "user": args.db_user,
          "password": args.db_pass, "database": args.db_name}
    if args.db_defaults:
        kw["read_default_file"] = args.db_defaults
    return pymysql.connect(**kw)


def q(cur, sql, params=()):
    cur.execute(sql, params)
    return cur.fetchall()


def build(cur, item_ids):
    # --- spawn tiers: creature id -> min map tier, plus max LADDER (>0) tier.
    # A raid boss with an extra open-world event spawn (Rajaxx 15341 on map
    # 1 as well as 509) must resolve to its raid map, not base.
    spawn_tier = {}
    spawn_max = {}
    for cid, c2, c3, c4, mapid in q(
            cur, "SELECT id, id2, id3, id4, map FROM creature"):
        t = MAP_TIER.get(mapid, 0)
        for c in (cid, c2, c3, c4):
            if not c:
                continue
            if c not in spawn_tier or t < spawn_tier[c]:
                spawn_tier[c] = t
            if t > 0 and (c not in spawn_max or t > spawn_max[c]):
                spawn_max[c] = t
    go_tier = {}
    for gid, mapid in q(cur, "SELECT id, map FROM gameobject"):
        t = MAP_TIER.get(mapid, 0)
        if gid and (gid not in go_tier or t < go_tier[gid]):
            go_tier[gid] = t

    tmpl = {e: (rank, loot, pp, sk) for e, rank, loot, pp, sk in q(
        cur, "SELECT entry, rank, loot_id, pickpocket_loot_id,"
             " skinning_loot_id FROM creature_template")}
    # loot entry -> mob entries using it (loot_id join): dragon loot entries
    # 2000146-49 belong to world-boss mobs, not to any spawn.
    loot_users = {}
    for e, loot in q(cur, "SELECT entry, loot_id FROM creature_template"
                          " WHERE loot_id<>0"):
        loot_users.setdefault(loot, []).append(e)
    # --- reference expansion
    refs = {}  # loot entry -> [ref ids]; gokey ("go", entry) for GO tables
    for entry, ref in q(cur, "SELECT entry, mincountOrRef FROM"
                             " creature_loot_template WHERE mincountOrRef < 0"):
        refs.setdefault(entry, []).append(-ref)
    for entry, ref in q(cur, "SELECT entry, mincountOrRef FROM"
                             " gameobject_loot_template WHERE mincountOrRef < 0"):
        refs.setdefault(("go", entry), []).append(-ref)
    ref_items = {}
    for entry, item in q(cur, "SELECT entry, item FROM reference_loot_template"):
        ref_items.setdefault(entry, []).append(item)

    # --- item -> min loot tier (walk every owning template once)
    loot_tier = {}

    def mark(item, tier):
        if item not in loot_tier or tier < loot_tier[item]:
            loot_tier[item] = tier

    # committed boss->map table for summoned/event bosses with loot rows but
    # no static spawn (boss_map.txt). Map 0 = world boss (tier 3 by rule).
    boss_map = {}
    with open(HERE / "boss_map.txt") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split(None, 2)
            boss_map[int(parts[0])] = int(parts[1])

    def owner_tier(e):
        """Tier of a loot owner, or None when unmappable (never guess 0).
        Priority: world boss (t3) > boss map > best ladder-map spawn.
        Open-world spawns are ignored when the creature also spawns in a
        ladder raid/dungeon map or is a mapped boss (Rajaxx event spawn)."""
        users = loot_users.get(e, [e])
        if any(u in WORLD_BOSSES for u in users):
            return 3
        maps = []
        for u in users:
            if u in boss_map:
                m = boss_map[u]
                maps.append(3 if m == 0 else MAP_TIER.get(m, 0))
        if maps:
            return min(maps)
        ladder = [spawn_max[u] for u in users if u in spawn_max]
        if ladder:
            # raid boss with an extra open-world event spawn (Rajaxx):
            # the raid map decides, not the world spawn.
            return max(ladder)
        if e in (2000146, 2000147, 2000148, 2000149):
            return 3
        return None

    for e, item in q(cur, "SELECT entry, item FROM creature_loot_template"
                          " WHERE mincountOrRef >= 0"):
        if e not in tmpl and e not in loot_users:
            continue
        t = owner_tier(e)
        if t is not None:
            mark(item, t)
    for table in ("pickpocketing_loot_template", "skinning_loot_template"):
        for e, item in q(cur, f"SELECT entry, item FROM {table}"
                              " WHERE mincountOrRef >= 0"):
            if e not in tmpl:
                continue
            t = owner_tier(e)
            if t is not None:
                mark(item, t)
    for e, item in q(cur, "SELECT entry, item FROM gameobject_loot_template"
                          " WHERE mincountOrRef >= 0"):
        if e in go_tier:
            mark(item, go_tier[e])
    for table in ("disenchant_loot_template", "fishing_loot_template",
                  "mail_loot_template", "item_loot_template"):
        try:
            for e, item in q(cur, f"SELECT entry, item FROM {table}"):
                if table == "item_loot_template":
                    # lootable containers resolve to the container's own
                    # tier (quest reward / vendor / mob loot), not base.
                    continue
                mark(item, 0)  # profession/world systems: base by definition
        except Exception:  # noqa: BLE001 - table missing on some cores
            pass
    for e, refids in refs.items():
        if isinstance(e, tuple):
            t = go_tier.get(e[1])
            if t is None:
                continue
        else:
            if e not in tmpl and e not in loot_users:
                continue
            t = owner_tier(e)
            if t is None:
                continue
        for ref in refids:
            for item in ref_items.get(ref, []):
                mark(item, t)
    # lootable containers (item_loot_template): contents inherit the
    # container's own tier (fixpoint: containers from quests resolve after
    # quest_reward is built below, so iterate to convergence).
    container_of = {}
    for e, item in q(cur, "SELECT entry, item FROM item_loot_template"):
        container_of.setdefault(e, []).append(item)

    # --- vendors / quests / recipes / item rows
    vendor_items = {i for (i,) in q(cur, "SELECT DISTINCT item FROM npc_vendor")}
    cond_vendor = {i for (i,) in q(
        cur, "SELECT DISTINCT item FROM npc_vendor WHERE condition_id <> 0")}
    quest_reward = {}
    for row in q(cur, "SELECT entry, RewItemId1, RewItemId2, RewItemId3,"
                      " RewItemId4, RewChoiceItemId1, RewChoiceItemId2,"
                      " RewChoiceItemId3, RewChoiceItemId4, RewChoiceItemId5,"
                      " RewChoiceItemId6 FROM quest_template"):
        for item in row[1:]:
            if item:
                quest_reward.setdefault(item, []).append(row[0])
    quest_info = {r[0]: r[1:] for r in q(
        cur, "SELECT entry, Type, SuggestedPlayers, ZoneOrSort,"
             " RequiredMinRepFaction, RequiredMinRepValue FROM quest_template")}
    req_source = {}  # turn-in component item -> [quest ids]
    for row in q(cur, "SELECT entry, ReqSourceId1, ReqSourceId2,"
                      " ReqSourceId3, ReqSourceId4 FROM quest_template"):
        for tok in row[1:]:
            if tok:
                req_source.setdefault(tok, []).append(row[0])
    for row in q(cur, "SELECT entry, ReqItemId1, ReqItemId2, ReqItemId3,"
                      " ReqItemId4 FROM quest_template"):
        for tok in row[1:]:
            if tok:
                req_source.setdefault(tok, []).append(row[0])
    # quest requirements per quest: ReqItemId + ReqSourceId + SrcItemId
    quest_req = {}
    for row in q(cur, "SELECT entry, ReqItemId1, ReqItemId2, ReqItemId3,"
                      " ReqItemId4, ReqSourceId1, ReqSourceId2, ReqSourceId3,"
                      " ReqSourceId4, SrcItemId FROM quest_template"):
        reqs = [x for x in row[1:] if x]
        if reqs:
            quest_req[row[0]] = reqs
    # container contents inherit the container's tier ONLY when the content
    # has no direct loot tier yet (raise-only: a quest-cache copy must not
    # drag an MC mob drop down to base).
    for _ in range(5):
        moved = False
        for cont, items in container_of.items():
            ct = loot_tier.get(cont)
            if ct is None:
                qs = quest_reward.get(cont, [])
                if qs:
                    ct = 0
                elif cont in vendor_items:
                    ct = 0
                else:
                    continue
            for it in items:
                if it not in loot_tier:
                    loot_tier[it] = ct
                    moved = True
        if not moved:
            break
    quest_prev = {}
    for qid, prev in q(cur, "SELECT entry, PrevQuestId FROM quest_template"
                             " WHERE PrevQuestId<>0"):
        quest_prev[qid] = prev
    # vendor rep factions via conditions (type 5 = reputation, value1 = faction)
    cond_rep = {}
    try:
        for cid, v1 in q(cur, "SELECT condition_entry, value1 FROM conditions"
                              " WHERE type=5"):
            cond_rep[cid] = v1
    except Exception:  # noqa: BLE001
        pass
    vendor_rep = {}  # item -> set(factions) from conditioned vendors
    for item, cid in q(cur, "SELECT item, condition_id FROM npc_vendor"
                            " WHERE condition_id<>0"):
        if cid in cond_rep:
            vendor_rep.setdefault(item, set()).add(cond_rep[cid])
    # honor/BG vendors: sell an honor-rank item => PvP vendor
    honor_items = {it for (it,) in q(
        cur, "SELECT entry FROM item_template WHERE required_honor_rank>0")}
    pvp_vendors = set()
    for v, it in q(cur, "SELECT entry, item FROM npc_vendor"):
        if it in honor_items:
            pvp_vendors.add(v)
    def quest_chain_tier(qid, depth=0):
        # walk PrevQuestId chain; first non-raid (non-62) quest sets base.
        # A pure Type-62 chain with no loot/token path stays quarantined.
        seen = set()
        while qid and qid not in seen and depth < 10:
            seen.add(qid)
            info = quest_info.get(qid, (0, 0, 0, 0, 0))
            if info[0] != 62:
                return 0
            qid = quest_prev.get(qid)
            depth += 1
        return None

    # recipe reverse map. Chain: pattern item (class 9) -> spellid_1 = craft
    # spell (Effect 24 CREATE, EffectItemType = product). The pattern's own
    # source (loot tier / vendor / quest / trainer-default-base) decides.
    creates = {}  # craft spell -> [products]
    for r in q(cur, "SELECT entry, effect1, effect2, effect3,"
                    " effectItemType1, effectItemType2, effectItemType3"
                    " FROM spell_template WHERE effect1=24 OR effect2=24"
                    " OR effect3=24"):
        effs = {1: r[1], 2: r[2], 3: r[3]}
        prods = [p for e, p in ((1, r[4]), (2, r[5]), (3, r[6]))
                 if effs[e] == 24 and p]
        if prods:
            creates[r[0]] = prods
    recipe_tier = {}  # product -> min tier over its patterns
    recipe_src = {}  # product -> evidence sample
    # reagent tiers: max ladder tier over a craft spell's reagents (each
    # reagent's own loot tier; vendor/quest/disenchant base = 0). Crafted
    # products take the MAX of pattern tier and reagent tier (Corehound Belt
    # needs MC Fiery Core -> t3; Lionheart's reagents are base -> stays t0).
    craft_reagents = {}
    for r in q(cur, "SELECT entry, reagent1, reagent2, reagent3, reagent4,"
                    " reagent5, reagent6, reagent7, reagent8 FROM spell_template"
                    " WHERE effect1=24 OR effect2=24 OR effect3=24"):
        regs = [x for x in r[1:] if x]
        if regs:
            craft_reagents[r[0]] = regs
    # pattern spellid_1 is usually a LEARN spell (effect 36) pointing at the
    # craft spell via effectTriggerSpell1; sometimes it IS the craft spell.
    learn_trig = {}
    for s, trig in q(cur, "SELECT entry, effectTriggerSpell1 FROM spell_template"
                          " WHERE effect1=36 AND effectTriggerSpell1<>0"):
        learn_trig[s] = trig
    for pattern, spell in q(cur, "SELECT entry, spellid_1 FROM item_template"
                                 " WHERE class=9 AND spellid_1<>0"):
        craft = learn_trig.get(spell, spell)
        if craft not in creates:
            continue
        t, sev = None, ""
        if pattern in loot_tier:
            t, sev = loot_tier[pattern], f"pattern-loot:t{loot_tier[pattern]}"
        elif pattern in vendor_items:
            t, sev = 0, "pattern-vendor"
        elif pattern in quest_reward:
            t, sev = 0, f"pattern-quest:{quest_reward[pattern][0]}"
        else:
            t, sev = 0, "pattern-trainer"
        reag_best = 0
        for reg in craft_reagents.get(craft, []):
            rt = loot_tier.get(reg, 0)
            if rt > reag_best:
                reag_best = rt
        if reag_best > (t or 0):
            t, sev = reag_best, f"reagents:t{reag_best}"
        for p in creates[craft]:
            if p not in recipe_tier or t < recipe_tier[p]:
                recipe_tier[p] = t
                recipe_src[p] = sev

    # items whose spellid_1 is an aura/proc, not a CREATE craft, with no
    # attested source (world drops, quest rewards missing from this DB,
    # Turtle content without loot rows): tier from item level, rounded up
    # to the first ladder tier whose content reaches that level. Too high
    # only delays an unlock; too low would hand out raid gear at t1.
    aura_items = {}
    for (entry, ilvl) in q(cur, "SELECT entry, item_level FROM item_template"
                                 " WHERE spellid_1<>0 AND class<>9"):
        aura_items[entry] = ilvl
    protos = {}
    if item_ids:
        ph = ",".join(["%s"] * len(item_ids))
        for r in q(cur, f"SELECT entry, name, quality, bonding,"
                        f" required_reputation_faction, required_reputation_rank,"
                        f" required_honor_rank, flags FROM item_template"
                        f" WHERE entry IN ({ph})", tuple(item_ids)):
            protos[r[0]] = r
    vendor_of_item = {}  # item -> [vendor entries selling it]
    for v, it in q(cur, "SELECT entry, item FROM npc_vendor"):
        vendor_of_item.setdefault(it, []).append(v)
    return {"loot_tier": loot_tier, "vendor": vendor_items,
            "cond_vendor": cond_vendor, "quest_reward": quest_reward,
            "quest_info": quest_info, "req_source": req_source,
            "quest_req": quest_req, "quest_prev": quest_prev,
            "recipe_tier": recipe_tier, "recipe_src": recipe_src,
            "quest_chain_tier": quest_chain_tier, "aura_items": aura_items,
            "vendor_rep": vendor_rep, "pvp_vendors": pvp_vendors,
            "vendor_of_item": vendor_of_item, "protos": protos,
            "raid_rep_tier": RAID_REP_TIER}


def nodisenchant_tier_bump(item, proto, tier, ev):
    """NO_DISENCHANT (0x8000) items with no loot/vendor/quest source whose
    name matches a PvP family are BG/PvP rewards missing vendor rows:
    quarantine at t7 (Frostwolf pendant). Others (Rhok'delar, Don Julio's,
    quest Tomes with no reward linkage in this DB) keep their item-level tier."""
    (entry, name, qual, bond, repf, repr_, honor, flags) = proto
    if (flags & 0x8000) and ev == ["proc-craft"]:
        low = (name or "").lower()
        if any(k in low for k in PVP_NAMES):
            return 7, ["nodisenchant-quarantine"]
    return tier, ev

# Highest item level per ladder tier (classic + Turtle raid loot).
ILVL_TIER = ((63, 1), (68, 2), (78, 3), (83, 4), (88, 5), (92, 6))


def ilvl_tier(ilvl):
    for top, tier in ILVL_TIER:
        if ilvl <= top:
            return tier
    return 7


def classify(item, ctx):
    loot_tier = ctx["loot_tier"]
    tier, ev = None, []

    if item in loot_tier:
        tier, ev = loot_tier[item], [f"loot:t{loot_tier[item]}"]
    if item in ctx["recipe_tier"]:
        rt = ctx["recipe_tier"][item]
        sev = ctx.get("recipe_src", {}).get(item, "")
        if tier is None:
            tier = rt
        ev.append(f"recipe:t{rt}" + (f":{sev}" if sev else ""))
    if item in ctx["vendor"]:
        ev.append("vendor")
    quests = ctx["quest_reward"].get(item, [])
    if quests:
        ev.append(f"quest:{quests[0]}")
        if tier is None:
            # raid quest = Type 62 ONLY (Type 1 elite and Type 81 dungeon are
            # base). A Type-62 quest chained behind a non-raid quest (e.g.
            # Quel'Serrar 7509 behind dungeon quest 7508) is base too.
            chained = None
            for x in quests:
                c = ctx["quest_chain_tier"](x) \
                    if "quest_chain_tier" in ctx else None
                if c is not None and (chained is None or c < chained):
                    chained = c
            if chained is not None:
                tier = chained
                ev.append("quest-chain")
            elif any(ctx["quest_info"].get(x, (0, 0, 0, 0, 0))[0] == 62
                     for x in quests):
                # raid-faction Friendly/Honored-gated quest rewards count as
                # available at that faction's tier (ZG trinkets -> t2 etc.).
                # (Owner rule; gate value still recorded in rep_flag.)
                fac_tier = None
                for x in quests:
                    info = ctx["quest_info"].get(x, (0, 0, 0, 0, 0))
                    fac = info[3]
                    if fac in ctx.get("raid_rep_tier", {}):
                        t = ctx["raid_rep_tier"][fac]
                        if fac_tier is None or t < fac_tier:
                            fac_tier = t
                if fac_tier is not None:
                    tier = fac_tier
                    ev.append(f"raid-rep-quest:t{fac_tier}")
                else:
                    tier = 7  # pure Type-62 chain, no path: quarantine
                    ev.append("raid-quest")
            else:
                tier = 0
    # turn-in products take the MAX tier over required components
    # (ReqItemId/ReqSourceId/SrcItemId loot tiers): raid turn-ins mix raid
    # tokens (Desecrated t6, Qiraji Bindings t5) with base reagents (Arcanite
    # Bar, Nexus Crystal) — the raid component decides, not min(). Quest
    # rewards likewise inherit required-item tiers (Dragonslayer's Ring needs
    # Head of Nefarian t3 -> t3). Requirements only ever raise the tier.
    comp_best = None
    seen_q = set()
    chain_q = list(quests)
    while chain_q:
        qid = chain_q.pop()
        if qid in seen_q:
            continue
        seen_q.add(qid)
        for tok in ctx.get("quest_req", {}).get(qid, []):
            t = loot_tier.get(tok)
            if t is not None and (comp_best is None or t > comp_best):
                comp_best = t
        prev = ctx.get("quest_prev", {}).get(qid)
        if prev:
            chain_q.append(prev)
    if comp_best is not None:
        if tier is None or "raid-quest" in ev or comp_best > tier:
            # a found component path overrides the quarantine (which only
            # means "no path found yet"); requirements otherwise only raise.
            tier = comp_best
        ev.append(f"token:t{comp_best}")
    if tier is None:
        if item in ctx["vendor"]:
            tier, ev = 0, ev + ["vendor"]
        elif item in ctx["recipe_tier"]:
            tier = ctx["recipe_tier"][item]
        elif item in ctx["aura_items"]:
            tier, ev = ilvl_tier(ctx["aura_items"][item]), ev + ["proc-craft"]
        else:
            # no source and no mapping: unknown, excluded until mapped.
            return None, ev
    return tier, ev
def is_pvp(item, proto, ctx):
    """Narrow PvP test (orchestrator rule): honor rank, BG-faction rep, sale
    by an honor vendor, or a known PvP name family. NO_DISENCHANT alone
    does NOT flag (it covers raid quest rewards like Zandalar Vindicator)."""
    (entry, name, qual, bond, repf, repr_, honor, flags) = proto
    if honor or item in AV_REWARDS:
        return True
    if repf in BG_FACTIONS:
        return True
    low = (name or "").lower()
    if any(k in low for k in PVP_NAMES):
        return True
    for v in ctx.get("vendor_of_item", {}).get(item, []):
        if v in ctx.get("pvp_vendors", set()):
            return True
    return False


def rep_flag(item, proto, ctx):
    """Rep requirement grouped by faction. Raid factions map to their tier
    (RAID_REP_TIER); anything else is plain excluded rep."""
    (entry, name, qual, bond, repf, repr_, honor, flags) = proto
    facs = set()
    if repf:
        facs.add(repf)
    for f in ctx.get("vendor_rep", {}).get(item, set()):
        facs.add(f)
    for x in ctx.get("quest_reward", {}).get(item, []):
        info = ctx.get("quest_info", {}).get(x)
        if info and info[3]:
            facs.add(info[3])
    if not facs:
        if item in ctx.get("cond_vendor", set()):
            return "rep", None
        return None, None
    raid = [RAID_REP_TIER[f] for f in facs if f in RAID_REP_TIER]
    other = [f for f in facs if f not in RAID_REP_TIER]
    if raid and not other:
        return "rep", min(raid)
    return "rep", None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--db-host", default=os.environ.get("HIRE_GEAR_DB_HOST", "localhost"))
    ap.add_argument("--db-user", default=os.environ.get("HIRE_GEAR_DB_USER", "mangos"))
    ap.add_argument("--db-pass", default=os.environ.get("HIRE_GEAR_DB_PASS", "mangos"))
    ap.add_argument("--db-name", default=os.environ.get("HIRE_GEAR_DB_NAME", "tw_world"))
    ap.add_argument("--db-defaults", default=os.environ.get("HIRE_GEAR_DB_DEFAULTS"))
    ap.add_argument("--no-raid-rep", action="store_true",
                    help="exclude raid-faction rep gear (default: allowed)")
    ap.add_argument("--raid-rep-allowed",
                    default=os.environ.get("HIRE_GEAR_RAID_REP_ALLOWED", "1"))
    args = ap.parse_args()

    ids = set()
    bc = json.load(gzip.open(DATA / "bisclassic.json.gz", "rt"))
    ids.update(r["item_id"] for r in bc["rows"])
    wic = json.loads((DATA / "wowisclassic.json").read_text())
    for r in wic["rows"]:
        ids.update(g["item_id"] for g in r["gear"])
    ts = json.loads((DATA / "turtle_sheets.json").read_text())
    ids.update(r["item_id"] for r in ts["rows"] if r["item_id"])
    ids = sorted(ids)
    print(f"items to classify: {len(ids)}", flush=True)

    db = connect(args)
    cur = db.cursor()
    ctx = build(cur, ids)
    print(f"in item_template: {len(ctx['protos'])}"
          f" loot-indexed: {len(ctx['loot_tier'])}", flush=True)

    raid_rep_on = not args.no_raid_rep and str(args.raid_rep_allowed) != "0"
    rows = []
    for item in sorted(ctx["protos"]):
        proto = ctx["protos"][item]
        (entry, name, qual, bond, repf, repr_, honor, flags) = proto
        tier, ev = classify(item, ctx)
        tier, ev = nodisenchant_tier_bump(item, proto, tier, ev)
        fl = []
        rf, raid_tier = rep_flag(item, proto, ctx)
        if rf:
            if raid_tier is not None and raid_rep_on:
                # raid-faction rep gear allowed from its tier: pin tier up,
                # keep the rep flag for visibility.
                if tier is None or raid_tier > tier:
                    tier = raid_tier
                ev.append(f"raid-rep:t{raid_tier}")
            fl.append("rep")
        if is_pvp(item, proto, ctx):
            fl.append("pvp")
        if qual == 5:
            fl.append("legendary")
        if tier is None:
            fl.append("unobtainable")
            tier = ""
        rows.append((item, name.replace(",", ";"), tier, "+".join(fl), ";".join(ev)))

    missing_ids = [i for i in ids if i not in ctx["protos"]]
    print(f"ids not in item_template: {len(missing_ids)} {missing_ids[:10]}",
          flush=True)

    OUT.parent.mkdir(parents=True, exist_ok=True)
    with open(OUT, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["item_id", "name", "tier", "flags", "evidence"])
        w.writerows(rows)
    print(f"wrote {len(rows)} -> {OUT}")


if __name__ == "__main__":
    sys.exit(main())
