---
phase: 07-release
plan: 02
type: summary
completed: 2026-05-19
commit: e6724ad
---

# F7-02 Summary — RELEASE_NOTES + Factory Presets + README

## What was built

All three tasks executed autonomously and committed in a single commit (`e6724ad`).

### T1 — RELEASE_NOTES.md ✅
- Created `RELEASE_NOTES.md` at repo root
- Version: `v1.0.0-beta.1 (2026-05-19)`
- Covers: 18 feature bullets, plugin format table, factory preset table, 5 known issues, build-from-source quickstart, planned-for-stable section
- Known issues documented accurately: auval (macOS 14+ Developer ID), SmartScreen (unsigned), font embedding, snapshot ramp, Linux best-effort

### T2 — README updates ✅
- Added `## Download` section before `## Installing Grano` with GitHub Releases link and zip contents table
- Updated macOS auval block: added Sonoma/Sequoia caveat — requires Developer ID; loads correctly in DAWs regardless
- Updated status table: F6d/F6e/F7 Pending → `F6 UI Polish + Advanced Engine ✅ Complete`, `F7 Release 🔄 In progress`, `F8 QA + Manual Acceptance Pending`

### T3 — 5 factory preset files ✅
Created `Resources/Presets/`:
- `Cloud.gpreset` — dense cloud, grainSize=200, density=45, stochasticDist=1 (Gaussian)
- `Drone.gpreset` — slow feedback sustain, grainSize=400, density=3, feedbackEnabled=1, feedbackGain=0.7
- `Rhythmic.gpreset` — Euclidean 3-in-8, patternEnabled=1, triggerMode=2, probability=0.85
- `Spectral.gpreset` — Freeze source, spectralMode=1, spectralAmount=0.8, subGrainDepth=2
- `Broken.gpreset` — Pareto timing, stochasticDist=4, motionEnabled=1, colorEnabled=1, flutterAmount=0.4, saturateAmount=0.6

All parameter IDs match `Source/Parameters.h`.

## Acceptance criteria

| AC | Result |
|----|--------|
| AC-1: RELEASE_NOTES.md with beta content | ✅ Pass |
| AC-2: README download section + auval caveat + status table | ✅ Pass |
| AC-3: 5 preset files in Resources/Presets/ | ✅ Pass |

## Deviations from plan

None. All tasks executed as specified. No code changes — content/docs only.

## Deferred issues

None new. Existing deferred items (auval signing, font embedding, snapshot ramp) are documented in RELEASE_NOTES.md known issues and STATE.md deferred table.

## Files created/modified

| File | Action |
|------|--------|
| `RELEASE_NOTES.md` | Created |
| `README.md` | Modified (3 sections) |
| `Resources/Presets/Cloud.gpreset` | Created |
| `Resources/Presets/Drone.gpreset` | Created |
| `Resources/Presets/Rhythmic.gpreset` | Created |
| `Resources/Presets/Spectral.gpreset` | Created |
| `Resources/Presets/Broken.gpreset` | Created |

## Loop status

```
PLAN ──▶ APPLY ──▶ UNIFY
  ✓        ✓        ✓
```
