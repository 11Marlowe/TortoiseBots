package armory

import (
	"encoding/binary"
	"os"
	"path/filepath"
	"testing"
)

// writeDBC lays out a minimal DBC file: header, fixed-size records, string block.
func writeDBC(t *testing.T, path string, fields uint32, records [][]uint32) {
	t.Helper()
	recSize := fields * 4
	raw := make([]byte, 20, 20+uint32(len(records))*recSize)
	copy(raw[0:4], "WDBC")
	binary.LittleEndian.PutUint32(raw[4:8], uint32(len(records)))
	binary.LittleEndian.PutUint32(raw[8:12], fields)
	binary.LittleEndian.PutUint32(raw[12:16], recSize)
	binary.LittleEndian.PutUint32(raw[16:20], 0) // empty string block
	for _, rec := range records {
		buf := make([]byte, recSize)
		for i := uint32(0); i < fields; i++ {
			var v uint32
			if int(i) < len(rec) {
				v = rec[i]
			}
			binary.LittleEndian.PutUint32(buf[i*4:i*4+4], v)
		}
		raw = append(raw, buf...)
	}
	if err := os.WriteFile(path, raw, 0o600); err != nil {
		t.Fatalf("write %s: %v", path, err)
	}
}

// padSkillLine widens a SkillLine row to the 22 fields the real file carries.
func padSkillLine(id, category uint32) []uint32 {
	rec := make([]uint32, 22)
	rec[0], rec[1] = id, category
	return rec
}

// classMaskBit covers the whole real mapping: class mask bit -> core class id,
// including the unused hero-class gap (bit 5) that must not shift Druid onto
// Shaman's slot.
func TestReadClassSpellsMapsClassMaskBits(t *testing.T) {
	dir := t.TempDir()
	writeDBC(t, filepath.Join(dir, "SkillLine.dbc"), 22, [][]uint32{
		padSkillLine(6, skillCategoryClass),  // Frost
		padSkillLine(38, skillCategoryClass), // Combat
		padSkillLine(164, 11),                // Blacksmithing (profession)
	})
	writeDBC(t, filepath.Join(dir, "SkillLineAbility.dbc"), 15, [][]uint32{
		{1, 6, 116, 0, 0x80},   // Frostbolt -> Mage
		{2, 38, 1752, 0, 0x08}, // Sinister Strike -> Rogue
		{3, 38, 2098, 0, 0x400 | 0x01}, // shared: Druid + Warrior
		{4, 164, 2018, 0, 0x400},       // Blacksmithing: not a class line
		{5, 38, 6603, 0, 0},            // no class ownership
	})

	byClass, err := readClassSpells(dir)
	if err != nil {
		t.Fatalf("readClassSpells: %v", err)
	}
	if !byClass[8][116] {
		t.Errorf("spell 116 not indexed for mage (class 8): %v", byClass[8])
	}
	if !byClass[4][1752] {
		t.Errorf("spell 1752 not indexed for rogue (class 4): %v", byClass[4])
	}
	if byClass[4][2018] {
		t.Error("profession skill line leaked into the class spellbook")
	}
	for _, class := range []uint8{4, 8, 1, 11} {
		if byClass[class][6603] {
			t.Errorf("classMask 0 row indexed for class %d", class)
		}
	}
	if !byClass[11][2098] || !byClass[1][2098] {
		t.Errorf("multi-class mask not expanded to every class: druid=%v warrior=%v", byClass[11], byClass[1])
	}
	if byClass[7][2098] {
		t.Error("shaman (class 7) indexed from a mask that does not cover it")
	}
}

func TestReadClassSpellsWithoutClassLines(t *testing.T) {
	dir := t.TempDir()
	writeDBC(t, filepath.Join(dir, "SkillLine.dbc"), 22, [][]uint32{padSkillLine(164, 11)})
	writeDBC(t, filepath.Join(dir, "SkillLineAbility.dbc"), 15, [][]uint32{{1, 164, 2018, 0, 0x400}})

	if _, err := readClassSpells(dir); err == nil {
		t.Fatal("expected an error when SkillLine.dbc carries no class skill lines")
	}
}

// The DBC dir is optional: without it the index stays empty, which is what the
// frontend reads as "no class data, use the name rules".
func TestClassSpellsForWithoutDBCDir(t *testing.T) {
	s := &Service{}
	if got := s.classSpellsFor(4); got != nil {
		t.Fatalf("classSpellsFor without DBCDir = %v, want nil", got)
	}
}

// Race/class defaults bring passive markers along; real starting abilities stay.
func TestIsStartingClassSpell(t *testing.T) {
	classSpells := map[uint32]bool{1752: true, 2098: true} // Sinister Strike, Eviscerate
	cases := []struct {
		name  string
		spell uint32
		attrs uint32
		want  bool
	}{
		{"starting ability", 1752, 0x50010, true},
		{"passive marker", 2098, 0x500D0, false},
		{"spell outside the class lines", 6603, 0x10, false},
	}
	for _, c := range cases {
		if got := isStartingClassSpell(c.spell, c.attrs, classSpells); got != c.want {
			t.Errorf("%s (spell %d, attrs %#x) = %v, want %v", c.name, c.spell, c.attrs, got, c.want)
		}
	}
}
