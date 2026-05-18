# STATE — Grano

This file is managed by PAUL. Do not edit manually.

## Current state

```yaml
phase: F5-modulation-matrix
loop_position: IDLE
current_plan: (none — 05-01 complete, ready to plan 05-02)
last_unified: .paul/phases/05-modulation-matrix/05-01-SUMMARY.md
session_start: 2026-05-18
```

## Current position

Milestone: v1.0 Initial Release
Phase: F5 — Modulation Matrix (6 of 8) — In Progress
Plan: 05-01 ✅ COMPLETE — ready for 05-02 ModMatrix
Status: PLAN 1 of 3 unified
Last activity: 2026-05-18 — Unified 05-01 (F5a: LFO core); 85/85 tests

Progress:
- Milestone: [█████████████░░░░░░░] ~70%
- F0: [████████████████████] 100% ✅
- F1: [████████████████████] 100% ✅
- F2: [████████████████████] 100% ✅
- F3: [████████████████████] 100% ✅
- F4: [████████████████████] 100% ✅
- F5: [███████░░░░░░░░░░░░░] 33% — 05-01 done, 05-02 next

## Loop position

```
PLAN ──▶ APPLY ──▶ UNIFY
  ✓        ✓        ✓     [05-01 complete — ready for /paul:plan 05-02]
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
| GranoLAF scoped to editor (not global) | F3 | Avoids polluting other JUCE components; setLookAndFeel(nullptr) in destructor |
| Grain size shadow added to WaveformDisplay | F3 | 10% mint fill + 25% right-edge; low opacity preserves waveform readability |
| FileChooser as unique_ptr member on editor | F2 | Keeps async callback alive; same load/error flow as filesDropped |

### Deferred issues
| Issue | Origin | Effort | Revisit |
|-------|--------|--------|---------|
| Migrate deprecated `juce::Font(String, float, int)` to `FontOptions` | F0 | S | F3 (GranoLAF build) |
| Stereo file downmix (proper L+R average) | F2 | S | F4 (when multi-channel engine work begins) |
| Error label 3-second auto-dismiss | F2 | XS | F3 (GranoLAF + timer infrastructure) |
| Playhead wired to APVTS `position` parameter | F2 | S | F3 (position param added) |
| Particle trail / fade-out animation (60 ms decay) | F2 | S | F6 polish |
| GranoLAF styling for WaveformDisplay + Load button | F2 | M | ✅ Done F3 |
| Audio response verify (knobs → engine, volume → silence) | F3 | XS | First non-WSL2 session |
| Embedded Inter + JetBrains Mono fonts via BinaryData | F3 | M | F6 |

### Blockers/Concerns
None.

## Session continuity

Last session: 2026-05-18
Stopped at: 05-01 unified — Lfo class, 10 APVTS params, 85/85 tests, commit bb7d921
Next action: /paul:plan 05-02 — ModMatrix + engine integration (LFO wiring, 8-slot routing)
Resume context: 85/85 tests. 42 APVTS params. Lfo::advanceSample() API ready. Two LFO instances go on PluginProcessor in 05-02.

## Phase history

| Phase | Plans | Completed | Commit |
|-------|-------|-----------|--------|
| F0 — Foundation | 1/1 | 2026-05-17 | d2f457a |
| F1 — Granular Engine | 2/2 | 2026-05-17 | 7645200 |
| F2 — Sample I/O | 2/2 | 2026-05-18 | 4327ac1 |
| F3 — Core Controls | 2/2 | 2026-05-18 | c728df5 |
| F4a — Motion (in F4) | 1/4 | 2026-05-18 | d9cd358 |
| F4b — Color (in F4) | 2/4 | 2026-05-18 | 1e2e5b9 |
| F4 CI hardening | 3/4 | 2026-05-18 | 1d0d11f |
| F4c — Pattern (in F4) | 4/4 | 2026-05-18 | fb44ed6 |
