"use strict";
// Run by commandersjs_test.go under node: app.js really routes every live view and every timer through the refresh gate (refresh.js),
// and index.html loads refresh.js before app.js and offers the picker on the Commanders tab and in the top bar.
const assert = require("assert");
const fs = require("fs");
const path = require("path");
const appPath = path.resolve(process.argv[2]);
const src = fs.readFileSync(appPath, "utf8");
const html = fs.readFileSync(path.join(path.dirname(appPath), "..", "..", "index.html"), "utf8");

// every timer: the offline watchdog (health stays live), the stream-fed Commanders fallback (applyCommanders goes through the gate), and
// the rest guarded, so a paused page does not poll
const timers = src.split("\n").filter((l) => /setInterval\(/.test(l));
const guarded = timers.filter((l) => /setInterval\(refreshGate\.guard\(/.test(l));
const others = timers.filter((l) => !/setInterval\(refreshGate\.guard\(/.test(l));
assert.strictEqual(guarded.length, 5, "activity, roster, expire and the demo's two timers are guarded: " + guarded.length);
assert.deepStrictEqual(others.map((l) => l.trim().replace(/\s+/g, " ").replace(/ \/\/.*$/, "")), ["setInterval(() => {", "setInterval(fetchCommanders, 30000);"],
  "only the health watchdog and the gated Commanders fallback run while paused:\n" + others.join("\n"));
assert.ok(/setInterval\(\(\) => \{\n\s+updateSnapshotAge\(\);/.test(src), "the unguarded arrow timer is the health watchdog");
assert.ok(!/setInterval\(fetchActivity|setInterval\(fetchRoster|setInterval\(expireCommanders/.test(src), "no unguarded poll");

// the live data goes through the gate
const fn = (name) => { const i = src.indexOf("  function " + name + "("); assert.ok(i >= 0, name); return src.slice(i, src.indexOf("\n  }\n", i)); };
assert.ok(/refreshGate\.submit\('snapshot'/.test(fn("handleStreamMessage")), "the roster snapshot is gated");
assert.ok(/refreshGate\.submit\('commanders'/.test(fn("applyCommanders")), "the Commanders report is gated");
const hs = fn("handleStreamMessage");
assert.ok(hs.indexOf("applyServerStatus(data.server)") < hs.indexOf("refreshGate.submit('snapshot'"), "server health is applied before (outside) the gate");
// the page remembers every report even when it shows fewer
const ac = fn("applyCommanders");
assert.ok(ac.indexOf("cmdNotePaths(") >= 0 && ac.indexOf("cmdNotePaths(") < ac.indexOf("refreshGate.submit('commanders'"), "paths are noted for every report, before the gate");
assert.ok(/refreshGate\.flush\(\)/.test(src), "the Refresh button shows what a paused page holds");
// the saved choice goes through try/catch helpers only
assert.ok(/RefreshRate\.load\(refreshStore\)/.test(src) && /RefreshRate\.save\(refreshStore/.test(src));
assert.ok(/const refreshStore = \(\(\) => \{ try \{ return window\.localStorage; \} catch \(e\) \{ return null; \} \}\)\(\);/.test(src), "even asking for localStorage is guarded");

// the page
assert.ok(html.indexOf("/static/js/refresh.js") > 0 && html.indexOf("/static/js/refresh.js") < html.indexOf("/static/js/app.js"), "refresh.js before app.js");
const tab = html.slice(html.indexOf('id="tab-commanders"'), html.indexOf('id="tab-issues"'));
assert.ok(/data-refresh-select/.test(tab) && /data-refresh-badge/.test(tab), "the picker and the paused badge are on the Commanders tab");
const top = html.slice(html.indexOf('<header class="topbar">'), html.indexOf("</header>"));
assert.ok(/data-refresh-select/.test(top) && /data-refresh-badge/.test(top), "...and in the top bar, so every live tab has them");
console.log("refresh wiring ok");
