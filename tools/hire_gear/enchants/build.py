#!/usr/bin/env python3
"""Per-tier BIS enchant table for issue #490 (offline data, no server code).

Inputs: ../data/wowisclassic.json (phase enchant per slot per class/spec),
        ../data/turtle_sheets.json (Naxx/K40 per-tab enchant names).
Output: ../data/enchants_by_tier.json:
  {spec_key: {tier 1..7: {slot: {spell, name, source}}}}.
Spell ids are SERVER ENCHANT SPELL ids (ai_playerbot_enchant_candidates.spellid /
PlayerbotAI::EnchantItemT(spellid) space), NOT SpellItemEnchantment ids.

Tier ladder (Appendix C, cumulative): 1 pre-raid, 2 ZG+AQ20+LowerKara,
3 MC+Ony+TimbermawHold, 4 BWL+ES, 5 AQ40, 6 Naxx, 7 Tower of Karazhan.

Rules:
- An enchant is available from the tier of the content that provides it
  (source tier); the pick at each tier is the newest source listing it
  (wowisclassic phase choice when its content is unlocked, Turtle Naxx/K40
  tab choice at 6/7), else best by role.
- Rep-gated enchants ALLOWED (AD/Cenarion/Timbermaw/ZG rows all eligible).
- Turtle neck/ring gemstones + blacksmith belt buckles from tier 1, BIS per
  role (turtle tab pick at 6/7, role default below).
- 2H-only rows (mask 1378) never on offhand; 2H-primary specs omit slot 16;
  shield specs take 20017; other OH mirrors MH; hunters take scope on 17.
"""
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
DATA = HERE.parent / "data"


def load_snapshot(name, fetcher):
    """Site snapshots are not vendored: fail clearly before writing anything."""
    path = DATA / name
    if not path.exists():
        sys.exit(f"{path} not found: run {fetcher} first "
                 "(site snapshots are generated locally, not vendored)")
    return json.loads(path.read_text())


WOW = load_snapshot("wowisclassic.json", "tools/hire_gear/sources/wowisclassic/scrape.py")
TS = load_snapshot("turtle_sheets.json", "tools/hire_gear/sources/turtle_sheets/fetch.py")

# 29 spec keys "<class>_<spec>"; pathName per runtime/HireSpecPolicy.h
# (druid bear+cat share the feral path; keys use gossip order).
SPECS = [
    ("warrior", "protection", "tank"), ("warrior", "arms", "dps"),
    ("warrior", "fury", "dps"),
    ("paladin", "protection", "tank"), ("paladin", "holy", "healer"),
    ("paladin", "retribution", "dps"),
    ("hunter", "beastmastery", "dps"), ("hunter", "marksmanship", "dps"),
    ("hunter", "survival", "dps"),
    ("rogue", "assassination", "dps"), ("rogue", "combat", "dps"),
    ("rogue", "subtlety", "dps"),
    ("priest", "holy", "healer"), ("priest", "discipline", "healer"),
    ("priest", "shadow", "dps"),
    ("shaman", "restoration", "healer"), ("shaman", "elemental", "dps"),
    ("shaman", "enhancement", "dps"), ("shaman", "tank", "tank"),
    ("mage", "frost", "dps"), ("mage", "fire", "dps"),
    ("mage", "arcane", "dps"),
    ("warlock", "affliction", "dps"), ("warlock", "demonology", "dps"),
    ("warlock", "destruction", "dps"),
    ("druid", "bear", "tank"), ("druid", "restoration", "healer"),
    ("druid", "balance", "dps"), ("druid", "cat", "dps"),
]

WOWMAP = {
    ("warrior", "protection"): ("warrior", "prot"),
    ("warrior", "arms"): ("warrior", "fury"),
    ("warrior", "fury"): ("warrior", "fury"),
    ("paladin", "protection"): ("paladin", "protection"),
    ("paladin", "holy"): ("paladin", "heal"),
    ("paladin", "retribution"): ("paladin", "ret"),
    ("hunter", "beastmastery"): ("hunter", "hunter"),
    ("hunter", "marksmanship"): ("hunter", "hunter"),
    ("hunter", "survival"): ("hunter", "hunter"),
    ("rogue", "assassination"): ("rogue", "dagger"),
    ("rogue", "combat"): ("rogue", "sword"),
    ("rogue", "subtlety"): ("rogue", "dagger"),
    ("priest", "holy"): ("priest", "holy"),
    ("priest", "discipline"): ("priest", "holy"),
    ("priest", "shadow"): ("priest", "shadow"),
    ("shaman", "restoration"): ("shaman", "resto"),
    ("shaman", "elemental"): ("shaman", "elem"),
    ("shaman", "enhancement"): ("shaman", "enhan"),
    ("shaman", "tank"): ("shaman", "enhan"),
    ("mage", "frost"): ("mage", "mage"),
    ("mage", "fire"): ("mage", "mage"),
    ("mage", "arcane"): ("mage", "mage"),
    ("warlock", "affliction"): ("warlock", "warlock"),
    ("warlock", "demonology"): ("warlock", "warlock"),
    ("warlock", "destruction"): ("warlock", "warlock"),
    ("druid", "bear"): ("druid", "tank"),
    ("druid", "restoration"): ("druid", "resto"),
    ("druid", "balance"): ("druid", "balance"),
    ("druid", "cat"): ("druid", "cat"),
}

