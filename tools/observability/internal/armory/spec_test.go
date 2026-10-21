package armory

import "testing"

// TestSpecFromTabPoints pins how the bot list names a spec: the tree with the
// most talent points, "Hybrid" on a tie, and no spec at all (empty) for a
// character without talent points — a placeholder name there made a character
// with no talents read as a spec ("Unspecified") in the dashboard.
func TestSpecFromTabPoints(t *testing.T) {
	tabs := map[uint32]string{261: "Elemental", 262: "Restoration", 263: "Enhancement"}
	cases := []struct {
		name string
		pts  map[uint32]uint32
		want string
	}{
		{"no talent points at all", map[uint32]uint32{}, ""},
		{"a single tree", map[uint32]uint32{263: 36}, "Enhancement"},
		{"the tree with the most points wins", map[uint32]uint32{261: 15, 263: 36}, "Enhancement"},
		{"equal points are a hybrid", map[uint32]uint32{261: 21, 263: 21}, "Hybrid"},
		{"a zero-point tree never wins", map[uint32]uint32{261: 0, 262: 7}, "Restoration"},
		{"a tab the DBC does not name is a hybrid", map[uint32]uint32{999: 10}, "Hybrid"},
		{"a tab with an empty name is a hybrid", map[uint32]uint32{999: 10, 263: 0}, "Hybrid"},
	}
	for _, c := range cases {
		if got := specFromTabPoints(c.pts, tabs); got != c.want {
			t.Errorf("%s: specFromTabPoints(%v) = %q, want %q", c.name, c.pts, got, c.want)
		}
	}
}

// TestSpecFromTabPointsIsStable checks the tie flag cannot leak between
// entries: map iteration order must not decide between a name and "Hybrid".
func TestSpecFromTabPointsIsStable(t *testing.T) {
	pts := map[uint32]uint32{261: 10, 262: 10, 263: 4}
	for i := range 50 {
		if got := specFromTabPoints(pts, map[uint32]string{261: "Elemental", 262: "Restoration", 263: "Enhancement"}); got != "Hybrid" {
			t.Fatalf("run %d: got %q, want Hybrid", i, got)
		}
	}
}
