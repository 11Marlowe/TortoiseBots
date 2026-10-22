package udp

import (
	"net"
	"strconv"
	"strings"
	"sync"
	"testing"
	"time"

	"tortoise-observability/internal/ringbuf"
	"tortoise-observability/internal/state"
)

// countingHub is read from the test goroutine while readLoop broadcasts, so it locks.
type countingHub struct {
	mu     sync.Mutex
	counts map[string]int
}

func (h *countingHub) Broadcast(event string, payload interface{}) {
	h.mu.Lock()
	defer h.mu.Unlock()
	h.counts[event]++
}

func (h *countingHub) count(event string) int {
	h.mu.Lock()
	defer h.mu.Unlock()
	return h.counts[event]
}

// serverInfoDatagram is processed with no side effects beyond one "server_info" broadcast.
const serverInfoDatagram = `{"v":12,"type":"SERVER_INFO"}`

func newForwardListener(t *testing.T, targets []string) (*Listener, *countingHub) {
	t.Helper()
	store := state.New(state.Config{RosterTTL: 30 * time.Second}, ringbuf.New(16))
	hub := &countingHub{counts: map[string]int{}}
	l := NewListener("127.0.0.1", 0, store, nil, nil, hub)
	if err := l.SetForwards(targets); err != nil {
		t.Fatalf("SetForwards: %v", err)
	}
	if err := l.Start(); err != nil {
		t.Fatalf("Start: %v", err)
	}
	t.Cleanup(l.Stop)
	return l, hub
}

func openTarget(t *testing.T) *net.UDPConn {
	t.Helper()
	c, err := net.ListenUDP("udp", &net.UDPAddr{IP: net.IPv4(127, 0, 0, 1), Port: 0})
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { _ = c.Close() })
	return c
}

func listenAddr(l *Listener) *net.UDPAddr {
	return l.conn.LocalAddr().(*net.UDPAddr)
}

func sendDatagram(t *testing.T, to *net.UDPAddr, data []byte) {
	t.Helper()
	c, err := net.DialUDP("udp", nil, to)
	if err != nil {
		t.Fatal(err)
	}
	defer c.Close()
	if _, err := c.Write(data); err != nil {
		t.Fatal(err)
	}
}

func readOne(t *testing.T, c *net.UDPConn) []byte {
	t.Helper()
	if err := c.SetReadDeadline(time.Now().Add(2 * time.Second)); err != nil {
		t.Fatal(err)
	}
	buf := make([]byte, 65535)
	n, _, err := c.ReadFromUDP(buf)
	if err != nil {
		t.Fatalf("target received nothing: %v", err)
	}
	return buf[:n]
}

func waitFor(t *testing.T, what string, cond func() bool) {
	t.Helper()
	deadline := time.Now().Add(2 * time.Second)
	for !cond() {
		if time.Now().After(deadline) {
			t.Fatalf("timed out waiting for %s", what)
		}
		time.Sleep(5 * time.Millisecond)
	}
}

func TestForwardSendsIdenticalBytesToEveryTarget(t *testing.T) {
	a := openTarget(t)
	b := openTarget(t)
	l, hub := newForwardListener(t, []string{a.LocalAddr().String(), b.LocalAddr().String()})

	sendDatagram(t, listenAddr(l), []byte(serverInfoDatagram))

	for _, c := range []*net.UDPConn{a, b} {
		if got := string(readOne(t, c)); got != serverInfoDatagram {
			t.Fatalf("forwarded bytes differ: %q", got)
		}
	}
	waitFor(t, "the normal processing of the datagram", func() bool { return hub.count("server_info") == 1 })
}

