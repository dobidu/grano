---
phase: 06-ui-polish-and-advanced
plan: 01
subsystem: ui
tags: [layout, GranoLAF, WaveformDisplay, LfoPanel, PluginEditor, glow, trails, polish]

requires:
  - phase: 05-03
    provides: LfoPanel, ModulationMatrixView, Snapshots, 66 APVTS params, editor 1000×820

provides:
  - Restructured title bar with GRANO logo top-left and hairline separator
  - kSectionGap=12 between all major sections (was kGap=4)
  - LOOP button moved to header alongside Load; knob row now 5 equal columns
  - GranoLAF 3-pass halo on rotary value arc; glow rim on toggled buttons
  - WaveformDisplay 3-frame trail ring buffer (0.30/0.15 alpha fade)
  - PluginEditor error auto-dismiss (3 s, generation counter)
  - LfoPanel live Hz/kHz rate readout label

affects: [F6-02, F6-03, F6-04, F6-05]

tech-stack:
  added: []
  patterns:
    - "kMargin/kHeaderH in anonymous namespace (shared between paint() and resized())"
    - "Error dismiss via generation counter + juce::Timer::callAfterDelay — both on message thread, no race"
    - "Particle trail ring: timerCallback shifts ring then captures, paint() uses stored frames"

key-files:
  modified:
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp
    - Source/UI/LookAndFeel/GranoLAF.cpp
    - Source/UI/WaveformDisplay.h
    - Source/UI/WaveformDisplay.cpp
    - Source/UI/LfoPanel.h
    - Source/UI/LfoPanel.cpp

key-decisions:
  - "Logo rendered with g.drawText() not GlyphArrangement — letter-spacing dropped until embedded fonts land"
  - "kSectionGap=12 / kItemGap=6 replaces single kGap=4 throughout resized()"
  - "Trail ring capture in timerCallback, not paint() — avoids double-capture; paint always sees consistent frame"

patterns-established:
  - "Shared layout constants (kMargin, kHeaderH) promoted to anonymous namespace so paint() and resized() stay in sync"
  - "Generation-counter pattern for callAfterDelay with possible re-triggers: ++gen, capture gen, check on fire"

duration: ~1h
started: 2026-05-19T00:00:00Z
completed: 2026-05-19T00:00:00Z
---

# Phase 6 Plan 01: UI Polish Summary

**Title bar restructure + GranoLAF halo glows + particle trails + error auto-dismiss + LFO rate readout — all deferred XS/S polish items from F2–F5 shipped; 93/93 tests green.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~1h |
| Tasks | 4 auto + 1 checkpoint |
| Tests | 93/93 pass (no regressions) |
| Files modified | 7 |
| Commit | ecb8aca |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-0: Layout title bar and spacing | Pass | GRANO top-left, separator line, kSectionGap=12, 5-column knob row, LOOP in header |
| AC-1: Knob halo glow visible | Pass | 3-pass concentric arcs at 0.04/0.09/0.18 alpha before solid arc |
| AC-2: Particle trail animation | Pass | 3-frame ring; trail1=0.30, trail2=0.15 alpha; human-verified |
| AC-3: Error auto-dismiss | Pass | callAfterDelay(3000) with generation counter; tested with .txt drop |
| AC-4: LFO rate readout | Pass | onValueChange lambda → Hz/kHz label; populates immediately on tab switch |
| AC-5: Tests still pass | Pass | 93/93 (100%) |

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| T1–T4 + checkpoint | `ecb8aca` | feat(ui): layout restructure, halo glows, particle trails, polish (F6a) |

## Files Modified

| File | Change | Purpose |
|------|--------|---------|
| `Source/PluginEditor.h` | Modified | Added `errorGen_` member |
| `Source/PluginEditor.cpp` | Modified | Anonymous namespace with kMargin/kHeaderH/kColourBorderMuted; paint() logo to title bar; separator line; resized() with kSectionGap=12, LOOP in header, 5-column knobs; showError() auto-dismiss |
| `Source/UI/LookAndFeel/GranoLAF.cpp` | Modified | 3-pass halo on value arc; glow rim on toggled drawButtonBackground |
| `Source/UI/WaveformDisplay.h` | Modified | timerCallback() moved to .cpp; paintParticles extended signature (+trail1, +trail2); kTrailFrames + trailSnaps_/trailCounts_ ring buffer members |
| `Source/UI/WaveformDisplay.cpp` | Modified | timerCallback() ring-advance + capture; paint() uses stored ring; paintParticles draws trail2/trail1/current in order |
| `Source/UI/LfoPanel.h` | Modified | Added `rateReadout_` Label member |
| `Source/UI/LfoPanel.cpp` | Modified | rateReadout_ constructed + visible; showLfo() wires onValueChange lambda + fires immediately; resized() trims rateSlider_ right to fit readout |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| g.drawText() for logo (not GlyphArrangement) | Embedded fonts not yet available; letter-spacing has no visual benefit with system mono | Simpler paint(), revisit when BinaryData fonts land in F6 |
| kSectionGap=12 / kItemGap=6 separate constants | Single kGap=4 was uniform but gave equal (wrong) weight to inter-section vs intra-section spacing | Cleaner visual hierarchy; all future resized() changes should use these names |
| Trail capture in timerCallback not paint() | Avoids capturing twice per frame; paint() always sees same frame data across all helpers | Consistent particle/playhead state within one paint cycle |

## Deviations from Plan

| Type | Description |
|------|-------------|
| Minor scope reduction | GlyphArrangement letter-spacing code dropped in favour of g.drawText() — deferred to when BinaryData fonts land |

## Deferred Items Resolved by This Plan

These items from STATE.md deferred list are now DONE:
- Error label 3-second auto-dismiss (F2) ✅
- Particle trail / fade-out animation (F2) ✅
- LFO rate display in Hz/BPM next to rate slider (F5c) ✅
- Font API deprecation (F0) — confirmed already correct in F3 (GranoLAF uses FontOptions{}) ✅

## Next Phase Readiness

**Ready for F6 plan 02 (Sub-grain + Stochastic Timing):**
- Layout stable — no further polish changes expected until F6-05 integration
- GranoLAF halo/glow established as visual language for future components
- WaveformDisplay trail ring runs at 30 Hz; additional particles from sub-grains will render automatically
- 93/93 tests; all 7 modified files build clean in Grano, GranoTests, Standalone, VST3

**Remaining deferred (not in this plan):**
- Stereo file downmix (F2)
- Embedded Inter + JetBrains Mono via BinaryData (F3)
- Drawable LFO waveform drag-editor (F5c)
- 250 ms ramp on snapshot recall (F5c)
- Color/Motion/Pattern mod destinations wired to engines (F5b)

**Blockers:** None.

---
*Phase: 06-ui-polish-and-advanced, Plan: 01*
*Completed: 2026-05-19*
