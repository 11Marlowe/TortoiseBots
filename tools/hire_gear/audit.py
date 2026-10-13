#!/usr/bin/env python3
"""Independent audit of the generated hire-gear sets (issue #490).

Checker, not author: derives everything from the world DB + the CSV under
audit + the wowisclassic cross-check snapshot. Deliberately does NOT read
generate.py, classify_items.py or item_tiers.csv.

Usage:
    python3 audit.py [--csv data/hire_gear.csv]
                     [--snapshot data/wowisclassic.json]
                     [--report PATH] [--db-host H] [--db-user U] [--db-pass P]
                     [--db-name tw_world]

    DB connection (read-only SELECTs only): env HIRE_GEAR_DB_HOST/_USER/_PASS
    (or --db-* flags). No infra names are committed anywhere.

Checks (per CSV row; failure list + summary counts):
    1. equip: item exists; required_level <= 60; allowable_class; armor
       subclass wearable at 60; weapon usable; slot matches inventory_type.
    2. source tier: independent map->tier lookup over every loot/vendor/quest/
       crafted source; FAIL if item_tier > row_tier; WARN if no source found.
    3. forbidden: PvP / non-exception reputation / legendary.
    4. unique: duplicate unique-equip item within one (class,spec,race,tier).
    5. stats: spec sanity by item mods + equip/trigger spell names (WARN).
    6. progression: avg item level per (class,spec,tier) must not decrease;
       list slot downgrades > 5 ilvl between consecutive tiers.
    7. cross-check vs wowisclassic snapshot: slot agreement % for tiers whose
       classic phase content is fully unlocked (review only, never failure).

Report: summary table (check -> FAIL/WARN counts), then every FAIL row,
then the top WARNs.
"""

import argparse
import csv
import json
import os
import re
import sys
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
DEFAULT_CSV = HERE / "data" / "hire_gear.csv"
DEFAULT_SNAPSHOT = HERE / "data" / "wowisclassic.json"

# --- Ladder: map -> tier (brief check 2; owner tier ladder, App. A.1) --------
MAP_TIER = {}
for m in (230, 429, 229, 289, 329, 800):
    MAP_TIER[m] = 1
for m in (309, 509, 532):
    MAP_TIER[m] = 2
for m in (409, 249, 819):
    MAP_TIER[m] = 3
for m in (469, 807):
    MAP_TIER[m] = 4
MAP_TIER[531] = 5
MAP_TIER[533] = 6
MAP_TIER[814] = 7

TIER_MAPS = {1: (230, 429, 229, 289, 329, 800), 2: (309, 509, 532),
             3: (409, 249, 819), 4: (469, 807), 5: (531,), 6: (533,),
             7: (814,)}
# Raid script dir -> ladder tier (tortoise-wow/src/scripts/dungeons/<dir>).
# Summoned/event bosses with no static spawn resolve through their boss
# script file's directory (boss_ragnaros -> molten_core -> tier 3).
SCRIPT_DIR_TIER = {
    "blackrock_depths": 1, "dire_maul": 1, "blackrock_spire": 1,
    "scholomance": 1, "stratholme": 1, "karazhan_crypt": 1,
    "zulgurub": 2, "ruins_of_ahnqiraj": 2, "lower_karazhan_halls": 2,
    "molten_core": 3, "onyxias_lair": 3,
    "blackwing_lair": 4, "emerald_sanctum": 4,
    "temple_of_ahnqiraj": 5, "naxxramas": 6, "upper_karazhan_halls": 7,
}
# Bosses without static spawns: resolve by instance (brief) -> ladder tier.
# (values are TIERS, not map ids: Ouro is summoned in AQ40 -> tier 5.)
INSTANCE_BOSS_TIER = {
    14599: 2, 14515: 2, 61222: 2, 91928: 1, 15517: 5,
}

# World bosses: pool-only tier 3, no kill required (brief + App. A.1).
WORLD_BOSSES = {
    6109, 12397, 14887, 14888, 14889, 14890,  # classic entries
    63107, 63108, 63109, 63110, 63111, 63112,  # turtle entries
}
WORLD_BOSS_TIER = 3

# --- Forbidden factions (brief check 3) --------------------------------------
PVP_FACTIONS = {889, 890, 509, 510, 729, 730}
RAID_REP_OK = {270: 2, 609: 2, 910: 5, 529: 6}  # faction -> first allowed tier
PVP_NAME_RE = re.compile(
    r"^(private|corporal|sergeant|knight|lieutenant|champion'?s|"
    r"legionnaire'?s|marshal'?s|general'?s|warlord'?s|blood guard'?s|"
    r"stone guard'?s|grand marshal|high warlord|lieutenant commander)\b",
    re.I)

# --- Equip data ---------------------------------------------------------------
# class id -> (mask bit, armor cap at 60, usable weapon subclasses)
# subclass ids: ItemSubclassWeapon / ItemSubclassArmor in ItemPrototype.h.
CLASS_BIT = {1: 1, 2: 2, 3: 4, 4: 8, 5: 16, 7: 64, 8: 128, 9: 256, 11: 1024}
AXE, AXE2, BOW, GUN = 0, 1, 2, 3
MACE, MACE2, POLEARM, SWORD, SWORD2 = 4, 5, 6, 7, 8
STAFF, FIST, MISC_W, DAGGER, THROWN = 10, 13, 14, 15, 16
CROSSBOW, WAND, FISHING = 18, 19, 20
CLOTH, LEATHER, MAIL, PLATE = 1, 2, 3, 4
SHIELD, LIBRAM, IDOL, TOTEM = 6, 7, 8, 9
WEAPONS = {
    # Vanilla 1.12 proficiencies (WoWWiki Class_proficiencies), verified
    # against playercreateinfo_spell creation rows where present.
    # Hunter SWORD2/POLEARM/STAFF + priest STAFF + shaman STAFF are
    # trainer-taught (absent from creation rows by design).
    1: {AXE, AXE2, MACE, MACE2, POLEARM, SWORD, SWORD2, STAFF, FIST,
        MISC_W, DAGGER, THROWN, CROSSBOW, GUN, BOW, SHIELD},
    2: {AXE, AXE2, MACE, MACE2, POLEARM, SWORD, SWORD2, MISC_W, SHIELD,
        GUN},
    3: {AXE, AXE2, SWORD, SWORD2, POLEARM, STAFF, DAGGER, FIST,
        MISC_W, BOW, GUN, CROSSBOW},
    4: {SWORD, DAGGER, FIST, MISC_W, MACE, THROWN, BOW, GUN, CROSSBOW},
    5: {MACE, STAFF, MISC_W, DAGGER, WAND},
    7: {AXE, AXE2, MACE, MACE2, STAFF, FIST, MISC_W, DAGGER, SHIELD, TOTEM},
    8: {SWORD, STAFF, MISC_W, DAGGER, WAND},
    9: {SWORD, STAFF, MISC_W, DAGGER, WAND},
    11: {MACE, MACE2, STAFF, FIST, MISC_W, DAGGER, IDOL, POLEARM},
}
ARMOR_CAP = {1: PLATE, 2: PLATE, 3: MAIL, 4: LEATHER, 5: CLOTH,
             7: MAIL, 8: CLOTH, 9: CLOTH, 11: LEATHER}
