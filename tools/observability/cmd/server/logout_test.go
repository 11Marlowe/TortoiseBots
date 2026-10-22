package main

import (
	"net/http"
	"net/http/httptest"
	"os"
	"regexp"
	"strings"
	"testing"
	"time"

	"tortoise-observability/internal/auth"
)

// sessionCookieCleared reports whether any Set-Cookie header on the response
// clears the session cookie (empty value, already expired).
func sessionCookieCleared(resp *http.Response) bool {
	for _, c := range resp.Cookies() {
		expired := c.MaxAge < 0 || (!c.Expires.IsZero() && c.Expires.Before(time.Now()))
		if c.Name == "tbm_session" && c.Value == "" && expired {
			return true
		}
	}
	return false
}

func newLogoutTestServer(t *testing.T) *httptest.Server {
	t.Helper()
	svc, err := auth.NewService(auth.Config{SecretKey: "logout-test-secret"})
	if err != nil {
		t.Fatalf("auth.NewService: %v", err)
	}
	mux := http.NewServeMux()
	mux.HandleFunc("/api/v1/auth/logout", logoutHandler(svc))
	srv := httptest.NewServer(mux)
	t.Cleanup(srv.Close)
	return srv
}

// noRedirect keeps the client from following the 302 so the test sees the raw response.
var noRedirect = &http.Client{
	CheckRedirect: func(*http.Request, []*http.Request) error { return http.ErrUseLastResponse },
}

func TestLogoutRejectsNonPOST(t *testing.T) {
	srv := newLogoutTestServer(t)
	for _, method := range []string{http.MethodGet, http.MethodHead} {
		req, err := http.NewRequest(method, srv.URL+"/api/v1/auth/logout", nil)
		if err != nil {
			t.Fatal(err)
		}
		resp, err := noRedirect.Do(req)
		if err != nil {
			t.Fatalf("%s: %v", method, err)
		}
		resp.Body.Close()
		if resp.StatusCode != http.StatusMethodNotAllowed {
			t.Errorf("%s: status = %d, want 405", method, resp.StatusCode)
		}
		if got := resp.Header.Get("Allow"); got != http.MethodPost {
			t.Errorf("%s: Allow = %q, want %q", method, got, http.MethodPost)
		}
		if sessionCookieCleared(resp) {
			t.Errorf("%s: response clears the session cookie; a non-POST must not log out", method)
		}
	}
}

func TestLogoutPOSTClearsCookieAndReturnsNoContent(t *testing.T) {
	srv := newLogoutTestServer(t)
	resp := postLogout(t, srv, srv.URL) // a browser's fetch POST always sends Origin
	if resp.StatusCode != http.StatusNoContent {
		t.Errorf("POST: status = %d, want 204", resp.StatusCode)
	}
	if loc := resp.Header.Get("Location"); loc != "" {
		t.Errorf("POST: Location = %q, want no redirect from the server", loc)
	}
	if !sessionCookieCleared(resp) {
		t.Errorf("POST: response does not clear the session cookie; Set-Cookie = %v", resp.Header.Values("Set-Cookie"))
	}
}

// Logout is stricter than the WebSocket stream: a POST with no Origin header is
// refused too, so a browser that omits Origin cannot be used to force a logout.
func TestLogoutRejectsPOSTWithoutOrigin(t *testing.T) {
	srv := newLogoutTestServer(t)
	resp := postLogout(t, srv, "")
	if resp.StatusCode != http.StatusForbidden {
		t.Errorf("POST without Origin: status = %d, want 403", resp.StatusCode)
	}
	if len(resp.Header.Values("Set-Cookie")) != 0 || sessionCookieCleared(resp) {
		t.Errorf("POST without Origin: response touches the session cookie; Set-Cookie = %v", resp.Header.Values("Set-Cookie"))
	}
}

// postLogout posts to the logout route; origin "" sends no Origin header, anything else is sent as the Origin header.
func postLogout(t *testing.T, srv *httptest.Server, origin string) *http.Response {
	t.Helper()
	req, err := http.NewRequest(http.MethodPost, srv.URL+"/api/v1/auth/logout", nil)
	if err != nil {
		t.Fatal(err)
	}
	if origin != "" {
		req.Header.Set("Origin", origin)
	}
	resp, err := noRedirect.Do(req)
	if err != nil {
		t.Fatal(err)
	}
	resp.Body.Close()
	return resp
}

// A cross-site HTML form POST is a top-level navigation, so the browser applies
// its Set-Cookie: POST-only is not enough. The Origin must match the host.
func TestLogoutRejectsCrossOriginPOST(t *testing.T) {
	srv := newLogoutTestServer(t)
	cases := []struct{ name, origin string }{
		{"foreign site", "http://evil.example"},
		{"foreign site, same port", "http://evil.example:" + strings.TrimPrefix(srv.URL, "http://127.0.0.1:")},
		{"same host, other port", "http://127.0.0.1:1"},
		{"sandboxed frame sends Origin: null", "null"},
		{"unparseable origin", "not a url"},
	}
	for _, c := range cases {
		t.Run(c.name, func(t *testing.T) {
			resp := postLogout(t, srv, c.origin)
			if resp.StatusCode != http.StatusForbidden {
				t.Errorf("Origin %q: status = %d, want 403", c.origin, resp.StatusCode)
			}
			if len(resp.Header.Values("Set-Cookie")) != 0 || sessionCookieCleared(resp) {
				t.Errorf("Origin %q: response touches the session cookie; Set-Cookie = %v", c.origin, resp.Header.Values("Set-Cookie"))
			}
		})
	}
}

func TestLogoutSameOriginPOSTClearsCookie(t *testing.T) {
	srv := newLogoutTestServer(t)
	resp := postLogout(t, srv, srv.URL)
	if resp.StatusCode != http.StatusNoContent {
		t.Errorf("same-origin POST: status = %d, want 204", resp.StatusCode)
	}
	if !sessionCookieCleared(resp) {
		t.Errorf("same-origin POST: session cookie not cleared; Set-Cookie = %v", resp.Header.Values("Set-Cookie"))
	}
}

// The test reads app.js itself: Go's test cache does not track files that a
// node process reads, so a check done by running node could go stale.
func TestDashboardLogoutFetchIsPOST(t *testing.T) {
	raw, err := os.ReadFile("../../web/static/js/app.js")
	if err != nil {
		t.Fatalf("read app.js: %v", err)
	}
	js := string(raw)
	const route = "/api/v1/auth/logout"
	n := strings.Count(js, route)
	if n != 1 {
		t.Fatalf("app.js mentions %s %d times, want exactly 1 (the logout fetch)", route, n)
	}
	i := strings.Index(js, route)
	end := i + 200
	if end > len(js) {
		end = len(js)
	}
	call := js[i:end]
	if j := strings.Index(call, ");"); j >= 0 {
		call = call[:j]
	}
	if !regexp.MustCompile(`method:\s*'POST'`).MatchString(call) {
		t.Errorf("app.js logout fetch does not use method: 'POST': %q", call)
	}
}

func TestDashboardLogoutIsPOSTButton(t *testing.T) {
	raw, err := os.ReadFile("../../web/index.html")
	if err != nil {
		t.Fatalf("read index.html: %v", err)
	}
	html := string(raw)
	if strings.Contains(html, `href="/api/v1/auth/logout"`) {
		t.Error(`index.html still links to /api/v1/auth/logout with href; a GET must not log out`)
	}
	if !strings.Contains(html, `id="logoutBtn"`) {
		t.Error(`index.html has no id="logoutBtn" button for the POST logout`)
	}
}
