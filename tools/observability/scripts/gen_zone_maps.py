#!/usr/bin/env python3
"""Generate the dashboard's zone art from your own WoW client, fully revealed.

The dashboard draws the client's WorldMap texture: 12 base tiles (a 4x3 grid of
256px textures, `Interface\\WorldMap\\<Zone>\\<Zone><1..12>.blp`) that render the
map in its unexplored style, plus one `WorldMapOverlay.dbc` texture per sub-area
(`Interface\\WorldMap\\<Zone>\\<Overlay><n>.blp`) which paints in the revealed
detail. This script composites the overlays at their DBC offsets and writes one
`<zone>.webp` per map into `--out`. Zones without overlays (every instance and
both continents) are written from the base tiles alone.

Client artwork is not shipped in the repository or the dashboard binaries, so
every operator generates it locally once. Canvas sizes come from
`web/data/map_canvas.json`: the daemon's world->map percentages are relative to
that canvas, so the art geometry must not move. Entries with
`"source": "mapsofmystery"` are hand-drawn maps shipped with the dashboard and
are skipped.

Requires: python3 -m pip install mpyq Pillow
Usage:
  python3 gen_zone_maps.py --data "/path/to/WoW 1.12/Data" \\
      [--overlay-dbc /path/to/WorldMapOverlay.dbc] [--out ../maps] [--dry-run]

`--out` defaults to `tools/observability/maps/`, the folder the dashboard binary
reads next to itself (`--maps-dir`). `--overlay-dbc` defaults to the newest
WorldMapOverlay.dbc readable from the client MPQs. Some Turtle clients keep it in
an encrypted patch that mpyq cannot open; pass the copy the server tooling
extracted (e.g. the one in the server's extracted dbc/ directory) so the offsets
match the shipped art.
"""

import argparse
import io
import json
import math
import os
import bz2
import re
import struct
import sys
import zlib

try:
    import mpyq
    from PIL import Image
except ImportError as exc:  # pragma: no cover - tooling guard
    sys.exit(f"missing dependency: {exc}. Run: python3 -m pip install mpyq Pillow")

# Highest patch priority first, so a later patch's file wins.
ARCHIVE_ORDER = [
    "patch-9.MPQ", "patch-8.MPQ", "patch-7.mpq", "patch-6.mpq", "patch-5.mpq",
    "patch-4.mpq", "patch-3.mpq", "patch-2.MPQ", "patch.MPQ", "interface.MPQ",
]
TILE = 256
GRID_W = 4  # the client lays the 12 detail tiles out 4 wide (WorldMapFrame.xml)
VISIBLE = (1002, 668)  # the part of the 1024x768 tile grid the client shows

# Client folder typos / renames vs the dashboard's zone file names.
FOLDER_ALIASES = {"darnassus": "Darnassis"}

WEBP_QUALITY = 90


class Archive(mpyq.MPQArchive):
    """mpyq plus encrypted files: Turtle ships its newer map art (patch-4/8/9) encrypted."""

    def _crypt(self, data, key):
        whole = len(data) & ~3
        return self._decrypt(data[:whole], key) + data[whole:]

    def read_file(self, filename, force_decompress=False):
        entry = self.get_hash_table_entry(filename)
        if entry is None:
            return None
        block = self.block_table[entry.block_table_index]
        if not block.flags & mpyq.MPQ_FILE_ENCRYPTED:
            return super().read_file(filename, force_decompress)
        if not block.flags & mpyq.MPQ_FILE_EXISTS or block.archived_size == 0:
            return None
        # Standard MPQ file key: hash of the bare file name, optionally offset-adjusted.
        key = self._hash(filename.split("\\")[-1], "TABLE")
        if block.flags & mpyq.MPQ_FILE_FIX_KEY:
            key = ((key + block.offset) ^ block.size) & 0xFFFFFFFF
        self.file.seek(block.offset + self.header["offset"])
        data = self.file.read(block.archived_size)
        compressed = block.flags & mpyq.MPQ_FILE_COMPRESS
        if block.flags & mpyq.MPQ_FILE_SINGLE_UNIT:
            data = self._crypt(data, key)
            return decompress(data) if compressed and block.size > block.archived_size else data
        sector_size = 512 << self.header["sector_size_shift"]
        sectors = (block.size + sector_size - 1) // sector_size
        table = self._crypt(data[:4 * (sectors + 1)], (key - 1) & 0xFFFFFFFF)
        positions = struct.unpack(f"<{sectors + 1}I", table)
        out, left = [], block.size
        for i in range(sectors):
            sector = self._crypt(data[positions[i]:positions[i + 1]], (key + i) & 0xFFFFFFFF)
            if compressed and len(sector) < min(sector_size, left):
                sector = decompress(sector)
            left -= len(sector)
            out.append(sector)
        return b"".join(out)


