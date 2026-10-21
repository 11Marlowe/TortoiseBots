"use strict";
// Run by commandersjs_test.go under node: the spec labels of the Bots tab and the bot profile (app.js).
// A shaman tank keeps its Enhancement spec for filters, chips and colours but says "Enhancement (Tank)"
// where the spec name is shown; a character with no talent points has no spec and is never given a
// placeholder name (a character with no talents used to read "Unspecified" in the list).
const assert = require("assert");
const fs = require("fs");
const vm = require("vm");
const src = fs.readFileSync(require("path").resolve(process.argv[2] || require("path").join(__dirname, "../../web/static/js/app.js")), "utf8");

function grab(name) {
  const re = new RegExp(`^  function ${name}\\(`, "m"), m = re.exec(src);
  assert.ok(m, `${name} not found in app.js`);
  const end = src.indexOf("\n  }\n", m.index);
  return src.slice(m.index, end + 4);
}
const ctx = { console };
vm.createContext(ctx);
vm.runInContext(grab("specLabel") + "\n" + grab("specCounts") + "\nthis.specLabel = specLabel; this.specCounts = specCounts;", ctx);
const { specLabel, specCounts } = ctx;

// ---- the shaman tank label
assert.strictEqual(specLabel("Enhancement", "shaman", { role: "tank" }), "Enhancement (Tank)", "a shaman tank says so");
assert.strictEqual(specLabel("Enhancement", "Shaman", { role: "tank" }), "Enhancement (Tank)", "the profile passes the class name capitalized");
assert.strictEqual(specLabel("Enhancement", "shaman", { role: "dps" }), "Enhancement", "a dps shaman is a plain Enhancement");
assert.strictEqual(specLabel("Enhancement", "shaman", { role: "healer" }), "Enhancement");
assert.strictEqual(specLabel("Enhancement", "shaman", null), "Enhancement", "offline: no live role, no label");
assert.strictEqual(specLabel("Enhancement", "shaman", {}), "Enhancement");
assert.strictEqual(specLabel("Elemental", "shaman", { role: "tank" }), "Elemental (Tank)", "any shaman the server put on the tank role");
assert.strictEqual(specLabel("Protection", "warrior", { role: "tank" }), "Protection", "only shamans: a warrior tank keeps its spec name");
assert.strictEqual(specLabel("Feral Combat", "druid", { role: "tank" }), "Feral Combat");
assert.strictEqual(specLabel("", "shaman", { role: "tank" }), "", "no spec stays no spec");
assert.strictEqual(specLabel(undefined, "shaman", { role: "tank" }), "");
assert.strictEqual(specLabel("Enhancement", "", { role: "tank" }), "Enhancement");

// ---- the spec chips: a row without a spec is never named
// (the counts come out of the vm context: compare them as JSON, not by prototype)
const counts = (rows) => JSON.stringify(Object.entries(specCounts(rows)).sort());
assert.strictEqual(counts([{ spec: "Enhancement" }, { spec: "Enhancement" }, { spec: "Hybrid" }]), '[["Enhancement",2],["Hybrid",1]]');
assert.strictEqual(counts([]), "[]", "no rows, no chips");
assert.strictEqual(counts([{ spec: "" }, { spec: "" }, {}]), "[]", "rows with no spec are counted under All specs only");
assert.strictEqual(counts([{ spec: "Arms" }, { spec: "" }]), '[["Arms",1]]');

// ---- the placeholder name is gone from the page
assert.ok(!/'Unspecified'/.test(src), "app.js must not name a spec 'Unspecified': a character with no talents has no spec");

console.log("speclabels_check.js OK");
