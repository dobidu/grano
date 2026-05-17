---
phase: 01-granular-engine
plan: 01
type: summary
completed: 2026-05-17
commit: 845013e
---

# SUMMARY — F1-01: Engine Data Structures

## What was built

Three RT-safe data structures forming the foundation of the granular engine:

| File | Description |
|------|-------------|
| `Source/Engine/EnvelopeShapes.h+cpp` | 5 window functions: Hann, Tukey (α=0.5), Gaussian (σ=0.2), Triangle, Rectangle |
| `Source/Engine/Grain.h` | Trivially-copyable grain descriptor struct (no heap members) |
| `Source/Engine/GrainPool.h+cpp` | 1024-slot lock-free pool, CAS acquire + hint scan, single-store release |
| `Tests/test_engine.cpp` | 14 Catch2 test cases (envelope math, pool invariants, grain fields) |
| `CMakeLists.txt` | Added EnvelopeShapes.cpp + GrainPool.cpp to plugin target |
| `Tests/CMakeLists.txt` | Added engine sources + test_engine.cpp to GranoTests target |

## Acceptance criteria

| AC | Result | Notes |
|----|--------|-------|
| AC-1: 5 correct window functions | ✅ | All shapes tested for range, boundary, midpoint, symmetry |
| AC-2: GrainPool manages 1024 slots, no heap | ✅ | Overflow → nullptr, slot reuse, distinct pointers verified |
| AC-3: Grain trivially copyable, fixed size | ✅ | `static_assert` + runtime REQUIRE both pass |
| AC-4: All Catch2 tests pass | ✅ | 14/14 pass; no regressions on existing "Plugin name is Grano" test |

## Deviations from plan

| Item | Plan | Actual | Reason |
|------|------|--------|--------|
| Gaussian boundary tolerance | `< 0.01f` | `< 0.1f` | σ=0.2 bell curve has boundary value ≈ 0.044 (exp(-0.5 × 2.5²)); not a code bug, test spec was too strict |

## Decisions made

- **Gaussian σ=0.2** (not σ=0.4 as originally spec'd in plan): boundary value ≈ 0.044, not near-zero. Separated Gaussian from strict-boundary test to reflect mathematical reality.
- **`jassert` not `assert`** in GrainPool::release: compiles out in Release build — correct for RT code.
- **CAS hint pattern**: `hint_` advances to `(idx+1) % Capacity` on each acquire, giving O(1) typical cost.

## Files created/modified

```
Source/Engine/EnvelopeShapes.h      [new]
Source/Engine/EnvelopeShapes.cpp    [new]
Source/Engine/Grain.h               [new]
Source/Engine/GrainPool.h           [new]
Source/Engine/GrainPool.cpp         [new]
Tests/test_engine.cpp               [new]
CMakeLists.txt                      [modified — 2 new sources]
Tests/CMakeLists.txt                [modified — test_engine.cpp + 2 engine sources]
```

## What's next

Plan 01-02: GranularEngine — FIFO + ThreadPool + processBlock + hard-coded sample playback + TSAN CI.