def decompress(data):
    if data[0] == 2:
        return zlib.decompress(data[1:], 15)
    if data[0] == 16:
        return bz2.decompress(data[1:])
    raise RuntimeError(f"unsupported MPQ compression {data[0]}")


class Client:
    def __init__(self, data_dir):
        self.archives = []
        self.folders = {}  # folder -> {lowercase filename: [archives, highest priority first]}
        canonical = {}  # lowercase folder -> first spelling seen (patches differ in case)
        # Patch file name case differs between clients; match it like Windows does.
        on_disk = {name.lower(): name for name in os.listdir(data_dir)}
        for name in ARCHIVE_ORDER:
            name = on_disk.get(name.lower())
            if name is None:
                continue
            path = os.path.join(data_dir, name)
            try:
                archive = Archive(path)
            except Exception as exc:
                print(f"  skip {name}: {exc}")
                continue
            self.archives.append((name, archive))
            for entry in archive.files:
                entry = entry.decode("utf8", "replace") if isinstance(entry, bytes) else entry
                m = re.match(r"Interface\\WorldMap\\([^\\]+)\\([^\\]+)$", entry)
                if m and m.group(2).lower().endswith(".blp"):
                    folder = canonical.setdefault(m.group(1).lower(), m.group(1))
                    self.folders.setdefault(folder, {}).setdefault(m.group(2).lower(), []).append(archive)

    def read(self, folder, name):
        # Some Turtle patch textures are DXT BLPs Pillow cannot decode; an older
        # copy of the same tile is the next best thing.
        for archive in self.folders.get(folder, {}).get(name.lower(), []):
            data = archive.read_file(f"Interface\\WorldMap\\{folder}\\{name}")
            try:
                return Image.open(io.BytesIO(data)).convert("RGBA")
            except Exception as exc:
                error = exc
        if name.lower() in self.folders.get(folder, {}):
            print(f"  {folder}\\{name}: unreadable texture skipped ({error})")
        return None

    def folder_for(self, stem):
        stem = FOLDER_ALIASES.get(stem, stem)
        for folder in self.folders:
            if folder.lower() == stem.lower():
                return folder
        for folder in self.folders:
            if folder.lower().replace(" ", "") == stem.lower().replace(" ", ""):
                return folder
        return None

    def newest_dbc(self, entry):
        for _, archive in self.archives:
            try:
                return archive.read_file(entry)
            except Exception:
                continue
        return None


def parse_overlays(raw):
    """WorldMapOverlay.dbc -> {world_map_area_id: [(name, w, h, ox, oy), ...]}."""
    records, fields, rec_size, _ = struct.unpack("<IIII", raw[4:20])
    base = 20
    strings = base + records * rec_size
    string_block = raw[strings:]

    def string_at(offset):
        end = string_block.index(b"\0", offset)
        return string_block[offset:end].decode("utf8", "replace")

    by_area = {}
    for i in range(records):
        rec = struct.unpack(f"<{fields}I", raw[base + i * rec_size:base + i * rec_size + rec_size])
        by_area.setdefault(rec[1], []).append((string_at(rec[8]), rec[9], rec[10], rec[11], rec[12]))
    return by_area


def compose(client, folder, overlays):
    """Base 4x3 tile grid with every overlay alpha-composited at its DBC offset."""
    grid = Image.new("RGBA", (GRID_W * TILE, 3 * TILE), (0, 0, 0, 255))
    tiles = 0
    for i in range(GRID_W * 3):
        row, col = divmod(i, GRID_W)
        tile = client.read(folder, f"{folder}{i + 1}.blp")
        if tile is not None:
            grid.alpha_composite(tile, (col * TILE, row * TILE))
            tiles += 1

    painted = 0
    for name, width, height, offset_x, offset_y in overlays:
        cols = math.ceil(width / TILE)
        rows = math.ceil(height / TILE)
        for row in range(rows):
            for col in range(cols):
                index = row * cols + col + 1
                tile = client.read(folder, f"{name}{index}.blp")
                if tile is None:
                    continue
                grid.alpha_composite(tile, (offset_x + col * TILE, offset_y + row * TILE))
                painted += 1
    return grid, tiles, painted


