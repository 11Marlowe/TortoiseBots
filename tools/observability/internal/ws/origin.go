package ws

import (
	"net/http"
	"net/url"
	"strings"
)

// SameOrigin reports whether a WebSocket upgrade may proceed: a browser
// always sends Origin, and it must match the host the dashboard was reached
// on, so a page on another site cannot open an authenticated stream with the
// visitor's cookies. Requests without an Origin header (non-browser clients)
// are allowed; an unparseable origin is rejected.
func SameOrigin(r *http.Request) bool {
	origin := r.Header.Get("Origin")
	if origin == "" {
		return true
	}
	u, err := url.Parse(origin)
	if err != nil {
		return false
	}
	return strings.EqualFold(u.Host, r.Host)
}
