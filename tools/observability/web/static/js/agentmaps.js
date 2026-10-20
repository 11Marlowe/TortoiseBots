"use strict";
// Agent maps tab (issue #1102): the pure logic of the read-only list of turtle-agent run maps, free of the page so a test can run
// it under node (internal/commandersjs). The daemon (internal/agentmaps) lists the folder and serves the files; app.js draws.
// Names come from the operator's own folder, so every one that reaches markup is escaped here (a file may be named
// "<script>alert(1)<script>.svg"), and the URL is percent-encoded so a name cannot introduce a path or a query.
(function (root) {
  function esc(value) {
    if (value === null || value === undefined) return "";
    return String(value)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;")
      .replace(/'/g, "&#39;");
  }

  // 1536 -> "1.5 KiB"; the dashboard reads sizes at a glance, not to the byte below a KiB.
  function formatSize(bytes) {
    const n = Number(bytes);
    if (!Number.isFinite(n) || n < 0) return "–";
    if (n < 1024) return n + " B";
    if (n < 1024 * 1024) return (n / 1024).toFixed(n < 10 * 1024 ? 1 : 0) + " KiB";
    return (n / (1024 * 1024)).toFixed(n < 10 * 1024 * 1024 ? 1 : 0) + " MiB";
  }

  // The server's unix seconds in the viewer's own zone, minute precision.
  function formatTime(unix) {
    const n = Number(unix);
    if (!Number.isFinite(n) || n <= 0) return "–";
    const d = new Date(n * 1000);
    if (isNaN(d.getTime())) return "–";
    const pad = (v) => String(v).padStart(2, "0");
    return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())} ${pad(d.getHours())}:${pad(d.getMinutes())}`;
  }

  // fetch() target for one map: the name is one percent-encoded path component, never a path.
  function fileURL(name) {
    return "/api/v1/agent-maps/files/" + encodeURIComponent(String(name || ""));
  }

  // An HTML replay needs its inline script; a run SVG needs none. Neither gets allow-same-origin, so the page in the frame has an
  // opaque origin and cannot read this dashboard's cookies, storage or DOM.
  function sandbox(kind) {
    return kind === "html" ? "allow-scripts" : "";
  }

  // The rows of the list: a button per map (data-url is what the click handler fetches, data-name what the viewer titles).
  function renderRows(files) {
    if (!Array.isArray(files) || !files.length) return "";
    return files.map((f) => {
      const name = f && typeof f.name === "string" ? f.name : "";
      const kind = f && f.kind === "html" ? "html" : "svg";
      const url = fileURL(name);
      return `<tr><td><button type="button" class="btn-link agent-map-open" data-url="${esc(url)}" data-name="${esc(name)}" title="Show ${esc(name)} inline">${esc(name)}</button></td>` +
        `<td class="cell-muted">${esc(kind.toUpperCase())}</td>` +
        `<td class="cell-muted">${esc(formatSize(f && f.size))}</td>` +
        `<td class="cell-muted">${esc(formatTime(f && f.mtime))}</td></tr>`;
    }).join("");
  }

  const AgentMapsView = { esc, formatSize, formatTime, fileURL, sandbox, renderRows };
  if (typeof module !== "undefined" && module.exports) module.exports = AgentMapsView;
  else root.AgentMapsView = AgentMapsView;
})(this);
