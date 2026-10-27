---
id: developers-donor-hierarchy
title: Donor Hierarchy & Porting Rules
category: developers
summary: Evidence priority and architectural guidelines when referencing mod-playerbots and upstream core.
tags: [donors, mod-playerbots, porting, rules]
relates_to:
  - developers-architecture
  - developers-strategy-engine
---

# Donor Hierarchy & Porting Rules

When implementing or diagnosing behavior in TortoiseBots, code must not be blindly copied. Different sources serve strictly different purposes.

## Evidence Hierarchy

When investigating bugs or porting features, evaluate evidence in this order:

1. **Actual Tortoise WoW Client / Server Runtime Behavior** (Ground Truth).
2. **Current Core Server Semantics** ([tortoise-wow](https://github.com/tortoise-wow/tortoise-wow)): engine lifecycle, session and packet behavior, and Turtle-specific DBC and spell data.
3. **mod-playerbots** ([mod-playerbots/mod-playerbots](https://github.com/mod-playerbots/mod-playerbots)):
   - **Primary mature behavior donor.**
   - Consult first for: combat rotations, healing prioritization, interrupt selections, CC logic, dungeon behavior, and rich tactics.
   - Strip all TBC/WotLK assumptions before porting.

## Porting Rules

```text
Turtle runtime mechanics  → current core first
Mature gameplay behavior  → mod-playerbots first
Architecture ownership    → TortoiseBots rules always win
```

### Harvest Behavior, Never Coupling
- Never copy global bot managers, baked-in player pointers, or core-level bot checks.
- Keep all state and logic inside `modules/TortoiseBots/`.
- Record all donor inspirations, file paths, and commit hashes in [data/donor-map.tsv](data/donor-map.tsv), and the source commit in the pull request.
