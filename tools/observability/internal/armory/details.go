package armory

import (
	"database/sql"
	"fmt"
	"regexp"
	"strconv"
	"strings"
	"sync"
)

// detailSelect lists the tooltip columns of world.item_template appended
// after the base item columns. Order must match scanDetailRow.
const detailSelect = `it.class, it.subclass, it.description, it.bonding,
	it.required_level, it.required_skill, it.allowable_class, it.allowable_race,
	it.max_count, it.stackable, it.container_slots, it.delay, it.ammo_type,
	it.dmg_min1, it.dmg_max1, it.dmg_type1,
	it.dmg_min2, it.dmg_max2, it.dmg_min3, it.dmg_max3,
	it.block, it.armor,
	it.holy_res, it.fire_res, it.nature_res, it.frost_res, it.shadow_res, it.arcane_res,
	it.stat_type1, it.stat_value1, it.stat_type2, it.stat_value2,
	it.stat_type3, it.stat_value3, it.stat_type4, it.stat_value4,
	it.stat_type5, it.stat_value5, it.stat_type6, it.stat_value6,
	it.stat_type7, it.stat_value7, it.stat_type8, it.stat_value8,
	it.stat_type9, it.stat_value9, it.stat_type10, it.stat_value10,
	it.spellid_1, it.spelltrigger_1, it.spellid_2, it.spelltrigger_2,
	it.spellid_3, it.spelltrigger_3, it.spellid_4, it.spelltrigger_4,
	it.spellid_5, it.spelltrigger_5,
	it.max_durability, it.sell_price`

// detailDests returns scan destinations for detailSelect in order.
func detailDests(d *ItemDetail, st, sv *[10]int32, sp *[5]uint32, tr *[5]uint8) []interface{} {
	return []interface{}{
		&d.Class, &d.SubClass, &d.Description, &d.Bonding,
		&d.RequiredLevel, &d.RequiredSkill, &d.AllowableClass, &d.AllowableRace,
		&d.MaxCount, &d.Stackable, &d.ContainerSlots, &d.Delay, &d.AmmoType,
		&d.DmgMin1, &d.DmgMax1, &d.DmgType1,
		&d.DmgMin2, &d.DmgMax2, &d.DmgMin3, &d.DmgMax3,
		&d.Block, &d.Armor,
		&d.ResHoly, &d.ResFire, &d.ResNature, &d.ResFrost, &d.ResShadow, &d.ResArcane,
		&st[0], &sv[0], &st[1], &sv[1], &st[2], &sv[2], &st[3], &sv[3], &st[4], &sv[4],
		&st[5], &sv[5], &st[6], &sv[6], &st[7], &sv[7], &st[8], &sv[8], &st[9], &sv[9],
		&sp[0], &tr[0], &sp[1], &tr[1], &sp[2], &tr[2], &sp[3], &tr[3], &sp[4], &tr[4],
		&d.MaxDurability, &d.SellPrice,
	}
}

// foldDetail compacts sparse stat/spell columns into the JSON slices.
func foldDetail(d *ItemDetail, st, sv [10]int32, sp [5]uint32, tr [5]uint8) {
	for i := range st {
		if sv[i] == 0 {
			continue
		}
		d.StatTypes = append(d.StatTypes, uint32(st[i]))
		d.StatValues = append(d.StatValues, sv[i])
	}
	for i := range sp {
		if sp[i] == 0 {
			continue
		}
		d.SpellIDs = append(d.SpellIDs, sp[i])
		d.SpellTriggers = append(d.SpellTriggers, uint32(tr[i]))
	}
}

type spellDetail struct {
	Entry       uint32
	Name        string
	Description string
	BP          [3]int32
	DS          [3]int32
	Amp         [3]uint32
	DurationIdx uint32
	ProcChance  uint32
}

var (
	spellDetailsMu    sync.RWMutex
	spellDetailsCache = make(map[uint32]*spellDetail)

	reDivS   = regexp.MustCompile(`\$/([0-9]+);s([1-3])`)
	reRefS   = regexp.MustCompile(`\$([0-9]+)s([1-3])`)
	reRefD   = regexp.MustCompile(`\$([0-9]+)d[0-9]?`)
	reS      = regexp.MustCompile(`\$([smq])([1-3])`)
	reO      = regexp.MustCompile(`\$o([1-3])`)
	reD      = regexp.MustCompile(`\$d[0-9]?`)
	reT      = regexp.MustCompile(`\$t([1-3])`)
	reH      = regexp.MustCompile(`\$h`)
	rePlural = regexp.MustCompile(`\$l([^:]+):([^;]+);`)
	reGender = regexp.MustCompile(`\$g([^:]+):([^;]+);`)
)