TSMAP = {
    ("warrior", "protection"): ("warrior", ["FProt Mit", "DTact Mit"]),
    ("warrior", "arms"): ("warrior", ["Arms BIS"]),
    ("warrior", "fury"): ("warrior", ["Fury Sword BIS", "Fury BIS"]),
    ("paladin", "protection"): ("paladin", ["Prot Mit BIS"]),
    ("paladin", "holy"): ("paladin", ["Holy BIS"]),
    ("paladin", "retribution"): ("paladin", ["AP Ret BIS"]),
    ("hunter", "beastmastery"): ("hunter", ["BM BIS"]),
    ("hunter", "marksmanship"): ("hunter", ["Marks BIS"]),
    ("hunter", "survival"): ("hunter", ["Surv BIS", "Surv Axe BIS"]),
    ("rogue", "assassination"): ("rogue", ["Dagger BIS"]),
    ("rogue", "combat"): ("rogue", ["Sword BIS", "Combat BIS"]),
    ("rogue", "subtlety"): ("rogue", ["Sub BIS"]),
    ("priest", "holy"): ("priest", ["Holy Throughput BIS", "Holy BIS"]),
    ("priest", "discipline"): ("priest", ["Holy Sustain BIS", "Holy BIS"]),
    ("priest", "shadow"): ("priest", ["Shadow BIS"]),
    ("shaman", "restoration"): ("shaman", ["Resto BIS"]),
    ("shaman", "elemental"): ("shaman", ["Elemental BIS",
                                         "Elemental Lightning BIS"]),
    ("shaman", "enhancement"): ("shaman", ["Enhance BIS"]),
    ("shaman", "tank"): ("shaman", ["Enhance BIS"]),
    ("mage", "frost"): ("mage", ["Frost BIS"]),
    ("mage", "fire"): ("mage", ["Fire BIS"]),
    ("mage", "arcane"): ("mage", ["Arcane BIS"]),
    ("warlock", "affliction"): ("warlock", ["Affliction BIS"]),
    ("warlock", "demonology"): ("warlock", ["Demo BIS"]),
    ("warlock", "destruction"): ("warlock", ["SM Ruin BIS"]),
    ("druid", "bear"): ("druid", ["Bear Mit BIS"]),
    ("druid", "restoration"): ("druid", ["Resto Rejuv Tree BIS"]),
    ("druid", "balance"): ("druid", ["Boomkin BIS"]),
    ("druid", "cat"): ("druid", ["Cat BIS"]),
}

SLOTS = [0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17]

