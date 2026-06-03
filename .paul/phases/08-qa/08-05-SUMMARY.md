---
phase: 08-qa
plan: 05
subsystem: ui
tags: [juce, tab-dots, knob-readout, cosmetic, mockup-polish]

requires:
  - phase: F8-02
    provides: ModuleTabPanel with 6 tabs

provides:
  - Colored accent dots left of each tab label
  - Numeric value readout (2 decimal places) under every Knob

affects: [08-06, future-ui-work]

tech-stack:
  added: []
  patterns:
    - "Knob value readout: slider_.onValueChange → juce::String(v, 2) → valueLabel_"
    - "Tab dot: paint() fillEllipse at left edge of tab, active=1.0f / inactive=0.35f alpha"

key-files:
  created: []
  modified:
    - Source/UI/ModuleTabPanel.cpp
    - Source/UI/Knob.h
    - Source/UI/Knob.cpp

key-decisions:
  - "Dot position: left edge (cx=i*tabW+10) not center — user requested after seeing centered version"
  - "Value precision: fixed juce::String(v, 2) — span-based formula unreliable (NormalisableRange skew affects getRange() return)"

patterns-established:
  - "WSL2 clock skew: must delete object files manually for Release config to force recompile"

duration: ~2h (incl. 3 build iterations due to clock skew)
started: 2026-06-03T10:00:00Z
completed: 2026-06-03T12:00:00Z
---

# F8 Plan 05: Colored Tab Dots + Knob Value Readout

**8px colored accent dots left of each tab label (module color-coded) + 2-decimal numeric value readout under every Knob — human verified.**

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: Colored tab dots | Pass | 6 dots, left-aligned, active=full / inactive=35% |
| AC-2: Knob numeric value | Pass | 2 decimal places, live update on drag |

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| T1+T2 + dot position fix + precision fix | `5987c7c` | Final: dots left-aligned, values 2 decimal |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `Source/UI/ModuleTabPanel.cpp` | Modified | paint(): 8px dots at tab left edge |
| `Source/UI/Knob.h` | Modified | Added valueLabel_ member |
| `Source/UI/Knob.cpp` | Modified | valueLabel_ init + onValueChange + resized() kValueH=12 |

## Deviations from Plan

| Type | Count | Impact |
|------|-------|--------|
| User-requested adjustment | 1 | Dot moved from bottom-center to left-edge |
| Implementation change | 1 | Value precision: span formula → fixed 2 decimals |

**Dot position:** Plan said "bottom center of tab strip". User at checkpoint said "put dot left of tab title". Changed cx from `tabW*0.5f` to `i*tabW+10`.

**Value precision:** `getTextFromValue()` gave full precision. Span-based formula unreliable due to NormalisableRange skew (getRange() may return normalized {0,1} not actual range). Fixed to `juce::String(v, 2)` — uniform 2 decimal places.

---
*Phase: 08-qa, Plan: 05*
*Completed: 2026-06-03*