func (s *Service) getSpellDetails(ids []uint32) map[uint32]*spellDetail {
	out := make(map[uint32]*spellDetail, len(ids))
	var missing []uint32
	seen := map[uint32]bool{}

	spellDetailsMu.RLock()
	for _, id := range ids {
		if id == 0 || seen[id] {
			continue
		}
		seen[id] = true
		if sd, ok := spellDetailsCache[id]; ok {
			out[id] = sd
		} else {
			missing = append(missing, id)
		}
	}
	spellDetailsMu.RUnlock()

	if len(missing) == 0 {
		return out
	}

	var marks []string
	var args []interface{}
	for _, id := range missing {
		marks = append(marks, "?")
		args = append(args, id)
	}

	q := fmt.Sprintf(`SELECT st.entry, COALESCE(st.name, ''), COALESCE(st.description, ''),
		COALESCE(st.effectBasePoints1, 0), COALESCE(st.effectBasePoints2, 0), COALESCE(st.effectBasePoints3, 0),
		COALESCE(st.effectDieSides1, 0), COALESCE(st.effectDieSides2, 0), COALESCE(st.effectDieSides3, 0),
		COALESCE(st.effectAmplitude1, 0), COALESCE(st.effectAmplitude2, 0), COALESCE(st.effectAmplitude3, 0),
		COALESCE(st.durationIndex, 0), COALESCE(st.procChance, 0)
		FROM %s.spell_template st
		WHERE st.entry IN (%s)`, s.cfg.WorldDB, strings.Join(marks, ","))

	rows, err := s.db.Query(q, args...)
	if err != nil {
		return out
	}
	defer rows.Close()

	var newDetails []*spellDetail
	var crossSpellIDs []uint32

	for rows.Next() {
		var sd spellDetail
		if err := rows.Scan(
			&sd.Entry, &sd.Name, &sd.Description,
			&sd.BP[0], &sd.BP[1], &sd.BP[2],
			&sd.DS[0], &sd.DS[1], &sd.DS[2],
			&sd.Amp[0], &sd.Amp[1], &sd.Amp[2],
			&sd.DurationIdx, &sd.ProcChance,
		); err != nil {
			continue
		}
		newDetails = append(newDetails, &sd)
		out[sd.Entry] = &sd

		if strings.Contains(sd.Description, "$") {
			for _, match := range reRefS.FindAllStringSubmatch(sd.Description, -1) {
				if len(match) >= 2 {
					if refID, err := strconv.ParseUint(match[1], 10, 32); err == nil {
						crossSpellIDs = append(crossSpellIDs, uint32(refID))
					}
				}
			}
			for _, match := range reRefD.FindAllStringSubmatch(sd.Description, -1) {
				if len(match) >= 2 {
					if refID, err := strconv.ParseUint(match[1], 10, 32); err == nil {
						crossSpellIDs = append(crossSpellIDs, uint32(refID))
					}
				}
			}
		}
	}

	spellDetailsMu.Lock()
	for _, sd := range newDetails {
		spellDetailsCache[sd.Entry] = sd
	}
	spellDetailsMu.Unlock()

	if len(crossSpellIDs) > 0 {
		var toFetch []uint32
		spellDetailsMu.RLock()
		for _, refID := range crossSpellIDs {
			if _, ok := spellDetailsCache[refID]; !ok && !seen[refID] {
				toFetch = append(toFetch, refID)
				seen[refID] = true
			}
		}
		spellDetailsMu.RUnlock()

		if len(toFetch) > 0 {
			refMap := s.getSpellDetails(toFetch)
			for k, v := range refMap {
				out[k] = v
			}
		}
	}

	return out
}

func fallbackSpellDesc(sd *spellDetail) string {
	n := sd.Name
	val := sd.BP[0] + 1
	if strings.Contains(n, "Increased Defense") {
		return fmt.Sprintf("Increased Defense +%d.", val)
	}
	if strings.Contains(n, "Increased Critical") {
		return fmt.Sprintf("Improves your chance to get a critical strike by %d%%.", val)
	}
	if strings.Contains(n, "Increased Hit") {
		return fmt.Sprintf("Improves your chance to hit by %d%%.", val)
	}
	if strings.Contains(n, "Increased Dodge") {
		return fmt.Sprintf("Increases your chance to dodge an attack by %d%%.", val)
	}
	if strings.Contains(n, "Increased Parry") {
		return fmt.Sprintf("Increases your chance to parry an attack by %d%%.", val)
	}
	if strings.Contains(n, "Increased Blocking") {
		return fmt.Sprintf("Increases your chance to block attacks with your shield by %d%%.", val)
	}
	if strings.Contains(n, "Block Value") {
		return fmt.Sprintf("Increases the block value of your shield by %d.", val)
	}
	if strings.Contains(n, "Attack Power") {
		return fmt.Sprintf("+%d Attack Power.", val)
	}
	if strings.Contains(n, "Increased Mana Regen") {
		return fmt.Sprintf("Restores %d mana per 5 sec.", val)
	}
	if strings.Contains(n, "Increase Spell Dam") {
		return fmt.Sprintf("Increases damage and healing done by magical spells and effects by up to %d.", val)
	}
	return n
}

