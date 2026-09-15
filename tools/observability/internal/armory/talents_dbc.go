package armory

// Talent layout from the operator's own DBC files (the same files mangosd
// reads via DataDir). No DBC data is shipped: paths resolve at runtime from
// Config.DBCDir, and everything degrades to the world talent/talenttab
// mirror tables when the dir is unset or the files are absent.

import (
	"encoding/binary"
	"fmt"
	"os"
	"path/filepath"
	"sync"
)

type dbcTalent struct {
	id    uint32
	tabID uint32
	row   uint32
	col   uint32
	ranks []uint32
}

type dbcTalentTab struct {
	id        uint32
	name      string
	classMask uint32
	page      uint32
}

type dbcCache struct {
	talents []dbcTalent
	tabs    []dbcTalentTab
	err     error
}

var (
	dbcMu    sync.Mutex
	dbcByDir = map[string]*dbcCache{}
)

func loadDBCTalents(dir string) ([]dbcTalent, []dbcTalentTab, error) {
	dbcMu.Lock()
	defer dbcMu.Unlock()
	if c, ok := dbcByDir[dir]; ok {
		return c.talents, c.tabs, c.err
	}
	c := &dbcCache{}
	c.talents, c.tabs, c.err = readTalentDBC(dir)
	dbcByDir[dir] = c
	return c.talents, c.tabs, c.err
}

func readDBCRecords(path string) (records [][]uint32, strings []byte, err error) {
	raw, err := os.ReadFile(path)
	if err != nil {
		return nil, nil, err
	}
	if len(raw) < 20 {
		return nil, nil, fmt.Errorf("dbc %s too short", path)
	}
	nRec := binary.LittleEndian.Uint32(raw[4:8])
	nFields := binary.LittleEndian.Uint32(raw[8:12])
	recSize := binary.LittleEndian.Uint32(raw[12:16])
	if uint32(len(raw)) < 20+nRec*recSize {
		return nil, nil, fmt.Errorf("dbc %s truncated", path)
	}
	// Talent.dbc: 21 int32 fields (id, tab, row, col, 5 ranks, padding, deps...).
	// TalentTab.dbc: 15 int32 fields (id, name offsets, masks, page).
	if nFields < 15 {
		return nil, nil, fmt.Errorf("dbc %s unexpected field count %d", path, nFields)
	}
	records = make([][]uint32, 0, nRec)
	for i := uint32(0); i < nRec; i++ {
		off := 20 + i*recSize
		row := make([]uint32, nFields)
		for f := uint32(0); f < nFields; f++ {
			row[f] = binary.LittleEndian.Uint32(raw[off+f*4 : off+f*4+4])
		}
		records = append(records, row)
	}
	return records, raw[20+nRec*recSize:], nil
}

func dbcString(block []byte, off uint32) string {
	if int(off) >= len(block) {
		return ""
	}
	end := int(off)
	for end < len(block) && block[end] != 0 {
		end++
	}
	return string(block[off:end])
}

func readTalentDBC(dir string) ([]dbcTalent, []dbcTalentTab, error) {
	tRecs, _, err := readDBCRecords(filepath.Join(dir, "Talent.dbc"))
	if err != nil {
		return nil, nil, err
	}
	tabRecs, tabStr, err := readDBCRecords(filepath.Join(dir, "TalentTab.dbc"))
	if err != nil {
		return nil, nil, err
	}
	tabs := make([]dbcTalentTab, 0, len(tabRecs))
	for _, r := range tabRecs {
		// Layout per core DBCStructure.h TalentTabEntry: id, name[8
		// offsets], nameFlags, spellIcon, raceMask, classMask, tabpage,
		// internalname. Field 1 is the enUS name offset.
		if len(r) < 14 {
			continue
		}
		tabs = append(tabs, dbcTalentTab{
			id:        r[0],
			name:      dbcString(tabStr, r[1]),
			classMask: r[12],
			page:      r[13],
		})
	}
	talents := make([]dbcTalent, 0, len(tRecs))
	for _, r := range tRecs {
		// Layout per TalentEntry: id, tab, row, col, rankID[5], padding,
		// dependsOn, ..., dependsOnRank, ..., dependsOnSpell.
		if len(r) < 9 {
			continue
		}
		var ranks []uint32
		for k := 4; k < 9 && k < len(r); k++ {
			if r[k] != 0 {
				ranks = append(ranks, r[k])
			} else {
				break
			}
		}
		if len(ranks) == 0 {
			continue
		}
		talents = append(talents, dbcTalent{
			id: r[0], tabID: r[1], row: r[2], col: r[3], ranks: ranks,
		})
	}
	return talents, tabs, nil
}

// talentsFromDBC builds class talent trees from the operator's DBC files.
// Only ranks the bot actually knows (character_spell) count as allocated.
func (s *Service) talentsFromDBC(guid uint32, classID uint32, known map[uint32]bool) ([]TalentTree, error) {
	talents, tabs, err := loadDBCTalents(s.cfg.DBCDir)
	if err != nil {
		return nil, err
	}
	mask := uint32(1) << (classID - 1)
	tabByID := map[uint32]dbcTalentTab{}
	for _, t := range tabs {
		if t.classMask&mask != 0 {
			tabByID[t.id] = t
		}
	}
	if len(tabByID) == 0 {
		return []TalentTree{}, nil
	}
	trees := []TalentTree{}
	byTab := map[uint32]int{}
	for _, t := range talents {
		tab, ok := tabByID[t.tabID]
		if !ok {
			continue
		}
		rank, active := uint32(0), uint32(0)
		for i, rs := range t.ranks {
			if known[rs] {
				rank = uint32(i + 1)
				active = rs
			}
		}
		idx, ok := byTab[t.tabID]
		if !ok {
			name := tab.name
			if name == "" {
				name = fmt.Sprintf("Tree %d", t.tabID)
			}
			trees = append(trees, TalentTree{TabID: t.tabID, Name: name, Page: tab.page, Talents: []TalentNode{}})
			idx = len(trees) - 1
			byTab[t.tabID] = idx
		}
		trees[idx].Talents = append(trees[idx].Talents, TalentNode{
			TalentID: t.id, Row: t.row, Col: t.col,
			Rank: rank, MaxRank: uint32(len(t.ranks)), SpellID: active, Name: s.spellName(t.ranks[0]),
		})
		if rank > 0 {
			trees[idx].Points += rank
		}
	}
	// Order trees by DBC page so Arms/Fury/Protection stay stable.
	for i := range trees {
		for j := i + 1; j < len(trees); j++ {
			if trees[j].Page < trees[i].Page {
				trees[i], trees[j] = trees[j], trees[i]
			}
		}
	}
	if trees == nil {
		trees = []TalentTree{}
	}
	return trees, nil
}
