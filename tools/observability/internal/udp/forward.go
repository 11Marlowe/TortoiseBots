package udp

import (
	"fmt"
	"log"
	"net"
	"time"
)

// forwardWriteTimeout bounds one forwarded write, so a slow or dead target
// cannot hold up the read loop.
const forwardWriteTimeout = 50 * time.Millisecond

// maxForwardTargets bounds the fan-out. Forwarding runs on the read loop, so
// with every target blocking, one datagram waits at most
// maxForwardTargets * forwardWriteTimeout (200 ms).
const maxForwardTargets = 4

// forwardLogInterval is the least time between two error lines for one target.
const forwardLogInterval = 30 * time.Second

// forwardTarget is one extra destination for every received datagram. Only the
// read loop touches lastLog, so it needs no lock.
type forwardTarget struct {
	addr    string
	conn    *net.UDPConn
	local   *net.UDPAddr // the connected socket's own address: the source of every copy it sends
	lastLog time.Time
}

// interfaceAddrs lists the host's interface addresses; a variable so tests can
// pin the list.
var interfaceAddrs = net.InterfaceAddrs

// SetForwards sets the extra UDP addresses that every received datagram is
// copied to. Call it before Start; an empty list turns forwarding off.
func (l *Listener) SetForwards(addrs []string) error {
	if err := validateForwards(l.addr, addrs); err != nil {
		return err
	}

	targets, err := openForwards(addrs)
	if err != nil {
		return err
	}
	l.forwards = targets
	return nil
}

// openForwards resolves each address once and opens a connected socket to it.
// Targets are not re-resolved while the daemon runs.
func openForwards(addrs []string) ([]*forwardTarget, error) {
	var targets []*forwardTarget
	for _, a := range addrs {
		raddr, err := net.ResolveUDPAddr("udp", a)
		if err != nil {
			closeTargets(targets)
			return nil, fmt.Errorf("forward target %s: %w", a, err)
		}
		conn, err := net.DialUDP("udp", nil, raddr)
		if err != nil {
			closeTargets(targets)
			return nil, fmt.Errorf("forward target %s: %w", a, err)
		}
		local, _ := conn.LocalAddr().(*net.UDPAddr)
		targets = append(targets, &forwardTarget{addr: a, conn: conn, local: local})
	}
	return targets, nil
}

// isOwnForward reports whether src is the address of one of this daemon's own
// forward sockets. A datagram from there has been sent by this daemon, so it
// reached its own listener through a loop, however the target was spelled.
func (l *Listener) isOwnForward(src *net.UDPAddr) bool {
	if src == nil {
		return false
	}
	for _, t := range l.forwards {
		if t.local != nil && t.local.Port == src.Port && t.local.IP.Equal(src.IP) {
			return true
		}
	}
	return false
}

// validateForwards refuses a target that would feed the daemon's output back
// into the socket it reads from: an unspecified address (0.0.0.0 or ::, which
// the kernel sends to the local host), the exact listen ip:port, and, when
// listening on the wildcard, the same port on any loopback or interface address
// of this host. A loop through a second daemon (A to B, B to A) cannot be
// detected here; the README says not to build one.
func validateForwards(listen string, targets []string) error {
	listenAddr, err := net.ResolveUDPAddr("udp", listen)
	if err != nil {
		return fmt.Errorf("listen address %s: %w", listen, err)
	}
	if len(targets) > maxForwardTargets {
		return fmt.Errorf("%d forward targets given: at most %d are allowed, since each one can hold up the read loop for %v", len(targets), maxForwardTargets, forwardWriteTimeout)
	}
	wildcard := len(listenAddr.IP) == 0 || listenAddr.IP.IsUnspecified()
	var local []net.IP
	for _, target := range targets {
		addr, err := net.ResolveUDPAddr("udp", target)
		if err != nil {
			return fmt.Errorf("forward target %s: %w", target, err)
		}
		if addr.Port == 0 {
			return fmt.Errorf("forward target %s: port must be 1-65535", target)
		}
		if len(addr.IP) == 0 || addr.IP.IsUnspecified() {
			return fmt.Errorf("forward target %s: an unspecified address (0.0.0.0 or ::) reaches this host itself; name the real host", target)
		}
		if addr.Port != listenAddr.Port {
			continue
		}
		loop := addr.IP.Equal(listenAddr.IP)
		if wildcard && !loop {
			if local == nil {
				local = localIPs()
			}
			loop = addr.IP.IsLoopback()
			for _, ip := range local {
				loop = loop || ip.Equal(addr.IP)
			}
		}
		if loop {
			return fmt.Errorf("forward target %s is an address of this host the listener %s reads: telemetry would loop back into the daemon", target, listen)
		}
	}
	return nil
}

// localIPs returns the addresses of this host's interfaces.
func localIPs() []net.IP {
	addrs, err := interfaceAddrs()
	if err != nil {
		return nil
	}
	var ips []net.IP
	for _, a := range addrs {
		switch v := a.(type) {
		case *net.IPNet:
			ips = append(ips, v.IP)
		case *net.IPAddr:
			ips = append(ips, v.IP)
		}
	}
	return ips
}

// forwardDatagram copies one received datagram to every forward target. A failed
// write is logged at most once per forwardLogInterval per target and never
// reaches the read loop, so processing always follows.
func (l *Listener) forwardDatagram(data []byte) {
	for _, t := range l.forwards {
		_ = t.conn.SetWriteDeadline(time.Now().Add(forwardWriteTimeout))
		if _, err := t.conn.Write(data); err != nil {
			if now := time.Now(); now.Sub(t.lastLog) >= forwardLogInterval {
				t.lastLog = now
				log.Printf("[UDP] Forward to %s failed: %v (repeats are silent for %v)", t.addr, err, forwardLogInterval)
			}
		}
	}
}

func (l *Listener) closeForwards() {
	closeTargets(l.forwards)
}

func closeTargets(targets []*forwardTarget) {
	for _, t := range targets {
		_ = t.conn.Close()
	}
}
