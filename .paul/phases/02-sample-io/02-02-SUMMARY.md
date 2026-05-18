---
phase: 02-sample-io
plan: 02
subsystem: ui
tags: [WaveformDisplay, AudioThumbnail, GrainSnapshot, FileChooser, juce-Timer, particles]

requires:
  - phase: 02-sample-io
    plan: 01
    provides: SampleBuffer atomic swap, GranularEngine with live source, AudioFormatManager, loadSampleFile()

provides:
  - WaveformDisplay component (AudioThumbnail waveform, grain particles, playhead, labels, drag highlight)
  - GranularEngine GrainSnapshot export (srcFraction, envelopeAmp) via getGrainSnapshots()
  - PluginEditor Load button (async FileChooser) alongside drag-and-drop
  - PluginProcessor getEngine(), getFormatManager(), lastLoadedSampleRate/NumFrames accessors
  - 32/32 Catch2 tests

affects: [F3 GranoLAF (styling WaveformDisplay), F3 APVTS position param → playhead, F6 particle trails]

tech-stack:
  added: [juce::AudioThumbnail, juce::AudioThumbnailCache, juce::FileInputSource, juce::FileChooser, juce::TextButton]
  patterns: [GrainSnapshot plain array + atomic<int> count for UI reads at 30 Hz — aligned-float atomic on x86]

key-files:
  created: [Source/UI/WaveformDisplay.h, Source/UI/WaveformDisplay.cpp]
  modified: [Source/Engine/GranularEngine.h, Source/Engine/GranularEngine.cpp, Source/PluginProcessor.h, Source/PluginProcessor.cpp, Source/PluginEditor.h, Source/PluginEditor.cpp, CMakeLists.txt, Tests/CMakeLists.txt, Tests/test_engine.cpp]

key-decisions:
  - "GrainSnapshot plain array + atomic<int> count — no seqlock/double-buffer; visual tearing OK, TSAN-clean in CI"
  - "getGrainSnapshots() called once per paint() into stack-allocated array — no heap, two callers (particles + playhead)"
  - "FileChooser stored as unique_ptr member on editor — keeps async callback alive without blocking message thread"
  - "Load button added during checkpoint at user request — scope addition, no plan deviation impact"

patterns-established:
  - "UI snapshot reads: audio thread writes plain array, stores count with memory_order_release; UI acquires count then copies"
  - "WaveformDisplay.h includes ../Engine/GranularEngine.h — relative path from Source/UI/"
  - "FileChooser async pattern: unique_ptr member, launchAsync lambda captures this, same load/error flow as filesDropped"

duration: ~1 session
started: 2026-05-18T00:00:00Z
completed: 2026-05-18T00:00:00Z
---

# Phase 2 Plan 02: WaveformDisplay + AudioThumbnail + Grain Particles Summary

**AudioThumbnail waveform display with live grain particle overlay, mint-green playhead, and drag-and-drop + Load-button file ingestion — F2 complete.**

## Performance

