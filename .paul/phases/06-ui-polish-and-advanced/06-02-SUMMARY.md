---
phase: 06-ui-polish-and-advanced
plan: 02
subsystem: engine
tags: [SubGrain, StochasticTiming, GranularEngine, recursion, distributions, parameters]

requires:
  - phase: 06-01
    provides: layout stable, 93/93 tests, APVTS 66 params

provides:
  - SubGrain.{h,cpp}: depth 0/1/2 recursion, pool-backed, scheduler thread only
  - StochasticTiming.{h,cpp}: 6 distributions (Uniform/Gaussian/Poisson/Exponential/Pareto/1/f)
  - APVTS params: subGrainDepth (int 0–2), stochasticDist (choice 0–5) — total 68
  - GranularEngine: StochasticTiming in SchedulerThread::run(), SubGrain in scheduleGrain()
  - 8 new Catch2 tests (4 SubGrain, 4 StochasticTiming) — 101/101

affects: [F6-03, F6-04, F6-05]

tech-stack:
  added:
    - Source/Engine/SubGrain.{h,cpp}
    - Source/Engine/StochasticTiming.{h,cpp}
  patterns:
    - "parentCopy = *g before FIFO finishedWrite — audio thread may consume slot immediately"
    - "prepareToWrite(maxExtra) + spawnInto() + finishedWrite(written) — n2 wrap omitted at normal densities"
    - "pSubGrainDepth_/pStochasticDist_ atomic pointers via setAdvancedParamPointers()"

key-files:
  created:
    - Source/Engine/SubGrain.h
    - Source/Engine/SubGrain.cpp
    - Source/Engine/StochasticTiming.h
    - Source/Engine/StochasticTiming.cpp
    - Tests/test_subgrain.cpp
    - Tests/test_stochastic.cpp
  modified:
    - Source/Engine/GranularEngine.h
    - Source/Engine/GranularEngine.cpp
    - Source/Parameters.h
    - Source/Parameters.cpp
    - Source/PluginProcessor.cpp
    - CMakeLists.txt
    - Tests/CMakeLists.txt
    - Tests/test_modulation.cpp

key-decisions:
  - "parentCopy of Grain taken before FIFO finishedWrite — audio thread owns slot immediately after"
  - "n2 FIFO wrap-around omitted for sub-grain spawn — prepareToWrite(maxExtra) fits in n1 at normal densities"
  - "StochasticTiming::Poisson implemented as Exponential inter-arrival (mathematically correct for Poisson process)"
  - "Pareto clamp tolerance widened to 20% in test — analytical E[X_clamped] = 0.83×mean at 5× ceiling"

duration: ~1.5h
started: 2026-05-19T00:00:00Z
completed: 2026-05-19T00:00:00Z
---

# Phase 6 Plan 02: Sub-grain + Stochastic Timing Summary

**SubGrain depth 0/1/2 recursion + 6 stochastic inter-grain timing distributions — pure engine additions; 101/101 tests; ab1f039.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~1.5h |
| Tasks | 3 auto + 1 checkpoint |
| Tests | 101/101 pass (8 new) |
| Files created | 6 |
| Files modified | 8 |
| Commit | ab1f039 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-0: Build and existing tests pass | Pass | 93→101 tests; all targets build clean |
| AC-1: SubGrain depth 0 transparent | Pass | depth=0 returns 0 extra grains; test #94 |
| AC-2: SubGrain depth 1 spawns 2–4 sub-grains | Pass | bounds + position constraints; test #95 |
| AC-3: SubGrain depth 2 within bounds | Pass | max 4×(1+3)=16 slots; test #96 |
| AC-4: StochasticTiming distributions statistically correct | Pass | mean ±20% (Pareto bias), positive, clamped; tests #98–101 |
| AC-5: No audio-thread regression | Pass | parentCopy taken before finishedWrite; all engine tests pass |
| AC-6: Human listening verify | Pass | Standalone launches, audio plays, 101/101; user approved |

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| T1–T4 | `ab1f039` | feat(engine): sub-grain recursion + stochastic timing distributions (F6b) |

