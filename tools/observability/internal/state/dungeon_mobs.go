package state

import (
	"sort"
	"sync"
	"time"

	"tortoise-observability/internal/model"
)

// DUNGEON_MOBS (issue #1101): the mobs, member ids and targets of every live
// dungeon run, reported by the module once per snapshot cycle. The state
// lives beside the roster in its own mutex: a 2 s datagram never waits on the
// roster lock, and Dungeons() copies the fresh entries before taking it (no
// nested locks).

const (
	// maxDungeonMobsPerRun and maxDungeonMobsMembers bound what one datagram
	// can make the daemon hold (the module sends at most 48 and 40).
	maxDungeonMobsPerRun  = 64
	maxDungeonMobsMembers = 40
	// maxDungeonMobsRuns bounds the whole map. The module sends at most 32
	// runs per cycle and rotates the rest (up to 96 live runs behind three
	// cycles); keep headroom above that so a rotation is never evicted.
	maxDungeonMobsRuns = 128
	// DungeonMobsStaleAfter is how long a run's mob entry stays valid: four
	// snapshot cycles. With the rotation above, a run may be refreshed only
	// every third cycle (~6 s), so four cycles leave one cycle of slack. A
	// run that leaves the instance stops being refreshed and its mobs vanish
	// a few seconds later; the module's "runs":[] marker clears the rest at
	// once.
	DungeonMobsStaleAfter = 8 * time.Second
)

// dungeonMobsEntry is one run's newest payload plus when it arrived.
type dungeonMobsEntry struct {
	seq uint64
	at  time.Time
	run model.DungeonMobsRun
}

// dungeonMobsState is the DUNGEON_MOBS side of the store. Keyed by the same
// run key as the roster (DungeonKey), so the join needs no matching pass.
type dungeonMobsState struct {
	mu      sync.Mutex
	session uint64
	// seq is the newest datagram sequence applied in this session: a part
	// older than it is a reordered datagram and ignored; parts of one cycle
	// share the seq and are all applied.
	seq  uint64
	runs map[string]dungeonMobsEntry
}

// ApplyDungeonMobs folds one DUNGEON_MOBS datagram into the store. Any
// session change starts from an empty map (the roster does the same: a new
// game-server process is a new session even if its clock is behind), and a
// datagram whose seq is older than the newest applied one is dropped (UDP
// reorder). An empty "runs" list with the newest seq ends every run.
func (s *Store) ApplyDungeonMobs(p *model.DungeonMobsPayload) {
	if p == nil {
		return
	}
	st := &s.dungeonMobs
	st.mu.Lock()
	defer st.mu.Unlock()

	if p.Session != st.session {
		st.session = p.Session
		st.seq = 0
		st.runs = make(map[string]dungeonMobsEntry)
	}
	if p.Seq < st.seq {
		return // a reordered datagram from an older cycle
	}
	if st.runs == nil {
		st.runs = make(map[string]dungeonMobsEntry)
	}

	now := s.now()
	if len(p.Runs) == 0 {
		// The module's end marker: the last run is gone. Only the newest seq
		// may clear, so a late part of an earlier cycle cannot wipe the map.
		st.seq = p.Seq
		st.runs = make(map[string]dungeonMobsEntry)
		return
	}
	st.seq = p.Seq

	for _, r := range p.Runs {
		key := DungeonKey(r.Map, r.Instance)
		st.runs[key] = dungeonMobsEntry{seq: p.Seq, at: now, run: trimDungeonMobsRun(r)}
	}
	st.pruneLocked(now)
}

// trimDungeonMobsRun caps one run to what the daemon keeps. The module already
// sends less; this is the belt to its braces (a hostile or buggy sender must
// not grow the store).
func trimDungeonMobsRun(r model.DungeonMobsRun) model.DungeonMobsRun {
	if len(r.Members) > maxDungeonMobsMembers {
		r.Members = r.Members[:maxDungeonMobsMembers]
	}
	if r.Mobs != nil {
		mobs := *r.Mobs
		if len(mobs) > maxDungeonMobsPerRun {
			mobs = mobs[:maxDungeonMobsPerRun]
		}
		r.Mobs = &mobs
	}
	return r
}

// pruneLocked drops entries past the stale limit and, only if a sender
// ignored every cap, the oldest runs past maxDungeonMobsRuns. The lock is held.
func (st *dungeonMobsState) pruneLocked(now time.Time) {
	for key, e := range st.runs {
		if now.Sub(e.at) > DungeonMobsStaleAfter {
			delete(st.runs, key)
		}
	}
	if len(st.runs) <= maxDungeonMobsRuns {
		return
	}
	keys := make([]string, 0, len(st.runs))
	for key := range st.runs {
		keys = append(keys, key)
	}
	sort.Slice(keys, func(i, j int) bool {
		a, b := st.runs[keys[i]], st.runs[keys[j]]
		if a.seq != b.seq {
			return a.seq > b.seq
		}
		return keys[i] < keys[j]
	})
	for _, key := range keys[maxDungeonMobsRuns:] {
		delete(st.runs, key)
	}
}

// freshDungeonMobs copies the entries still inside the stale limit. Called
// before Dungeons() takes the roster lock, so the two locks are never nested.
func (s *Store) freshDungeonMobs() map[string]dungeonMobsEntry {
	st := &s.dungeonMobs
	st.mu.Lock()
	defer st.mu.Unlock()

	now := s.now()
	st.pruneLocked(now)
	out := make(map[string]dungeonMobsEntry, len(st.runs))
	for key, e := range st.runs {
		out[key] = e
	}
	return out
}

// joinDungeonMobs adds a fresh DUNGEON_MOBS entry to one run view: the
// members' ids and targets by guid, and the mob list (a reported-empty list
// stays a non-nil empty slice, so the page knows the layers have data).
func joinDungeonMobs(run *model.DungeonRun, e dungeonMobsEntry) {
	byGUID := make(map[uint32]model.DungeonMobsMember, len(e.run.Members))
	for _, m := range e.run.Members {
		byGUID[m.GUID] = m
	}
	for i := range run.Members {
		if m, ok := byGUID[run.Members[i].GUID]; ok {
			run.Members[i].ID = m.ID
			run.Members[i].Target = m.Target
		}
	}
	if e.run.Mobs == nil {
		return
	}
	mobs := make([]model.CommanderMob, len(*e.run.Mobs))
	copy(mobs, *e.run.Mobs)
	mapID := e.run.MobsMap
	run.Mobs = &mobs
	run.MobsMap = &mapID
	run.MobsTotal = e.run.MobsTotal
}
