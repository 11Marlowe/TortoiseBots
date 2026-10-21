#!/usr/bin/env python3
"""Validate PremadeSpec talent links against Tortoise Talent/TalentTab DBC.

Mirrors TortoiseBots TalentSpec::GetTalents/SortTalents/ReadTalents/CheckTalents:
  - class talents filtered by TalentTab ClassMask, sorted by (tabPage,row,col)
    with tabPage() = 1 when TalentTabID==41 else tabpage field
  - link parsed exactly like ReadTalents(link): leading '-' bump tab, per-talent
    chars consumed in sorted order, '-' separators bump tab, truncation = 0
  - checks: rank in [0,maxRank], row gating, DependsOn with core zero-based
    DependsOnRank (need allocated > DependsOnRank), DependsOnSpell when the
    required spell is itself a talent rank, point budget, monotonic checkpoints
Usage:
  python3 tools/talents/validate_presets.py --dbc <dbc-dir> --conf <conf> [--rate 1]
"""
import argparse
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

MAX_TALENT_RANK = 5

# Shaman tank-build markers (issue #1125): TortoiseBots reads the tank build
# from these two talents alone (runtime/ShamanTankDetectionPolicy.h,
# kShamanTankTalent*), so no shaman premade other than 7.3 may allocate them -
# a DPS link that took one would make every bot on it read as a tank. Matched
# by rank-1 spell id, so a DBC reshuffle cannot silently defeat the rule.
SHAMAN_TANK_KEY_TALENT_SPELLS = {51381, 45951}  # Totemic Alignment, Spirit Armor
SHAMAN_TANK_SPEC = 3
SHAMAN_CLASS = 7

# The C++ half of the tank identity below the first key talent (issue #1125):
# the levelling-layout read in AiFactory is level-gated by this constant, and
# the LFT role mask keys off the key talents themselves. Keep them honest.
LEVELING_POLICY_PATH = 'runtime/ShamanTankLevelingPolicy.h'


def policy_constant(module_root, path, name):
    text = (Path(module_root) / path).read_text(encoding='utf-8', errors='replace')
    m = re.search(rf'\b{name}\s*=\s*(\d+)', text)
    if not m:
        raise ValueError(f'{name} not found in {path}')
    return int(m.group(1))


def load_dbc(path):
    data = Path(path).read_bytes()
    magic, records, fields, recsize, strsize = struct.unpack('<4sIIII', data[:20])
    return [struct.unpack('<' + 'i' * fields, data[20 + i * recsize:20 + (i + 1) * recsize])
            for i in range(records)]


def tab_page(tab_id, tabpage_field):
    return 1 if tab_id == 41 else tabpage_field