func formatSpellDesc(sd *spellDetail, durMap map[uint32]int32, allSpells map[uint32]*spellDetail) string {
	if sd == nil {
		return ""
	}
	desc := strings.TrimSpace(sd.Description)
	if desc == "" {
		return fallbackSpellDesc(sd)
	}

	getSVal := func(idx int, targetSD *spellDetail) (string, int32) {
		if idx < 1 || idx > 3 {
			return "0", 0
		}
		b := targetSD.BP[idx-1]
		d := targetSD.DS[idx-1]
		min := b + 1
		max := b + d
		if d > 1 && min != max {
			return fmt.Sprintf("%d to %d", min, max), min
		}
		return fmt.Sprintf("%d", min), min
	}

	lastVal := int32(1)

	// 1. Division: $/60;s1
	desc = reDivS.ReplaceAllStringFunc(desc, func(m string) string {
		sub := reDivS.FindStringSubmatch(m)
		if len(sub) == 3 {
			divN, _ := strconv.Atoi(sub[1])
			idx, _ := strconv.Atoi(sub[2])
			if divN > 0 && idx >= 1 && idx <= 3 {
				val := (sd.BP[idx-1] + 1) / int32(divN)
				lastVal = val
				return fmt.Sprintf("%d", val)
			}
		}
		return m
	})

	// 2. Cross-spell: $8083s1
	desc = reRefS.ReplaceAllStringFunc(desc, func(m string) string {
		sub := reRefS.FindStringSubmatch(m)
		if len(sub) == 3 {
			refID, _ := strconv.ParseUint(sub[1], 10, 32)
			idx, _ := strconv.Atoi(sub[2])
			if refSD, ok := allSpells[uint32(refID)]; ok {
				sStr, n := getSVal(idx, refSD)
				lastVal = n
				return sStr
			}
		}
		return m
	})

	// Cross-spell duration: $12345d
	desc = reRefD.ReplaceAllStringFunc(desc, func(m string) string {
		sub := reRefD.FindStringSubmatch(m)
		if len(sub) >= 2 {
			refID, _ := strconv.ParseUint(sub[1], 10, 32)
			if refSD, ok := allSpells[uint32(refID)]; ok {
				durMs := durMap[refSD.DurationIdx]
				if durMs >= 3600000 {
					return fmt.Sprintf("%d hour", durMs/3600000)
				} else if durMs >= 60000 {
					return fmt.Sprintf("%d min", durMs/60000)
				} else if durMs > 0 {
					return fmt.Sprintf("%d sec", durMs/1000)
				}
			}
		}
		return m
	})

	// 3. $s1..3, $m1..3, $q1..3
	desc = reS.ReplaceAllStringFunc(desc, func(m string) string {
		sub := reS.FindStringSubmatch(m)
		if len(sub) == 3 {
			idx, _ := strconv.Atoi(sub[2])
			sStr, n := getSVal(idx, sd)
			lastVal = n
			return sStr
		}
		return m
	})

	// 4. $o1..3 (over-time total)
	desc = reO.ReplaceAllStringFunc(desc, func(m string) string {
		sub := reO.FindStringSubmatch(m)
		if len(sub) == 2 {
			idx, _ := strconv.Atoi(sub[1])
			if idx >= 1 && idx <= 3 {
				durMs := durMap[sd.DurationIdx]
				amp := sd.Amp[idx-1]
				ticks := int32(1)
				if durMs > 0 && amp > 0 {
					ticks = durMs / int32(amp)
				}
				val := (sd.BP[idx-1] + 1) * ticks
				lastVal = val
				return fmt.Sprintf("%d", val)
			}
		}
		return m
	})

	// 5. $d (duration)
	desc = reD.ReplaceAllStringFunc(desc, func(m string) string {
		durMs := durMap[sd.DurationIdx]
		if durMs >= 3600000 {
			return fmt.Sprintf("%d hour", durMs/3600000)
		} else if durMs >= 60000 {
			return fmt.Sprintf("%d min", durMs/60000)
		} else if durMs > 0 {
			return fmt.Sprintf("%d sec", durMs/1000)
		}
		return ""
	})

	// 6. $t1..3
	desc = reT.ReplaceAllStringFunc(desc, func(m string) string {
		sub := reT.FindStringSubmatch(m)
		if len(sub) == 2 {
			idx, _ := strconv.Atoi(sub[1])
			if idx >= 1 && idx <= 3 && sd.Amp[idx-1] > 0 {
				return fmt.Sprintf("%d", sd.Amp[idx-1]/1000)
			}
		}
		return "1"
	})

	// 7. $h (proc chance)
	desc = reH.ReplaceAllString(desc, fmt.Sprintf("%d%%", sd.ProcChance))

	// 8. $lsingular:plural;
	desc = rePlural.ReplaceAllStringFunc(desc, func(m string) string {
		sub := rePlural.FindStringSubmatch(m)
		if len(sub) == 3 {
			if lastVal == 1 {
				return sub[1]
			}
			return sub[2]
		}
		return m
	})

	// 9. $gmasc:fem;
	desc = reGender.ReplaceAllStringFunc(desc, func(m string) string {
		sub := reGender.FindStringSubmatch(m)
		if len(sub) == 3 {
			return sub[1]
		}
		return m
	})

	return strings.TrimSpace(desc)
}

