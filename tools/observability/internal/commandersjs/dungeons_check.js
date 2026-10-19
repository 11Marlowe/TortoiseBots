"use strict";
// Run by commandersjs_test.go under node: the Dungeons tab's pure logic (run states, copy labels, party rows and raid subgroups).
const assert = require("assert");
const path = require("path");
const CmdView = require(path.resolve(process.argv[2]));
const D = require(path.resolve(process.argv[3]));

// ---- copy labels: two copies of one dungeon read apart, a lone group has no copy number
const runs = [
  { key: "409:14", map: 409, instance: 14, members: [] },
  { key: "409:13", map: 409, instance: 13, members: [] },
  { key: "70:5", map: 70, instance: 5, members: [] },
];
const labels = D.copyLabels(runs);
assert.deepStrictEqual(labels, { "409:13": 1, "409:14": 2 }); // ordered by instance id, whatever the report order
assert.deepStrictEqual(D.copyLabels([runs[2]]), {});         // a lone group: no copy number
assert.deepStrictEqual(D.copyLabels(null), {});

// ---- the short state of a run, worst first
const m = (over) => Object.assign({ name: "Bot", state: "idle", dead: false }, over);
assert.strictEqual(D.runState({ members: [m({ state: "moving" }), m({ state: "idle" })] }), "moving");
assert.strictEqual(D.runState({ members: [m({ state: "moving" }), m({ state: "combat" })] }), "fighting");
assert.strictEqual(D.runState({ members: [m({ state: "combat" }), m({ dead: true, state: "dead" })] }), "someone dead");
assert.strictEqual(D.runState({ members: [m({ state: "busy" }), m({ state: "resting" })] }), "busy");
assert.strictEqual(D.runState({ members: [m({ state: "resting" })] }), "resting");
assert.strictEqual(D.runState({ members: [m({ state: "idle" })] }), "idle");
assert.strictEqual(D.runState({ members: [] }), "");
assert.strictEqual(D.runState(null), "");

// ---- the party rows: the player first, then subgroup, then name; bars and words filled in
const run = { key: "409:13", map: 409, instance: 13, members: [
  { name: "Zug", guid: 3, player: false, class: "warrior", role: "tank", level: 60, hp: 2500, max_hp: 5000, power: 0, max_power: 0, subgroup: 2, state: "combat" },
  { name: "Thralia", guid: 1, player: true, class: "shaman", role: "", level: 60, hp: 3000, max_hp: 4000, power: 2000, max_power: 4000, power_type: "mana", subgroup: 1, state: "combat" },
  { name: "Aria", guid: 2, player: false, class: "mage", role: "dps", level: 60, hp: 0, max_hp: 3500, power: 1000, max_power: 5000, power_type: "mana", subgroup: 1, state: "dead" },
] };
const rows = D.memberRows(run);
assert.deepStrictEqual(rows.map((r) => r.name), ["Thralia", "Aria", "Zug"]); // player first, then subgroup, then name
const thralia = rows[0];
assert.strictEqual(thralia.player, true);
assert.strictEqual(thralia.roleLabel, "Player");
assert.strictEqual(thralia.role, "other");       // a player carries no AI role
assert.strictEqual(thralia.hp, 75);              // pct, not raw
assert.strictEqual(thralia.powerKind, "mana");
const aria = rows[1];
assert.strictEqual(aria.role, "dead");           // a 0-HP corpse greys out
assert.strictEqual(aria.hpText, "0 / 3500");
const zug = rows[2];
assert.strictEqual(zug.role, "tank");
assert.strictEqual(zug.hasPower, false);         // a warrior has no power bar

// ---- a party reads as one unlabelled group, a raid as compact subgroup blocks
assert.deepStrictEqual(D.memberGroups(D.memberRows(run)).length, 1);
assert.strictEqual(D.memberGroups(D.memberRows(run))[0].label, "");
const raid = { key: "409:1", map: 409, instance: 1, members: [] };
for (let s = 1; s <= 8; s++)
  for (let i = 1; i <= 5; i++)
    raid.members.push({ name: "Bot" + s + "-" + i, guid: s * 100 + i, class: "warrior", role: "dps", level: 60, hp: 1, max_hp: 1, power: 0, max_power: 0, subgroup: s, state: "moving" });
const groups = D.memberGroups(D.memberRows(raid));
assert.strictEqual(groups.length, 8); // one block per subgroup, not 40 rows
assert.deepStrictEqual(groups.map((g) => g.label), [1, 2, 3, 4, 5, 6, 7, 8].map((s) => "Group " + s));
assert.strictEqual(groups[0].rows.length, 5);
// Without subgroups in the snapshot (a pre-v12 module): chunks of 5 in the order reported.
const noSub = { key: "409:1", map: 409, instance: 1, members: raid.members.map((r) => Object.assign({}, r, { subgroup: 0 })) };
const chunks = D.memberGroups(D.memberRows(noSub));
assert.strictEqual(chunks.length, 8);
assert.strictEqual(chunks[0].label, "Group 1");
assert.deepStrictEqual(chunks[0].rows.map((r) => r.name), ["Bot1-1", "Bot1-2", "Bot1-3", "Bot1-4", "Bot1-5"]);

// ---- seen-for wording rides on the page's age text
assert.strictEqual(D.seenText(45), CmdView.ageText(45));
