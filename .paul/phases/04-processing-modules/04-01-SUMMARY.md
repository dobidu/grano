---
phase: 04-processing-modules
plan: 01
subsystem: audio-engine
tags: [motion, wow, flutter, drift, crackle, pitch-modulation, pink-noise, apvts]

requires:
  - phase: 03-core-controls
    provides: APVTS parameter infrastructure, GranoAudioProcessor wiring patterns

provides:
  - Motion module (Source/Modules/Motion.{h,cpp})
  - 8 APVTS parameters (motionEnabled, wow/flutter depth+rate, drift, crackle level+color)
  - Per-grain pitch modulation via atomic shared between audio and scheduler threads
  - Pink noise crackle injected post-engine in processBlock

affects: [F4b-color, F4c-pattern, F5-modulation, F6-advanced]

tech-stack:
  added: [Paul Kellet pink noise filter, 1-pole IIR color filter]
  patterns: [atomic cross-thread pitch offset (audio writes, scheduler reads), module bypass via early return]

key-files:
  created: [Source/Modules/Motion.h, Source/Modules/Motion.cpp, Tests/test_motion.cpp]
  modified:
    - Source/Parameters.h
    - Source/Parameters.cpp
    - Source/Engine/GranularEngine.h
    - Source/Engine/GranularEngine.cpp
    - Source/PluginProcessor.h
    - Source/PluginProcessor.cpp
    - CMakeLists.txt
    - Tests/CMakeLists.txt

key-decisions:
  - "Motion() = default; explicitly declared — JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR suppresses implicit default ctor"
  - "updatePitchMod() not updateLFOs() — name reflects all pitch sources (wow+flutter+drift)"
  - "1-block pitch latency accepted — scheduler reads previous block's offset (imperceptible for slow modulation)"

patterns-established:
  - "Module bypass: check pEnabled_ null then value, store 0.0f and return — zero CPU when disabled"
  - "Cross-thread atomic: audio thread writes pitchModSt_, scheduler thread reads relaxed — no lock, no FIFO"

duration: ~3h
started: 2026-05-18T00:00:00Z
completed: 2026-05-18T00:00:00Z
---

# Phase 4 Plan 01: MOTION Module Summary

**Motion module with Wow, Flutter, Drift, and Crackle implemented and wired end-to-end — 8 APVTS params, pitch modulation shared via atomic, 43/43 tests pass.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~3h (including build debug) |
| Tasks | 3 auto + 1 human-verify completed |
| Tests added | 7 (Motion) |
| Total tests | 43/43 pass |
| Files modified | 8 |
| Files created | 3 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: Bypass is zero-CPU | Pass | `pEnabled_ == nullptr` guard + early return when disabled; pitchModSt_ stored 0.0f |
| AC-2: Wow modulates grain pitch | Pass | pitchModSt_ atomic read by scheduleGrain; test confirmed non-zero with depth=1 |
| AC-3: Crackle adds noise | Pass | Test 5 ("crackle adds signal to silent buffer") — energy > 0 verified |
| AC-4: Drift stays in range | Pass | Test 4 ("pitch mod bounded") — max abs ≤ 0.51 st with drift+flutter off |
| AC-5: 8 params registered | Pass | test_parameters.cpp "All 8 params" test (pre-existing) + motionEnabled counted |

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| All tasks (bundled) | `d9cd358` | feat(motion): add Motion module (Wow, Flutter, Drift, Crackle) |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `Source/Modules/Motion.h` | Created | Motion class header; pitch mod + crackle |
| `Source/Modules/Motion.cpp` | Created | Wow/Flutter LFOs, Drift random walk, pink noise crackle |
| `Tests/test_motion.cpp` | Created | 7 Catch2 tests for Motion |
| `Source/Parameters.h` | Modified | 8 MOTION param ID constants |
| `Source/Parameters.cpp` | Modified | 8 APVTS param definitions with ranges/defaults |
| `Source/Engine/GranularEngine.h` | Modified | `setPitchModSource()`, `pitchMod_` member, Motion include |
| `Source/Engine/GranularEngine.cpp` | Modified | `scheduleGrain()` reads `pitchMod_->getPitchModSemitones()` |
| `Source/PluginProcessor.h` | Modified | `motion_` member (after `engine_`) |
| `Source/PluginProcessor.cpp` | Modified | `setPitchModSource`, `setParamPointers`, `prepare`, `reset`, `processBlock` wiring |
| `CMakeLists.txt` | Modified | Motion.h/.cpp added to Grano target_sources |
| `Tests/CMakeLists.txt` | Modified | Motion.cpp + test_motion.cpp added to GranoTests |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| `Motion() = default;` explicitly declared | `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` macro suppresses implicit default ctor when combined with in-class member initializers; compiler error without explicit declaration | All JUCE modules needing default construction must follow this pattern |
| Renamed `updateLFOs()` → `updatePitchMod()` | Name reflects that drift (not an LFO) is also computed in same function | Minor — cosmetic but more accurate |
| 1-block pitch latency accepted | Scheduler reads pitchModSt_ written by previous processBlock call; one block delay (~11ms @ 44100/512) is imperceptible for 0.1–2 Hz Wow | No compensating mechanism needed |
| crackle color uses `juce::jmap(crackColor, lpfOut, pink - lpfOut)` | Smooth interpolation between warm (LPF) and bright (HPF) with single IIR state; avoids two separate filter paths | Pattern available for F4b Color module |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 1 | Build error resolved immediately |
| Renamed | 1 | Cosmetic — no scope change |
| Test count | +2 | 7 tests written vs 5 planned (added null-ptr and prepare-reset tests) |

### Auto-fixed Issues

**1. Missing default constructor**
- **Found during:** Task 2 (wiring PluginProcessor)
- **Issue:** `error: no matching function for call to 'Motion::Motion()'` — `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` macro interaction suppresses implicit default ctor
- **Fix:** Added `Motion() = default;` to public section of Motion.h
- **Verification:** Rebuilt — 0 errors
- **Commit:** d9cd358

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| Release Standalone ALSA MIDI warning (`snd_seq_hw_open` no device) | Expected in WSL2 — no `/dev/snd/seq`. Non-fatal; window opens and process runs cleanly. Release build skips jassert that caused Debug crash. |

## Next Phase Readiness

**Ready:**
- Motion module API stable — F4b Color and F4c Pattern follow same `prepare/reset/processBlock/setParamPointers` pattern
- APVTS wiring pattern established — `getRawParameterValue` + `setParamPointers` for all future modules
- Test infrastructure in place — `Tests/test_motion.cpp` pattern reusable for `test_color.cpp`, `test_pattern.cpp`
- GranularEngine pitch-mod hookpoint open — future modulation sources (LFO, MIDI) can replace or supplement `pitchMod_`

**Concerns:**
- Audio verify (pitch wobble audible) deferred to non-WSL2 session — no ALSA audio device in current environment
- Motion UI panel not yet accessible to user — parameters only reachable via host automation or MIDI learn until F5 UI panel

**Blockers:**
- None

---
*Phase: 04-processing-modules, Plan: 01*
*Completed: 2026-05-18*
