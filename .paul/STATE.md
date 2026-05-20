# STATE — Grano

This file is managed by PAUL. Do not edit manually.

## Current state

```yaml
phase: F8-qa
loop_position: IDLE
current_plan: null
last_unified: .paul/phases/07-release/07-03-SUMMARY.md
session_start: 2026-05-20
```

## Current position

Milestone: v1.0 Initial Release
Phase: F8 — QA and Manual Acceptance Testing — Pending
Plan: none (F8 not yet planned)
Status: F7 complete ✅ — awaiting /paul:plan F8
Last activity: 2026-05-20 — F7 unified; v1.0.0-beta.1 tagged and pushed

Progress:
- Milestone: [███████████████████░] ~98%
- F0: [████████████████████] 100% ✅
- F1: [████████████████████] 100% ✅
- F2: [████████████████████] 100% ✅
- F3: [████████████████████] 100% ✅
- F4: [████████████████████] 100% ✅
- F5: [████████████████████] 100% ✅
- F6: [████████████████████] 100% ✅ — 06-01 ✅ 06-02 ✅ 06-03 ✅ 06-04 ✅ 06-05 ✅
- F7: [████████████████████] 100% ✅ — 07-01 ✅ 07-02 ✅ 07-03 ✅
- F8: [░░░░░░░░░░░░░░░░░░░░] 0% — pending plan

## Loop position

```
PLAN ──▶ APPLY ──▶ UNIFY
  ○        ○        ○     [F8 not yet planned]
```

## Accumulated context

### Decisions
| Decision | Phase | Impact |
|----------|-------|--------|
| JUCE performRealOnlyInverseTransform normalises by 1/N internally | F6c | Do NOT divide output by kFFTSize after IFFT — double-normalization makes output 2048× too quiet |
| feedbackGain hard-clamped to 0.95 in process() | F6c | Prevents divergence regardless of param value; LP filter: k = 1 - clamp(damp); damp=0 → passthrough |
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
| EnvelopeShape::Gaussian (not Gauss) | F6e | Enum value in EnvelopeShapes.h is Gaussian; any future UI or test code must use ::Gaussian |
| CurveEditor uses applyEnvelope() | F6e | Visual thumbnails use same code path as engine — no drift between UI and audio |
| openFileChooser captures slot in lambda | F6e | Avoids pendingSlot_ race; correct slot guaranteed even if user rapidly clicks multiple slot buttons |

### Deferred issues
| Issue | Origin | Effort | Revisit |
|-------|--------|--------|---------|
| ~~Stereo file downmix (proper L+R average)~~ | ~~F2~~ | ~~S~~ | ✅ Resolved in F6d |
| Embedded Inter + JetBrains Mono fonts via BinaryData | F3 | M | F7 |
| Audio response verify (knobs → engine, volume → silence) | F3 | XS | First non-WSL2 session |
| Drawable waveform drag-editor in LfoPanel | F5c | M | post-v1.0 |
| 250ms ramp on snapshot recall | F5c | XS | F7 |
| Color/Motion/Pattern mod destinations consumed by engines | F5b | M | post-v1.0 |
| Spectrogram secondary display | F6 exit criteria | M | post-v1.0 |
| Full pitch-curve control-point editor | F6 scope-out | L | post-v1.0 |
| CurveEditor frontend-design polish pass | F6e skill gap | XS | F7 |
| auval CI validation (macOS 14+ requires Developer ID) | F7-01 | S | F7-03 signing pass |

### Blockers/Concerns
None.

## Session continuity

Last session: 2026-05-20
Stopped at: F7 fully unified — all 3 plans complete, v1.0.0-beta.1 tagged and pushed
Next action: /paul:plan F8 (QA + Manual Acceptance Testing)
Resume context: F8 is the final phase. No plans exist yet. Create the first F8 plan to build TESTING.md and run acceptance tests on Windows + macOS.

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
| F6c — FeedbackPath + Spectral | 3/5 | 2026-05-19 | a1ea272 |
| F6d — MultiSampleBank | 4/5 | 2026-05-19 | b17ab73 |
| F6e — CurveEditor + Multi-slot UI | 5/5 | 2026-05-19 | 827a4b9 |
| F7-01 — CI hardening (pluginval-10) | 1/3 | 2026-05-19 | (multiple) |
| F7-02 — RELEASE_NOTES + presets + README | 2/3 | 2026-05-19 | e6724ad |
| F7-03 — BUILD.md fixes + v1.0.0-beta.1 tag | 3/3 | 2026-05-20 | 7ec136a |
