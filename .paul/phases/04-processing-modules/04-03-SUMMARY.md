# SUMMARY — 04-03: Multi-platform CI & build hardening

**Completed:** 2026-05-18
**Commit:** 1d0d11f

## Acceptance criteria results

- [x] JUCE version defined once (`env.JUCE_VERSION: "8.0.7"`); all 3 jobs use it
- [x] No `--branch master` remaining for JUCE clones
- [x] macOS CI configure step: `-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0`
- [x] CMakeLists.txt: `CMAKE_OSX_DEPLOYMENT_TARGET=11.0` default for local macOS builds
- [x] CMakeLists.txt: `/utf-8` for MSVC
- [x] Both Linux jobs (linux-best-effort + tsan) have JUCE cache + conditional clone
- [x] Linux build steps use `-j$(nproc)`
- [ ] CI green on push — **pending** (user must push and verify Actions tab)

## Files modified

| File | Change |
|------|--------|
| `CMakeLists.txt` | +8 lines: macOS deployment target 11.0, MSVC /utf-8 |
| `.github/workflows/ci.yml` | +30 lines: JUCE_VERSION env var, pinned tag, macOS target, Linux cache ×2, -j$(nproc) ×1 |

## Decisions

- JUCE pinned to `8.0.7` — known-good JUCE 8 stable tag as of plan date. Update `JUCE_VERSION` in ci.yml when upgrading.
- macOS 11.0 target chosen: first Apple Silicon release, satisfies Xcode 15 App Store requirements.
- Linux JUCE caching uses same key format as `build` job — consistent across all 3 jobs.

## Deferred

- Verify CI green (Task 6 checkpoint) — user action required: `git push && watch Actions tab`
- Consider upgrading to `ubuntu-24.04` in a future plan (JUCE deps are stable on 22.04 for now)
