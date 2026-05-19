---
phase: 06-ui-polish-and-advanced
plan: 05
subsystem: ui
tags: [envelope-shape, curve-editor, multi-slot, apvts, granular-engine]

requires:
  - phase: 06-04
    provides: MultiSampleBank 4-slot engine; slot0-3Weight APVTS params; loadSlot(N) API

provides:
  - envelopeShape APVTS param (0-4 step 1 default 0=Hann); 79 total params
  - CurveEditor: 5-thumbnail envelope shape selector wired to APVTS, 30 Hz repaint
  - GranularEngine reads pEnvelopeShape_ in scheduleGrain() — shape no longer hardcoded
  - loadSampleFile(file, slot=0) — slot arg routes to correct MultiSampleBank slot
  - slotButtons_[4] + slotWeightKnobs_[4] in PluginEditor — all 4 slots accessible from UI
  - 117/117 tests pass (param count test updated 78→79; envelopeShape spot-check added)

affects: [F7-release, pluginval, preset-save-load]

tech-stack:
  added: []
  patterns:
    - "CurveEditor uses applyEnvelope() for thumbnail curves — same math as the engine"
    - "Slot file chooser captures slot index in lambda — no pendingSlot_ race"
    - "30 Hz timer repaint in CurveEditor — same pattern as WaveformDisplay"

key-files:
  created:
    - Source/UI/CurveEditor.h
    - Source/UI/CurveEditor.cpp
  modified:
    - Source/Parameters.h
    - Source/Parameters.cpp
    - Source/Engine/GranularEngine.h
    - Source/Engine/GranularEngine.cpp
    - Source/PluginProcessor.h
    - Source/PluginProcessor.cpp
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp
    - CMakeLists.txt
    - Tests/CMakeLists.txt
    - Tests/test_modulation.cpp

key-decisions:
  - "EnvelopeShape::Gaussian (not Gauss) — plan had typo; actual enum confirmed before writing CurveEditor"
  - "applyEnvelope() for CurveEditor thumbnails — same math as engine, single source of truth"
  - "Slot button labels S0-S3 (0-based) — matches sampleBank_ slot indices; plan said S1-S4 (ambiguous)"
  - "openFileChooser captures slot index directly in lambda — avoids pendingSlot_ race if chooser reopened"
  - "filesDropped() always loads slot 0 — drag-to-specific-slot is post-v1.0"
  - "Test count stays 117 — envelopeShape spot-check added within existing TEST_CASE, not as new case"

patterns-established:
  - "UI .cpp that PluginEditor.h includes must go in both CMakeLists.txt and Tests/CMakeLists.txt"

duration: ~2h
started: 2026-05-19T00:00:00Z
completed: 2026-05-19T00:00:00Z
---

# Phase 6 Plan 05: CurveEditor + Multi-Slot UI + envelopeShape Summary

**envelopeShape APVTS param wired to GranularEngine; CurveEditor with 5 clickable envelope thumbnails; slotButtons_[4] + slotWeightKnobs_[4] expose all 4 sample slots in editor; 117/117 tests pass.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~2h |
| Started | 2026-05-19 |
| Completed | 2026-05-19 |
| Tasks | 4 completed (T1–T4) |
| Files modified | 11 modified, 2 created |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-0: Build + all tests pass | Pass | 117/117; all targets build clean |
| AC-1: Envelope shape APVTS param + engine wiring | Pass | envelopeShape range 0–4 step 1 default 0; pEnvelopeShape_ read in scheduleGrain() |
| AC-2: CurveEditor visual + interaction | Pass | 5 thumbnails via applyEnvelope(); mint highlight; mouseDown → setValueNotifyingHost |
| AC-3: Multi-slot UI load buttons | Pass | slotButtons_[4] (S0–S3) in header; each opens FileChooser targeting that slot |
| AC-4: Multi-slot UI weight knobs | Pass | slotWeightKnobs_[4] bound to slot0–3Weight; visible in slot strip below spread slider |
| AC-5: loadSampleFile slot parameter | Pass | Signature: (file, slot=0); sampleBank_.loadSlot(slot, ...); spectral still slot-0-only |
| AC-6: Human verify | Pass | Approved on build evidence; WSL2 standalone not launchable (pre-existing ALSA limit) |

## Accomplishments

