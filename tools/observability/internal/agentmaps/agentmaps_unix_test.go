//go:build unix

package agentmaps

import (
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"sync"
	"syscall"
	"testing"
	"time"
)

// A FIFO swapped in after the check must neither be followed nor block the request: the open is non-blocking and the opened
// descriptor's regular check refuses it. Without O_NONBLOCK this test hangs until its deadline.
func TestHandleFileDoesNotBlockOnFIFOSwappedAfterCheck(t *testing.T) {
	dir := t.TempDir()
	write(t, dir, "fifo.svg", "<svg/>")
	d, err := New(dir)
	if err != nil {
		t.Fatal(err)
	}
	var once sync.Once
	testHookBeforeOpen = func(path string) {
		if filepath.Base(path) != "fifo.svg" {
			return
		}
		once.Do(func() {
			if err := os.Remove(path); err != nil {
				t.Errorf("remove: %v", err)
				return
			}
			if err := syscall.Mkfifo(path, 0o600); err != nil {
				t.Errorf("mkfifo: %v", err)
			}
		})
	}
	t.Cleanup(func() { testHookBeforeOpen = nil })
	done := make(chan *httptest.ResponseRecorder, 1)
	go func() {
		rec := httptest.NewRecorder()
		d.HandleFile(rec, httptest.NewRequest(http.MethodGet, FilePrefix+"fifo.svg", nil))
		done <- rec
	}()
	select {
	case rec := <-done:
		if rec.Code != 404 {
			t.Fatalf("FIFO -> %d", rec.Code)
		}
	case <-time.After(2 * time.Second):
		t.Fatal("request hung on a FIFO swapped in after the check")
	}
}
