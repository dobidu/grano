# SPECIAL FLOWS — Grano

This file lists skills that must be loaded into the Claude Code session before `/paul:apply` executes any plan. PAUL blocks execution if a required skill is not confirmed loaded.

---

## Required Skills

| Skill | Work Type | Priority | Notes |
|-------|-----------|----------|-------|
| `/caveman full` | All sessions | required | Cuts ~65% of output tokens. Run once per session, or use `--with-init`. |
| `/graphify` | All sessions involving file reads | required | Consult the codebase knowledge graph before file reads. Saves token cost on large file traversals. |
| `frontend-design` (built-in skill) | UI components, LookAndFeel work | required for F2, F3, F5, F6 | Anthropic's frontend-design skill for visual quality. Use for any work that touches `Source/UI/`. |

---

## Recommended Skills (not blocking)

| Skill | Work Type | Notes |
|-------|-----------|-------|
| `/caveman-commit` | After applying any plan | Generates Conventional Commit messages. |
| `/caveman-review` | Code review of PRs | Inline `Lxx: 🔴 reason. Fix:` comments. |
| `/caveman-stats --share` | Weekly | Track token savings. |
| `/graphify query` | Whenever orientation is needed | Faster than grepping. |

---

## Skill loading checklist (PAUL enforces)

Before `/paul:apply`, PAUL will confirm that all required skills above are loaded. If any are missing, PAUL halts and instructs the user to load them.

Manual confirmation:

```
/caveman full
/graphify .  (or graphify query "..." for confirmation it's active)
```

For `frontend-design`, the skill is built into the Anthropic skills system and loads automatically when relevant tasks are detected. PAUL trusts the skill manifest.

---

## Domain-specific guardrails

Beyond skill loading, PAUL enforces these project-specific rules during `/paul:apply`:

1. **Real-time safety**: any code added to `Source/Engine/` or anywhere that runs on the audio thread must not allocate, lock, or throw. PAUL will flag suspicious patterns (`new`, `delete`, `std::vector::push_back`, `std::mutex`, `std::async`, `throw`) for review before commit.

2. **Artist name neutrality**: no commit message, code comment, identifier, asset filename, or documentation may name a reference artist, band, or commercial plugin product. PAUL scans new content for a blocklist and flags matches.

3. **APVTS as single source**: any new parameter must be added to `Source/Parameters.cpp` and bound via APVTS. PAUL flags direct `float`/`std::atomic<float>` members on the processor that look like parameters.

4. **CMake-first**: all new source files must be added to `CMakeLists.txt`. PAUL flags new `.cpp`/`.h` files in `Source/` that are not registered in CMake.

5. **Tests follow code**: any new public class in `Source/Engine/` or `Source/Modulation/` must have at least one Catch2 test under `Tests/`. PAUL flags missing test files at unify time, not apply time (so apply can complete; tests are a closure gate).

---

## Blocklist (for guardrail #2)

Names to flag if they appear in any new file content (case-insensitive substring match, word boundary preferred):

```
aphex, twin, autechre, flying, lotus, daedalus, brainfeeder,
boards of canada, warp records, drukqs, syro, confield,
oversteps, bola, gescom, plaid, squarepusher,
serum, vital, omnisphere, falcon, pigments, portal, quanta,
phase plant, absynth, kontakt, max/msp, max for live, monome
```

If any term in this list appears in a diff, PAUL will pause and ask the user to rename. The blocklist is owned by this file and can be edited.

---

## Notes on tool combination

The intended cooperation:

1. **Caveman** keeps each Claude response short. Cumulative savings across F1-F7 are substantial.
2. **Graphify** ensures each file read is informed by the knowledge graph, not by greedy grep. Particularly valuable in F4-F6 when the codebase fans out.
3. **PAUL** enforces structure: every unit of work is planned, applied, and unified. No orphan changes.
4. **frontend-design** ensures the UI work matches the design spec, not generic generated UI.

Together they keep an experimental, ambitious C++ project disciplined.
