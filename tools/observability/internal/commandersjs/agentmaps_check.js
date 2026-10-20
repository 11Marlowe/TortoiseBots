"use strict";
// Run by commandersjs_test.go under node: the Agent maps tab's pure logic (the read-only list, its sizes and the viewer's URL and
// sandbox). A file name comes from the operator's folder, so a name that looks like markup must never reach the page as markup.
const assert = require("assert");
const path = require("path");
const A = require(path.resolve(process.argv[2]));

// ---- sizes read at a glance
assert.strictEqual(A.formatSize(0), "0 B");
assert.strictEqual(A.formatSize(1023), "1023 B");
assert.strictEqual(A.formatSize(1024), "1.0 KiB");
assert.strictEqual(A.formatSize(1536), "1.5 KiB");
assert.strictEqual(A.formatSize(20 * 1024), "20 KiB");
assert.strictEqual(A.formatSize(2 * 1024 * 1024), "2.0 MiB");
assert.strictEqual(A.formatSize(40 * 1024 * 1024), "40 MiB");
assert.strictEqual(A.formatSize(-1), "–");
assert.strictEqual(A.formatSize("nope"), "–");

// ---- the URL is one percent-encoded component, never a path or a query
assert.strictEqual(A.fileURL("run.svg"), "/api/v1/agent-maps/files/run.svg");
assert.strictEqual(A.fileURL("run f1.svg"), "/api/v1/agent-maps/files/run%20f1.svg");
assert.strictEqual(A.fileURL("a?b#c.svg"), "/api/v1/agent-maps/files/a%3Fb%23c.svg");
assert.strictEqual(A.fileURL("<script>.svg"), "/api/v1/agent-maps/files/%3Cscript%3E.svg");
assert.strictEqual(A.fileURL("../../etc/passwd"), "/api/v1/agent-maps/files/..%2F..%2Fetc%2Fpasswd");

// ---- the framed replay gets its script; the SVG does not; neither shares this origin
assert.strictEqual(A.sandbox("html"), "allow-scripts");
assert.strictEqual(A.sandbox("svg"), "");
assert.ok(A.sandbox("html").indexOf("allow-same-origin") < 0);

// ---- a hostile file name is escaped in every place it lands
const hostile = `<script>alert(1)<script>.svg`;
const quote = `x" onmouseover="alert(1).svg`;
const rows = A.renderRows([
  { name: hostile, size: 2048, mtime: 1700000000, kind: "svg" },
  { name: quote, size: 1, mtime: 1700000000, kind: "html" },
  { name: "run-f1.svg", size: 10, mtime: 0, kind: "svg" },
]);
assert.ok(rows.indexOf("<script") < 0, "raw markup reached the list: " + rows);
assert.ok(rows.indexOf("&lt;script&gt;alert(1)&lt;script&gt;.svg") >= 0, rows);
assert.ok(rows.indexOf("&quot; onmouseover=&quot;") >= 0, rows);
assert.ok(rows.indexOf('data-url="/api/v1/agent-maps/files/%3Cscript%3Ealert(1)%3Cscript%3E.svg"') >= 0, rows);
assert.ok(rows.indexOf("2.0 KiB") >= 0 && rows.indexOf("HTML") >= 0 && rows.indexOf("SVG") >= 0, rows);
assert.strictEqual(A.renderRows([]), "");
assert.strictEqual(A.renderRows(null), "");
