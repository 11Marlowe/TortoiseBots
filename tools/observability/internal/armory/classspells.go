package armory

// Class spellbook membership from the operator's own DBC files.
//
// The client builds a class spellbook out of SkillLineAbility.dbc: every row
// whose skill line sits in the SkillLine "Class Skills" category and whose
// class mask covers the character's class. Nothing else in the stack answers
// "is this a class spell" reliably: the world mirrors of both tables are empty
// (tw_world.skilllineability ships with 0 rows), the rank text in
// spell_template.nameSubtext is missing on many real class abilities (Taunt,
// pet commands, weapon buffs), and spell_template.spellFamilyName is 0 for a
// good part of them. With no DBCDir the index is empty and the frontend keeps
// its name/rank heuristics.

import (
	"fmt"
	"path/filepath"
	"sync"
)

const (
	// skillCategoryClass is SKILL_CATEGORY_CLASS in SkillLineCategory.dbc.
	skillCategoryClass = 7
	// spellAttrPassive is SPELL_ATTR_PASSIVE (SpellDefines.h). Race/class
	// defaults bring passive markers like "Rogue Passive (DND)" along, and
	// those are not spellbook entries.
	spellAttrPassive = 0x40
)

// classFromMaskBit maps a SkillLineAbility class mask bit to the core class id
// (SharedDefines.h: CLASS_WARRIOR..CLASS_DRUID). Bit 5 is the unused hero class
// and bit 9 the unused class 10, so the mask skips straight to Druid at bit 10.
var classFromMaskBit = []uint8{1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 11}

var (
	classSpellMu    sync.Mutex
	classSpellByDir = map[string]map[uint8]map[uint32]bool{}
)

// classSpellsFor returns the spell ids the given class owns per the operator's
// SkillLine/SkillLineAbility DBCs, or nil when DBCDir is unset/unreadable.
func (s *Service) classSpellsFor(classID uint8) map[uint32]bool {
	if s.cfg.DBCDir == "" || classID == 0 {
		return nil
	}
	classSpellMu.Lock()
	defer classSpellMu.Unlock()
	if byClass, cached := classSpellByDir[s.cfg.DBCDir]; cached {
		return byClass[classID]
	}
	byClass, err := readClassSpells(s.cfg.DBCDir)
	if err != nil {
		byClass = nil // cache the miss: every profile would fail the same way
	}
	classSpellByDir[s.cfg.DBCDir] = byClass
	return byClass[classID]
}

// readClassSpells indexes class skill lines and their spells by class id.
func readClassSpells(dir string) (map[uint8]map[uint32]bool, error) {
	lineRecs, _, err := readDBCRecords(filepath.Join(dir, "SkillLine.dbc"))
	if err != nil {
		return nil, err
	}
	// SkillLine.dbc: id, categoryId, ... (category 7 = Class Skills).
	classLines := map[uint32]bool{}
	for _, r := range lineRecs {
		if len(r) > 1 && r[1] == skillCategoryClass {
			classLines[r[0]] = true
		}
	}
	if len(classLines) == 0 {
		return nil, fmt.Errorf("SkillLine.dbc has no class skill lines")
	}

	abilityRecs, _, err := readDBCRecords(filepath.Join(dir, "SkillLineAbility.dbc"))
	if err != nil {
		return nil, err
	}
	// SkillLineAbility.dbc: id, skillId, spellId, raceMask, classMask, ...
	byClass := map[uint8]map[uint32]bool{}
	for _, r := range abilityRecs {
		if len(r) < 5 || !classLines[r[1]] || r[4] == 0 {
			continue
		}
		for bit, classID := range classFromMaskBit {
			if classID == 0 || r[4]&(1<<uint(bit)) == 0 {
				continue
			}
			if byClass[classID] == nil {
				byClass[classID] = map[uint32]bool{}
			}
			byClass[classID][r[2]] = true
		}
	}
	return byClass, nil
}

// startingSpellIds returns the race/class default spells that belong to the
// character's class spellbook. These are the spells Player::LearnDefaultSpells
// adds on creation and on every login as *dependent* spells, which
// Player::_SaveSpells deliberately never persists — so a DB-only view misses
// the entire starting spellbook (Sinister Strike, Heroic Strike, ...).
// playercreateinfo_spell also carries passive markers, weapon/armor skills and
// DND helpers; the class skill line + passive filter drops them.
func (s *Service) startingSpellIds(race, class uint8, classSpells map[uint32]bool) ([]uint32, error) {
	if race == 0 || class == 0 || len(classSpells) == 0 {
		return nil, nil
	}
	q := fmt.Sprintf(`
		SELECT p.Spell, COALESCE(st.attributes, 0)
		FROM %s.playercreateinfo_spell p
		LEFT JOIN %s.spell_template st ON st.entry = p.Spell
		WHERE p.race = ? AND p.class = ?`, s.cfg.WorldDB, s.cfg.WorldDB)
	rows, err := s.db.Query(q, race, class)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var ids []uint32
	for rows.Next() {
		var spell, attrs uint32
		if err := rows.Scan(&spell, &attrs); err != nil {
			return nil, err
		}
		if isStartingClassSpell(spell, attrs, classSpells) {
			ids = append(ids, spell)
		}
	}
	return ids, rows.Err()
}

// isStartingClassSpell decides whether a playercreateinfo_spell row is part of
// the character's class spellbook: a member of a class skill line and not a
// passive marker ("Rogue Passive (DND)", weapon/armor proficiency helpers).
func isStartingClassSpell(spell, attrs uint32, classSpells map[uint32]bool) bool {
	return classSpells[spell] && attrs&spellAttrPassive == 0
}