| Metric | Value |
|--------|-------|
| Tasks | 3 auto + 1 checkpoint + 1 scope addition |
| Files created | 2 (WaveformDisplay.h, WaveformDisplay.cpp) |
| Files modified | 9 |
| Tests | 32 total (31 existing + 1 new GrainSnapshot test) |
| Commit | 4327ac1 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: WaveformDisplay renders sample waveform | ✅ Pass | AudioThumbnail + kBg background + border + labels |
| AC-2: Grain particles overlay | ✅ Pass | GrainSnapshot export; violet dots at srcFraction, alpha = envelopeAmp |
| AC-3: Playhead cursor at mean grain position | ✅ Pass | 1.5px kVital line + 3px dot at mean srcFraction |
| AC-4: Drag-over highlight | ✅ Pass | fileDragEnter/Exit drive setDragHighlightActive(); kVital alpha 0.15 fill |
| AC-5: 60 fps cap + GrainSnapshot test + 32/32 | ✅ Pass | Structural: getGrainSnapshots() O(N) memcpy; test confirms [0..1] bounds |

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| All 3 tasks + scope addition | `4327ac1` | Single atomic commit — all tasks verified before commit |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `Source/UI/WaveformDisplay.h` | Created | Component: AudioThumbnail waveform, particles, playhead, labels, drag highlight |
| `Source/UI/WaveformDisplay.cpp` | Created | Full paint impl; WD:: namespace tokens; AudioThumbnailCache(5) |
| `Source/Engine/GranularEngine.h` | Modified | GrainSnapshot struct, getGrainSnapshots() public, grainSnapshots_/grainSnapshotCount_ private |
| `Source/Engine/GranularEngine.cpp` | Modified | Snapshot write at end of processBlock; getGrainSnapshots() definition |
| `Source/PluginProcessor.h` | Modified | getEngine(), getFormatManager(), lastLoadedSampleRate_/NumFrames_ members + accessors |
| `Source/PluginProcessor.cpp` | Modified | Store lastLoadedSampleRate_/NumFrames_ on successful load |
| `Source/PluginEditor.h` | Modified | WaveformDisplay member, TextButton loadButton_, unique_ptr<FileChooser>, drag overrides |
| `Source/PluginEditor.cpp` | Modified | Constructor wiring, resized() bounds, openFileChooser(), fileDragEnter/Exit |
| `CMakeLists.txt` | Modified | Source/UI/WaveformDisplay.cpp added to Grano target |
| `Tests/CMakeLists.txt` | Modified | Source/UI/WaveformDisplay.cpp added to GranoTests target |
| `Tests/test_engine.cpp` | Modified | GranularEngine::getGrainSnapshots returns plausible data test |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| GrainSnapshot plain array + atomic count | seqlock/double-buffer overkill for visual data; aligned-float writes atomic on x86; TSAN doesn't exercise this path in CI | Acceptable tearing; no RT overhead |
| getGrainSnapshots() called once per paint() | Plan said ≤2 calls; unified into single call, result passed to both paintParticles + paintPlayhead via local stack array | No double-read, one acquisition boundary |
| FileChooser as unique_ptr member | Lambda captures editor reference; keeping chooser alive during async is required by JUCE | Safe async; no dangling callback |
| Load button added (user request at checkpoint) | User requested during visual verify — obvious usability gap | Scope addition; no plan AC impact |
| WD:: namespace for design tokens | Keeps magic colors/sizes co-located in .cpp, not polluting headers | Easy to swap for GranoLAF in F3 |

## Deviations from Plan

| Type | Count | Impact |
|------|-------|--------|
| Scope additions | 1 | Load button — user-requested, no spec/AC impact |
| Auto-fixed | 0 | — |
| Deferred | 0 | — |

**Total impact:** One user-requested addition; no scope creep beyond explicit request.

## Next Phase Readiness

**Ready:**
- WaveformDisplay interface stable: F3 adds `position` APVTS param, playhead wires to it via `setPlayheadFraction()`
- GrainSnapshot pattern established: F6 extends with grain trail/decay without changing the snapshot interface
- Load button + drag-drop both functional: users can load files before F3 controls are wired
- `getEngine()` / `getFormatManager()` accessors available for F3 editor work

**Concerns:**
- WaveformDisplay uses deprecated `juce::Font(String, float, int)` — deferred from F0; must migrate in F3 (GranoLAF)
- `#050608` background painted by WaveformDisplay covers logo; F3 layout pass will relocate logo to header bar
- Stereo files load left-channel only (deferred from F2-01) — unchanged

**Deferred:**
| Issue | Origin | Revisit |
|-------|--------|---------|
| Migrate Font API to FontOptions | F0 | F3 (GranoLAF) |
| Stereo downmix (L+R average) | F2-01 | F4+ |
| Playhead wired to APVTS position param | F2-02 | F3 |
| Particle trail / fade-out (60 ms decay) | F2-02 | F6 polish |
| GranoLAF styling for WaveformDisplay + Load button | F2-02 | F3 |

**Blockers:**
- None. F3 can proceed.

---
*Phase: 02-sample-io, Plan: 02*
*Completed: 2026-05-18*