def save_at(grid, path, width, height, dry_run):
    """Write the visible composite at the manifest canvas size, padded black.

    The dashboard trims black padding to find the drawing, so 1024x768 canvases
    must keep the client's 1002x668 visible area, not the full tile grid.
    """
    if dry_run:
        return
    out = Image.new("RGB", (width, height), (0, 0, 0))
    out.paste(grid.convert("RGB").crop((0, 0) + VISIBLE), (0, 0))
    out.save(path, "WEBP", quality=WEBP_QUALITY, method=6)


def wma_id_for(file, zone_maps, zones):
    """WorldMapArea id for an art file: via zone_maps.json, else by zone name."""
    for area_id, zone in zone_maps.items():
        if zone["file"] == file:
            bounds = zones.get(f"{zone.get('map', 0)}_{area_id}")
            return bounds["wma_id"] if bounds else None
    stem = file[:-5]
    for bounds in zones.values():
        if bounds["name"].lower() == stem:
            return bounds["wma_id"]
    return None


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--data", default=os.environ.get("WOW_DATA"), help="WoW client Data directory")
    parser.add_argument("--overlay-dbc", default=os.environ.get("WORLDMAP_OVERLAY_DBC"),
                        help="WorldMapOverlay.dbc to use (default: newest readable client copy)")
    parser.add_argument("--data-dir", default=os.path.join(here, "..", "web", "data"),
                        help="directory holding map_canvas.json, zone_maps.json and zones.json")
    parser.add_argument("--out", default=os.path.join(here, "..", "maps"),
                        help="directory to write the *.webp artwork into")
    parser.add_argument("--dry-run", action="store_true", help="report work without writing files")
    args = parser.parse_args()

    if not args.data:
        parser.error("--data (or WOW_DATA) is required")

    dbc_path = args.overlay_dbc
    if dbc_path:
        with open(dbc_path, "rb") as handle:
            dbc_raw = handle.read()
        print(f"overlays: {dbc_path}")
    else:
        print(f"client:   {args.data}")
    client = Client(args.data)
    if dbc_path is None:
        dbc_raw = client.newest_dbc("DBFilesClient\\WorldMapOverlay.dbc")
        if dbc_raw is None:
            sys.exit("no readable WorldMapOverlay.dbc in the client; pass --overlay-dbc")
        print("overlays: newest readable WorldMapOverlay.dbc from the client MPQs")
    overlays_by_area = parse_overlays(dbc_raw)

    with open(os.path.join(args.data_dir, "map_canvas.json")) as handle:
        canvas = json.load(handle)
    with open(os.path.join(args.data_dir, "zone_maps.json")) as handle:
        zone_maps = json.load(handle)
    with open(os.path.join(args.data_dir, "zones.json")) as handle:
        zones = json.load(handle)

    if not args.dry_run:
        os.makedirs(args.out, exist_ok=True)
    written = shipped = failed = 0
    for file, size in sorted(canvas.items()):
        if size["source"] != "client":
            shipped += 1
            continue
        folder = client.folder_for(file[:-5])
        if folder is None:
            print(f"  {file}: no client WorldMap folder, skipped")
            failed += 1
            continue
        overlays = overlays_by_area.get(wma_id_for(file, zone_maps, zones), [])
        grid, tiles, painted = compose(client, folder, overlays)
        if not tiles:
            print(f"  {file}: base tiles missing, skipped")
            failed += 1
            continue
        save_at(grid, os.path.join(args.out, file), size["width"], size["height"], args.dry_run)
        print(f"  {file}: {painted}/{len(overlays)} overlays -> {size['width']}x{size['height']}")
        written += 1

    print(f"\n{written} written to {args.out}, {shipped} shipped with the dashboard, {failed} failed")
    if failed:
        sys.exit(1)

if __name__ == "__main__":
    main()
