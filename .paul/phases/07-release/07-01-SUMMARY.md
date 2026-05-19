---
phase: 07-release
plan: 01
subsystem: ci
tags: [pluginval, auval, ci, github-actions, codesigning, catch2, msvc]

requires:
  - phase: F6
    provides: complete plugin build (VST3 + AU + Standalone)
provides:
  - pluginval at strictness-level 10 on Win + macOS in CI
  - engine null-weight default fixed (equal weight when APVTS not wired)
  - MSVC Catch2 atomic<float> comparison fixed
  - Timing-robust GranularEngine integration tests

affects: [F7-02, F7-03]

tech-stack:
  added: []
  patterns:
    - "codesign --sign - (ad-hoc) cannot satisfy macOS 14+ requirement for AU loading in auval"
    - "std::atomic<float>: must call .load() before Catch2 REQUIRE on MSVC"
    - "null pSlotWeight_ defaults to 1.0f (equal weight) so headless/test engine still picks loaded slots"

key-files:
  created: []
  modified:
    - .github/workflows/ci.yml
    - Tests/test_modulation.cpp
    - Source/Engine/GranularEngine.cpp
    - Tests/test_sample_io.cpp

key-decisions:
  - "auval removed from CI: macOS 14+ requires Developer ID; deferred to F7-03 signing pass"
  - "null slot weight defaults 1.0f not 0.0f: avoids silent failure when engine used without APVTS"
  - "test_sample_io timing: 400ms sleep + 50 blocks for reliable grain capture on loaded CI runners"

patterns-established:
  - "Integration tests that rely on scheduler thread: sleep >= 4× max grain interval, measure >= 50 blocks"

duration: ~3h (multi-push CI iteration)
started: 2026-05-19T00:00:00Z
completed: 2026-05-19T00:00:00Z
---

# Phase 7 Plan 01: CI Validation Hardening Summary

**pluginval bumped to strictness-10 on Win + macOS; auval removed (macOS 14+ requires Developer ID); three auto-fixed bugs discovered during CI iteration.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~3h (4 CI pushes to converge) |
| Tasks | 2 (T1 auto + T2 human-verify) |
| Files modified | 4 |
| Pushes to CI | 5 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: pluginval strictness 10 (Win + macOS) | **Pass** | Both platforms at level 10, timeout 60000ms |
| AC-2: auval step on macOS CI | **Fail → Deferred** | macOS Sequoia (15) won't load unsigned AU; ad-hoc sign insufficient; removed and deferred to F7-03 |
| AC-3: CI green on push | **Pass** | All jobs green after fixes; 117/117 tests passing |

## Accomplishments

- pluginval `--strictness-level 10` on Windows and macOS — primary validation gate green
- Fixed MSVC Catch2 bug: `std::atomic<float>` deref in `REQUIRE` fails without `.load()`
- Fixed engine bug: null `pSlotWeight_` defaulted to `0.0f`, silently excluding loaded slots from `pickSlot` when no APVTS wired (affected headless / test usage)
- Hardened two timing-sensitive integration tests: `400ms sleep + 50 blocks` vs `150ms + 20 blocks`

## Task Commits

| Task | Commit | Type | Description |
|------|--------|------|-------------|
| T1: ci.yml pluginval 5→10 + auval | `11143d1` | ci | bump pluginval strictness-10; add auval step |
| Auto-fix: MSVC Catch2 atomic | `28dfdd2` | fix | .load() for atomic<float> in MSVC Catch2 assertion |
| Auto-fix: engine null weight | `b0dd901` | fix | default slot weight 1.0 when param not wired |
| Auto-fix: test timing | `484d976` | test | 400ms sleep + 50 blocks for robust CI timing |
| auval attempt: ad-hoc sign | `3e0b544` | ci | codesign --sign - (unsuccessful on macOS 15) |
| auval removed | `ddf5536` | ci | remove auval — requires Developer ID on macOS 14+ |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `.github/workflows/ci.yml` | Modified | pluginval 5→10; timeout 30000→60000; auval added then removed |
| `Tests/test_modulation.cpp` | Modified | `.load()` on `std::atomic<float>` for MSVC Catch2 compat |
| `Source/Engine/GranularEngine.cpp` | Modified | null `pSlotWeight_` default `0.0f` → `1.0f` |
| `Tests/test_sample_io.cpp` | Modified | `sleep(150→400)`, `20→50` blocks in two engine integration tests |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| auval removed from CI | macOS Sequoia refuses to load unsigned AU bundles; ad-hoc `codesign --sign -` insufficient; no Developer ID available in CI | AU format validation deferred to F7-03 signing pass or first signed release |
| null slot weight defaults 1.0f | Equal weight is the correct headless semantic — all loaded slots available; 0.0f silently excludes everything | Engine now usable without APVTS (tests, standalone headless) |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 3 | Essential bug fixes discovered via CI failures |
| Scope removal | 1 | auval removed; macOS 14+ codesigning blocker |

**Total impact:** Scope narrowed (no auval), but three pre-existing bugs caught and fixed.

### Auto-fixed Issues

**1. MSVC: `std::atomic<float>` in Catch2 `REQUIRE`**
- Found during: T2 Windows CI run
- Issue: `REQUIRE(*apvts.getRawParameterValue(...) == Approx(0.0f))` — MSVC can't decompose `std::atomic<float>&` in Catch2 expression decomposer; `Approx` undeclared without `Catch::` prefix
- Fix: `REQUIRE(apvts.getRawParameterValue(...)->load(std::memory_order_relaxed) == 0.0f)`
- Files: `Tests/test_modulation.cpp:214`

**2. Engine: null slot weight 0.0f excludes loaded slots**
- Found during: T2 macOS CI — test #103 `GranularEngine uses SampleBuffer source when loaded` always failed
- Issue: `pSlotWeight_[s]` null → weight `0.0f` → `pickSlot` returns null → no source → engine silent even with slot loaded
- Fix: default to `1.0f` (equal weight); empty slots excluded by `len > 0` guard in `pickSlot`
- Files: `Source/Engine/GranularEngine.cpp:113`

**3. Test timing: scheduler thread not scheduled during fast processBlock loop**
- Found during: T2 macOS CI — tests #103 and #104 failing intermittently
- Issue: `sleep(150ms)` + 20 fast processBlocks ran entirely before scheduler thread could fire grains; Uniform distribution wait up to 150ms, measurements ran in ~5ms wall time
- Fix: `sleep(400ms)` guarantees 2-4 grains in FIFO; 50 blocks ensures grains processed
- Files: `Tests/test_sample_io.cpp`

### Deferred Items

- **auval CI validation**: requires Developer ID certificate; deferred to F7-03 signing pass

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| auval exit 2 on macOS Sequoia despite ad-hoc signing | Component found but couldn't load; `codesign --sign -` insufficient; step removed |
| test #103 failing even after weight fix | Separate timing issue; 150ms sleep insufficient on loaded CI runners |

## Next Phase Readiness

**Ready:**
- CI green on Win + macOS with pluginval-10 at every push
- 117/117 tests passing
- Release zip job ready for tag push
- Engine null-weight bug fixed (cleaner test isolation going forward)

**Concerns:**
- AU format not validated in CI; auval must run before real release

**Blockers:**
- None for F7-02 (presets + README + RELEASE_NOTES)
- F7-03: need Developer ID or Notarization Service for auval + actual codesigning

---
*Phase: 07-release, Plan: 01*
*Completed: 2026-05-19*