# spell -> (name, source, unlock tier). Tiers verified vs live tw_world;
# see report for per-row evidence. 1 pre-raid, 2 ZG+AQ20+LowerKara,
# 3 MC+Ony+TimbermawHold(819), 4 BWL+ES, 5 AQ40, 6 Naxx, 7 Turtle/Kara40.
CAT = {
    15340: ("Lesser Arcanum of Rumination (+150 mana)", "DM Lorekeeper Lydros q4463", 1),
    15389: ("Lesser Arcanum of Constitution (+100 HP)", "DM Lorekeeper Lydros q4481", 1),
    15391: ("Lesser Arcanum of Tenacity (+125 armor)", "DM Lorekeeper Lydros q4482", 1),
    15394: ("Lesser Arcanum of Resilience (+20 FR)", "DM Lorekeeper Lydros q4483", 1),
    15397: ("Lesser Arcanum of Voracity, str (+8 str)", "DM Lorekeeper Lydros q4484", 1),
    15400: ("Lesser Arcanum of Voracity, stam (+8 stam)", "DM Lorekeeper Lydros q4484-family", 1),
    15402: ("Lesser Arcanum of Voracity, agi (+8 agi)", "DM Lorekeeper Lydros q4484-family", 1),
    15404: ("Lesser Arcanum of Voracity, int (+8 int)", "DM Lorekeeper Lydros q4484-family", 1),
    15406: ("Lesser Arcanum of Voracity, spi (+8 spi)", "DM Lorekeeper Lydros q4484-family", 1),
    15427: ("Greater Arcanum, mana (+85 mana/+14 FR, legs)", "DM Athenaeum chain", 1),
    15429: ("Greater Arcanum, HP (+75 HP/+14 FR, legs)", "DM Athenaeum chain", 1),
    15439: ("Greater Arcanum, armor (+110 armor/+14 FR, legs)", "DM Athenaeum chain", 1),
    15441: ("Greater Arcanum, str (+10 str/+14 FR, legs)", "DM Athenaeum chain", 1),
    15444: ("Greater Arcanum, stam (+10 stam/+14 FR, legs)", "DM Athenaeum chain", 1),
    15446: ("Greater Arcanum, agi (+10 agi/+14 FR, legs)", "DM Athenaeum chain", 1),
    15449: ("Greater Arcanum, int (+10 int/+14 FR, legs)", "DM Athenaeum chain", 1),
    15458: ("Greater Arcanum, spi (+10 spi/+14 FR, legs)", "DM Athenaeum chain", 1),
    15463: ("Legendary Arcanum, melee (+10 str/sta/+110 ar, legs)", "DM chain extension", 1),
    15490: ("Legendary Arcanum, caster (+10 int/spi/+100 mana, legs)", "DM chain extension", 1),
    22840: ("Arcanum of Rapidity (+1% haste)", "DM quest 7478 Lydros", 1),
    22844: ("Arcanum of Focus (+12 heal/spell dmg)", "DM quest 7483/7484 Lydros", 1),
    22846: ("Arcanum of Protection (+1% dodge)", "DM quest 7485 Lydros", 1),
    28163: ("Ice Guard (+10 FrR)", "EPL quest 9211 Mataus", 1),
    28165: ("Shadow Guard (+10 ShR)", "EPL quest 9213 Mataus", 1),
    45739: ("Wolfsheart (Gift of Ferocity)", "quest 41030/41031 (zone 493 TBD)", 1),
    56536: ("Invocation of Greater Voracity (+10 str)", "quest 41743 Gamemaster's Boons", 2),
    56537: ("Invocation of Greater Constitution (+12 stam)", "quest 41743 Gamemaster's Boons", 2),
    56538: ("Invocation of Shadow Resistance (+20 ShR)", "quest 41743 Gamemaster's Boons", 2),
    24149: ("Presence of Might (+7 def/+10 stam/+15 block)", "ZG quest 8184 Zanza (Friendly)", 2),
    24160: ("Syncretist's Sigil (+7 def/+10 stam/+24 heal)", "ZG quest 8185 Zanza (Friendly)", 2),
    24161: ("Death's Embrace (+28 AP/+1% dodge)", "ZG quest 8186 Zanza (Friendly)", 2),
    24162: ("Falcon's Call (+24 RAP/+10 stam/+1% hit)", "ZG quest 8187 Zanza (Friendly)", 2),
    24163: ("Vodouisant's Vigilant Embrace (+13 heal/dmg/+15 int)", "ZG quest 8188 Zanza (Friendly)", 2),
    24164: ("Presence of Sight (+18 heal/dmg/+1% spell hit)", "ZG quest 8189 Zanza (Friendly)", 2),
    24165: ("Hoodoo Hex (+18 heal/dmg/+10 stam)", "ZG quest 8190 Zanza (Friendly)", 2),
    24167: ("Prophetic Aura (+4 mp5/+10 stam/+24 heal)", "ZG quest 8191 Zanza (Friendly)", 2),
    24168: ("Animist's Caress (+10 int/+10 stam/+24 heal)", "ZG quest 8192 Zanza (Friendly)", 2),
    28161: ("Savage Guard (+10 NR)", "ZG quest 9208-9210 Zanza (Honored)", 2),
    24420: ("Zandalar Signet of Serenity (+33 healing)", "quest 8246 (Exalted)", 2),
    24421: ("Zandalar Signet of Mojo (+18 spell dmg/heal)", "quest 8246 (Exalted)", 2),
    24422: ("Zandalar Signet of Might (+30 AP)", "quest 8246 (Exalted)", 2),
    48103: ("Zandalar Signet of Tenacity (+14 stam)", "8246-family presumed", 2),
    57151: ("Invocation of Shattering (+40 ArP)", "quest 41079 Hierophant Nerseus", 7),
    57152: ("Invocation of Greater Protection (+2% dodge)", "quest 41080 Hierophant Nerseus", 7),
    57153: ("Invocation of Expansive Mind (+25 spell pen)", "quest 41081 Hierophant Nerseus", 7),
    57154: ("Invocation of Greater Arcane Fortitude (+25 ArcR)", "quest 41082 Hierophant Nerseus", 7),
    22593: ("Flame Mantle of the Dawn (+5 FR)", "AD vendor (Revered)", 1),
    22594: ("Frost Mantle of the Dawn (+5 FrR)", "AD vendor (Revered)", 1),
    22596: ("Shadow Mantle of the Dawn (+5 ShR)", "AD vendor (Revered)", 1),
    22597: ("Nature Mantle of the Dawn (+5 NR)", "AD vendor (Revered)", 1),
    22598: ("Arcane Mantle of the Dawn (+5 ArcR)", "AD vendor (Revered)", 1),
    22599: ("Chromatic Mantle of the Dawn (+5 all res)", "AD vendor (Exalted, 25 Valor)", 1),
    29467: ("Power of the Scourge (+15 dmg/+1% spell crit)", "Sapphiron drop, Naxx", 6),
    29475: ("Resilience of the Scourge (+31 heal/+5 mp5)", "Sapphiron drop, Naxx", 6),
    29480: ("Fortitude of the Scourge (+16 stam/+100 armor)", "Sapphiron drop, Naxx", 6),
    29483: ("Might of the Scourge (+26 AP/+1% crit)", "Sapphiron drop, Naxx", 6),
    57155: ("Sigil of Leeching (+2% lifesteal)", "quest 40911 Vaenar Hollowstar", 7),
    57156: ("Sigil of Quickness (+2% haste)", "quest 40910 Vaenar Hollowstar", 7),
    57157: ("Sigil of Penetration (+40 ArP, shoulder)", "quest 40912 Vaenar Hollowstar", 7),
    57185: ("Sigil of Resilience (1% resilience)", "Hyjal family presumed", 7),
    20025: ("Enchant Chest: Greater Stats (+4 all)", "world drop formula", 1),
    20026: ("Enchant Chest: Major Health (+100 HP)", "vendor Qia", 1),
    20028: ("Enchant Chest: Major Mana (+100 mana)", "world drop formula", 1),
    22725: ("Enchant Chest: Defense +5 (Core Armor Kit)", "item 18251 Core Armor Kit", 1),
    57135: ("Enchanted Armor Kit (+9 heal/spell dmg)", "Turtle (used by Naxx BIS lists)", 6),
    57119: ("Enchant Chest: Mighty Mana (+150 mana)", "Solnius, Emerald Sanctum", 4),
    57164: ("Copper Belt Buckle (+1 stam)", "blacksmithing crafted", 1),
    57167: ("Bronze Belt Buckle (+2 stam)", "blacksmithing crafted", 1),
    57170: ("Iron Belt Buckle (+25 ArP)", "blacksmithing crafted", 1),
    57173: ("Mithril Belt Buckle (+6 agi)", "blacksmithing crafted", 1),
    57176: ("Thorium Belt Buckle (+4 all res)", "blacksmithing crafted", 1),
    57179: ("Arcanite Belt Buckle (+5 def)", "blacksmithing crafted", 1),
    57182: ("Dreamsteel Belt Buckle (+10 int)", "blacksmithing crafted 300", 1),
    57186: ("Bloody Belt Buckle (1% resilience)", "blacksmithing crafted 300", 1),
    40: ("Gold Belt Buckle (+14 RAP)", "blacksmithing crafted", 1),
    41: ("Dragonscale Belt Buckle (+9 fire dmg)", "blacksmithing crafted", 1),
    52: ("Dark Iron Belt Buckle (+10 FR)", "blacksmithing crafted", 1),
    54: ("Enchanted Thorium Belt Buckle (+10 ArcR)", "blacksmithing crafted", 1),
    73: ("Obsidian Belt Buckle (+10 str)", "blacksmithing crafted", 1),
    121: ("Steel Belt Buckle (+15 block)", "blacksmithing crafted", 1),
    41651: ("Truesilver Belt Buckle (+8 healing)", "blacksmithing crafted", 1),
    13890: ("Enchant Boots: Minor Speed (run speed)", "world formula", 1),
    20020: ("Enchant Boots: Greater Stamina (+7 stam)", "world drop formula", 1),
    20023: ("Enchant Boots: Greater Agility (+7 agi)", "world drop formula", 1),
    20024: ("Enchant Boots: Spirit (+5 spi)", "world drop formula", 1),
    56543: ("Enchant Boots: Major Intellect (+9 int)", "Shen'dralar vendor DM (Honored+)", 1),
    57142: ("Enchant Boots: Greater Spirit (+12 spi)", "quest 40883 Evandil Nightwind", 7),
    57127: ("Enchant Boots: Superior Stamina (+15 stam)", "Lower Kara trio drop", 2),
    57148: ("Enchant Boots: Vampirism (+1% lifesteal)", "Lower Kara trio drop", 2),
    13931: ("Enchant Bracer: Deflection (+3 def)", "world formula", 1),
    13948: ("Enchant Gloves: Minor Haste", "pool row (Turtle Naxx lists use it)", 1),
    20008: ("Enchant Bracer: Greater Intellect (+7 int)", "world drop formula", 1),
    20009: ("Enchant Bracer: Superior Spirit (+9 spi)", "world drop formula", 1),
    20010: ("Enchant Bracer: Superior Strength (+9 str)", "world drop formula", 1),
    20011: ("Enchant Bracer: Superior Stamina (+9 stam)", "world drop formula", 1),
    23801: ("Enchant Bracer: Mana Regeneration (+4 mp5)", "AD vendor (Honored)", 1),
    23802: ("Enchant Bracer: Healing Power (+24 healing)", "AD vendor (Revered)", 1),
    57028: ("Enchant Bracer: Spell Power (+12 dmg)", "Gelkis vendor (Exalted)", 1),
    57030: ("Enchant Bracer: Greater Agility (+7 agi)", "Magram vendor (Exalted)", 1),
    57144: ("Enchant Bracer: Greater Deflection (+5 def)", "quest 40884 Evandil Nightwind", 7),
    20012: ("Enchant Gloves: Greater Agility (+7 agi)", "Legashi Rogue drop", 1),
    20013: ("Enchant Gloves: Greater Strength (+7 str)", "BRS Spirestone Mystic drop", 1),
    46086: ("Enchant Gloves: Major Strength (+9 str)", "quest 80739 Bradley Steel", 1),
    25073: ("Enchant Gloves: Shadow Power (+20 shadow)", "AQ20/40-shared boss drop", 2),
    25074: ("Enchant Gloves: Frost Power (+20 frost)", "AQ20/40-shared boss drop", 2),
    25078: ("Enchant Gloves: Fire Power (+20 fire)", "AQ20/40-shared boss drop", 2),
    25079: ("Enchant Gloves: Healing Power (+30 healing)", "AQ20/40-shared boss drop", 2),
    25080: ("Enchant Gloves: Superior Agility (+15 agi)", "AQ family incl. Kurinnaxx", 2),
    25072: ("Enchant Gloves: Threat (+2% threat)", "Emperor Vek'nilash AQ40", 5),
    46601: ("Enchant Gloves: Arcane Power (+20 arcane)", "Lower Kara mobs", 2),
    46602: ("Enchant Gloves: Nature Power (+20 nature)", "Solnius, Emerald Sanctum", 4),
    46603: ("Enchant Gloves: Holy Power (+20 holy)", "source unknown (Turtle family)", 7),
    13882: ("Enchant Cloak: Lesser Agility (+3 agi)", "world formula", 1),
    20014: ("Enchant Cloak: Greater Resistance (+5 all res)", "world drop formula", 1),
    20015: ("Enchant Cloak: Superior Defense (+70 armor)", "vendors Lorelae/Argnuhl", 1),
    25081: ("Enchant Cloak: Greater Fire Resistance (+15 FR)", "Cenarion vendor Kania (Friendly)", 1),
    25082: ("Enchant Cloak: Greater Nature Resistance (+15 NR)", "Cenarion vendor Kania (Honored)", 2),
    25083: ("Enchant Cloak: Stealth (stealth+)", "AQ family drop", 2),
    25086: ("Enchant Cloak: Dodge (+1% dodge)", "AQ family drop", 2),
    25084: ("Enchant Cloak: Subtlety (threat-)", "Emperor Vek'lor AQ40", 5),
    49016: ("Enchant Cloak: Agility (+7 agi)", "Archdruid Kronn, Timbermaw Hold (819)", 3),
    49018: ("Enchant Cloak: Greater Shadow Resistance (+15 ShR)", "Selenaxx Foulheart, Timbermaw Hold (819)", 3),
    57117: ("Enchant Cloak: Greater Arcane Resistance (+15 ArcR)", "Lower Kara trash", 2),
    13898: ("Fiery Weapon (fiery proc)", "Pyroguard Emberseer UBRS", 1),
    20029: ("Icy Chill (frost proc)", "world drop", 1),
    20030: ("Enchant 2H Weapon: Superior Impact (+9 dmg, 2H)", "Blackhand Elite UBRS", 1),
    20031: ("Enchant Weapon: Superior Striking (+5 dmg)", "Blackrock Spire drop", 1),
    20032: ("Enchant Weapon: Lifestealing (lifesteal proc)", "Scholomance drop", 1),
    20033: ("Enchant Weapon: Unholy Weapon (shadow proc)", "Stratholme drop", 1),
    20034: ("Crusader (str proc)", "Scarlet Spellbinder EPL drop", 1),
    20035: ("Enchant 2H Weapon: Major Spirit (+9 spi, 2H)", "Scholomance drop", 1),
    20036: ("Enchant 2H Weapon: Major Intellect (+9 int, 2H)", "Crimson Sorcerer Strat drop", 1),
    23799: ("Enchant Weapon: Strength (+15 str)", "Lokhtos Darkbargainer BRD", 1),
    23800: ("Enchant Weapon: Agility (+15 agi)", "Meilosh Timbermaw (Honored)", 1),
    23803: ("Enchant Weapon: Mighty Spirit (+20 spi)", "Lokhtos Darkbargainer", 1),
    23804: ("Enchant Weapon: Mighty Intellect (+22 int)", "Lokhtos Darkbargainer", 1),
    27837: ("Enchant 2H Weapon: Agility (+25 agi, 2H)", "Meilosh Timbermaw (Friendly)", 1),
    22749: ("Enchant Weapon: Spell Power (+30 dmg)", "Incindis MC", 3),
    22750: ("Enchant Weapon: Healing Power (+55 healing)", "Incindis MC", 3),
    36942: ("Enchant Weapon: Rift Tear", "Draenei Exiles Revered chain (unverified)", 7),
    49020: ("Enchant 2H Weapon: Nature Damage (+40 nature, 2H)", "Ormanos the Cracked, Timbermaw Hold (819)", 3),
    49022: ("Enchant 2H Weapon: Shadow Damage (+40 shadow, 2H)", "Selenaxx Foulheart, Timbermaw Hold (819)", 3),
    20017: ("Enchant Shield: Greater Stamina (+7 stam)", "shield-only", 1),
    12460: ("Sniper Scope (+7 dmg)", "engineering/crafted", 1),
    22779: ("Biznicks 247x128 Accurascope (+3% hit)", "schematic: MC bosses", 3),
    36945: ("Facetted Crystal Scope (+2% crit)", "Draenei Exiles Revered chain (unverified)", 7),
    46101: ("Ancient Dwarven Gemstone (+3 heal/dmg)", "Turtle custom (vendor TBD)", 1),
    49013: ("Crystalized Topaz (+9 arcane dmg)", "Turtle custom (vendor TBD)", 1),
    51028: ("Radiant Ember Gemstone", "Turtle custom (vendor TBD)", 1),
    51029: ("Illuminated Gemstone (+4 spi)", "Turtle custom (vendor TBD)", 1),
    51030: ("Pure Shining Moonstone (+6 arcane dmg)", "Turtle custom (vendor TBD)", 1),
    51031: ("Burning Star Gemstone (+5 str)", "Turtle custom (vendor TBD)", 1),
    51032: ("Glowing Ruby Gemstone (+1% lifesteal)", "Turtle custom (vendor TBD)", 1),
    51033: ("Glittering Sapphire Gemstone (+9 frost dmg)", "Turtle custom (vendor TBD)", 1),
    51034: ("Shining Sapphire Gemstone (+6 int)", "Turtle custom (vendor TBD)", 1),
    51035: ("Brilliant Opal Gemstone (+6 agi)", "Turtle custom (vendor TBD)", 1),
    51036: ("Elegant Emerald Gemstone (+8 NR)", "Turtle custom (vendor TBD)", 1),
    51037: ("Enchanted Emerald Gemstone (+9 nature heal)", "Turtle custom (vendor TBD)", 1),
    51038: ("Beautiful Diamond Gemstone (+12 healing)", "Turtle custom (vendor TBD)", 1),
    51039: ("Tempered Azerothian Gemstone (+6 stam)", "Turtle custom (vendor TBD)", 1),
    51040: ("Unstable Arcane Gemstone (+8 ArcR)", "Turtle custom (vendor TBD)", 1),
    51041: ("Gloomy Diamond Gemstone (+8 FR)", "Turtle custom (vendor TBD)", 1),
    51042: ("Azerothian Ruby Gemstone (+6 str)", "Turtle custom (vendor TBD)", 1),
    51043: ("Arcane Emerald Gemstone (+5 spell dmg)", "Turtle custom (vendor TBD)", 1),
    51044: ("Flawless Black Gemstone (+3 all res)", "Turtle custom (vendor TBD)", 1),
    51045: ("Stunning Imperial Gemstone (+3 all stats)", "Turtle custom (vendor TBD)", 1),
    51077: ("Gorgeous Mountain Gemstone", "Turtle custom (vendor TBD)", 1),
}
W2H = {20030, 20035, 20036, 27837, 49020, 49022}

