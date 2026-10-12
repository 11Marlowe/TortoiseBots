#!/usr/bin/env python3
"""Scrape wowisclassic.com BIS pages into a JSON snapshot (polite: 1 req/s).

Matrix (from raw research): 19 class/spec combos x phases pre-raid,1..6.
Writes: ../../data/wowisclassic.json

Row: {class, spec, phase, slot, item_id, name, enchant_spell, enchant_name}
  slot: equip slot 0..18 resolved via world DB inventory_type; head/leg
  arcanums and shoulder inscriptions resolve by name keyword instead.
"""
import json
import re
import sys
import time
import urllib.request
from pathlib import Path

SRC = Path(__file__).resolve().parent
OUT = SRC / ".." / ".." / "data" / "wowisclassic.json"
UA = {"User-Agent": "TortoiseBots-hire-gear-research/1.0 (offline BIS snapshot; 1 req/s)"}

MATRIX = {
    "druid": ["balance", "cat", "resto", "tank"],
    "hunter": ["hunter"],
    "mage": ["mage"],
    "paladin": ["heal", "protection", "ret"],
    "priest": ["holy", "shadow"],
    "rogue": ["dagger", "sword"],
    "shaman": ["elem", "enhan", "resto"],
    "warlock": ["warlock"],
    "warrior": ["fury", "prot"],
}
PHASES = ["pre-raid", "1", "2", "3", "4", "5", "6"]

# fixed gear order on the page (enchants interleaved after their slot's item)
GEAR_ORDER = ["mh", "oh", "ranged",
              "head", "head_ench", "neck", "shoulder", "shoulder_ench",
              "back", "chest", "wrist", "hands", "waist", "legs", "legs_ench",
              "feet", "ring1", "ring2", "trinket1", "trinket2"]

# inventory_type -> equip slot (0..18); 0 = non-equippable (arcanum/scope/inscription)
INVTYPE_SLOT = {1: 0, 2: 1, 3: 2, 16: 14, 5: 4, 9: 8, 10: 9, 6: 5,
                7: 6, 8: 7, 11: 10, 12: 13, 4: 4, 19: 4,
                13: 15, 14: 15, 17: 16, 15: 17, 18: 17, 26: 17, 28: 17,
                21: 15, 22: 15}
ENCHANT_SLOT_BY_NAME = [("arcanum", 0), ("libram", 0), ("scourge", 2), ("inscription", 2),
                        ("signet", 2), ("zg enchant", 0), ("cloak", 14), ("chest", 4),
                        ("bracer", 8), ("glove", 9), ("boot", 7), ("weapon", 15),
                        ("shield", 16), ("scope", 17)]

ANCHOR_RE = re.compile(
    r"<a href=\"https://classic\.wowhead\.com/(item|spell)=(\d+)/([a-z0-9-]*)\"([^>]*)>")


def fetch(url):
    req = urllib.request.Request(url, headers=UA)
    with urllib.request.urlopen(req, timeout=30) as r:
        return r.read().decode("utf-8", "replace")


def parse_page(html):
    """Return (gear_ids, enchant_ids) in page order.

    The page lists weapons first (mh, oh, each with weapon enchant, then
    ranged + scope), then armor head->offhand. Enchant/spell anchors and
    head-leg-arcanum *item* anchors (11645-style) carry data-wh-icon-size
    and belong to the preceding slot; ring/trinket pairs and relics are
    plain. Walk anchors in order, attaching marked anchors to the last
    gear slot; bare anchors advance the gear cursor.
    """
    anchors = [(m.group(1), int(m.group(2)), m.group(3),
                "data-wh-icon-size" in (m.group(4) or ""))
               for m in ANCHOR_RE.finditer(html)]
    gear, enchants = [], []  # (item_id, name) / (slot_index, spell_id, name)
    for kind, ident, name, marked in anchors:
        if kind == "item" and not marked:
            gear.append((ident, name))
        elif gear:
            enchants.append((len(gear) - 1, ident, name))
    return gear, enchants


def invtype_slot(invtype, name):
    if invtype in INVTYPE_SLOT:
        return INVTYPE_SLOT[invtype]
    low = (name or "").lower()
    for kw, slot in ENCHANT_SLOT_BY_NAME:
        if kw in low:
            return slot
    if "totem" in low or "idol" in low or "libram" in low:
        return 17
    return None


def main():
    only = sys.argv[1:]  # optional subset like warrior/fury
    rows, meta = [], {"pages": {}, "errors": []}
    n = 0
    for cls, specs in MATRIX.items():
        for spec in specs:
            for phase in PHASES:
                key = f"{cls}/{spec}/{phase}"
                if only and not any(key.startswith(o) for o in only):
                    continue
                url = (f"https://www.wowisclassic.com/en/best-in-slot/{cls}/"
                       f"?phase={phase}&specialization={spec}")
                time.sleep(1.0)  # polite: 1 req/s
                try:
                    html = fetch(url)
                except Exception as e:  # noqa: BLE001
                    meta["errors"].append({"page": key, "error": str(e)})
                    continue
                gear, enchants = parse_page(html)
                meta["pages"][key] = {"items": len(gear),
                                      "enchants": len(enchants)}
                rows.append({"page": key, "class": cls, "spec": spec,
                             "phase": phase, "gear": gear, "enchants": enchants})
                n += 1
                print(f"{key}: {len(gear)} items {len(enchants)} enchants", flush=True)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps({"source": "wowisclassic",
                               "base": "https://www.wowisclassic.com/en/best-in-slot/",
                               "fetched": "2026-10-12",
                               "ua": UA["User-Agent"],
                               "rate": "1 req/s",
                               "pages": meta["pages"], "errors": meta["errors"],
                               "gear_order": GEAR_ORDER,
                               "rows": [{"class": r["class"], "spec": r["spec"],
                                         "phase": r["phase"],
                                         "gear": [{"item_id": i, "name": nm}
                                                  for i, nm in r["gear"]],
                                         "enchants": [{"gear_index": gi,
                                                       "spell_id": s, "name": nm}
                                                      for gi, s, nm in r["enchants"]]}
                                        for r in rows]}, indent=1) + "\n")
    print(f"pages={n} errors={len(meta['errors'])} -> {OUT}")


if __name__ == "__main__":
    sys.exit(main())