func TestDeadForwardTargetDoesNotBreakProcessing(t *testing.T) {
	dead := openTarget(t)
	deadAddr := dead.LocalAddr().String()
	_ = dead.Close()
	live := openTarget(t)
	l, hub := newForwardListener(t, []string{deadAddr, live.LocalAddr().String()})

	const n = 20
	start := time.Now()
	for i := 0; i < n; i++ {
		sendDatagram(t, listenAddr(l), []byte(serverInfoDatagram))
	}
	waitFor(t, "all datagrams processed", func() bool { return hub.count("server_info") == n })
	if elapsed := time.Since(start); elapsed > time.Second {
		t.Fatalf("20 datagrams with a dead target took %v", elapsed)
	}
	for i := 0; i < n; i++ {
		if got := string(readOne(t, live)); got != serverInfoDatagram {
			t.Fatalf("live target copy %d: %q", i, got)
		}
	}
}

func TestSetForwardsRejectsInvalidAddress(t *testing.T) {
	for _, bad := range []string{"", "no-port-here", "127.0.0.1", "127.0.0.1:0", "127.0.0.1:99999"} {
		store := state.New(state.Config{RosterTTL: 30 * time.Second}, ringbuf.New(16))
		l := NewListener("127.0.0.1", 0, store, nil, nil, &countingHub{counts: map[string]int{}})
		if err := l.SetForwards([]string{bad}); err == nil {
			t.Fatalf("forward target %q was accepted", bad)
		}
	}
}

func TestSetForwardsRefusesItsOwnListenAddress(t *testing.T) {
	probe := openTarget(t)
	port := probe.LocalAddr().(*net.UDPAddr).Port
	_ = probe.Close()

	store := state.New(state.Config{RosterTTL: 30 * time.Second}, ringbuf.New(16))
	l := NewListener("127.0.0.1", port, store, nil, nil, &countingHub{counts: map[string]int{}})
	if err := l.SetForwards([]string{"127.0.0.1:" + strconv.Itoa(port)}); err == nil {
		t.Fatal("a forward target equal to the listen address was accepted")
	}
}

func TestValidateForwards(t *testing.T) {
	cases := []struct {
		name    string
		listen  string
		targets []string
		refused bool
	}{
		{"no targets", "127.0.0.1:9195", nil, false},
		{"other port", "127.0.0.1:9195", []string{"127.0.0.1:9196"}, false},
		{"other ip same port", "127.0.0.1:9195", []string{"127.0.0.2:9195"}, false},
		{"remote host same port", "0.0.0.0:9195", []string{"10.0.0.5:9195"}, false},
		{"same address", "127.0.0.1:9195", []string{"127.0.0.1:9195"}, true},
		{"one of several", "127.0.0.1:9195", []string{"127.0.0.1:9196", "127.0.0.1:9195"}, true},
		{"wildcard listen, loopback target", "0.0.0.0:9195", []string{"127.0.0.1:9195"}, true},
		{"invalid target", "127.0.0.1:9195", []string{"nope"}, true},
		{"port zero target", "127.0.0.1:9195", []string{"127.0.0.1:0"}, true},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			err := validateForwards(tc.listen, tc.targets)
			if (err != nil) != tc.refused {
				t.Fatalf("validateForwards(%q, %v) = %v, refused want %v", tc.listen, tc.targets, err, tc.refused)
			}
		})
	}
}

// freePort returns a UDP port that was free a moment ago on 127.0.0.1.
func freePort(t *testing.T) int {
	t.Helper()
	probe := openTarget(t)
	port := probe.LocalAddr().(*net.UDPAddr).Port
	_ = probe.Close()
	return port
}

// loopListener starts a listener on host:port whose forward sockets are opened
// WITHOUT validation, to reproduce a loop that validateForwards would refuse
// (or that an unvalidated spelling would slip past).
func loopListener(t *testing.T, host string, port int, target string) (*Listener, *countingHub) {
	t.Helper()
	store := state.New(state.Config{RosterTTL: 30 * time.Second}, ringbuf.New(16))
	hub := &countingHub{counts: map[string]int{}}
	l := NewListener(host, port, store, nil, nil, hub)
	targets, err := openForwards([]string{target})
	if err != nil {
		t.Fatalf("openForwards: %v", err)
	}
	l.forwards = targets
	if err := l.Start(); err != nil {
		t.Fatalf("Start: %v", err)
	}
	t.Cleanup(l.Stop)
	return l, hub
}