# wowisclassic consumable-item idents -> spell id (verified via
# item_template.spellid_1).
ITEM2SPELL = {
    11622: 15340, 11642: 15389, 11643: 15391, 11644: 15394, 11645: 15397,
    11646: 15400, 11647: 15402, 11648: 15404, 11649: 15406,
    18329: 22840, 18330: 22844, 18331: 22846,
    22635: 28161, 22636: 28163, 22638: 28165,
    18283: 22779, 10548: 12460,
    19782: 24149, 19783: 24160, 19784: 24161, 19785: 24162, 19786: 24163,
    19787: 24164, 19788: 24165, 19789: 24167, 19790: 24168,
    20076: 24421, 20077: 24422, 20078: 24420,
    23545: 29467, 23547: 29475, 23548: 29483, 23549: 29480,
    18182: 22599, 18251: 22725,
}

# Turtle sheet enchant display name -> spell id.
TSNAME2SPELL = {
    "Lesser Arcanum of Voracity": 15402,
    "Arcanum of Rapidity": 22840, "Arcanum of Focus": 22844,
    "Arcanum of Protection": 22846, "Gift of Ferocity": 45739,
    "Presence of Might": 24149, "Syncretist's Sigil": 24160,
    "Death's Embrace": 24161, "Falcon's Call": 24162,
    "Vodouisant's Vigilant Embrace": 24163, "Presence of Sight": 24164,
    "Hoodoo Hex": 24165, "Prophetic Aura": 24167,
    "Animist's Caress": 24168, "Zandalar Signet of Serenity": 24420,
    "Zandalar Signet of Mojo": 24421, "Zandalar Signet of Might": 24422,
    "Power of the Scourge": 29467, "Resilience of the Scourge": 29475,
    "Fortitude of the Scourge": 29480, "Might of the Scourge": 29483,
    "Sigil of Quickness": 57156,
    "Enchant Chest - Greater Stats": 20025, "Enchanted Armor Kit": 57135,
    "Enchant Boots - Greater Agility": 20023,
    "Enchant Boots - Superior Stamina": 57127,
    "Enchant Bracer - Greater Agility": 57030,
    "Enchant Bracer - Superior Strength": 20010,
    "Enchant Bracer - Superior Stamina": 20011,
    "Enchant Bracer - Spell Power": 57028,
    "Enchant Bracer - Healing Power": 23802,
    "Enchant Gloves - Major Strength": 46086,
    "Enchant Gloves - Minor Haste": 13948,
    "Enchant Gloves - Superior Agility": 25080,
    "Enchant Gloves - Healing Power": 25079,
    "Enchant Gloves - Threat": 25072,
    "Enchant Gloves - Fire Power": 25078,
    "Enchant Gloves - Frost Power": 25074,
    "Enchant Gloves - Shadow Power": 25073,
    "Enchant Gloves - Arcane Power": 46601,
    "Enchant Gloves - Nature Power": 46602,
    "Enchant Cloak - Lesser Agility": 13882,
    "Enchant Cloak - Greater Resistance": 20014,
    "Enchant Cloak - Subtlety": 25084, "Enchant Cloak - Dodge": 25086,
    "Enchant Cloak - Agility": 49016,
    "Enchant Cloak - Superior Defense": 20015,
    "Enchant Weapon - Crusader": 20034,
    "Enchant Weapon - Spell Power": 22749,
    "Enchant Weapon - Healing Power": 22750,
    "Enchant Weapon - Agility": 23800,
    "Enchant Weapon - Strength": 23799,
    "Enchant 2H Weapon - Agility": 27837,
    "Enchant Shield - Greater Stamina": 20017,
    "Enchant Shield - Lesser Block": 13689,
    "Stunning Imperial Gemstone": 51045,
    "Arcane Emerald Gemstone": 51043,
    "Brilliant Opal Gemstone": 51035,
    "Beautiful Diamond Gemstone": 51038,
    "Azerothian Ruby Gemstone": 51042,
    "Radiant Ember Gemstone": 51028,
    "Gorgeous Mountain Gemstone": 51077,
    "Enchanted Emerald Gemstone": 51037,
    "Glittering Sapphire Gemstone": 51033,
    "Pure Shining Moonstone": 51030,
    "Dreamsteel Belt Buckle": 57182, "Mithril Belt Buckle": 57173,
    "Obsidian Belt Buckle": 73, "Dragonscale Belt Buckle": 41,
    "Arcanite Belt Buckle": 57179,
}

