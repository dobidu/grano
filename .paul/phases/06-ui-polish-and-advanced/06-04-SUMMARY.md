---
phase: 06-ui-polish-and-advanced
plan: 04
subsystem: engine
tags: [multi-sample, grain-source, weighted-random, stereo-downmix, apvts]

requires:
  - phase: 06-03
    provides: FeedbackPath + SpectralProcessor as alternate grain sources; source priority chain established

provides:
  - MultiSampleBank: 4-slot SampleBuffer wrapper with RT-safe weighted-random pickSlot()
  - GranularEngine migrated from single SampleBuffer to MultiSampleBank
  - PluginProcessor migrated: sampleBank_ replaces sampleBuffer_
  - Stereo downmix fix (F2 deferred): L+R averaged to mono on file load
  - 4 APVTS params: slot0Weight–slot3Weight (78 total)
  - 7 new Catch2 tests (117 total)

affects: [06-05-integration, ui-slot-loading, mod-matrix-slot-weight-destinations]

tech-stack:
  added: []
  patterns:
    - "Multi-slot hot-swap: trySwapAll() returns bool; caller flushes grains on true"
    - "RT-safe weighted selection: stack-only cumulative table in pickSlot(); no heap, no locks"
    - "Default weights (1,0,0,0) → backward compatible with prior single-slot path"

key-files:
  created:
    - Source/Engine/MultiSampleBank.h
    - Source/Engine/MultiSampleBank.cpp
    - Tests/test_multisample.cpp
  modified:
    - Source/Engine/GranularEngine.h
    - Source/Engine/GranularEngine.cpp
    - Source/PluginProcessor.h
    - Source/PluginProcessor.cpp
    - Source/Parameters.h
    - Source/Parameters.cpp
    - CMakeLists.txt
    - Tests/CMakeLists.txt
    - Tests/test_modulation.cpp
    - Tests/test_sample_io.cpp

key-decisions:
  - "trySwapAll() returns bool so GranularEngine::processBlock can flush stale grain source pointers atomically"
  - "pickSlot() uses stack-only cumulative weight array — no allocation on scheduler thread"
  - "Default slot0Weight=1.0, others=0.0 — exact backward-compat with prior single-SampleBuffer path"
  - "Stereo downmix: L+R * 0.5 averaged into mono via temp stereo buffer (resolves F2 deferral)"
  - "getSampleBuffer() on PluginProcessor preserved (returns slot 0 ref) — WaveformDisplay unchanged"

patterns-established:
  - "Multi-source grain engine pattern: source priority chain (Spectral > Feedback > Bank > sine) — bank slot takes the old sampleSource_ position"
  - "CMake stale after adding sources to Tests/CMakeLists.txt: always re-run cmake -B build before building tests"

duration: ~3h
started: 2026-05-19T00:00:00Z
completed: 2026-05-19T00:00:00Z
---

# Phase 6 Plan 04: MultiSampleBank Summary

**4-slot weighted-random grain source bank with RT-safe pickSlot(), stereo downmix fix, and full GranularEngine/PluginProcessor migration; 117/117 tests pass.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~3h (including cmake stale debug) |
| Started | 2026-05-19 |
| Completed | 2026-05-19 |
| Tasks | 4 completed (T1–T4) |
| Files modified | 10 modified, 3 created |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-0: Build + all tests pass | Pass | 117/117; all targets build clean |
| AC-1: pickSlot RT-safety | Pass | Stack-only cumulative table; no alloc, no locks; verified by inspection |
| AC-2: Weighted random proportions | Pass | test_multisample: ~75% slot 0 at [3,1,0,0] weights with ±6% tolerance; 10000 samples |
| AC-3: Zero-weight / empty slot exclusion | Pass | Zero-weight → never selected; unloaded slot excluded even with weight > 0 |
| AC-4: trySwapAll flushes active grains | Pass | Returns bool; engine resets activeGrainCount_ to 0 on true |
| AC-5: Stereo downmix | Pass | reader->numChannels >= 2 → L+R * 0.5 into mono; else single-channel read unchanged |
| AC-6: Default weights backward compat | Pass | slot0Weight=1.0 default → identical behavior to prior SampleBuffer path |
| AC-7: Human listening verify | Pass | Approved by user (WSL2 Standalone not launchable; approved on test evidence) |

## Accomplishments

