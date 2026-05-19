---
phase: 05-modulation-matrix
plan: 02
subsystem: modulation
tags: [mod-matrix, lfo, cross-mod, routing, apvts, atomic, grain-engine]

requires:
  - phase: 05-01
    provides: Lfo class, advanceSample() API, 42 APVTS params

provides:
  - ModMatrix class (Source/Modulation/ModMatrix.{h,cpp})
  - 24 new APVTS params (slot1Source..slot8Amount) → 66 total
  - Lfo::setRateModOffset() for LFO1→LFO2Rate cross-modulation
  - GranularEngine reads kPitchShift (×24 st) + kPositionJitter offsets
  - PluginProcessor wires lfo1_, lfo2_, modMatrix_; processBlock chain updated
  - 6 new ModMatrix Catch2 tests → 91 total

affects: [F5-snapshots, F5-ui, F6-engine]

tech-stack:
  added: [ModMatrix 8-slot routing, slotParamID() runtime helper]
  patterns:
    - "modMatrix_.processBlock() before engine_.processBlock() — mod values ready when scheduler fires"
    - "accum[] stack array → store atomic — no alloc, no lock in hot path"
    - "explicit ModMatrix() = default needed when JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR present (GCC implicit suppression)"

key-files:
  created: [Source/Modulation/ModMatrix.h, Source/Modulation/ModMatrix.cpp]
  modified:
    - Source/Modulation/Lfo.h
    - Source/Modulation/Lfo.cpp
    - Source/Parameters.h
    - Source/Parameters.cpp
    - Source/Engine/GranularEngine.h
    - Source/Engine/GranularEngine.cpp
    - Source/PluginProcessor.h
    - Source/PluginProcessor.cpp
    - Tests/test_modulation.cpp
    - CMakeLists.txt
    - Tests/CMakeLists.txt

key-decisions:
  - "explicit ModMatrix() = default — GCC suppresses implicit default ctor when JUCE_DECLARE_NON_COPYABLE present + non-trivial members"
  - "accum[kNumDests] stack float array, then atomic store — avoids any alloc/lock on audio thread"
  - "LFO1 advanced before LFO2 in processBlock — one-block cross-mod latency, acceptable for music"
  - "lfo2RateMod scale = amt * 8000.0f — maps [-1,1] to [-8000,+8000] Hz offset range"
  - "slotParamID() runtime helper instead of 24 constexpr IDs for loop-based APVTS registration"

patterns-established:
  - "ModMatrix owns both LFO advances; PluginProcessor does NOT call lfo1_.processBlock() separately"
  - "Forward-declare ModMatrix in GranularEngine.h; full include only in .cpp to minimize rebuild cascade"
  - "juce::jlimit(0,1) on posJitter after adding mod offset — clamped before grain scheduling"

duration: ~1h
started: 2026-05-18T00:00:00Z
completed: 2026-05-18T00:00:00Z
---

# Phase 5 Plan 02: ModMatrix + Engine Integration Summary

**8-slot mod routing matrix implemented — LFO1/LFO2 route to 29 destinations including cross-mod; GranularEngine consumes pitch and jitter offsets; 42→66 APVTS params; 85→91 tests.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~1h |
| Tasks | 3 completed |
| Tests added | 6 (ModMatrix) |
| Total tests | 91/91 pass |
| Files created | 2 |
| Files modified | 11 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: LFO1→kPitchShift non-zero | Pass | Square LFO gives deterministic ±1 output; 20-block loop detects maxAbs > 0.5 |
| AC-2: Bypass → zero offset | Pass | All slots None → all getModOffset() calls return 0 |
| AC-3: Amount linear scaling | Pass | amount=0.5 gives exactly half of amount=1.0 output |
| AC-4: Cross-mod LFO2Rate | Pass | 5 blocks of LFO1→kLfo2Rate; LFO2 produces >10 zero-crossings in 1000 samples |
| AC-5: Engine reads pitch mod | Pass | scheduleGrain() adds modMatrix_->getModOffset(kPitchShift) × 24 st |
| AC-6: 66 APVTS params | Pass | proc.getParameters().size() == 66 |
| AC-7: Null LFOs safe | Pass | nullptr LFOs → 0.0f from all getModOffset() calls, no crash |

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| Tasks 1-3 (bundled) | `281378d` | feat(modulation): add ModMatrix + LFO wiring (F5b) |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `Source/Modulation/ModMatrix.h` | Created | 8-slot matrix, Dest enum (29 entries), modSums_ atomics |
| `Source/Modulation/ModMatrix.cpp` | Created | processBlock: LFO advance → cross-mod → accum → atomic store |
| `Source/Modulation/Lfo.h` | Modified | Added setRateModOffset() + rateModOffset_ atomic member |
| `Source/Modulation/Lfo.cpp` | Modified | effectiveRate += rateModOffset_, clamped ≥ 0.01 Hz |
| `Source/Parameters.h` | Modified | 24 slot param constants + slotParamID() helper decl |
| `Source/Parameters.cpp` | Modified | 24 slot params (loop × 8) + slotParamID() impl |
| `Source/Engine/GranularEngine.h` | Modified | setModMatrixSource(), ModMatrix* modMatrix_ (forward decl) |
| `Source/Engine/GranularEngine.cpp` | Modified | scheduleGrain() reads kPitchShift + kPositionJitter offsets |
| `Source/PluginProcessor.h` | Modified | lfo1_, lfo2_, modMatrix_ members; Lfo+ModMatrix includes |
| `Source/PluginProcessor.cpp` | Modified | LFO param wiring, slot param wiring, processBlock chain |
| `CMakeLists.txt` | Modified | ModMatrix.cpp added to Grano target_sources |
| `Tests/CMakeLists.txt` | Modified | ModMatrix.cpp + test_modulation.cpp expanded |
| `Tests/test_modulation.cpp` | Modified | 6 new ModMatrix tests; AC-8 updated 42→66 |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| `ModMatrix() = default` explicit | GCC suppresses implicit default ctor when JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR present and class has non-trivial members — Lfo had same macro but different member layout | Required fix; pattern to document for F6 classes |
| LFO advance in ModMatrix::processBlock (not PluginProcessor) | ModMatrix owns the LFO advance cycle; prevents double-advance bugs | PluginProcessor never calls lfo1_.processBlock() separately |
| lfo2RateMod *= 8000.0f scale | Maps [-1,1] LFO depth to ±8 kHz rate range; covers full audio-rate LFO spectrum | Cross-mod can be extreme — UI will need range display in 05-03 |

## Deviations from Plan

| Type | Description |
|------|-------------|
| Bug fix | ModMatrix() implicit default ctor suppressed by GCC — required explicit `= default` declaration |
| Test update | AC-8 test updated from 42 to 66 param count (expected change) |

## Next Phase Readiness

**Ready for 05-03 (Snapshots + UI panels):**
- ModMatrix::getModOffset() API stable — LfoPanel + ModulationMatrixView can read live values
- Both LFO instances live on PluginProcessor with full APVTS wiring
- 29-destination Dest enum canonical and frozen (matches Parameters.cpp kModDests order)
- 91/91 tests passing; 66 APVTS params

**Deferred:**
- Color/Motion/Pattern mod destinations stored in modSums_ but NOT consumed by their modules yet (F6 scope)
- kGrainSize, kDensity, kMasterVol etc. have modSums entries but engine ignores them until F6

**Blockers:** None.

---
*Phase: 05-modulation-matrix, Plan: 02*
*Completed: 2026-05-18*