# Role fallback (slot -> role -> spell) when no source lists a pick.
# caster splits from dps: no pre-raid spell-power weapon/glove rows exist in
# the pool, so casters take Mighty Intellect (Lokhtos, t1-legal) on weapons
# and Minor Haste (in-pool slot-9 row, haste proc usable by casters) on
# gloves until the AQ spell-power gloves (t2) arrive.
ROLE_FALLBACK = {
    0: {"tank": 15389, "healer": 15406, "caster": 15404, "dps": 15402},
    6: {"tank": 15444, "healer": 15449, "caster": 15449, "dps": 15446},
    2: {"tank": 22597, "healer": 22598, "caster": 22598, "dps": 22599},
    4: {"tank": 20026, "healer": 20028, "caster": 20028, "dps": 20025},
    7: {"tank": 20020, "healer": 20024, "caster": 56543, "dps": 20023},
    8: {"tank": 20011, "healer": 23802, "caster": 20008, "dps": 20010},
    9: {"tank": 25072, "healer": 25079, "caster": 13948, "dps": 20012},
    14: {"tank": 20015, "healer": 20014, "caster": 20014, "dps": 13882},
    15: {"tank": 20034, "healer": 22750, "caster": 23804, "dps": 20034},
    16: {"tank": 20017, "healer": 20017, "caster": 23804, "dps": 20034},
}
ROLE_GEM = {"tank": 51045, "healer": 51038, "caster": 51043,
            "melee": 51042, "hunter": 51035}
