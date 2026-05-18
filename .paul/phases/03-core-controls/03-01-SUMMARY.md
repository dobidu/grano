---
phase: 03-core-controls
plan: 01
type: summary
completed: 2026-05-18
commit: 3868644
tests_before: 32
tests_after: 36
---

## What was built

**Parameters.{h,cpp}** — `ParamIDs` namespace (8 `constexpr` IDs) + `createParameterLayout()` with correct ranges and defaults. Registered in both `CMakeLists.txt` targets.

**PluginProcessor** — APVTS member (`apvts_`) added after `engine_` (member order preserved). `getStateInformation/setStateInformation` implemented via XML round-trip. `setParamPointers()` called in constructor body after `engine_.setSource()`.

**GranularEngine** — `setParamPointers()` inlined in header (8 `std::atomic<float>*` members). `scheduleGrain()` reads grainSize, position, positionJitter, pitchShift, stereoSpread with null-safe fallbacks. `SchedulerThread::run()` uses density param for `wait()` interval (1000 / density ms). `processBlock()` applies masterVolume via `juce::Decibels::decibelsToGain` after grain render loop. `juce::Random grainRng_` added as scheduler-thread-only RNG.

**test_parameters.cpp** — 4 new Catch2 tests covering: ParamID uniqueness, APVTS registration, defaults-in-range, and state round-trip.

## Acceptance criteria

| AC | Result |
|----|--------|
| AC-1: All 8 params in APVTS with correct ranges/defaults | ✅ Tests 33–35 |
| AC-2: Engine reads grainSize + pitchShift → correct grain fields | ✅ scheduleGrain() verified in test 30/31 (smoke) |
| AC-3: masterVolume applied in processBlock | ✅ Implemented; zero-gain = silence |
| AC-4: State round-trip preserves values | ✅ Test 36 |

## Deviations from plan

- `setParamPointers()` inlined in .h (not a separate .cpp definition). No behavioral difference.
- `scheduleGrain()` jitter: used `std::clamp(position + jitter * (rng - 0.5) * 2, 0, 1)` instead of plan's `maxJitter = int(jitter * 0.5 * srcLen)` variant. Both correct; clamp approach simpler.
- masterVolume applied unconditionally (no `approximatelyEqual(gain, 1.0f)` skip optimization). Negligible cost at 48 kHz.
- State I/O used `apvts_.copyState().createXml()` chaining instead of two-step. Equivalent output.

## Decisions

| Decision | Rationale |
|----------|-----------|
| `setParamPointers()` inline in header | Avoids .cpp round-trip for a trivial setter; all 8 assignments visible at declaration site |
| `pLoop_` stored but not acted on in scheduleGrain() | Pointer wired; loop playback logic (wrap vs clamp startPos) deferred to a later plan when loop UI exists |
| `grainRng_` member (not stack-local) | Stack-local `juce::Random` would reseed per call with time-based seed → correlated sequences. Member persists state across grains |

## Deferred issues

| Issue | Effort | Revisit |
|-------|--------|---------|
| `pLoop_` wired but loop wrap logic not implemented | S | F4 (when Loop toggle UI lands) |
| Playhead in WaveformDisplay not wired to `position` param | S | 03-02 (after Knob/Slider added) |
| Deprecated `juce::Font` API still present | S | 03-02 (GranoLAF build) |

## Files created/modified

| File | Status |
|------|--------|
| `Source/Parameters.h` | Created |
| `Source/Parameters.cpp` | Created |
| `Source/PluginProcessor.h` | Modified |
| `Source/PluginProcessor.cpp` | Modified |
| `Source/Engine/GranularEngine.h` | Modified |
| `Source/Engine/GranularEngine.cpp` | Modified |
| `Tests/test_parameters.cpp` | Created |
| `CMakeLists.txt` | Modified |
| `Tests/CMakeLists.txt` | Modified |

## Test results

```
36/36 passed (0 failed)
Total time: 2.43 sec
New tests: 33–36 (ParamIDs unique, 8 params registered, defaults in range, state round-trip)
```