- MultiSampleBank.{h,cpp}: 4-slot hot-swap wrapper; pickSlot() fully RT-safe (stack arrays, no heap, no locks)
- GranularEngine fully migrated: sampleSource_ removed; bankSource_ + pSlotWeight_[4] replace it; grain flush preserved via trySwapAll() return value
- Stereo downmix bug (F2 deferred since plan 02-02) resolved: L+R averaged to mono on load
- 7 Catch2 tests in test_multisample.cpp; param count updated 74→78 in test_modulation.cpp
- test_sample_io.cpp migrated from setSource/SampleBuffer API to setBank/MultiSampleBank

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| T1+T2+T3 (all auto tasks) | `b17ab73` | MultiSampleBank + engine migration + stereo downmix + tests |
| T4 (human verify) | — | Approved; no code change |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `Source/Engine/MultiSampleBank.h` | Created | 4-slot bank interface; pickSlot() + trySwapAll() + getSlot() |
| `Source/Engine/MultiSampleBank.cpp` | Created | pickSlot() cumulative weight selection; trySwapAll/processRetiredAll forwarding |
| `Tests/test_multisample.cpp` | Created | 7 Catch2 tests for MultiSampleBank |
| `Source/Engine/GranularEngine.h` | Modified | Replaced sampleSource_ with bankSource_; added pSlotWeight_[4]; added setBank/setBankParamPointers |
| `Source/Engine/GranularEngine.cpp` | Modified | processBlock: trySwapAll(); scheduleGrain: pickSlot(); grain snapshot: bankSource_->getNumSamples(0) |
| `Source/PluginProcessor.h` | Modified | sampleBank_ replaces sampleBuffer_; getSampleBuffer() returns slot 0; timerCallback uses processRetiredAll() |
| `Source/PluginProcessor.cpp` | Modified | Constructor: setBank + setBankParamPointers; loadSampleFile: stereo downmix + loadSlot(0); triggerSpectral: getReadPointer(0) |
| `Source/Parameters.h` | Modified | slot0Weight–slot3Weight param IDs |
| `Source/Parameters.cpp` | Modified | 4 AudioParameterFloat params; slot0Weight default=1.0, others=0.0 |
| `CMakeLists.txt` | Modified | MultiSampleBank.cpp added to Grano target sources |
| `Tests/CMakeLists.txt` | Modified | MultiSampleBank.cpp + test_multisample.cpp added to GranoTests |
| `Tests/test_modulation.cpp` | Modified | Param count 74→78; 4 new slot weight spot-checks |
| `Tests/test_sample_io.cpp` | Modified | setSource → setBank; SampleBuffer sb → MultiSampleBank bank |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| trySwapAll() returns bool | Engine needs to know if ANY slot swapped to flush stale grain pointers; atomic check per slot; returning bool avoids extra atomic in engine | Engine flushes activeGrains_ on any slot swap — correct for all slot counts |
| pickSlot() uses stack cumulative table | RT safety: no heap alloc; kNumSlots=4 so array size is bounded at compile time | Pattern for future expansion; must stay stack-only if kNumSlots grows |
| Default slot0Weight=1.0, others=0.0 | Backward compat: existing presets/saves that don't have slot weight params get slot 0 always selected — exact prior behavior | No user-visible regression when loading old state |
| getSampleBuffer() preserved | WaveformDisplay and editor callers access slot 0 by existing API; no UI changes needed in this plan | 06-05 can add per-slot UI without touching engine interface |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 3 | Essential; no scope creep |
| Scope additions | 0 | — |
| Deferred | 0 | — |

**Total impact:** Three auto-fixes for missed/stale references; no scope changes.

### Auto-fixed Issues

**1. Stale sampleSource_ reference in grain snapshot section**
- **Found during:** T2 build
- **Issue:** GranularEngine.cpp line 287 still referenced `sampleSource_` after migration; `error: 'sampleSource_' was not declared in this scope`
- **Fix:** Replaced with `bankSource_ != nullptr ? bankSource_->getNumSamples(0) : 0`
- **Files:** `Source/Engine/GranularEngine.cpp:287`
- **Verification:** Clean build after fix

**2. CMake stale link command (test objects missing from link)**
- **Found during:** T3 test run (0 tests registered despite 117 expected)
- **Issue:** After adding sources to Tests/CMakeLists.txt, cmake had a stale link.txt that only included JUCE module .o files — test TU object files were compiled but not linked; root cause: cmake regeneration needed after source list change in an incremental build
- **Fix:** `cmake -B build -DGRANO_BUILD_TESTS=ON` forced regeneration; link.txt now includes all test + plugin source objects
- **Files:** `build/Tests/CMakeFiles/GranoTests.dir/link.txt` (generated)
- **Verification:** 117/117 tests pass after regeneration

**3. test_sample_io.cpp: setSource API → setBank API**
- **Found during:** T3 build (after cmake regeneration exposed previously-not-linked test TU)
- **Issue:** Two test cases used `engine.setSource(&sb)` which was removed during migration
- **Fix:** Updated both test cases to use `MultiSampleBank bank; engine.setBank(&bank);`; added `#include "Engine/MultiSampleBank.h"`
- **Files:** `Tests/test_sample_io.cpp`
- **Verification:** Build clean; 117/117 pass

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| WSL2: Standalone aborts on ALSA MIDI device not found | Pre-existing WSL2 limitation; user approved T4 on test evidence + prior sessions |
| CMake unity build stale after source additions | Re-run `cmake -B build` whenever adding sources to Tests/CMakeLists.txt — added to key patterns |

## Next Phase Readiness

**Ready:**
- MultiSampleBank API stable; 06-05 can add UI load buttons for slots 1–3 without engine changes
- Source priority chain complete: Spectral > Feedback > Bank(4 slots) > sine
- APVTS at 78 params; slot weight params exposed and bindable to UI sliders
- getSampleBuffer() returns slot 0 ref — WaveformDisplay integration unchanged

**Concerns:**
- Slot weights currently not exposed in UI — slots 1–3 unreachable from user perspective until 06-05
- 250ms snapshot recall ramp still deferred from F5c
- CMake reconfigure needed if test sources added without full rebuild

**Blockers:** None

---
*Phase: 06-ui-polish-and-advanced, Plan: 04*
*Completed: 2026-05-19*