ROLE_BUCKLE = {"tank": 57173, "healer": 57182, "caster": 57182,
               "melee": 73, "hunter": 57173}

# 2H-primary specs (MH may take 2H rows; slot 16 omitted).
TWOHAND = {("warrior", "arms"), ("paladin", "retribution"),
           ("shaman", "enhancement"), ("druid", "bear"), ("druid", "cat"),
           ("hunter", "beastmastery"), ("hunter", "marksmanship"),
           ("hunter", "survival")}
# Shield offhand specs.
SHIELD = {("warrior", "protection"), ("paladin", "protection"),
          ("paladin", "holy"), ("priest", "holy"), ("priest", "discipline"),
          ("shaman", "restoration"), ("shaman", "elemental"),
          ("shaman", "tank")}

TIER_NAMES = {1: "pre-raid", 2: "ZG+AQ20+LowerKara", 3: "MC+Ony+Timbermaw",
              4: "BWL+ES", 5: "AQ40", 6: "Naxx", 7: "Tower of Karazhan"}


def role_of(cls, spec, hire_role):
    if hire_role == "tank":
        return "tank"
    if hire_role == "healer":
        return "healer"
    if cls in ("mage", "priest", "warlock") or spec in (
            "balance", "elemental", "shadow"):
        return "caster"
    if cls == "hunter":
        return "hunter"
    return "melee"