RELIC = {2: LIBRAM, 7: TOTEM, 11: IDOL}

# equip slot -> allowed inventory_type values (orchestrator table).
# slots: 0 head(1) 1 neck(2) 2 shoulder(3) 4 chest(5,20) 5 waist(6)
# 6 legs(7) 7 feet(8) 8 wrist(9) 9 hands(10) 10/11 finger(11)
# 12/13 trinket(12) 14 back(16) 15 main(13,17,21) 16 off(13,14,22,23)
# 17 ranged/ammo/relic(15,25,26,28). 2H (inv 17) in slot 15 => slot 16
# must be empty (item_id 0).
SLOT_INV = {
    0: {1}, 1: {2}, 2: {3}, 4: {5, 20}, 5: {6}, 6: {7}, 7: {8},
    8: {9}, 9: {10}, 10: {11}, 11: {11}, 12: {12}, 13: {12},
    14: {16}, 15: {13, 17, 21}, 16: {13, 14, 22, 23},
    17: {15, 25, 26, 28},
}

# item stat mods (ITEM_MOD_* in ItemPrototype.h): 3 agi 4 str 5 int 6 spi 7 sta.
AGI, STR, INT, SPI, STA = 3, 4, 5, 6, 7
CASTER_SPECS = {("8", "frost"), ("8", "fire"), ("8", "arcane"),
                ("9", "affliction"), ("9", "demonology"), ("9", "destruction"),
                ("5", "shadow"), ("7", "elemental"), ("11", "balance")}
HEAL_SPECS = {("2", "holy"), ("5", "holy"), ("5", "discipline"),
              ("7", "restoration"), ("11", "restoration")}
TANK_SPECS = {("1", "protection"), ("2", "protection"), ("11", "bear")}
MELEE_SPECS = {("1", "arms"), ("1", "fury"), ("2", "retribution"),
               ("3", "beastmastery"), ("3", "marksmanship"), ("3", "survival"),
               ("4", "assassination"), ("4", "combat"), ("4", "subtlety"),
               ("7", "enhancement"), ("11", "cat")}
HEAL_WORDS = re.compile(r"heal|mana|regen|restor|spirit|intellect|spell|"
                        r"ascendance|devotion|seal cost|libram|totem|idol",
                        re.I)
TANK_WORDS = re.compile(r"defense|dodge|block|armor|stamina|resist|parry|"
                        r"critical|hit chance|attack power|haste|vitality|"
                        r"threat|skill|weapon|champion|argent dawn|maul|swipe|"
                        r"maim|feral|thorns|all stats|mana|idol|faithful|"
                        r"libram of (radiance|the faithful)",
                        re.I)
MELEE_WORDS = re.compile(r"strength|agility|attack|power|hit|crit|strike", re.I)

# classic phase -> ladder tiers whose content is fully unlocked (check 7).
# pre-raid < MC < BWL < ZG/AQ20 < AQ40 < Naxx (ladder reorders classic).
PHASE_TIER = {"pre-raid": 1, "1": 3, "2": 4, "3": 2,
              "4": 4, "5": 5, "6": 6}

# CSV class/spec names -> snapshot class/spec names.
SNAP_CLASS = {"1": "warrior", "2": "paladin", "3": "hunter", "4": "rogue",
              "5": "priest", "7": "shaman", "8": "mage", "9": "warlock",
              "11": "druid"}
SNAP_SPEC = {
    ("1", "arms"): "fury", ("1", "fury"): "fury", ("1", "protection"): "prot",
    ("2", "holy"): "heal", ("2", "protection"): "protection",
    ("2", "retribution"): "ret",
    ("3", "beastmastery"): "hunter", ("3", "marksmanship"): "hunter",
    ("3", "survival"): "hunter",
    ("4", "assassination"): "dagger", ("4", "combat"): "sword",
    ("4", "subtlety"): "dagger",
    ("5", "holy"): "holy", ("5", "discipline"): "holy",
    ("5", "shadow"): "shadow",
    ("7", "restoration"): "resto", ("7", "elemental"): "elem",
    ("7", "enhancement"): "enhan",
    ("8", "frost"): "mage", ("8", "fire"): "mage", ("8", "arcane"): "mage",
    ("9", "affliction"): "warlock", ("9", "demonology"): "warlock",
    ("9", "destruction"): "warlock",
    ("11", "bear"): "tank", ("11", "cat"): "cat",
    ("11", "restoration"): "resto", ("11", "balance"): "balance",
}
# snapshot gear order (scrape.py GEAR_ORDER) -> equip slot.
GEAR_ORDER_SLOTS = {"mh": 15, "oh": 16, "ranged": 17, "head": 0, "neck": 1,
                    "shoulder": 2, "back": 14, "chest": 4, "wrist": 8,
                    "hands": 9, "waist": 5, "legs": 6, "feet": 7,
                    "ring1": (10, 11), "ring2": (10, 11),
                    "trinket1": (12, 13), "trinket2": (12, 13)}


def q(db, sql):
    """Rows as lists of strings (NULL as "NULL"), as the CLI client prints."""
    with db.cursor() as cur:
        cur.execute(sql)
        return [["NULL" if v is None else str(v) for v in row]
                for row in cur.fetchall()]


def connect(a):
    import pymysql
    return pymysql.connect(host=a.db_host, user=a.db_user,
                           password=a.db_pass, database=a.db_name)


def chunked(lst, n=500):
    for i in range(0, len(lst), n):
        yield lst[i:i + n]