## Files Created

| File | Purpose |
|------|---------|
| `Source/Engine/SubGrain.h` | Depth 0/1/2 spawn interface |
| `Source/Engine/SubGrain.cpp` | spawnOne() helper + spawnInto() recursion |
| `Source/Engine/StochasticTiming.h` | 6-distribution enum + nextIntervalMs() |
| `Source/Engine/StochasticTiming.cpp` | Box-Muller Gaussian, Exponential, Pareto (α=1.5), Voss 1/f |
| `Tests/test_subgrain.cpp` | 4 TEST_CASEs: depth 0/1/2/cap |
| `Tests/test_stochastic.cpp` | 4 TEST_CASEs: positive/mean/tail/clamp |

## Files Modified

| File | Change |
|------|--------|
| `Source/Engine/GranularEngine.h` | Includes SubGrain/StochasticTiming; setAdvancedParamPointers(); pSubGrainDepth_/pStochasticDist_ atomics |
| `Source/Engine/GranularEngine.cpp` | SchedulerThread::run() uses StochasticTiming; scheduleGrain() parentCopy + SubGrain::spawnInto() |
| `Source/Parameters.h` | Added subGrainDepth + stochasticDist IDs |
| `Source/Parameters.cpp` | Added AudioParameterInt(subGrainDepth) + AudioParameterChoice(stochasticDist) |
| `Source/PluginProcessor.cpp` | Calls setAdvancedParamPointers() after setParamPointers() |
| `CMakeLists.txt` | SubGrain.cpp + StochasticTiming.cpp added to Grano target |
| `Tests/CMakeLists.txt` | SubGrain.cpp + StochasticTiming.cpp + test_subgrain.cpp + test_stochastic.cpp added |
| `Tests/test_modulation.cpp` | APVTS count updated 66→68; spot-checks for new param IDs added |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| parentCopy before finishedWrite | Audio thread can consume FIFO slot immediately after finishedWrite(); parentCopy is stack-local and safe | Required for correct sub-grain spawning |
| n2 FIFO wrap omitted | prepareToWrite(maxExtra) fits in n1 at normal grain densities; wrap-around adds complexity for negligible benefit | Sub-grains silently skipped if FIFO wraps — acceptable at v1.0 |
| Poisson = Exponential inter-arrival | Correct definition: inter-arrival time for a Poisson process is Exp(λ) | Name is user-facing; implementation is statistically exact |
| Pareto test tolerance 20% | E[X_clamped] = 0.83×mean analytically for α=1.5, clamp at 5×mean — not a bug | Documented in test comment; all other distributions pass 15% |

## Deviations from Plan

| Type | Description |
|------|-------------|
| Minor spec deviation | Test tolerance widened 15%→20% — Pareto's analytical clamped mean is 82.8 for mean=100 at 5× ceiling; not implementation error |
| Minor T1 deviation | param count test in test_modulation.cpp (not test_parameters.cpp) updated — found via grep during qualify |

## Next Phase Readiness

**Ready for F6 plan 03 (Feedback Path + Spectral Processor):**
- SubGrain depth max=2 in pool (worst: 1 + 4 + 12 = 17 grains per trigger; pool cap 1024)
- StochasticTiming active at all depths > 0 — textural variation confirmed audible
- 101/101 tests; no regressions; 68 APVTS params
- FeedbackPath + SpectralProcessor classes needed: Source/Engine/{FeedbackPath,SpectralProcessor}.{h,cpp}

**Remaining deferred:**
- Sub-grain UI controls (currently depth=0 default; no panel to change it)
- Stereo file downmix (F2)
- Embedded fonts (F3)
- Drawable LFO drag-editor (F5c)
- 250ms snapshot ramp (F5c)
- Color/Motion/Pattern mod destinations (F5b)

**Blockers:** None.

---
*Phase: 06-ui-polish-and-advanced, Plan: 02*
*Completed: 2026-05-19*