INVTYPE_EQUIP = {1: 0, 2: 1, 3: 2, 16: 14, 5: 4, 4: 4, 19: 4, 20: 4,
                9: 8, 10: 9, 6: 5, 7: 6, 8: 7, 11: 10, 12: 13,
                13: 15, 21: 15, 22: 15, 17: 15,
                14: 16, 23: 16, 28: 16,
                15: 17, 18: 17, 25: 17, 26: 17}
# inventory_type of every wowisclassic gear item, dumped from tw_world
# item_template (work/invtypes.tsv, 575/575 resolved).
INVTYPE = {}


def load_invtypes():
    p = HERE / "work" / "invtypes.tsv"
    for line in p.read_text().splitlines():
        entry, inv = line.split()
        INVTYPE[int(entry)] = int(inv)


def build_wow_lookup():
    """(wclass, wspec, phase) -> list of (slot, spell).

    Slots resolve via the GEAR ITEM's inventory_type (DB), not page
    position: pages omit mh/oh/ranged/relic freely, so positions shift.
    Head/leg class enchants (arcanum-item anchors and ZG-sigil spell
    anchors) attach to BOTH head and legs; per-slot newest-pick keeps
    them in sync, as the sources do.
    """
    load_invtypes()
    out = {}
    for r in WOW["rows"]:
        lst = []
        for e in r["enchants"]:
            gi = e["gear_index"]
            if gi >= len(r["gear"]):
                continue
            ident = e["spell_id"]
            low = e["name"].lower()
            if ident == 18251:
                # Core Armor Kit icon (spell 22725, +5 defense): the chest
                # row is the kit; the same icon on gloves/legs/feet rows
                # is the page's kit-stack shorthand, not a per-slot kit.
                # Keep chest rows only.
                item_id = r["gear"][gi]["item_id"]
                inv = INVTYPE.get(item_id)
                if INVTYPE_EQUIP.get(inv) != 4:
                    continue
            if ident == 22725:
                # same +5-defense enchant id (2503) as a bracer spell row
                # ("defense-3"): the pool has it on slots 4/6/7/9 but no
                # slot-8 row, so bracer rows are unservable today; keep
                # only rows the pool can serve (drop slot 8).
                item_id = r["gear"][gi]["item_id"]
                inv = INVTYPE.get(item_id)
                if INVTYPE_EQUIP.get(inv) == 8:
                    continue
            spell = ITEM2SPELL.get(ident, ident)
            if spell not in CAT:
                continue
            if "arcanum" in low or "sigil" in low or "embrace" in low or \
                    "falcon" in low or "hex" in low or "aura" in low or \
                    "caress" in low or "presence-of" in low or \
                    "guard" in low or "invocation" in low or \
                    "ferocity" in low or "sight" in low or \
                    "might" in low and "presence" in low:
                lst.append((0, spell))
                lst.append((6, spell))
                continue
            item_id = r["gear"][gi]["item_id"]
            inv = INVTYPE.get(item_id)
            if inv is None:
                continue
            slot = INVTYPE_EQUIP.get(inv)
            if slot is None or slot in (12, 13, 5, 1, 10, 11):
                continue  # trinket/waist/neck/rings: not formula enchants
            if slot in (15, 16) and "scope" in low:
                slot = 17
            if slot == 17 and spell not in (22779, 12460):
                continue  # ranged item carrying a non-scope anchor: skip
            lst.append((slot, spell))
        out[(r["class"], r["spec"], r["phase"])] = lst
    return out


def build_ts_lookup():
    """(class, tab-frag-match) rows per list: (class, tab, list) -> slots."""
    out = {}
    for r in TS["rows"]:
        if r["list_name"] not in ("Naxx", "K40") or not r["enchant"]:
            continue
        out.setdefault((r["class"], r["tab"], r["list_name"]), {})[r["slot"]] = \
            r["enchant"].replace("Formula: ", "")
    return out


TS_SLOT2EQUIP = {"Head": 0, "Neck": 1, "Shoulders": 2, "Chest": 4,
                 "Waist": 5, "Legs": 6, "Feet": 7, "Wrist": 8, "Hands": 9,
                 "Ring 1": 10, "Ring 2": 11, "Back": 14, "Main Hand": 15,
                 "Offhand": 16}


def ts_tab(ts, tcls, frags, lst):
    for frag in frags:
        hit = [k for k in ts if k[0] == tcls and frag in k[1] and k[2] == lst]
        if hit:
            return ts[hit[0]]
    return {}