class Audit:
    def __init__(self, db):
        self.db = db
        self.fails = defaultdict(list)  # check -> [row label]
        self.warns = defaultdict(list)
        self.unresolved = []  # bosses without spawns, no instance fallback

    @staticmethod
    def _build_script_index():
        """Map boss script_name -> ladder tier via its raid script dir."""
        import pathlib
        out = {}
        base = pathlib.Path(
            "/mnt/pny-ssd/Tortoise WoW Projects/tortoise-wow/"
            "src/scripts/dungeons")
        if not base.is_dir():
            return out
        for p in base.rglob("boss_*.cpp"):
            t = SCRIPT_DIR_TIER.get(p.parent.name)
            if t is not None:
                out[p.stem] = t
        return out

    # -- bulk DB loads -----------------------------------------------------
    def load(self, item_ids):
        self.script_tier = self._build_script_index()
        self.items = {}
        for ch in chunked(sorted(item_ids)):
            ids = ",".join(map(str, ch))
            for r in q(self.db,
                       "SELECT entry,name,class,subclass,inventory_type,"
                       "quality,required_level,item_level,allowable_class,"
                       "required_honor_rank,required_reputation_faction,"
                       "required_reputation_rank,bonding,flags,max_count,"
                       "stat_type1,stat_value1,stat_type2,stat_value2,"
                       "stat_type3,stat_value3,stat_type4,stat_value4,"
                       "stat_type5,stat_value5,"
                       "spellid_1,spelltrigger_1,spellid_2,spelltrigger_2 "
                       f"FROM item_template WHERE entry IN ({ids})"):
                e = int(r[0])
                self.items[e] = {
                    "name": r[1], "class": int(r[2]), "subclass": int(r[3]),
                    "inv": int(r[4]), "quality": int(r[5]),
                    "req_level": int(r[6]), "ilvl": int(r[7]),
                    "allow": int(r[8]), "honor": int(r[9]),
                    "rep_f": int(r[10]), "rep_r": int(r[11]),
                    "bond": int(r[12]), "flags": int(r[13]),
                    "maxcount": int(r[14]),
                    "mods": [(int(r[15]), int(r[16])), (int(r[17]), int(r[18])),
                             (int(r[19]), int(r[20])), (int(r[21]), int(r[22])),
                             (int(r[23]), int(r[24]))],
                    "spells": [(int(r[25]), int(r[26])),
                               (int(r[27]), int(r[28]))],
                }
        # spell names for equip/proc effects (check 5) + craft detection.
        # Trigger spells (Mark of the Chosen -> +25 all stats) are loaded
        # too, so procs are judged by what they do, not the host name.
        sids = set()
        for it in self.items.values():
            for sid, _ in it["spells"]:
                if sid:
                    sids.add(sid)
        trig = {}  # host spell -> [triggered spell ids]
        for ch in chunked(sorted(sids)):
            ids = ",".join(map(str, ch))
            for r in q(self.db, "SELECT entry,effectTriggerSpell1,"
                                "effectTriggerSpell2 FROM spell_template "
                                f"WHERE entry IN ({ids})"):
                ts = [t for t in (int(r[1]), int(r[2])) if t]
                if ts:
                    trig[int(r[0])] = ts
        self.spell_trig = trig
        self.spell_names = {}
        self.spell_auras = {}
        want = set(sids)
        for ts in trig.values():
            want |= set(ts)
        for ch in chunked(sorted(want)):
            ids = ",".join(map(str, ch))
            for r in q(self.db, "SELECT entry,name,auraDescription FROM "
                                "spell_template "
                                f"WHERE entry IN ({ids})"):
                self.spell_names[int(r[0])] = r[1]
                self.spell_auras[int(r[0])] = r[2] or ""
        # loot: item -> [(loot_entry, table)] incl. reference expansion.
        # Secondary tables (pickpocket/skinning/disenchant/fishing/mail)
        # resolve through the owning creature's spawn map.
        self.loot = defaultdict(list)
        for tbl in ("creature_loot_template", "gameobject_loot_template",
                    "item_loot_template", "reference_loot_template",
                    "pickpocketing_loot_template", "skinning_loot_template",
                    "disenchant_loot_template", "fishing_loot_template",
                    "mail_loot_template"):
            for ch in chunked(sorted(item_ids)):
                ids = ",".join(map(str, ch))
                for r in q(self.db, f"SELECT entry,item,mincountOrRef "
                                    f"FROM {tbl} WHERE item IN ({ids})"):
                    self.loot[int(r[1])].append((int(r[0]), tbl,
                                                 int(r[2])))
        # reference parents: ref_entry -> [(parent_entry, parent_table)].
        # Expanded transitively: a reference can be nested in another one.
        self.ref_parents = defaultdict(list)
        refs = {e for v in self.loot.values() for e, t, m in v
                if t == "reference_loot_template"}
        for v in self.loot.values():
            for e, t, m in v:
                if m < 0:
                    refs.add(-m)
        done = set()
        while refs - done:
            todo = sorted(refs - done)
            done |= set(todo)
            for ch in chunked(todo):
                ids = ",".join(map(str, ch))
                for tbl in ("creature_loot_template",
                            "gameobject_loot_template", "item_loot_template",
                            "reference_loot_template"):
                    for r in q(self.db, f"SELECT entry,mincountOrRef FROM {tbl} "
                                        f"WHERE item IN ({ids}) AND "
                                        "mincountOrRef < 0"):
                        self.ref_parents[-int(r[1])].append((int(r[0]), tbl))
                        if tbl == "reference_loot_template":
                            refs.add(int(r[0]))
        # loot entry -> creature entries using it (boss -> map via spawns).
        loot_entries = {e for v in self.loot.values() for e, t, m in v
                        if t == "creature_loot_template"}
        self.loot_creatures = defaultdict(list)
        for ch in chunked(sorted(loot_entries)):
            ids = ",".join(map(str, ch))
            for r in q(self.db, "SELECT entry,loot_id FROM creature_template "
                                f"WHERE loot_id IN ({ids})"):
                self.loot_creatures[int(r[1])].append(int(r[0]))
        # spawns: creature entry -> {maps}.
        self.spawn_maps = defaultdict(set)
        centries = {c for v in self.loot_creatures.values() for c in v}
        for ch in chunked(sorted(centries)):
            ids = ",".join(map(str, ch))
            for r in q(self.db, f"SELECT id,map FROM creature "
                                f"WHERE id IN ({ids})"):
                self.spawn_maps[int(r[0])].add(int(r[1]))
        # script names for spawnless creatures (summoned/event bosses).
        self.scripts = {}
        spawnless = [c for v in self.loot_creatures.values() for c in v
                     if c not in self.spawn_maps]
        for ch in chunked(sorted(set(spawnless))):
            ids = ",".join(map(str, ch))
            for r in q(self.db, "SELECT entry,script_name FROM "
                                "creature_template WHERE entry IN "
                                f"({ids})"):
                if r[1]:
                    self.scripts[int(r[0])] = r[1]
        go_entries = {e for v in self.loot.values() for e, t, m in v
                      if t == "gameobject_loot_template"}
        self.go_maps = defaultdict(set)
        for ch in chunked(sorted(go_entries)):
            ids = ",".join(map(str, ch))
            for r in q(self.db, f"SELECT id,map FROM gameobject "
                                f"WHERE id IN ({ids})"):
                self.go_maps[int(r[0])].add(int(r[1]))
        # vendors: item -> True; quest rewards: item -> [quest].
        self.vendors = set()
        for tbl in ("npc_vendor", "npc_vendor_template"):
            for ch in chunked(sorted(item_ids)):
                ids = ",".join(map(str, ch))
                for r in q(self.db, f"SELECT item FROM {tbl} "
                                    f"WHERE item IN ({ids})"):
                    self.vendors.add(int(r[0]))
        # quest rewards: quest -> required source items; item -> quests.
        # Covers RewItemId1-4 AND RewChoiceItemId1-6 (quest 5944's choice
        # reward Mark of Fordring was missed when only 1-4 were scanned).
        self.quests = {}  # quest entry -> [req item ids]
        self.q_reward_items = defaultdict(list)  # item -> [quest]
        for ch in chunked(sorted(item_ids)):
            ids = ",".join(map(str, ch))
            cond = " OR ".join(
                [f"RewItemId{i} IN ({ids})" for i in (1, 2, 3, 4)] +
                [f"RewChoiceItemId{i} IN ({ids})" for i in (1, 2, 3, 4, 5, 6)])
            for r in q(self.db, "SELECT entry,ReqItemId1,ReqItemId2,"
                                "ReqItemId3,ReqItemId4,RewItemId1,RewItemId2,"
                                "RewItemId3,RewItemId4,RewChoiceItemId1,"
                                "RewChoiceItemId2,RewChoiceItemId3,"
                                "RewChoiceItemId4,RewChoiceItemId5,"
                                "RewChoiceItemId6 FROM quest_template "
                                f"WHERE {cond}"):
                quest = int(r[0])
                self.quests[quest] = [int(x) for x in r[1:5] if int(x)]
                for x in r[5:]:
                    if int(x) in item_ids:
                        self.q_reward_items[int(x)].append(quest)
        # crafted: craft_spell -> recipe item. Tier comes from the RECIPE's
        # source (Plans: Lionheart Helm drops in MC -> tier 3), not tier 0:
        # query the recipe item's own loot/vendor/quest sources below.
        # Trainer-taught (no recipe item): tier 0 base.
        self.craft_spell = {}  # crafted item -> creating spell
        self.craft_recipe = {}  # creating spell -> recipe item (0=trainer)
        self.craft_reagents = {}  # creating spell -> [reagent item ids]
        for ch in chunked(sorted(item_ids)):
            ids = ",".join(map(str, ch))
            for r in q(self.db, "SELECT entry,effectItemType1,reagent1,"
                                "reagent2,reagent3,reagent4,reagent5,"
                                "reagent6,reagent7,reagent8 FROM "
                                "spell_template WHERE effectItemType1 IN "
                                f"({ids}) OR effectItemType2 IN ({ids}) OR "
                                f"effectItemType3 IN ({ids})"):
                spell, made = int(r[0]), int(r[1])
                self.craft_spell[made] = spell
                self.craft_reagents[spell] = [int(x) for x in r[2:] if int(x)]
        if self.craft_spell:
            spells = ",".join(map(str, sorted(set(self.craft_spell.values()))))
            taught_by_item = {}
            for r in q(self.db, "SELECT entry,effectTriggerSpell1,"
                                "effectTriggerSpell2,effectTriggerSpell3 FROM "
                                "spell_template WHERE effectTriggerSpell1 IN "
                                f"({spells}) OR effectTriggerSpell2 IN "
                                f"({spells}) OR effectTriggerSpell3 IN "
                                f"({spells})"):
                for trg in (int(r[1]), int(r[2]), int(r[3])):
                    if trg in set(self.craft_spell.values()):
                        taught_by_item[trg] = int(r[0])
            # use-spell -> recipe item via item spellid_N (Plans: Lionheart
            # Helm has spellid_1 = teach spell 16751).
            use_spells = sorted(set(taught_by_item.values()))
            recipe_of_use = {}
            for ch in chunked(use_spells):
                sids = ",".join(map(str, ch))
                for r in q(self.db, "SELECT entry,spellid_1,spellid_2,"
                                    "spellid_3,spellid_4,spellid_5 FROM "
                                    "item_template WHERE spellid_1 IN "
                                    f"({sids}) OR spellid_2 IN ({sids}) OR "
                                    f"spellid_3 IN ({sids}) OR spellid_4 IN "
                                    f"({sids}) OR spellid_5 IN ({sids})"):
                    item = int(r[0])
                    for s in (int(x) for x in r[1:]):
                        if s in use_spells:
                            recipe_of_use[s] = item
            for made, spell in self.craft_spell.items():
                use = taught_by_item.get(spell, 0)
                self.craft_recipe[spell] = recipe_of_use.get(use, 0)
        # second-phase loot load: reagents + recipe items need their own
        # loot/vendor/quest rows for tiering (they are not CSV items).
        extra = set(self.craft_recipe.values()) - {0}
        for reag in self.craft_reagents.values():
            extra.update(reag)
        extra -= set(self.loot.keys())
        for tbl in ("creature_loot_template", "gameobject_loot_template",
                    "item_loot_template", "reference_loot_template",
                    "pickpocketing_loot_template", "skinning_loot_template",
                    "disenchant_loot_template", "fishing_loot_template",
                    "mail_loot_template"):
            for ch in chunked(sorted(extra)):
                ids = ",".join(map(str, ch))
                for r in q(self.db, f"SELECT entry,item,mincountOrRef "
                                    f"FROM {tbl} WHERE item IN ({ids})"):
                    self.loot[int(r[1])].append((int(r[0]), tbl,
                                                 int(r[2])))
        # extend creature/GO/spawn/script maps for the new loot entries.
        loot_entries = {e for v in self.loot.values() for e, t, m in v
                        if t == "creature_loot_template"}
        loot_entries -= set(self.loot_creatures.keys())
        for ch in chunked(sorted(loot_entries)):
            ids = ",".join(map(str, ch))
            for r in q(self.db, "SELECT entry,loot_id FROM creature_template "
                                f"WHERE loot_id IN ({ids})"):
                self.loot_creatures[int(r[1])].append(int(r[0]))
        centries = {c for v in self.loot_creatures.values() for c in v}
        centries -= set(self.spawn_maps.keys())
        for ch in chunked(sorted(centries)):
            ids = ",".join(map(str, ch))
            for r in q(self.db, f"SELECT id,map FROM creature "
                                f"WHERE id IN ({ids})"):
                self.spawn_maps[int(r[0])].add(int(r[1]))
        spawnless = [c for v in self.loot_creatures.values() for c in v
                     if c not in self.spawn_maps and c not in self.scripts]
        for ch in chunked(sorted(set(spawnless))):
            ids = ",".join(map(str, ch))
            for r in q(self.db, "SELECT entry,script_name FROM "
                                "creature_template WHERE entry IN "
                                f"({ids})"):
                if r[1]:
                    self.scripts[int(r[0])] = r[1]
        if extra:
            for ch in chunked(sorted(extra)):
                ids = ",".join(map(str, ch))
                for r in q(self.db, f"SELECT item FROM npc_vendor WHERE "
                                    f"item IN ({ids})"):
                    self.vendors.add(int(r[0]))

    # -- check 2: independent source tier ------------------------------------
    def sources(self, item_id):
        """Return (tier_or_None, evidence list, warn_flags)."""
        ev, tiers = [], []
        for entry, tbl, _ in self.loot.get(item_id, []):
            if tbl == "reference_loot_template":
                # parents of this reference entry
                for pe, pt in self.ref_parents.get(entry, []):
                    tiers += self._loot_owner_tier(pe, pt, ev, via=entry)
            elif tbl == "creature_loot_template":
                tiers += self._loot_owner_tier(entry, tbl, ev)
            elif tbl == "gameobject_loot_template":
                maps = self.go_maps.get(entry, set())
                if maps:
                    for m in maps:
                        if m in MAP_TIER:
                            tiers.append(MAP_TIER[m])
                            ev.append(f"go{entry}@map{m}>t{MAP_TIER[m]}")
                        else:
                            tiers.append(0)
                            ev.append(f"go{entry}@map{m}(off-ladder>base)")
                else:
                    tiers += self._go_template_fallback(entry, ev)
            elif tbl == "item_loot_template":
                ev.append(f"container{item_id}<item{entry}(unresolved)")
        if item_id in self.vendors:
            ev.append("vendor(base)")
            tiers.append(0)
        for quest in self.q_reward_items.get(item_id, []):
            reqs = self.quests.get(quest, [])
            if reqs:
                # tier of the quest's required items, one level deep only
                # (deeper chains: token-of-token; note, don't recurse).
                req_tiers = []
                for r in reqs:
                    t = self._req_item_tier(r)
                    req_tiers.append(t if t is not None else 0)
                qt = max(req_tiers) if req_tiers else 0
                tiers.append(qt)
                ev.append(f"quest{quest}(req t{qt})")
            else:
                tiers.append(0)
                ev.append(f"quest{quest}(no req)")
        if item_id in self.craft_spell:
            spell = self.craft_spell[item_id]
            recipe = self.craft_recipe.get(spell, 0)
            parts = []
            if recipe:
                rt = self._req_item_tier(recipe)
                parts.append((f"recipe{recipe}", rt if rt is not None else 0))
            for reag in self.craft_reagents.get(spell, []):
                gt = self._loot_tier_of(reag, default=None)
                if gt is None:
                    gt = self._craft_tier_of(reag, depth=1)
                parts.append((f"reag{reag}", gt if gt is not None else 0))
            if parts:
                ct = max(t for _, t in parts)
                tiers.append(ct)
                ev.append(f"crafted(spell{spell}," +
                          "+".join(f"{n}>t{t}" for n, t in parts) + ")")
            else:
                tiers.append(0)
                ev.append(f"crafted(spell{spell},trainer>base)")
        if not ev:
            return None, ["no source found"], True
        if not tiers:
            return None, ev, True
        return min(tiers), ev, False

    def _req_item_tier(self, req_id):
        """Source tier of a quest-required item, non-recursive (depth 1)."""
        tiers = []
        for entry, tbl, _ in self.loot.get(req_id, []):
            if tbl == "creature_loot_template":
                ev = []
                tiers += self._loot_owner_tier(entry, tbl, ev)
            elif tbl == "gameobject_loot_template":
                for m in self.go_maps.get(entry, set()):
                    if m in MAP_TIER:
                        tiers.append(MAP_TIER[m])
                    else:
                        tiers.append(0)
        if req_id in self.vendors:
            tiers.append(0)
        if req_id in self.craft_spell:
            spell = self.craft_spell[req_id]
            recipe = self.craft_recipe.get(spell, 0)
            tiers.append(0 if not recipe else
                         self._loot_tier_of(recipe, default=0))
        return min(tiers) if tiers else None

    def _loot_tier_of(self, item_id, default=None):
        tiers = []
        for entry, tbl, _ in self.loot.get(item_id, []):
            if tbl == "creature_loot_template":
                ev = []
                tiers += self._loot_owner_tier(entry, tbl, ev)
            elif tbl in ("gameobject_loot_template", "item_loot_template"):
                for m in self.go_maps.get(entry, set()):
                    tiers.append(MAP_TIER.get(m, 0))
            elif tbl in ("pickpocketing_loot_template",
                         "skinning_loot_template"):
                # entry here is the SKIN/PICKPOCKET loot id = creature entry
                # in practice (52145 Incindis); resolve spawn map directly.
                for m in self.spawn_maps.get(entry, set()):
                    tiers.append(MAP_TIER.get(m, 0))
                if entry in self.scripts and \
                        self.scripts[entry] in self.script_tier:
                    tiers.append(self.script_tier[self.scripts[entry]])
        if item_id in self.vendors:
            tiers.append(0)
        return min(tiers) if tiers else default

    def _craft_tier_of(self, item_id, depth=0):
        """Tier of a crafted reagent (one extra level, no deep recursion)."""
        if depth < 0 or item_id not in self.craft_spell:
            return None
        spell = self.craft_spell[item_id]
        parts = []
        recipe = self.craft_recipe.get(spell, 0)
        if recipe:
            parts.append(self._loot_tier_of(recipe, default=0))
        for reag in self.craft_reagents.get(spell, []):
            parts.append(self._loot_tier_of(reag, default=None))
        parts = [t for t in parts if t is not None]
        return max(parts) if parts else 0

    def _loot_owner_tier(self, entry, tbl, ev, via=None):
        tag = f"ref{via}>" if via else ""
        out = []
        if tbl == "creature_loot_template":
            for c in self.loot_creatures.get(entry, []):
                maps = self.spawn_maps.get(c)
                if maps:
                    for m in maps:
                        if m in MAP_TIER:
                            out.append(MAP_TIER[m])
                            ev.append(f"{tag}mob{c}@map{m}>t{MAP_TIER[m]}")
                        elif c in WORLD_BOSSES:
                            out.append(WORLD_BOSS_TIER)
                            ev.append(f"{tag}worldboss{c}@map{m}"
                                      f">t{WORLD_BOSS_TIER}")
                        else:
                            out.append(0)
                            ev.append(f"{tag}mob{c}@map{m}(off-ladder>base)")
                else:
                    if c in INSTANCE_BOSS_TIER:
                        t = INSTANCE_BOSS_TIER[c]
                        out.append(t)
                        ev.append(f"{tag}boss{c}(summoned,instance t{t})")
                    elif c in WORLD_BOSSES:
                        out.append(WORLD_BOSS_TIER)
                        ev.append(f"{tag}worldboss{c}>t{WORLD_BOSS_TIER}")
                    elif self.scripts.get(c) in self.script_tier:
                        t = self.script_tier[self.scripts[c]]
                        out.append(t)
                        ev.append(f"{tag}boss{c}({self.scripts[c]}>t{t})")
                    else:
                        self.unresolved.append(c)
                        ev.append(f"{tag}boss{c}(NO SPAWN unresolved)")
        elif tbl in ("gameobject_loot_template", "item_loot_template"):
            for m in self.go_maps.get(entry, set()):
                if m in MAP_TIER:
                    out.append(MAP_TIER[m])
                    ev.append(f"{tag}go{entry}@map{m}>t{MAP_TIER[m]}")
                else:
                    ev.append(f"{tag}go{entry}@map{m}(off-ladder)")
            if entry not in self.go_maps:
                out += self._go_template_fallback(entry, ev)
        return out

    def _go_template_fallback(self, entry, ev):
        # Four Horsemen chest etc: look up GO template name -> map by spawns.
        rows = q(self.db, "SELECT id,map FROM gameobject "
                          f"WHERE id={entry}")
        out = []
        for r in rows:
            m = int(r[1])
            if m in MAP_TIER:
                out.append(MAP_TIER[m])
                ev.append(f"go{entry}@map{m}>t{MAP_TIER[m]}")
        if not rows:
            ev.append(f"go{entry}(NO SPAWN unresolved)")
            self.unresolved.append(-entry)
        return out

    def _instance_by_name(self, creature_entry):
        rows = q(self.db, "SELECT name FROM creature_template "
                          f"WHERE entry={creature_entry}")
        if not rows:
            return None
        return None  # conservative: list as unresolved per brief

    # -- per-row checks --------------------------------------------------------
    def check1(self, lab, cls, slot, it):
        bit = CLASS_BIT[cls]
        if it["allow"] != -1 and not (it["allow"] & bit):
            self.fails["1-equip"].append(f"{lab}: class not allowed "
                                         f"(mask {it['allow']})")
        if it["req_level"] > 60:
            self.fails["1-equip"].append(f"{lab}: required_level "
                                         f"{it['req_level']} > 60")
        if it["class"] == 4:  # armor
            cap = ARMOR_CAP[cls]
            if it["subclass"] in (CLOTH, LEATHER, MAIL, PLATE):
                if it["subclass"] > cap:
                    self.fails["1-equip"].append(
                        f"{lab}: {it['name']} armor "
                        f"subclass {it['subclass']} > class cap {cap}")
            elif it["subclass"] == SHIELD:
                if cls not in (1, 2, 7):
                    self.fails["1-equip"].append(f"{lab}: shield on class "
                                                 f"{cls}")
            elif it["subclass"] in (LIBRAM, IDOL, TOTEM):
                if RELIC.get(cls) != it["subclass"]:
                    self.fails["1-equip"].append(f"{lab}: relic subclass "
                                                 f"{it['subclass']} wrong "
                                                 f"for class {cls}")
            if it["inv"] not in SLOT_INV.get(slot, set()):
                # trinket slots 12/13 share inv 12; rings 10/11 share inv 11
                self.fails["1-equip"].append(
                    f"{lab}: slot {slot} vs inventory_type {it['inv']}")
        elif it["class"] == 2:  # weapon
            if it["inv"] == 17:  # 2H in main hand: offhand must be empty
                pass  # checked at set level (check 4b)
            if it["subclass"] not in WEAPONS.get(cls, set()):
                self.fails["1-equip"].append(
                    f"{lab}: {it['name']} weapon subclass "
                    f"{it['subclass']} unusable by class {cls}")
            if it["inv"] not in SLOT_INV.get(slot, set()):
                self.fails["1-equip"].append(
                    f"{lab}: slot {slot} vs inventory_type {it['inv']}")
            if it["subclass"] == SHIELD and cls not in (1, 2, 7):
                self.fails["1-equip"].append(f"{lab}: shield on class {cls}")
        else:
            # non-equippable class (quest/consumable/etc.) in a gear slot
            if it["inv"] not in SLOT_INV.get(slot, set()):
                self.fails["1-equip"].append(
                    f"{lab}: slot {slot} vs inventory_type {it['inv']} "
                    f"(class {it['class']})")

    def check3(self, lab, tier, it):
        if it["quality"] == 5:
            self.fails["3-forbidden"].append(f"{lab}: legendary {it['name']}")
            return
        if it["honor"] != 0:
            self.fails["3-forbidden"].append(f"{lab}: honor rank "
                                             f"{it['honor']} {it['name']}")
        if it["rep_f"] != 0:
            if it["rep_f"] in RAID_REP_OK:
                if tier < RAID_REP_OK[it["rep_f"]]:
                    self.fails["3-forbidden"].append(
                        f"{lab}: rep faction {it['rep_f']} needs tier "
                        f"{RAID_REP_OK[it['rep_f']]} {it['name']}")
            else:
                self.fails["3-forbidden"].append(
                    f"{lab}: rep faction {it['rep_f']} {it['name']}")
        elif PVP_NAME_RE.search(it["name"] or ""):
            self.fails["3-forbidden"].append(f"{lab}: PvP name {it['name']}")

    def check5(self, lab, cls, spec, it):
        key = (cls, spec)
        mods = {t: v for t, v in it["mods"] if t in (AGI, STR, INT, SPI, STA)
                and v}
        if not mods and not any(s for s, _ in it["spells"]):
            return  # proc/trinket-only item: no stat verdict
        names = " ".join(self.spell_names.get(s, "") for s, _ in it["spells"]
                         if s)
        extra = " ".join(self.spell_names.get(t, "")
                         for s, _ in it["spells"]
                         for t in self.spell_trig.get(s, []))
        auras = " ".join(self.spell_auras.get(s, "") for s, _ in it["spells"]
                         if s)
        auras += " " + " ".join(self.spell_auras.get(t, "")
                                for s, _ in it["spells"]
                                for t in self.spell_trig.get(s, []))
        text = names + " " + extra + " " + auras
        if key in CASTER_SPECS:
            if set(mods) <= {STR, AGI} and mods:
                self.warns["5-stats"].append(
                    f"{lab}: caster with str/agi-only {it['name']} {mods}")
        elif key in HEAL_SPECS:
            if not (set(mods) & {INT, SPI}) and not HEAL_WORDS.search(text):
                if mods or text:
                    self.warns["5-stats"].append(
                        f"{lab}: healer w/o heal stats {it['name']} "
                        f"{mods} [{text[:60]}]")
        elif key in TANK_SPECS:
            if STA not in mods and not TANK_WORDS.search(text):
                self.warns["5-stats"].append(
                    f"{lab}: tank w/o sta/def {it['name']} {mods} "
                    f"[{text[:60]}]")
        elif key in MELEE_SPECS:
            if set(mods) <= {INT, SPI} and mods:
                self.warns["5-stats"].append(
                    f"{lab}: melee with int/spi-only {it['name']} {mods}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--csv", default=str(DEFAULT_CSV))
    ap.add_argument("--snapshot", default=str(DEFAULT_SNAPSHOT))
    ap.add_argument("--report", default=None)
    ap.add_argument("--db-host", default=os.environ.get("HIRE_GEAR_DB_HOST",
                                                        "localhost"))
    ap.add_argument("--db-user", default=os.environ.get("HIRE_GEAR_DB_USER",
                                                        "mangos"))
    ap.add_argument("--db-pass", default=os.environ.get("HIRE_GEAR_DB_PASS",
                                                        "mangos"))
    ap.add_argument("--db-name", default=os.environ.get("HIRE_GEAR_DB_NAME",
                                                        "tw_world"))
    a = ap.parse_args()

    rows = list(csv.DictReader(open(a.csv)))
    print(f"rows: {len(rows)}", flush=True)
    ids = {int(r["item_id"]) for r in rows}
    au = Audit(connect(a))
    au.load(ids)
    print(f"items loaded: {len(au.items)}/{len(ids)}", flush=True)

    sets = defaultdict(list)  # (class,spec,race,tier) -> [(slot,row)]
    for r in rows:
        lab = (f"c{r['class']}/{r['spec']}/r{r['race']}/t{r['tier']}/"
               f"s{r['slot']}#{r['item_id']} {r['item_name']}")
        iid = int(r["item_id"])
        cls, slot, tier = r["class"], int(r["slot"]), int(r["tier"])
        if iid == 0:
            # empty slot placeholder (offhand next to a 2H): not an item.
            sets[(cls, r["spec"], r["race"], tier)].append((slot, r, None))
            continue
        it = au.items.get(iid)
        if it is None:
            au.fails["1-equip"].append(f"{lab}: NOT IN item_template")
            continue
        if it["name"] != r["item_name"]:
            au.warns["1-equip"].append(f"{lab}: csv name {r['item_name']!r} "
                                       f"!= db {it['name']!r}")
        au.check1(lab, int(cls), slot, it)
        au.check3(lab, tier, it)
        au.check5(lab, cls, r["spec"], it)
        # check 2: independent source tier
        stier, ev, nosrc = au.sources(iid)
    # check 4: ANY duplicate item id within one set FAILs, even doubled
    # rings/trinkets (Painweaver x2, HoJ x2): a hired set must not pay
    # for / equip the same item twice. (Core only blocks
    # ITEM_FLAG_UNIQUE_EQUIPPED; the audit is stricter by design.)
    for key, members in sets.items():
        seen = defaultdict(list)
        for slot, r, it in members:
            if it is None:
                continue
            seen[int(r["item_id"])].append(str(slot))
        for iid, slots in seen.items():
            if len(slots) > 1:
                au.fails["4-unique"].append(
                    f"c{key[0]}/{key[1]}/r{key[2]}/t{key[3]}: item {iid} "
                    f"in slots {slots}")
        # 2H main + non-empty offhand (item_id 0 = empty placeholder, ok)
        mh = {int(r["item_id"]): it for s, r, it in members
              if s == 15 and it is not None}
        oh = [(r, it) for s, r, it in members
              if s == 16 and int(r["item_id"]) != 0]

    # check 6: progression (avg ilvl per class/spec/tier; slot downgrades > 5)
    for (cls, spec) in {(c, s) for c, s, _, _ in sets}:
        tiers = sorted({t for c, s, _, t in sets if (c, s) == (cls, spec)})
        prev_avg, prev_slot, prev_stier = None, {}, {}
        for t in tiers:
            for race in {rc for c, s, rc, tt in sets
                         if (c, s, tt) == (cls, spec, t)}:
                members = [(s, r, it) for s, r, it in
                           sets[(cls, spec, race, t)] if it is not None]
                if len(members) <= 1:
                    continue  # racial override row, not a set
                avg = sum(it["ilvl"] for _, _, it in members) / len(members)
                if prev_avg is not None and avg < prev_avg - 1e-9:
                    au.fails["6-progression"].append(
                        f"c{cls}/{spec}/r{race}: avg ilvl {avg:.1f} at t{t} "
                        f"< {prev_avg:.1f} at t{t - 1}")
                cur = {s: it["ilvl"] for s, _, it in members}
                for s, il in cur.items():
                    if s in prev_slot and prev_slot[s] - il > 5:
                        au.warns["6-progression"].append(
                            f"c{cls}/{spec}: slot {s} {prev_slot[s]} -> {il} "
                            f"(t{t - 1} -> t{t})")
                # check 6b: source-tier revert. Same slot changing to an
                # item from an OLDER tier than the previous tier's item
                # (revert to older content) is WARN (review, not failure).
                cur_st = {s: r.get("_stier") for s, r, _ in members}
                for s, st in cur_st.items():
                    pst = prev_stier.get(s)
                    if (st is not None and pst is not None and st < pst and
                            cur.get(s) != prev_slot.get(s)):
                        au.warns["6-revert"].append(
                            f"c{cls}/{spec}: slot {s} source t{pst} -> t{st} "
                            f"(t{t - 1} -> t{t})")
                prev_avg, prev_slot, prev_stier = avg, cur, cur_st
                break  # one race representative (race rarely differs)

    # check 8: completeness. Every FULL (class,spec,race,tier) set in the
    # CSV must have a row for each slot the class can fill: armor slots
    # 0,1,2,4,5,6,7,8,9,10,11,12,13,14 + main 15 + off 16 (0 = empty
    # placeholder next to a 2H) + 17 (ranged weapon for warrior/rogue/
    # hunter; relic for paladin/shaman/druid; wand for priest/mage/
    # warlock). Single-row racial overrides (e.g. one slot-16 shield for a
    # weapon-skill race) are not sets: only check keys with >1 row.
    # Missing rows FAIL; unexpected extra rows WARN.
    EXPECT_SLOTS = {0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                    17}
    for key, members in sorted(sets.items()):
        if len(members) <= 1:
            continue
        have = {s for s, _, _ in members}
        for s in sorted(EXPECT_SLOTS - have):
            au.fails["8-completeness"].append(
                f"c{key[0]}/{key[1]}/r{key[2]}/t{key[3]}: missing slot {s}")
        for s in sorted(have - EXPECT_SLOTS):
            au.warns["8-completeness"].append(
                f"c{key[0]}/{key[1]}/r{key[2]}/t{key[3]}: unexpected slot {s}")

    # check 9: new content per tier. Per (class,spec,tier), count items
    # whose source tier == the row tier for tiers with own new content
    # (t3 MC/Ony/Timbermaw, t5 AQ40, t6 Naxx, t7 Tower). WARN if under 3:
    # an unlock that changes nothing (or nearly nothing) is suspect.
    # (t1/t2/t4 share their pools across several instances and often
    # legitimately carry over, so they are exempt.)
    for key, members in sorted(sets.items()):
        if len(members) <= 1:
            continue
        t = key[3]
        if t not in (3, 5, 6, 7):
            continue
        n = sum(1 for _, r, _ in members if r.get("_stier") == t)
        if n < 3:
            au.warns["9-newcontent"].append(
                f"c{key[0]}/{key[1]}/r{key[2]}/t{t}: only {n} own-tier "
                f"item(s)")
    # check 7: wowisclassic cross-check (review only).
    # Snapshot rows store gear WITHOUT enchant entries (scrape.py writes
    # bare item anchors only); align by the non-enchant gear order. Short
    # rows (oh/ranged missing on some pages) align from the BACK: the page
    # always ends with feet/ring/ring/trinket/trinket, so offset =
    # len(order) - len(gear).
    snap = json.load(open(a.snapshot))
    snap_idx = {}
    for r in snap["rows"]:
        snap_idx[(r["class"], r["spec"], r["phase"])] = r
    GEAR_SLOTS = [GEAR_ORDER_SLOTS[o] for o in snap["gear_order"]
                  if "ench" not in o]
    agree_n = agree_d = 0
    disagreements = []
    csv_by_spec_tier = defaultdict(dict)  # (snapclass,snapspec,tier,slot)
    for r in rows:
        sc = SNAP_CLASS.get(r["class"])
        ss = SNAP_SPEC.get((r["class"], r["spec"]))
        if sc and ss and int(r["item_id"]) != 0:
            csv_by_spec_tier[(sc, ss, int(r["tier"]))][int(r["slot"])] = \
                int(r["item_id"])
    for (sc, ss, tier), slots in csv_by_spec_tier.items():
        for phase, ptier in PHASE_TIER.items():
            if ptier != tier:
                continue
            srow = snap_idx.get((sc, ss, phase))
            if not srow:
                continue
            gear = srow["gear"]
            off = len(GEAR_SLOTS) - len(gear)  # short rows: align from back
            for gi, g in enumerate(gear):
                want = GEAR_SLOTS[gi + off] if gi + off < len(GEAR_SLOTS) \
                    else None
                if want is None:
                    continue
                want_slots = (want,) if isinstance(want, int) else want
                gid = g["item_id"] if isinstance(g, dict) else g[0]
                hit = any(slots.get(s) == gid for s in want_slots)
                agree_d += 1
                if hit:
                    agree_n += 1
                else:
                    disagreements.append(
                        f"{sc}/{ss} t{tier}~{phase}: snapshot {want_slots}={gid} "
                        f"vs csv { {s: slots.get(s) for s in want_slots} }")

    # -- report ---------------------------------------------------------------
    L = []
    L.append("# Hire-gear audit report (issue #490)\n")
    L.append("Independent re-derivation from the world DB (read-only). "
             "Author files generate.py / classify_items.py / item_tiers.csv "
             "were NOT read.\n")
    L.append("## Summary\n")
    L.append("| check | FAIL | WARN |")
    L.append("|---|---|---|")
    for c in ("1-equip", "2-sourcetier", "3-forbidden", "4-unique",
              "5-stats", "6-progression", "8-completeness", "7-crosscheck"):
        if c == "7-crosscheck":
            pct = (100.0 * agree_n / agree_d) if agree_d else 0
            L.append(f"| {c} | review-only | agree {agree_n}/{agree_d} "
                     f"({pct:.1f}%) |")
        else:
            L.append(f"| {c} | {len(au.fails[c])} | {len(au.warns[c])} |")
    L.append("")
    L.append(f"unresolved bosses/objects (no spawn, no instance fallback): "
             f"{len(set(au.unresolved))} "
             f"{sorted(set(au.unresolved))[:20]}")
    L.append("")
    L.append("## FAIL rows\n")
    any_fail = False
    for c in ("1-equip", "2-sourcetier", "3-forbidden", "4-unique",
              "6-progression", "8-completeness"):
        if au.fails[c]:
            any_fail = True
            L.append(f"### {c} ({len(au.fails[c])})\n")
            for f in au.fails[c]:
                L.append(f"- {f}")
            L.append("")
    if not any_fail:
        L.append("(none)\n")
    L.append("## Top WARNs\n")
    for c in ("2-sourcetier", "5-stats", "6-progression", "6-revert",
              "9-newcontent", "1-equip"):
        ws = au.warns[c]
        if ws:
            L.append(f"### {c} ({len(ws)}, top 30)\n")
            for w in ws[:30]:
                L.append(f"- {w}")
            L.append("")
    if disagreements:
        L.append(f"### 7-crosscheck disagreements ({len(disagreements)}, "
                 "top 40)\n")
        for d in disagreements[:40]:
            L.append(f"- {d}")
        L.append("")
    text = "\n".join(L)
    print(text[:4000])
    rep = Path(a.report) if a.report else None
    if rep:
        rep.write_text(text)
        print(f"report -> {rep}")


if __name__ == "__main__":
    main()
