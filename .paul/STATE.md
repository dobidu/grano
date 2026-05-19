# STATE — Grano

This file is managed by PAUL. Do not edit manually.

## Current state

```yaml
phase: F6-ui-polish-and-advanced
loop_position: APPLY
current_plan: .paul/phases/06-ui-polish-and-advanced/06-03-PLAN.md
last_unified: .paul/phases/06-ui-polish-and-advanced/06-02-SUMMARY.md
session_start: 2026-05-19
```

## Current position

Milestone: v1.0 Initial Release
Phase: F6 — UI Polish + Sub-grain + Spectral (7 of 8) — In Progress
Plan: 06-03 — FeedbackPath + SpectralProcessor (applying)
Status: F6 plan 3/5 in progress
Last activity: 2026-05-19 — 06-03 APPLY started; FeedbackPath + SpectralProcessor

Progress:
- Milestone: [████████████████░░░░] ~83%
- F0: [████████████████████] 100% ✅
- F1: [████████████████████] 100% ✅
- F2: [████████████████████] 100% ✅
- F3: [████████████████████] 100% ✅
- F4: [████████████████████] 100% ✅
- F5: [████████████████████] 100% ✅
- F6: [████████░░░░░░░░░░░░] ~40% — 06-01 ✅ 06-02 ✅

## Loop position

```
PLAN ──▶ APPLY ──▶ UNIFY
  ✓        ○        ○     [06-03 in progress — FeedbackPath + SpectralProcessor]
```

## Accumulated context

### Decisions
| Decision | Phase | Impact |
|----------|-------|--------|
| parentCopy before FIFO finishedWrite in scheduleGrain() | F6b | Audio thread owns slot immediately after finishedWrite; copy must be taken beforehand for sub-grain spawning |
| StochasticTiming::Poisson = Exponential inter-arrival | F6b | Mathematically correct: Poisson process inter-arrival time is Exp(λ); name is user-facing |
| Pareto test tolerance 20% (not 15%) | F6b | E[X_clamped] = 0.83×mean analytically for α=1.5 at 5× clamp ceiling; not a bug — documented in test |
| GranoCombinedState wrapper for serialization | F5c | APVTS state type is already "GranoState" — outer wrapper must be different for disambiguation; old format still falls through to else-if branch |
| UI .cpp files belong in GranoTests too | F5c | PluginEditor.cpp compiled into GranoTests references all editor-included UI files; pattern: add any UI .cpp that PluginEditor includes |
| ModMatrix() = default explicit required | F5b | GCC suppresses implicit default ctor with JUCE_DECLARE_NON_COPYABLE + non-trivial members — add explicit default to any new class with this macro |
| ModMatrix owns LFO advances in processBlock() | F5b | PluginProcessor must NOT call lfo1_.processBlock() separately; ModMatrix is sole LFO driver |
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
| Stereo file downmix (proper L+R average) | F2 | S | F6-04 (multi-sample engine) |
| Embedded Inter + JetBrains Mono fonts via BinaryData | F3 | M | F6-05 (integration) |
| Audio response verify (knobs → engine, volume → silence) | F3 | XS | First non-WSL2 session |
| Drawable waveform drag-editor in LfoPanel | F5c | M | F6-02+ |
| 250ms ramp on snapshot recall | F5c | XS | F6-04 |
| Color/Motion/Pattern mod destinations consumed by engines | F5b | M | F6-03+ |

### Blockers/Concerns
None.

## Session continuity

Last session: 2026-05-19
Stopped at: 06-02 unified — SubGrain depth 0/1/2, StochasticTiming 6 distributions; 101/101; ab1f039
Next action: /paul:plan F6 (plan 03) — Feedback Path + Spectral Processor
Resume context: 101/101 tests. 68 APVTS params. SubGrain and StochasticTiming active in engine. GranularEngine.h includes both new headers. New classes needed: FeedbackPath.{h,cpp} (internal feedback loop) and SpectralProcessor.{h,cpp} (FFT freeze/blur, juce::dsp::FFT) in Source/Engine/.

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
| F5a — LFO core | 1/3 | 2026-05-18 | (in 281378d) |
| F5b — ModMatrix + engine | 2/3 | 2026-05-18 | 281378d |
| F5c — Snapshots + UI | 3/3 | 2026-05-19 | 3697bcd |
| F6a — UI Polish | 1/5 | 2026-05-19 | ecb8aca |
| F6b — Sub-grain + Stochastic | 2/5 | 2026-05-19 | ab1f039 |
