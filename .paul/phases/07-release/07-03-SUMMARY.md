---
phase: 07-release
plan: 03
type: summary
status: complete
completed: 2026-05-20
---

# SUMMARY — 07-03: BUILD.md fixes + v1.0.0-beta.1 tag

## What was built

Two stale inaccuracies in BUILD.md were corrected, the annotated release tag
`v1.0.0-beta.1` was created and pushed to origin, and a bonus full documentation
audit was performed across five files.

## Acceptance criteria results

| AC | Description | Result |
|----|-------------|--------|
| AC-1 | BUILD.md pluginval CI line reads "strictness 10 on every push to main" | ✅ PASS |
| AC-2 | BUILD.md auval section has macOS 14+ Sonoma/Sequoia caveat | ✅ PASS |
| AC-3 | Annotated tag v1.0.0-beta.1 pushed to origin | ✅ PASS |

Verified:
- `grep "strictness 10 on every push" BUILD.md` → match found (commit `a4c3903`)
- `grep "Sonoma" BUILD.md` → match found (commit `a4c3903`)
- `git tag -l "v1.0.0-beta.1"` → present; pushed to origin

## Deviations from plan

**Out-of-plan bonus work** (approved by user as "review, update and complete if
necessary, all documentation"):

A documentation audit commit (`7ec136a`) made the following corrections:

| File | Change |
|------|--------|
| `README.md` | Parameter count 74 → 79; added `envelopeShape` row; added slot weight table; pluginval strictness 5 → 10 |
| `Resources/Presets/Broken.gpreset` | `wowAmount`→`wowDepth`, `flutterAmount`→`flutterDepth` (wrong IDs per Parameters.h) |
| `Resources/Presets/Spectral.gpreset` | `spectralAmount`→`spectralBlurAmount` (wrong ID per Parameters.h) |
| `ARCHITECTURE.md` | Font BinaryData claim corrected to reflect actual fallback implementation |
| `DESIGN_SPEC.md` | Asset checklist updated with accurate deferred/post-v1.0 status |

These were bugs in prior deliverables, not scope creep.

## Decisions made

- auval validation remains deferred: macOS 14+ (Sonoma/Sequoia) requires a Developer
  ID code-signing certificate. Unsigned builds produce exit code 2. The AU loads and
  runs correctly in DAWs regardless. Documented in RELEASE_NOTES.md, README.md,
  BUILD.md. Revisit when Developer ID is obtained.

## Files created/modified

| File | Action | Commit |
|------|--------|--------|
| `BUILD.md` | Modified — CI strictness + auval caveat | `a4c3903` |
| `README.md` | Modified — param count, envelopeShape, slot weights, pluginval | `7ec136a` |
| `Resources/Presets/Broken.gpreset` | Fixed — wrong param IDs | `7ec136a` |
| `Resources/Presets/Spectral.gpreset` | Fixed — wrong param ID | `7ec136a` |
| `ARCHITECTURE.md` | Fixed — font embedding accuracy | `7ec136a` |
| `DESIGN_SPEC.md` | Fixed — asset checklist status | `7ec136a` |

## F7 phase complete

Plans 07-01 ✅, 07-02 ✅, 07-03 ✅ — all unified.

F7 exit criteria status:
- [x] `pluginval --strictness-level 10` passes on Windows + macOS VST3 (CI)
- [x] 5 factory presets ship: Cloud, Drone, Rhythmic, Spectral, Broken
- [x] RELEASE_NOTES.md written for v1.0.0-beta.1
- [x] README updated for end users
- [x] Beta release tagged v1.0.0-beta.1 on GitHub
- [ ] `auval` on macOS — deferred (Developer ID required)
- [ ] Code-signed installers — deferred (cert not yet obtained)
- [ ] Audio demos — deferred (post-beta recording pass)

Beta is shippable. Remaining items are documented and non-blocking for the testing phase.

Next: `/paul:plan F8` (QA + manual acceptance testing)
