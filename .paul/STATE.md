# STATE — Grano

This file is managed by PAUL. Do not edit manually.

## Current state

```yaml
phase: F3-core-controls
loop_position: IDLE
current_plan: null
last_unified: .paul/phases/02-sample-io/02-02-SUMMARY.md
session_start: 2026-05-18
```

## Current position

Milestone: v1.0 Initial Release
Phase: F3 — Core Controls and APVTS (4 of 8) — ready to plan
Plan: none (awaiting /paul:plan)
Status: F2 complete and unified — ready for F3
Last activity: 2026-05-18 — Unified 02-02 (WaveformDisplay + grain particles + Load button)

Progress:
- Milestone: [██████░░░░░░░░░░░░░░] ~33%
- F0: [████████████████████] 100% ✅
- F1: [████████████████████] 100% ✅
- F2: [████████████████████] 100% ✅
- F3: [░░░░░░░░░░░░░░░░░░░░] 0% (awaiting plan)

## Loop position

```
PLAN ──▶ APPLY ──▶ UNIFY
  ✓        ✓        ✓     [F2 complete — start F3 with /paul:plan]
```

## Accumulated context

### Decisions
| Decision | Phase | Impact |
|----------|-------|--------|
| Tests compile plugin sources directly (not via JUCE shared-code target) | F0 | Pattern for all future test targets in F1+ |
| Font API deprecation deferred | F0 | Must migrate to `FontOptions` in F3 when GranoLAF is built |
| DAW smoke via WSLg Standalone | F0 | Win/mac smoke covered by CI on first push |
| Single juce::Thread (not ThreadPool) for grain scheduler in F1 | F1 | ThreadPool deferred to F6 — SPSC FIFO requires single producer; upgrade when parallel grain gen needed |
| Guard samples (+2) in testSample_ | F1 | Prevents srcInt+1 OOB for pitchRatio ≤ 2.0; pattern preserved in SampleBuffer (numAudioSamples param excludes guards) |
| AbstractFifo (cap 4) for SampleBuffer retired slots | F2 | Single atomic retired_ would lose buffers on burst loads; FIFO handles up to 4 between 30 Hz cleanup |
| numAudioSamples parameter on setPending() | F2 | Guard samples must not count as playable frames; stored separately to avoid reading buffer metadata on audio thread |
| trySwap() called unconditionally per processBlock (no flag) | F2 | Simpler than newSourcePending_ atomic — trySwap short-circuits when nothing pending |
| Left-channel-only read for stereo files | F2 | Avoids amplitude doubling; proper stereo downmix deferred to F4+ |
| MessageManager guard on startTimerHz | F2 | GranoAudioProcessor compiled into GranoTests; MM guard prevents JUCE assertion in headless CI |
| GrainSnapshot plain array + atomic count | F2 | seqlock overkill for visual data; aligned-float atomic on x86; TSAN-clean in CI |
| WD:: namespace for design tokens | F2 | Colors co-located in .cpp; swap for GranoLAF constants in F3 |
| FileChooser as unique_ptr member on editor | F2 | Keeps async callback alive; same load/error flow as filesDropped |

### Deferred issues
| Issue | Origin | Effort | Revisit |
|-------|--------|--------|---------|
| Migrate deprecated `juce::Font(String, float, int)` to `FontOptions` | F0 | S | F3 (GranoLAF build) |
| Stereo file downmix (proper L+R average) | F2 | S | F4 (when multi-channel engine work begins) |
| Error label 3-second auto-dismiss | F2 | XS | F3 (GranoLAF + timer infrastructure) |
| Playhead wired to APVTS `position` parameter | F2 | S | F3 (position param added) |
| Particle trail / fade-out animation (60 ms decay) | F2 | S | F6 polish |
| GranoLAF styling for WaveformDisplay + Load button | F2 | M | F3 |

### Blockers/Concerns
None.

## Session continuity

Last session: 2026-05-18
Stopped at: F2 unified — both plans complete
Next action: /paul:plan (F3 — Core Controls and APVTS)
Resume context: F3 adds APVTS with 8 parameters, GranoLAF skeleton, custom Knob/Slider components. See ROADMAP.md §F3 for exit criteria. Key prior context: Parameters.{h,cpp} is F3's central deliverable; all future phases read from it. WaveformDisplay playhead needs APVTS `position` param wired in F3.

## Phase history

| Phase | Plans | Completed | Commit |
|-------|-------|-----------|--------|
| F0 — Foundation | 1/1 | 2026-05-17 | d2f457a |
| F1 — Granular Engine | 2/2 | 2026-05-17 | 7645200 |
| F2 — Sample I/O | 2/2 | 2026-05-18 | 4327ac1 |