// A datagram that arrives from one of the daemon's own forward sockets is a
// loop. It must be neither forwarded again nor processed, so one datagram from
// outside is processed exactly once.
func TestOwnForwardSocketDatagramIsNotLooped(t *testing.T) {
	cases := []struct {
		name         string
		host, target string
	}{
		{"specific listen, target is the listen address", "127.0.0.1", "127.0.0.1:%d"},
		{"wildcard listen, target is a loopback address", "0.0.0.0", "127.0.0.1:%d"},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			port := freePort(t)
			l, hub := loopListener(t, tc.host, port, strings.Replace(tc.target, "%d", strconv.Itoa(port), 1))
			sendDatagram(t, &net.UDPAddr{IP: net.IPv4(127, 0, 0, 1), Port: port}, []byte(serverInfoDatagram))
			waitFor(t, "the first processing", func() bool { return hub.count("server_info") >= 1 })
			time.Sleep(300 * time.Millisecond)
			if got := hub.count("server_info"); got != 1 {
				t.Fatalf("one datagram was processed %d times in 300 ms", got)
			}
			_ = l
		})
	}
}

func TestIsOwnForwardMatchesLocalAddrOnly(t *testing.T) {
	target := openTarget(t)
	targets, err := openForwards([]string{target.LocalAddr().String()})
	if err != nil {
		t.Fatal(err)
	}
	defer closeTargets(targets)
	l := &Listener{forwards: targets}
	own := targets[0].conn.LocalAddr().(*net.UDPAddr)
	if !l.isOwnForward(own) {
		t.Fatalf("a forward socket's LocalAddr %v was not recognised", own)
	}
	other := &net.UDPAddr{IP: own.IP, Port: own.Port + 1}
	if l.isOwnForward(other) {
		t.Fatal("a different port from the same host was taken for a forward socket")
	}
	if l.isOwnForward(&net.UDPAddr{IP: net.IPv4(10, 1, 2, 3), Port: own.Port}) {
		t.Fatal("a different host with the same port was taken for a forward socket")
	}
}

func TestValidateForwardsLoopShapes(t *testing.T) {
	// A fake interface list keeps the wildcard case independent of the host.
	old := interfaceAddrs
	interfaceAddrs = func() ([]net.Addr, error) {
		return []net.Addr{
			&net.IPNet{IP: net.IPv4(192, 168, 7, 20), Mask: net.CIDRMask(24, 32)},
			&net.IPNet{IP: net.ParseIP("fd00::20"), Mask: net.CIDRMask(64, 128)},
		}, nil
	}
	defer func() { interfaceAddrs = old }()

	cases := []struct {
		name    string
		listen  string
		target  string
		refused bool
	}{
		{"wildcard listen, own LAN address same port", "0.0.0.0:9195", "192.168.7.20:9195", true},
		{"wildcard listen, own IPv6 address same port", "[::]:9195", "[fd00::20]:9195", true},
		{"wildcard listen, own LAN address other port", "0.0.0.0:9195", "192.168.7.20:9196", false},
		{"wildcard listen, foreign address same port", "0.0.0.0:9195", "192.168.7.99:9195", false},
		{"specific listen, 0.0.0.0 target", "127.0.0.1:9195", "0.0.0.0:9195", true},
		{"wildcard listen, 0.0.0.0 target", "0.0.0.0:9195", "0.0.0.0:9195", true},
		{"unspecified target on another port", "127.0.0.1:9195", "0.0.0.0:9196", true},
		{"IPv6 unspecified target", "127.0.0.1:9195", "[::]:9196", true},
		{"specific listen, exact ip:port", "192.168.7.20:9195", "192.168.7.20:9195", true},
		{"specific listen, own LAN address other IP", "192.168.7.20:9195", "192.168.7.21:9195", false},
		{"empty listen host is a wildcard", ":9195", "192.168.7.20:9195", true},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			err := validateForwards(tc.listen, []string{tc.target})
			if (err != nil) != tc.refused {
				t.Fatalf("validateForwards(%q, %q) = %v, refused want %v", tc.listen, tc.target, err, tc.refused)
			}
		})
	}
}