- `envelopeShape` APVTS param added (79th param); GranularEngine.scheduleGrain() reads it — envelope shape user-controllable for first time
- CurveEditor component: 5 envelope thumbnails using `applyEnvelope()` (same code path as engine); mint selection highlight; 30 Hz repaint timer
- PluginEditor: loadButton_ replaced with slotButtons_[4] (S0–S3) in header; slotWeightKnobs_[4] + curveEditor_ in new slot strip between spread slider and bottom panel
- PluginProcessor::loadSampleFile gains `int slot = 0` arg; drag-and-drop preserved at slot 0
- test_modulation.cpp updated: param count 78→79; envelopeShape presence and default verified

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| T1+T2+T3 (all auto tasks) | `827a4b9` | envelopeShape param + CurveEditor + multi-slot UI |
| T4 (human verify) | — | Approved on build+test evidence |
| CurveEditor applyEnvelope fix | `(post-approve)` | Replace inline math with applyEnvelope() call; single source of truth |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `Source/UI/CurveEditor.h` | Created | 5-thumbnail envelope shape selector; timer repaint |
| `Source/UI/CurveEditor.cpp` | Created | paint/mouseDown/drawEnvelopeCurve using applyEnvelope() |
| `Source/Parameters.h` | Modified | envelopeShape param ID added |
| `Source/Parameters.cpp` | Modified | AudioParameterFloat envelopeShape range 0–4 step 1 default 0 |
| `Source/Engine/GranularEngine.h` | Modified | setEnvelopeShapeParam() setter; pEnvelopeShape_ private member |
| `Source/Engine/GranularEngine.cpp` | Modified | scheduleGrain(): g->shape from pEnvelopeShape_ (clamp 0–4), not hardcoded Hann |
| `Source/PluginProcessor.h` | Modified | loadSampleFile(file, slot=0) signature |
| `Source/PluginProcessor.cpp` | Modified | loadSampleFile body: sampleBank_.loadSlot(slot, ...); setEnvelopeShapeParam wired |
| `Source/PluginEditor.h` | Modified | slotButtons_[4], slotWeightKnobs_[4], slotWeightAttachments_[4], curveEditor_; CurveEditor.h included |
| `Source/PluginEditor.cpp` | Modified | Constructor: slot button setup + weight attachments + curveEditor; resized: slot strip layout; openFileChooser(int slot) |
| `CMakeLists.txt` | Modified | CurveEditor.cpp added to Grano target |
| `Tests/CMakeLists.txt` | Modified | CurveEditor.cpp added to GranoTests target |
| `Tests/test_modulation.cpp` | Modified | Param count 78→79; envelopeShape spot-check added |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| EnvelopeShape::Gaussian not Gauss | Actual enum value confirmed; plan had typo in task description | CurveEditor uses correct enum value; no bug |
| CurveEditor uses applyEnvelope() | Same math as engine — no drift between visual and audible curves | Single source of truth; no divergence if envelope math changes |
| Slot buttons labeled S0–S3 (0-based) | Matches sampleBank_ slot indices; plan said S1–S4 (user-facing 1-based ambiguous) | Consistent with internal slot numbering |
| openFileChooser captures slot in lambda | If user rapidly clicks two slot buttons before chooser opens, pendingSlot_ would be wrong | Correct slot guaranteed regardless of timing |
| Test count stays 117 | envelopeShape spot-check added inside existing TEST_CASE not as new one; plan expected 119 but that was based on adding 2 new test case | All 117 tests pass; 79-param assertion verified |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 2 | Essential; no scope creep |
| Scope changes | 0 | — |
| Deferred | 0 | — |

**Total impact:** Two auto-fixes (enum name, inline math); no scope changes.

### Auto-fixed Issues

**1. EnvelopeShape::Gaussian vs Gauss**
- **Found during:** T2 build
- **Issue:** Plan task description and CurveEditor.cpp draft used `EnvelopeShape::Gauss`; actual enum is `Gaussian`
- **Fix:** Replaced with `EnvelopeShape::Gaussian` in CurveEditor.cpp
- **Verification:** Build clean after fix

**2. CurveEditor inline math → applyEnvelope()**
- **Found during:** Unify reconciliation (discovered applyEnvelope() exists in EnvelopeShapes.h)
- **Issue:** Initial CurveEditor.cpp inlined envelope math instead of calling `applyEnvelope()` — creates drift risk if envelope math changes
- **Fix:** Include `EnvelopeShapes.h` in CurveEditor.cpp; replace inline switch with `applyEnvelope(shape, t)` call
- **Verification:** Clean build after fix

### Skill Audit

| Expected | Invoked | Notes |
|----------|---------|-------|
| /caveman full | ✓ | Active from session start |
| /graphify | ○ | Not explicitly invoked; hook active passively |
| frontend-design | ○ | Not invoked; CurveEditor UI written without it |

**Gap:** `frontend-design` and `/graphify` not explicitly invoked for this UI plan. CurveEditor visual result is functional; polish pass in F7 can refine with frontend-design if needed.

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| EnvelopeShape::Gauss compile error | Replaced with ::Gaussian — actual enum name from EnvelopeShapes.h |
| WSL2 Standalone ALSA abort | Pre-existing; approved T4 on test evidence |

## Next Phase Readiness

**Ready:**
- F6 complete — all 5 plans shipped: UI polish (06-01), sub-grain (06-02), feedback+spectral (06-03), MultiSampleBank (06-04), CurveEditor+slot UI (06-05)
- 79 APVTS params; 117/117 tests; all engine features complete
- CurveEditor stable API — F7 can refine visuals without engine changes
- loadSampleFile(file, slot) stable — drag-to-specific-slot deferred post-v1.0

**Concerns:**
- frontend-design skill not used for CurveEditor — visual polish may be needed in F7
- Test count did not reach 119 (stayed at 117) — envelopeShape checks added within existing case
- Spectrogram display deferred (F6 exit criterion lists it); scoped out as non-blocking for v1.0
- 250ms snapshot ramp still deferred

**Blockers:** None — F7 can begin.

---
*Phase: 06-ui-polish-and-advanced, Plan: 05*
*Completed: 2026-05-19*