def parse_link(link, sorted_talents):
    """Replicate TalentSpec::ReadTalents(link). Returns dict talentId->rank."""
    alloc = {t[0]: 0 for t in sorted_talents}
    pos = 0
    tab = 0
    n = len(link)

    def at(p):
        return link[p] if 0 <= p < n else ''

    if at(pos) == '-':
        pos += 1
        tab += 1
    if at(pos) == '-':
        pos += 1
        tab += 1
    for t in sorted_talents:
        tid, ttab, row, col = t[0], t[1], t[2], t[3]
        # need tabpage of this talent
        if t[5] != tab:  # t[5] precomputed page
            pass
        if t[5] == tab:
            ch = at(pos)
            if ch == ' ' or ch == '#':
                break
            if ch == '':
                break
            if ch not in '012345':
                raise ValueError(f'bad char {ch!r} in link {link!r}')
            alloc[tid] = int(ch)
            pos += 1
            if pos <= n:
                if at(pos) == '-':
                    pos += 1
                    tab += 1
            if pos <= n:
                if at(pos) == '-':
                    pos += 1
                    tab += 1
        if pos > n - 1:
            break
    return alloc


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dbc', required=True)
    ap.add_argument('--conf', required=True)
    ap.add_argument('--rate', type=float, default=1.0)
    ap.add_argument('--module-root', default='.',
                    help='module checkout root holding runtime/ShamanTank*Policy.h')
    args = ap.parse_args()

    dbc = Path(args.dbc)
    talents = load_dbc(dbc / 'Talent.dbc')
    tabs = load_dbc(dbc / 'TalentTab.dbc')
    tab_page_of = {}
    tab_class = {}
    for t in tabs:
        tab_page_of[t[0]] = tab_page(t[0], t[13])
        tab_class[t[0]] = t[12] & 0xFFFFFFFF

    # Enrich talents: (id, tab, row, col, ranks..., DependsOn, DependsOnRank, DependsOnSpell, page)
    enriched = []
    for t in talents:
        page = tab_page_of.get(t[1], 99)
        enriched.append((t[0], t[1], t[2], t[3], t[4], t[5], t[6], t[7], t[8],
                          t[13], t[16], t[20] & 0xFFFFFFFF, page, t))
    talent_by_id = {e[0]: e for e in enriched}
    spell_to_talent = {}
    for e in enriched:
        for r in range(MAX_TALENT_RANK):
            sid = e[4 + r] & 0xFFFFFFFF if e[4 + r] else 0
            if sid:
                spell_to_talent.setdefault(sid, (e[0], r))

    conf_text = Path(args.conf).read_text(encoding='utf-8', errors='replace')
    links = re.findall(r'^AiPlayerbot\.PremadeSpecLink\.(\d+)\.(\d+)\.(\d+)\s*=\s*(\S+)', conf_text, re.M)
    if not links:
        print('no PremadeSpecLink rows found')
        return 1

    groups = defaultdict(list)
    for cls, spec, level, link in links:
        groups[(int(cls), int(spec))].append((int(level), link))

    failures = []
    total = 0
    # Shaman premade allocations per level, for the tank-identity checks below:
    # {level: {spec: {talentId: rank}}}.
    shaman_allocs = defaultdict(dict)
    for (cls, spec), rows in sorted(groups.items()):
        mask = 1 << (cls - 1)
        class_t = [e for e in enriched if tab_class.get(e[1], 0) & mask]
        class_t.sort(key=lambda e: (e[12], e[2], e[3]))
        if not class_t:
            failures.append(f'class {cls} spec {spec}: no talents for mask {mask:#x}')
            continue
        # attach page for parse_link tuples: (id, tab, row, col, ..., page at idx5)
        parse_list = [(e[0], e[1], e[2], e[3], 0, e[12]) for e in class_t]
        # parse_list entries: idx0=id, idx5=page
        rows.sort()
        prev = None
        tank_marker_seen = False
        for level, link in rows:
            total += 1
            try:
                alloc = parse_link(link, parse_list)
            except ValueError as ex:
                failures.append(f'{cls}.{spec}.{level} {link}: {ex}')
                continue
            # rank bounds
            for e in class_t:
                maxrank = sum(1 for r in range(MAX_TALENT_RANK) if e[4 + r])
                if alloc[e[0]] < 0 or alloc[e[0]] > maxrank:
                    failures.append(f'{cls}.{spec}.{level}: talent {e[0]} rank {alloc[e[0]]} max {maxrank}')
            # row gating per page
            for page in sorted(set(e[12] for e in class_t)):
                tl = [e for e in class_t if e[12] == page]
                pts = 0
                for e in tl:
                    if alloc[e[0]] > 0 and e[2] * 5 > pts:
                        failures.append(
                            f'{cls}.{spec}.{level}: talent {e[0]} row {e[2]} needs {e[2]*5} lower pts have {pts}')
                    pts += alloc[e[0]]
            # DependsOn (core zero-based: need allocated > DependsOnRank)
            for e in class_t:
                if alloc[e[0]] > 0 and e[9]:
                    have = alloc.get(e[9], 0)
                    if have <= e[10]:
                        failures.append(
                            f'{cls}.{spec}.{level}: talent {e[0]} needs talent {e[9]} rank {e[10]+1} have {have}')
            # DependsOnSpell (talent-spell only)
            for e in class_t:
                if alloc[e[0]] > 0 and e[11]:
                    if e[11] in spell_to_talent:
                        dep_id, dep_idx = spell_to_talent[e[11]]
                        if alloc.get(dep_id, 0) <= dep_idx:
                            failures.append(
                                f'{cls}.{spec}.{level}: talent {e[0]} needs spell {e[11]} from talent {dep_id} rank {dep_idx+1}')
            # Tank-build markers stay in the 7.3 spec (issue #1125): the
            # detection read trusts these two talents alone. The other way
            # round, the tank link must keep one of them at some checkpoint,
            # or every bot on it silently falls back to the DPS kit.
            if cls == SHAMAN_CLASS and spec != SHAMAN_TANK_SPEC:
                for e in class_t:
                    if alloc[e[0]] > 0 and any(
                            (e[4 + r] & 0xFFFFFFFF) in SHAMAN_TANK_KEY_TALENT_SPELLS
                            for r in range(MAX_TALENT_RANK)):
                        failures.append(
                            f'{cls}.{spec}.{level}: key tank talent {e[0]} allocated outside the '
                            f'{SHAMAN_CLASS}.{SHAMAN_TANK_SPEC} tank build')
            elif cls == SHAMAN_CLASS and any(
                    alloc[e[0]] > 0 and any(
                        (e[4 + r] & 0xFFFFFFFF) in SHAMAN_TANK_KEY_TALENT_SPELLS
                        for r in range(MAX_TALENT_RANK))
                    for e in class_t):
                tank_marker_seen = True
            budget = int((level - 9) * args.rate) if level >= 10 else 0
            spent = sum(alloc.values())
            if spent > budget:
                failures.append(f'{cls}.{spec}.{level}: spends {spent} > budget {budget}')
            if prev is not None:
                for tid, r in alloc.items():
                    if r < prev.get(tid, 0):
                        failures.append(f'{cls}.{spec}.{level}: talent {tid} decreases {prev.get(tid,0)}->{r}')
                        break
            prev = alloc
            if cls == SHAMAN_CLASS:
                shaman_allocs[level][spec] = alloc
        if cls == SHAMAN_CLASS and spec == SHAMAN_TANK_SPEC and not tank_marker_seen:
            failures.append(
                f'{cls}.{spec}: the tank build allocates no key tank talent '
                f'({sorted(SHAMAN_TANK_KEY_TALENT_SPELLS)}) at any checkpoint, so the '
                f'tank-spec read (issue #1125) would never match it')

    # Issue #1125, the levels below the first key talent: the tank identity is
    # read from the spent levelling layout there (AiFactory ->
    # runtime/ShamanTankLevelingPolicy.h), and that read is level-gated by
    # kShamanTankFirstTalentLevel and trusted only where the layout singles the
    # tank entry out. Pin both against this conf and the DBC.
    tank_by_level = {level: by_spec[SHAMAN_TANK_SPEC]
                     for level, by_spec in shaman_allocs.items() if SHAMAN_TANK_SPEC in by_spec}
    key_tank_talent_ids = set()
    for spell_id in sorted(SHAMAN_TANK_KEY_TALENT_SPELLS):
        found = spell_to_talent.get(spell_id)
        if not found:
            failures.append(f'Talent.dbc: key tank talent spell {spell_id} is not a talent rank')
        else:
            key_tank_talent_ids.add(found[0])

    try:
        first_talent_level = policy_constant(args.module_root, LEVELING_POLICY_PATH,
                                             'kShamanTankFirstTalentLevel')
    except (OSError, ValueError) as ex:
        failures.append(f'{LEVELING_POLICY_PATH}: {ex}')
        first_talent_level = None

    key_talent_levels = sorted(level for level, alloc in tank_by_level.items()
                               if any(alloc.get(tid, 0) > 0 for tid in key_tank_talent_ids))
    if first_talent_level is not None:
        if not key_talent_levels:
            failures.append(f'{SHAMAN_CLASS}.{SHAMAN_TANK_SPEC}: no checkpoint allocates a key tank talent')
        elif key_talent_levels[0] != first_talent_level:
            failures.append(
                f'{LEVELING_POLICY_PATH}: kShamanTankFirstTalentLevel is {first_talent_level} but the '
                f'{SHAMAN_CLASS}.{SHAMAN_TANK_SPEC} link first allocates a key tank talent at '
                f'{key_talent_levels[0]}')

        # The read is trusted from the first level the tank layout is the only
        # match up to the key-talent level. It has to cover that whole window:
        # a level inside it where another premade matches the same layout would
        # silently drop the identity the read exists for. Below the window (the
        # 10-14 links are identical) the read correctly falls back to DPS.
        unique_below = sorted(level for level, alloc in tank_by_level.items()
                              if level < first_talent_level
                              and all(alloc != other for spec, other in shaman_allocs[level].items()
                                      if spec != SHAMAN_TANK_SPEC))
        if not unique_below:
            failures.append(
                f'{SHAMAN_CLASS}.{SHAMAN_TANK_SPEC}: the levelling link is never distinguishable from '
                f'another shaman premade below level {first_talent_level}, so the layout read would '
                f'never identify the build')
        else:
            missing = [level for level in sorted(tank_by_level)
                       if unique_below[0] <= level < first_talent_level and level not in unique_below]
            if missing:
                failures.append(
                    f'{SHAMAN_CLASS}.{SHAMAN_TANK_SPEC}: the levelling link is not distinguishable at '
                    f'level {missing[0]}, inside the {unique_below[0]}-{first_talent_level - 1} window '
                    f'the layout read is trusted for')
            print(f'shaman tank identity: key talents at {key_talent_levels}, layout read window '
                  f'{unique_below[0]}-{first_talent_level - 1}')

    print(f'checked {total} links, {len(failures)} failures')
    for f in failures:
        print('FAIL', f)
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
