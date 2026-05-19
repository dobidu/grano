---
phase: 06-ui-polish-and-advanced
plan: 03
subsystem: engine
tags: [feedback, spectral, fft, juce-dsp, ring-buffer, rt-safe]

requires:
  - phase: 06-02
    provides: SubGrain recursion + StochasticTiming wired into engine; 101 tests passing

provides:
  - FeedbackPath RT-safe ring buffer (4 s @ 48 kHz) — audio output re-injected as grain source
  - SpectralProcessor 2048-pt FFT Freeze/Blur — alternate grain source, worker-thread only
  - Source priority chain in scheduleGrain(): Spectral > Feedback > SampleBuffer > sine
  - 6 new APVTS params (74 total): feedbackEnabled, feedbackGain, feedbackDamp, spectralEnabled, spectralMode, spectralBlurAmount
  - 9 Catch2 tests (5 feedback, 4 spectral); 110/110 total

affects: [06-04-multisample, 06-05-ui-integration, F7-release]

tech-stack:
  added: [juce::dsp::FFT (order 11), juce::Random]
  patterns:
    - RT-safe ring buffer with atomic write position (std::array + std::atomic<int>)
    - Worker-thread-only FFT processing; audio thread reads pre-computed output via getReadPointer()
    - Source priority via nullptr + atomic-bool checks, no branching overhead on audio thread

key-files:
  created:
    - Source/Engine/FeedbackPath.h
    - Source/Engine/FeedbackPath.cpp
    - Source/Engine/SpectralProcessor.h
    - Source/Engine/SpectralProcessor.cpp
    - Tests/test_feedback.cpp
    - Tests/test_spectral.cpp
  modified:
    - Source/Engine/GranularEngine.h
    - Source/Engine/GranularEngine.cpp
    - Source/Parameters.h
    - Source/Parameters.cpp
    - Source/PluginProcessor.h
    - Source/PluginProcessor.cpp
    - CMakeLists.txt
    - Tests/CMakeLists.txt
    - Tests/test_modulation.cpp

key-decisions:
  - "JUCE performRealOnlyInverseTransform normalises by 1/N internally — do NOT divide again"
  - "feedbackGain hard-clamped to 0.95 in process() — prevents divergence without exposing param limit"
  - "LP filter: lpState = lpState*(1-k) + mono*k where k=1-clamp(damp); damp=1 → passthrough, damp=0 → DC"
  - "triggerSpectralProcessIfEnabled() on sample load path — spectral stays synced to current source"
  - "setLatencySamples(kFFTSize) when spectralEnabled, 0 when disabled"

patterns-established:
  - "Alternate grain sources follow SampleBuffer interface: getReadPointer()/getNumSamples()"
  - "Source priority via const bool flags in scheduleGrain(); order is comment-documented"

duration: 1 session (2026-05-19)
started: 2026-05-19T00:00:00Z
completed: 2026-05-19T00:00:00Z
---

# Phase 6 Plan 03: FeedbackPath + SpectralProcessor Summary

**Internal feedback ring buffer and FFT spectral processor wired as alternate grain sources with 74 APVTS params and 110/110 Catch2 tests passing.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | 1 session |
| Started | 2026-05-19 |
| Completed | 2026-05-19 |
| Tasks | 4 completed (T1–T4) |
| Files created | 6 |
| Files modified | 9 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-0: Build and tests pass | Pass | 110/110 (was 101; +9 new tests) |
| AC-1: FeedbackPath RT-safe and stable | Pass | No alloc/locks; gain hard-clamped 0.95; 10000-block divergence test passes |
| AC-2: FeedbackPath as grain source | Pass | feedbackEnabled → scheduleGrain uses FB buffer |
| AC-3: SpectralProcessor Freeze non-silent | Pass | RMS > 1e-4 confirmed in test |
| AC-4: SpectralProcessor Blur phase-varies | Pass | diffRms > 1e-3 (two runs); magnitude within ±20% |
| AC-5: Source priority chain | Pass | Spectral > Feedback > SampleBuffer > sine; tested via unit + human verify |
| AC-6: Latency reporting | Pass | prepareToPlay() sets setLatencySamples(2048) / 0 |
| AC-7: Human listening verify | Pass | User confirmed "all working, beautiful" on Windows binary |

## Accomplishments

- FeedbackPath: pre-allocated 4 s ring buffer, one-pole LP filter, gain hard-clamp 0.95, atomic write position — fully RT-safe.
- SpectralProcessor: juce::dsp::FFT order 11 (2048-pt), Hann window, Freeze (zero-phase) and Blur (random-phase + magnitude jitter) modes, called from message/worker thread only.
- GranularEngine scheduleGrain() priority chain: Spectral > Feedback > SampleBuffer > 440 Hz sine, with null-pointer guards.
- PluginProcessor: wires both sources in constructor; calls feedbackPath_.process() after each engine mix block; triggers spectral re-process on sample load; sets latency samples per spectral state.
- 6 new APVTS params, bringing total to 74.

