package ws

import (
	"net/http"
	"testing"
)

func originRequest(host, origin string) *http.Request {
	r, err := http.NewRequest(http.MethodGet, "http://"+host+"/api/v1/stream", nil)
	if err != nil {
		panic(err)
	}
	if origin != "" {
		r.Header.Set("Origin", origin)
	}
	r.Host = host
	return r
}

func TestSameOrigin(t *testing.T) {
	cases := []struct {
		name  string
		host  string
		origin string
		want  bool
	}{
		{"no origin header (non-browser client)", "127.0.0.1:8095", "", true},
		{"same host and port", "127.0.0.1:8095", "http://127.0.0.1:8095", true},
		{"same host and port over https", "127.0.0.1:8095", "https://127.0.0.1:8095", true},
		{"host match is case-insensitive", "Dashboard.example:8095", "http://dashboard.EXAMPLE:8095", true},
		{"default port elided by the browser", "127.0.0.1", "http://127.0.0.1", true},
		{"different host", "127.0.0.1:8095", "http://evil.example:8095", false},
		{"different port", "127.0.0.1:8095", "http://127.0.0.1:1111", false},
		{"host-only origin against port-bearing host", "127.0.0.1:8095", "http://127.0.0.1", false},
		{"null origin (sandboxed frame)", "127.0.0.1:8095", "null", false},
		{"origin with control character", "127.0.0.1:8095", "http://evil.example:8095/\x00", false},
		{"unparseable relative origin", "127.0.0.1:8095", "not a url", false},
	}

	for _, c := range cases {
		t.Run(c.name, func(t *testing.T) {
			if got := SameOrigin(originRequest(c.host, c.origin)); got != c.want {
				t.Errorf("SameOrigin(host %q, origin %q) = %v, want %v", c.host, c.origin, got, c.want)
			}
		})
	}
}