def main():
    wow = build_wow_lookup()
    ts = build_ts_lookup()
    unresolved = []
    result = {}
    for cls, spec, hire_role in SPECS:
        key = f"{cls}_{spec}"
        wcls, wspec = WOWMAP[(cls, spec)]
        role = role_of(cls, spec, hire_role)
        fb_role = role if role in ("tank", "healer", "caster") else "dps"
        # progression: slot -> [(unlock tier, spell, label)] in recency order
        prog = {}
        for phase in ["pre-raid", "1", "2", "3", "4", "5", "6"]:
            for slot, spell in wow.get((wcls, wspec, phase), []):
                prog.setdefault(slot, []).append(
                    (CAT[spell][2], spell, f"wowisclassic {wcls}/{wspec} P{phase}"))
        tcls, frags = TSMAP[(cls, spec)]
        for lst, ttier in (("Naxx", 6), ("K40", 7)):
            for sheet_slot, ename in ts_tab(ts, tcls, frags, lst).items():
                if sheet_slot not in TS_SLOT2EQUIP:
                    continue
                slot = TS_SLOT2EQUIP[sheet_slot]
                if slot in (1, 10, 11, 5):
                    continue  # gems/buckles handled per-tier below
                spell = TSNAME2SPELL.get(ename)
                if spell is None or spell not in CAT:
                    unresolved.append(f"{key} {lst} {sheet_slot}: "
                                      f"unmapped turtle name {ename!r}")
                    continue
                prog.setdefault(slot, []).append(
                    (ttier, spell, f"Turtle {lst} {tcls}"))
        tiers = {}
        for tier in range(1, 8):
            row = {}
            for slot in SLOTS:
                if slot == 17 and role != "hunter":
                    continue
                if slot in (1, 10, 11):
                    gem = ROLE_GEM[role]
                    if tier >= 7:
                        gem = ts_pick(ts, tcls, frags, "K40", "Neck", gem,
                                      key, unresolved)
                    elif tier >= 6:
                        gem = ts_pick(ts, tcls, frags, "Naxx", "Neck", gem,
                                      key, unresolved)
                    spell = gem
                    label = "gemstone"
                elif slot == 5:
                    buckle = ROLE_BUCKLE[role]
                    if tier >= 7:
                        buckle = ts_pick(ts, tcls, frags, "K40", "Waist",
                                         buckle, key, unresolved)
                    elif tier >= 6:
                        buckle = ts_pick(ts, tcls, frags, "Naxx", "Waist",
                                         buckle, key, unresolved)
                    spell = buckle
                    label = "buckle"
                elif slot == 16 and (cls, spec) in SHIELD:
                    spell, label = 20017, "shield"
                elif slot == 16:
                    # Offhand: mirror MH when 1H-legal, else role fallback.
                    # 2H-primary specs normally wear no OH (K40 Bear Mit is
                    # the 1H+branch exception); a row here is harmless when
                    # the slot is empty (EnchantItemT no-ops on no item).
                    mh = pick(prog.get(15, []), tier)
                    if mh is not None and mh[0] not in W2H:
                        spell, label = mh[0], mh[1] + " (OH mirrors MH)"
                    else:
                        fb = ROLE_FALLBACK[16][fb_role]
                        spell, label = fb, "role-fallback"
                else:
                    hit = pick(prog.get(slot, []), tier)
                    if hit is None:
                        fb = ROLE_FALLBACK.get(slot, {}).get(fb_role)
                        if fb is None:
                            continue
                        spell, label = fb, "role-fallback"
                    else:
                        spell, label = hit
                    if slot == 16 and spell in W2H:
                        alt = [(s, lb) for t, s, lb in prog.get(slot, [])
                               if t <= tier and s not in W2H]
                        if alt:
                            spell, label = alt[-1]
                name, source, _t = CAT[spell]
                if label == "role-fallback":
                    source = source + " [role-fallback]"
                elif label not in ("gemstone", "buckle", "shield") and \
                        not label.startswith(("wowisclassic", "Turtle")):
                    source = source + f" [{label}]"
                row[str(slot)] = {"spell": spell, "name": name,
                                  "source": source,
                                  "pick": label}
            tiers[str(tier)] = row
        result[key] = tiers
    out = DATA / "enchants_by_tier.json"
    out.write_text(json.dumps(
        {"source": "hire-gear enchants (issue #490)",
         "id_space": "enchant SPELL id "
                     "(ai_playerbot_enchant_candidates.spellid; "
                     "PlayerbotAI::EnchantItemT(spellid) argument)",
         "tiers": {str(t): n for t, n in TIER_NAMES.items()},
         "specs": result}, indent=1) + "\n")
    print(f"specs={len(result)} -> {out}")
    for u in unresolved[:40]:
        print("UNRESOLVED: " + u)
    print(f"unresolved={len(unresolved)}")


def pick(cands, tier):
    """Newest listing whose source tier is unlocked; None if empty."""
    best = None
    for t, s, lb in cands:
        if t <= tier:
            best = (s, lb)
    return best


BUCKLES = {40, 41, 52, 54, 73, 121, 41651, 57164, 57167, 57170, 57173,
           57176, 57179, 57182, 57186}


def ts_pick(ts, tcls, frags, lst, sheet_slot, default, key, unresolved):
    tab = ts_tab(ts, tcls, frags, lst)
    ename = tab.get(sheet_slot)
    if not ename:
        return default
    spell = TSNAME2SPELL.get(ename)
    if spell is None or spell not in CAT:
        unresolved.append(f"{key} {lst} {sheet_slot}: unmapped {ename!r}")
        return default
    if sheet_slot == "Waist" and spell not in BUCKLES:
        # sheet data error (e.g. druid Boomkin Naxx waist lists a bracer
        # enchant): ignore, keep the role-default buckle.
        unresolved.append(f"{key} {lst} Waist: not a buckle: {ename!r}")
        return default
    return spell


if __name__ == "__main__":
    sys.exit(main())