## Task Commits

| Task | Commit | Type | Description |
|------|--------|------|-------------|
| T1+T2+T3 (combined) | `a1ea272` | feat(engine) | feedback path + spectral processor (F6c) |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `Source/Engine/FeedbackPath.h` | Created | RT-safe ring buffer interface |
| `Source/Engine/FeedbackPath.cpp` | Created | process(): LP filter, gain clamp, ring write |
| `Source/Engine/SpectralProcessor.h` | Created | FFT Freeze/Blur interface |
| `Source/Engine/SpectralProcessor.cpp` | Created | Hann window, FFT, spectral manipulation, IFFT |
| `Source/Engine/GranularEngine.h` | Modified | Added setFeedbackSource(), setSpectralSource(), param setters, private pointers |
| `Source/Engine/GranularEngine.cpp` | Modified | Source priority chain in scheduleGrain() |
| `Source/Parameters.h` | Modified | 6 new param ID constants |
| `Source/Parameters.cpp` | Modified | 6 new APVTS param definitions |
| `Source/PluginProcessor.h` | Modified | FeedbackPath + SpectralProcessor members, triggerSpectralProcessIfEnabled() decl |
| `Source/PluginProcessor.cpp` | Modified | Constructor wiring, processBlock() feedback call, latency reporting, load trigger |
| `CMakeLists.txt` | Modified | Added FeedbackPath.cpp, SpectralProcessor.cpp |
| `Tests/CMakeLists.txt` | Modified | Added FeedbackPath.cpp, SpectralProcessor.cpp, test_feedback.cpp, test_spectral.cpp |
| `Tests/test_feedback.cpp` | Created | 5 tests: capacity, silence, write, stability 10000 blocks, gain clamp |
| `Tests/test_spectral.cpp` | Created | 4 tests: Freeze non-silent, Blur non-silent, Blur variance, size |
| `Tests/test_modulation.cpp` | Modified | Param count updated 68→74; 6 new spot-checks |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| No manual 1/N normalization after IFFT | JUCE's `performRealOnlyInverseTransform` normalises internally; dividing again made output 2048× too quiet | Discovered via test_spectral Blur variance failure; fix: remove the division line |
| feedbackGain clamped in process() not in param range | Hard safety ceiling regardless of param value; prevents edge cases on param bypass | process() uses `std::min(gain, 0.95f)` |
| LP filter coefficient: k = 1 - clamp(damp) | damp=0 → k=1 → passthrough; damp=1 → k=0 → DC block | Consistent with intuitive "damp" semantics |
| triggerSpectralProcessIfEnabled() on sample load | Spectral output stays in sync with loaded source; avoids stale FFT from prior file | Called from loadSampleFile() success path |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 1 | Essential DSP correctness fix |
| Scope additions | 0 | — |
| Deferred | 0 | — |

**Total impact:** One critical bug caught and fixed during T3 verification. No scope creep.

### Auto-fixed Issues

**1. DSP: SpectralProcessor double-normalization**
- **Found during:** T3 (test_spectral Blur variance test)
- **Issue:** Plan template included `output_[i] = fftBuf_[i] * (1.0f / kFFTSize)`. JUCE's `performRealOnlyInverseTransform` already normalises by 1/N internally; the extra division produced output 2048× too quiet (~2.44e-4 amplitude), causing diffRms = 3.15e-4 < threshold 1e-3.
- **Fix:** Removed the multiplication; changed to `output_[i] = fftBuf_[i]`.
- **Files:** `Source/Engine/SpectralProcessor.cpp`
- **Verification:** All 110 tests pass including diffRms > 1e-3.

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| WSL2 Standalone MIDI assertion on launch | Non-critical (no MIDI device present); audio works; user tested on Windows binary instead |

## Next Phase Readiness

**Ready:**
- FeedbackPath and SpectralProcessor implement same interface as SampleBuffer (getReadPointer/getNumSamples) — 06-04 MultiSampleBank can follow identical pattern.
- 74 APVTS params all defined; 06-05 UI integration can bind controls immediately.
- 110/110 tests — clean base for F6d.

**Concerns:**
- Spectral latency (2048 samples / ~42 ms) only reported in prepareToPlay(); does not update dynamically when param toggled at runtime. Acceptable for v1.0; dynamic latency update deferred.
- FeedbackPath reads from audio-thread-written buffer on scheduler thread without synchronisation (aligned float, relaxed atomic pos — same racy-read policy as GrainSnapshot). Fine on x86; may need revision for ARM strict ordering if plugin targets mobile.

**Blockers:** None.

---
*Phase: 06-ui-polish-and-advanced, Plan: 03*
*Completed: 2026-05-19*
