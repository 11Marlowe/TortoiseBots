#!/usr/bin/env python3
"""Parse vendored BestInSlotClassic Lua data into normalized JSON.

Reads bis_data.lua, bis_ranking.lua, items.lua from a BestInSlotClassic
checkout at the pinned commit (path as argv[1]; the Lua files are not
vendored). Writes: ../../data/bisclassic.json.gz

Row: {class, class_name, spec, spec_name, faction, phase, slot, slot_name,
      equip_slot, item_id, rank, races, offhand}
  class/spec : numeric ids as used by the addon (CLASS_IDX; SpecId = VALUE)
  phase      : P1..P6 (classic phases; remap to hire tiers happens later)
  slot       : addon INVSLOT id; equip_slot = slot-1 (EquipmentSlots 0..18)
  rank       : per-phase rank, lower is better, 1 ~= BIS
  races      : null (all) or list of race ids (1 Human .. 8 Troll)
  offhand    : bool, item is an offhand alternative for that slot
"""
import gzip
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
SRC = Path(sys.argv[1]) if len(sys.argv) > 1 else HERE
OUT = HERE / ".." / ".." / "data" / "bisclassic.json.gz"

CLASS_NAMES = {1: "Warrior", 2: "Paladin", 3: "Hunter", 4: "Rogue",
               5: "Priest", 7: "Shaman", 8: "Mage", 9: "Warlock", 11: "Druid"}
SPEC_NAMES = {
    1: {1: "Fury", 2: "Protection (Threat)", 3: "Protection (Mitigation)"},
    2: {1: "Holy", 2: "Protection", 3: "Retribution (AP)", 4: "Retribution (Spelladin)"},
    3: {1: "Normal", 2: "Melee Weave"},
    4: {1: "Swords", 2: "Daggers"},
    5: {1: "Holy / Discipline", 2: "Shadow"},
    7: {1: "Restoration", 2: "Enhancement"},
    8: {1: "Frost/Fire", 2: "Zero SP", 3: "Alternative"},
    9: {1: "Any"},
    11: {1: "Feral Tank (Value)", 2: "Feral Tank (Mitigation)", 3: "Feral Tank (Threat)",
         4: "Feral DPS", 5: "Restoration", 6: "Balance"},
}
SLOT_NAMES = {0: "Ammo", 1: "Head", 2: "Neck", 3: "Shoulder", 4: "Shirt",
              5: "Chest", 6: "Belt", 7: "Leg", 8: "Boot", 9: "Wrist",
              10: "Glove", 11: "MainRing", 12: "OffRing", 13: "MainTrinket",
              14: "OffTrinket", 15: "Back", 16: "MainHand", 17: "OffHand",
              18: "Ranged"}

ITEM_RE = re.compile(
    r"BIS_ITEMS\[(\d+)\] = \{Phase = ([\d.]+), PvP = (\w+), PvPRank = (\d+), "
    r"Raid = (\w+), WorldBoss = (\w+), TwoHands = (\w+), Faction = (\{[^}]*\}|nil), "
    r"Slot = (\d+)")
ENTRY_RE = re.compile(
    r"\{\s*classId = (\d+),\s*specId = (\d+),\s*offHand = (\w+),\s*"
    r"races = (\{[^}]*\}|nil),\s*"
    r'P1 = "([^"]*)",\s*P2 = "([^"]*)",\s*P3 = "([^"]*)",\s*'
    r'P4 = "([^"]*)",\s*P5 = "([^"]*)",\s*P6 = "([^"]*)"\s*\}')
RACE_ID_RE = re.compile(r"(\d+)")


def parse_races(raw):
    if raw == "nil":
        return None
    return [int(x) for x in RACE_ID_RE.findall(raw)] or None


def main():
    items = {}  # item_id -> dict(phase, raid, worldboss, pvp, twohands, faction, slot)
    for line in (SRC / "items.lua").read_text().splitlines():
        m = ITEM_RE.search(line)
        if m:
            items[int(m.group(1))] = {
                "phase": float(m.group(2)), "pvp": m.group(3) == "true",
                "raid": m.group(5) == "true", "worldboss": m.group(6) == "true",
                "twohands": m.group(7) == "true", "slot": int(m.group(9))}

    links = 0
    for line in (SRC / "bis_data.lua").read_text().splitlines():
        if line.startswith("table.insert(BIS_LINKS"):
            links += 1

    rows = {}  # dedupe key -> row
    for line in (SRC / "bis_ranking.lua").read_text().splitlines():
        m0 = re.search(r"BIS_TOOLTIP_RANKING\[(\d+)\]", line)
        if not m0:
            continue
        item_id = int(m0.group(1))
        info = items.get(item_id)
        if info is None:
            continue
        for m in ENTRY_RE.finditer(line):
            # nearest preceding faction marker on the line
            alli = line.rfind('["Alliance"]', 0, m.start())
            horde = line.rfind('["Horde"]', 0, m.start())
            faction = "Alliance" if alli > horde else "Horde"
            cls, spec = int(m.group(1)), int(m.group(2))
            offhand = m.group(3) == "true"
            races = parse_races(m.group(4))
            for i, phase in enumerate(("P1", "P2", "P3", "P4", "P5", "P6")):
                rank_raw = m.group(5 + i)
                if rank_raw == "":
                    continue
                try:
                    rank = int(rank_raw)
                except ValueError:
                    continue
                if rank <= 0:
                    continue  # 0 = unranked for that phase (tooltip filler)
                key = (cls, spec, faction, phase, info["slot"], item_id, rank,
                       tuple(races) if races else None, offhand)
                rows[key] = {
                    "class": cls, "class_name": CLASS_NAMES.get(cls, str(cls)),
                    "spec": spec,
                    "spec_name": SPEC_NAMES.get(cls, {}).get(spec, str(spec)),
                    "faction": faction, "phase": phase,
                    "slot": info["slot"],
                    "slot_name": SLOT_NAMES.get(info["slot"], str(info["slot"])),
                    "equip_slot": info["slot"] - 1,
                    "item_id": item_id, "rank": rank,
                    "races": races, "offhand": offhand}

    doc = {"source": "bisclassic",
           "upstream": "https://github.com/Warkdev/BestInSlotClassic",
           "commit": "302ab4ecca0c86b43441710dab840af5edd3ca3e",
           "fetched": "2026-10-12",
           "license": "GPLv2 (see sources/bisclassic/LICENSE.txt)",
           "candidate_links": links,
           "rows": sorted(rows.values(),
                          key=lambda r: (r["class"], r["spec"], r["phase"],
                                         r["slot"], r["rank"], r["item_id"]))}
    OUT.parent.mkdir(parents=True, exist_ok=True)
    raw = json.dumps(doc, separators=(",", ":"), sort_keys=True).encode()
    with gzip.GzipFile(OUT, "wb", mtime=0) as f:  # mtime 0: reproducible bytes
        f.write(raw)
    classes = sorted({r["class"] for r in doc["rows"]})
    print(f"rows={len(doc['rows'])} items={len({r['item_id'] for r in doc['rows']})}"
          f" classes={classes} links={links} -> {OUT}")


if __name__ == "__main__":
    sys.exit(main())
