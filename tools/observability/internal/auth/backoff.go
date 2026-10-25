package auth

import (
	"sync"
	"time"
)

// LoginBackoff is a simple in-memory per-IP throttle for the login endpoint:
// after maxFailures consecutive failed logins an IP is blocked for
// blockDuration. A failure older than blockDuration stops counting, a
// successful login forgets the IP, and the map is pruned so a spray of
// one-off addresses cannot grow it without bound. Nothing here is persisted.
type LoginBackoff struct {
	mu            sync.Mutex
	failures      map[string]*loginFailures
	maxFailures   int
	blockDuration time.Duration
	maxEntries    int
	now           func() time.Time // swappable for tests
}

type loginFailures struct {
	count        int
	blockedUntil time.Time
	last         time.Time
}

const (
	loginMaxFailures   = 5
	loginBlockDuration = 5 * time.Minute
	loginMaxEntries    = 10000
)

func NewLoginBackoff() *LoginBackoff {
	return &LoginBackoff{
		failures:      map[string]*loginFailures{},
		maxFailures:   loginMaxFailures,
		blockDuration: loginBlockDuration,
		maxEntries:    loginMaxEntries,
		now:           time.Now,
	}
}

// Allow reports whether a login attempt from ip may proceed.
func (b *LoginBackoff) Allow(ip string) bool {
	b.mu.Lock()
	defer b.mu.Unlock()
	f, ok := b.failures[ip]
	if !ok {
		return true
	}
	return b.now().After(f.blockedUntil)
}

// Fail records a failed login from ip; the maxFailures-th failure in a row
// blocks the IP for blockDuration. When the map is full, entries older than
// blockDuration are dropped (their lockout has expired and the count resets
// on the next use); if it is still full — an address flood — new addresses
// are not tracked until room frees up, so the scan stays bounded instead of
// running on every attempt.
func (b *LoginBackoff) Fail(ip string) {
	now := b.now()
	b.mu.Lock()
	defer b.mu.Unlock()
	f, ok := b.failures[ip]
	if !ok {
		if len(b.failures) >= b.maxEntries {
			b.pruneLocked(now)
			if len(b.failures) >= b.maxEntries {
				return
			}
		}
		f = &loginFailures{}
		b.failures[ip] = f
	}
	if now.Sub(f.last) > b.blockDuration {
		f.count = 0
		f.blockedUntil = time.Time{}
	}
	f.last = now
	f.count++
	if f.count >= b.maxFailures {
		f.blockedUntil = now.Add(b.blockDuration)
		f.count = 0
	}
}

// Reset forgets the IP's failures (after a successful login).
func (b *LoginBackoff) Reset(ip string) {
	b.mu.Lock()
	defer b.mu.Unlock()
	delete(b.failures, ip)
}

// pruneLocked drops entries whose last failure is older than the block
// window: such an entry is dead weight (the lockout has expired and a
// further failure starts counting from zero again).
func (b *LoginBackoff) pruneLocked(now time.Time) {
	for ip, f := range b.failures {
		if now.Sub(f.last) > b.blockDuration {
			delete(b.failures, ip)
		}
	}
}
