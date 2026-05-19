---
phase: 05-modulation-matrix
plan: 03
subsystem: modulation
tags: [snapshots, lfo-ui, mod-matrix-ui, apvts, valuetree, editor]

requires:
  - phase: 05-01
    provides: Lfo class, APVTS params
  - phase: 05-02
    provides: ModMatrix class, 66 APVTS params

provides:
  - Snapshots class (Source/Modulation/Snapshots.{h,cpp}) — 4-slot ValueTree capture/recall
  - LfoPanel (Source/UI/LfoPanel.{h,cpp}) — tab-switched LFO1/LFO2 panel, all controls APVTS-bound
  - ModulationMatrixView (Source/UI/ModulationMatrixView.{h,cpp}) — 8-row routing matrix, APVTS-bound
  - PluginProcessor Snapshots member + GranoCombinedState serialization
  - PluginEditor A/B/C/D snapshot buttons + 200px bottom panel + height 820px
  - 91→93 Catch2 tests

affects: [F6-ui-polish, F6-engine]

tech-stack:
  added: [Snapshots ValueTree serialization, ComboBoxAttachment, ButtonAttachment]
  patterns:
    - "GranoCombinedState wrapper for serialization: APVTS state + Snapshots as separate children — avoids name collision with APVTS internal state type"
    - "LfoPanel tab-switches by resetting all unique_ptr attachments then creating new ones for the selected LFO index"
    - "UI sources added to GranoTests because PluginEditor.cpp is compiled into GranoTests — same pattern as Knob.cpp, WaveformDisplay.cpp"

key-files:
  created:
    - Source/Modulation/Snapshots.h
    - Source/Modulation/Snapshots.cpp
    - Source/UI/LfoPanel.h
    - Source/UI/LfoPanel.cpp
    - Source/UI/ModulationMatrixView.h
    - Source/UI/ModulationMatrixView.cpp
  modified:
    - Source/PluginProcessor.h
    - Source/PluginProcessor.cpp
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp
    - CMakeLists.txt
    - Tests/CMakeLists.txt
    - Tests/test_modulation.cpp

key-decisions:
  - "GranoCombinedState wrapper (not GranoState) — APVTS state type is already 'GranoState'; outer wrapper must use a different name or forward/backward-compat branching breaks"
  - "LfoPanel.cpp + ModulationMatrixView.cpp added to GranoTests — PluginEditor.cpp is compiled into GranoTests and references them; plan said don't but link error forced it"

patterns-established:
  - "Tab-switched panel via attachment reset: reset unique_ptr<Attach> → create new one with new paramID; no JUCE component churn"
  - "Snapshots serialise() wraps each slot in 'SlotN' ValueTree child; forward-compat: missing slots silently skipped on deserialise()"

duration: ~1h
started: 2026-05-19T00:00:00Z
completed: 2026-05-19T00:00:00Z
---

# Phase 5 Plan 03: Snapshots + UI Panels Summary

**A/B/C/D parameter snapshots, LfoPanel (LFO1/LFO2 tabs + all APVTS controls), and ModulationMatrixView (8-row routing matrix) shipped — F5 Modulation Matrix phase complete. 93/93 tests.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~1h |
| Tasks | 2 auto + 1 checkpoint (human-verify) |
| Tests added | 2 (Snapshots round-trip, isOccupied) |
| Total tests | 93/93 pass |
| Files created | 6 |
| Files modified | 7 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: Snapshots round-trip | Pass | Test 91: save 50ms, change 200ms, recall → 50ms ±0.5 |
| AC-2: Snapshots persist across instances | Pass | serialise()/deserialise() implemented; GranoCombinedState wrapper |
| AC-3: LfoPanel APVTS-bound | Pass | Human-verified — LFO1/LFO2 tabs, waveform/rate/depth/phase/sync all bound |
| AC-4: ModulationMatrixView 8 rows | Pass | Human-verified — 8 rows with source/dest/amount, 29 dest options |
| AC-5: Snapshot A/B/C/D functional | Pass | Human-verified — Ctrl+click saves, click recalls |
| AC-6: ≥ 93 tests | Pass | 93/93 (100%) |

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| Tasks 1-3 + checkpoint | `3697bcd` | feat(modulation): Snapshots + LfoPanel + ModulationMatrixView (F5c) |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `Source/Modulation/Snapshots.h` | Created | 4-slot ValueTree snapshot interface |
| `Source/Modulation/Snapshots.cpp` | Created | save/recall/serialise/deserialise implementation |
| `Source/UI/LfoPanel.h` | Created | LFO1/LFO2 tab panel header |
| `Source/UI/LfoPanel.cpp` | Created | Tab switching via attachment reset; all sliders APVTS-bound |
| `Source/UI/ModulationMatrixView.h` | Created | 8-row routing matrix header |
| `Source/UI/ModulationMatrixView.cpp` | Created | 8 rows × source/dest/amount with APVTS attachments |
| `Source/PluginProcessor.h` | Modified | Added Snapshots snapshots_ member + getSnapshots() accessor |
| `Source/PluginProcessor.cpp` | Modified | GranoCombinedState serialization (getState/setState) |
| `Source/PluginEditor.h` | Modified | Added lfoPanel_, modMatrixView_, snapButtons_[4] members |
| `Source/PluginEditor.cpp` | Modified | Constructor init, Ctrl+click snap wiring, resized 820px |
| `CMakeLists.txt` | Modified | Snapshots.cpp, LfoPanel.cpp, ModulationMatrixView.cpp added |
| `Tests/CMakeLists.txt` | Modified | Same sources + UI panels (needed for GranoTests link) |
| `Tests/test_modulation.cpp` | Modified | 2 Snapshots tests (round-trip, isOccupied) |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| `GranoCombinedState` wrapper for serialization | APVTS internal state type is already "GranoState" — outer wrapper must differ to allow `tree.hasType()` disambiguation | Old-format fallback still works via `else if (tree.hasType(apvts_.state.getType()))` |
| LfoPanel.cpp + ModulationMatrixView.cpp in GranoTests | PluginEditor.cpp compiled into GranoTests; linker requires all referenced symbols | Consistent with existing pattern: Knob.cpp, WaveformDisplay.cpp also in GranoTests |

## Deviations from Plan

| Type | Description |
|------|-------------|
| Bug fix | Plan said "GranoState" wrapper — changed to "GranoCombinedState" to avoid APVTS type collision |
| Scope addition | Plan said NOT to add UI .cpp files to GranoTests — linker error required adding them |

## Next Phase Readiness

**Ready for F6 (UI Polish + Sub-grain + Spectral):**
- Snapshots API stable: save()/recall()/isOccupied() usable from any UI component
- LfoPanel/ModulationMatrixView compile and link in both Grano and GranoTests targets
- 93/93 tests; 66 APVTS params frozen for F5
- Editor height 820px with 200px bottom panel established — F6 polish adds glow/particle-trail/animation

**Deferred (from F5):**
- Drawable waveform drag-editor in LfoPanel (click-drag point editor — F6)
- 250ms ramp on snapshot recall (F6 polish)
- Color/Motion/Pattern mod destinations consumed by engines (F6 scope)
- LFO rate display in Hz/BPM next to rate slider (F6 polish)

**Blockers:** None.

---
*Phase: 05-modulation-matrix, Plan: 03*
*Completed: 2026-05-19*