// resolveSpellNames fills human-readable proc names and formatted descriptions
// from the operator's own spell_template in one query.
func (s *Service) resolveSpellNames(d *ItemDetail) {
	if len(d.SpellIDs) == 0 {
		return
	}
	d.SpellNames = make([]string, len(d.SpellIDs))
	d.SpellDescs = make([]string, len(d.SpellIDs))

	sds := s.getSpellDetails(d.SpellIDs)
	var durMap map[uint32]int32
	if s.cfg.DBCDir != "" {
		durMap, _ = loadDBCSpellDuration(s.cfg.DBCDir)
	}
	if durMap == nil {
		durMap = map[uint32]int32{}
	}

	for i, sid := range d.SpellIDs {
		if sd, ok := sds[sid]; ok && sd != nil {
			d.SpellNames[i] = sd.Name
			d.SpellDescs[i] = formatSpellDesc(sd, durMap, sds)
		}
	}
}
func (s *Service) spellName(spellID uint32) string {
	return s.spellNames([]uint32{spellID})[spellID]
}

type SpellInfo struct {
	Name        string
	Icon        string
	Description string
}

// spellInfos resolves spell display names and icon names in one batch query.
func (s *Service) spellInfos(ids []uint32) map[uint32]SpellInfo {
	out := map[uint32]SpellInfo{}
	seen := map[uint32]bool{}
	var args []interface{}
	var marks []string
	for _, id := range ids {
		if id == 0 || seen[id] {
			continue
		}
		seen[id] = true
		marks = append(marks, "?")
		args = append(args, id)
	}
	if len(marks) == 0 {
		return out
	}
	q := fmt.Sprintf(`SELECT st.entry, COALESCE(st.name, ''), COALESCE(si.Name, ''), COALESCE(st.description, ''), COALESCE(st.spellIconId, 0)
		FROM %s.spell_template st
		LEFT JOIN %s.spellicon si ON si.ID = st.spellIconId
		WHERE st.entry IN (%s)`, s.cfg.WorldDB, s.cfg.WorldDB, strings.Join(marks, ","))
	rows, err := s.db.Query(q, args...)
	if err != nil {
		return out
	}
	defer rows.Close()
	for rows.Next() {
		var entry, iconID uint32
		var name, icon, desc string
		if err := rows.Scan(&entry, &name, &icon, &desc, &iconID); err != nil {
			continue
		}
		if icon == "" && iconID != 0 {
			icon = s.spellIconByID(iconID)
		}
		out[entry] = SpellInfo{Name: name, Icon: icon, Description: desc}
	}
	return out
}

// spellNames resolves many spell display names in one query.
func (s *Service) spellNames(ids []uint32) map[uint32]string {
	infos := s.spellInfos(ids)
	out := map[uint32]string{}
	for k, v := range infos {
		out[k] = v.Name
	}
	return out
}

// firstRanks collects rank spell ids of class talents for batch naming and descriptions.
func firstRanks(talents []dbcTalent, tabByID map[uint32]dbcTalentTab) []uint32 {
	ids := make([]uint32, 0, len(talents)*2)
	for _, t := range talents {
		if _, ok := tabByID[t.tabID]; !ok || len(t.ranks) == 0 {
			continue
		}
		for _, r := range t.ranks {
			if r != 0 {
				ids = append(ids, r)
			}
		}
	}
	return ids
}

func scanDetailTail(rows *sql.Rows, d *ItemDetail, s *Service) error {
	var st, sv [10]int32
	var sp [5]uint32
	var tr [5]uint8
	if err := rows.Scan(detailDests(d, &st, &sv, &sp, &tr)...); err != nil {
		return err
	}
	foldDetail(d, st, sv, sp, tr)
	if s != nil {
		s.resolveSpellNames(d)
	}
	return nil
}
