# STATE — Grano

This file is managed by PAUL. Do not edit manually.

## Current state

```yaml
phase: F2-sample-io
loop_position: IDLE
current_plan: null
last_unified: .paul/phases/02-sample-io/02-01-SUMMARY.md
session_start: 2026-05-17
```

## Current position

Milestone: v1.0 Initial Release
Phase: F2 — Sample I/O and Waveform Display (3 of 8) — 1 of 2 plans complete
Plan: 02-01 unified ✓ — 02-02 (WaveformDisplay + particles) pending
Status: Ready for 02-02 PLAN (requires frontend-design skill)
Last activity: 2026-05-17 — Unified 02-01 (SampleBuffer + AudioFormatManager + drag-drop + GranularEngine wiring)

Progress:
- Milestone: [████░░░░░░░░░░░░░░░░] ~22%
- F0: [████████████████████] 100% ✅
- F1: [████████████████████] 100% ✅
- F2: [██████████░░░░░░░░░░] 50% (1/2 plans complete)

## Loop position

```
PLAN ──▶ APPLY ──▶ UNIFY
  ✓        ✓        ✓     [02-01 complete — ready for 02-02 PLAN]
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

### Deferred issues
| Issue | Origin | Effort | Revisit |
|-------|--------|--------|---------|
| Migrate deprecated `juce::Font(String, float, int)` to `FontOptions` | F0 | S | F3 (GranoLAF build) |
| Stereo file downmix (proper L+R average) | F2 | S | F4 (when multi-channel engine work begins) |
| Error label 3-second auto-dismiss | F2 | XS | F3 (GranoLAF + timer infrastructure) |

### Blockers/Concerns
None — F2-02 requires frontend-design skill for WaveformDisplay spec.

## Session continuity

Last session: 2026-05-17
Stopped at: 02-01 unified (SampleBuffer + AudioFormatManager + drag-drop + GranularEngine wiring), commit 09786f1
Next action: /paul:plan F2 plan 02-02 (WaveformDisplay + AudioThumbnail + grain particles)
Resume context: frontend-design skill required for 02-02 (WaveformDisplay DESIGN_SPEC.md waveform colours, particle spec). Plan 02-02 covers: WaveformDisplay component, juce::AudioThumbnail, playhead cursor (1.5 px, #7CF5C4), grain particle overlay (#C77CFF), 60 fps cap.

## Phase history

| Phase | Plans | Completed | Commit |
|-------|-------|-----------|--------|
| F0 — Foundation | 1/1 | 2026-05-17 | d2f457a |
| F1 — Granular Engine | 2/2 | 2026-05-17 | 7645200 |
| F2 — Sample I/O | 1/2 | — | 09786f1 (partial) |
