# ARCHITECTURE — Grano

This document describes the software architecture of the Grano plugin: classes, threading, data flow, and the design choices that shape them.

For project-level context (goals, stack, ADRs), see `.paul/PROJECT.md`. For the roadmap, see `.paul/ROADMAP.md`. For visual design, see `DESIGN_SPEC.md`.

---

## Module overview

```
                          ┌──────────────────────────┐
                          │   GranoAudioProcessor    │
                          │   (juce::AudioProcessor) │
                          └────┬───────────────┬─────┘
                               │               │
                               │ owns          │ owns
                               ▼               ▼
                  ┌──────────────────┐  ┌──────────────────┐
                  │ GranularEngine   │  │     APVTS        │
                  │ (audio core)     │  │ (parameters)     │
                  └──┬───────┬────┬──┘  └──────────────────┘
                     │       │    │
              owns   │   uses│    │ owns
                     ▼       ▼    ▼
        ┌─────────┐ ┌─────────┐ ┌──────────────────┐
        │ Grain   │ │ Sample  │ │  Sample-clock    │
        │ Pool    │ │ Bank    │ │  scheduler       │
        │         │ │         │ │  (audio thread)  │
        └─────────┘ └─────────┘ └──────────────────┘

                          ┌──────────────────────────┐
                          │  GranoAudioProcessorEditor│
                          │  (juce::AudioProcessorEditor)│
                          └────┬─────────────────────┘
                               │ contains
                               ▼
                  ┌──────────────────┐
                  │ WaveformDisplay  │
                  │ Knobs / Sliders  │
                  │ Module panels    │
                  │ LFO panel        │
                  │ Snapshot bar     │
                  └──────────────────┘
```

The processor owns the engine and the parameter state. The editor displays both, binding controls to APVTS and reading engine telemetry via lock-free atomics.

---

## Threading model

Two classes of threads cooperate. Neither ever blocks the other.

(Until 08-08 a third, "GrainScheduler" `juce::Thread` timed grains with wall-clock
`wait(ms)` and handed descriptors to the audio thread through an SPSC FIFO. That
made offline bounces render at the wrong density — a fast offline render got almost
no grains — snapped onsets to block boundaries, and let Sync/Euclidean drift off the
host grid. It was replaced by a sample clock on the audio thread; see R-01 in
`.paul/phases/08-qa/08-08-REVIEW.md`.)

### Audio thread (host-owned)

- Runs `GranoAudioProcessor::processBlock(buffer, midi)` once per host block.
- Reads APVTS parameter atomics via cached `getRawParameterValue()` pointers
  (never string lookups inside `processBlock`).
- **Schedules grains on a sample clock**: the engine keeps the distance (in
  samples, fractional) to the next grain event and spawns each grain at its exact
  sample offset inside the block. Sync/Euclidean steps are computed from the host
  PPQ position while the transport plays (bar-aligned, no drift); otherwise a
  free-running clock is used.
- Runs modulation, scheduling and Motion in **32-sample sub-blocks**, so LFO and
  wow/flutter/drift values are sampled close to each grain's spawn time.
- Mixes active grains into the output buffer.
- Updates lock-free telemetry (grain snapshot array + atomic count, loop-sweep
  position) for the UI.
- **Forbidden**: allocation (`new`, `malloc`, container resize), freeing memory,
  locks, system calls, exceptions, virtual calls through unbounded chains.

### Message thread / UI thread (JUCE-owned)

- Runs editor paint, mouse, keyboard events.
- Reads APVTS atomics and engine telemetry for display.
- Updates controls via `APVTS::SliderAttachment` and similar.
- **Sample loading** runs here (`loadSampleFile`): decodes via
  `juce::AudioFormatManager` and hands the buffer to `MultiSampleBank`.
- **Spectral FFT** runs here (30 Hz timer, after a load / parameter change / state
  restore), writing a 2048-sample source buffer.
- Frees retired sample buffers (30 Hz timer).
- Allocates freely.

### Communication primitives

| Direction | Mechanism | Reasoning |
|---|---|---|
| UI → Audio (parameter changes) | APVTS atomic load | Lock-free, host automation works. |
| Message thread → Audio (sample swap) | `SampleBuffer` pending record `{buffer, length, generation}` swapped atomically; old buffer leaves in the same record through an `AbstractFifo` retire queue | One pointer exchange keeps buffer and length paired. The audio thread never allocates or frees: if the retire queue is full the swap waits a block. |
| Audio → Message (spectral source ready) | Slot load generation (`getCurrentGeneration`) | The FFT waits until slot 0 holds the generation that was just loaded, so it never reads the previous sample. |
| Audio → UI (telemetry) | Grain snapshot array + `std::atomic<int>` count; atomic loop-sweep fraction | UI reads opportunistically at 30 Hz; visual tearing accepted. |

---

## The granular engine in detail

### `Grain` (trivially copyable)

Represents one in-flight grain. Fields:

```cpp
struct Grain
{
    const float* source;        // mono source pointer (+2 guard samples past sourceLen)
    int   startPos;             // start index into source
    int   sourceLen;            // playable length, excluding guards
    int   lengthSamples;        // grain length in output samples
    float pitchRatio;           // read-position slope; output duration is constant
    float pan;                  // -1.0 (L) to +1.0 (R)
    EnvelopeShape shape;
    int   elapsed;              // output samples rendered so far
    bool  reversed;
    int   startDelay;           // samples to wait before sounding (sample-accurate onset)
    float gain;                 // overlap normalisation
    int   wrapLen;              // > 0: source is a ring of this length (feedback)
};
```

Playback position is an **integer** sample counter with a **double** read position
(`startPos + elapsed * pitchRatio`): exact pitch, full sub-sample precision on long
sources. `fitGrainToSource()` pulls a grain back so its whole read span
(`lengthSamples * pitchRatio`) lies inside the source — every grain plays its full
envelope at any pitch or position.

### `GrainPool`

Fixed-size array of `Grain` slots, pre-allocated. Capacity 1024. Slot allocation is a
CAS scan with a rotating hint index — no free-list, no lock. Both acquire and
release now happen on the audio thread.

The active list is capped at `MaxActiveGrains = 256`; grains beyond that are dropped.

### `GranularEngine`

Each `processBlock(subBlock)` (called per 32-sample sub-block by the processor):

1. `trySwapAll()` on the sample bank; if a new buffer swapped in, drops all active
   grains (their source pointers are stale).
2. **Runs the sample clock** for the sub-block: spawns every due grain with
   `startDelay` = its offset in the block (Pattern gate, source pick, position with
   loop sweep + mod + jitter, quantized pitch + Motion offset, pan, envelope,
   overlap gain, sub-grains placed in time inside the parent window).
3. Renders every active grain (skipping its `startDelay`), releasing finished ones.
4. Writes the grain snapshot telemetry and loop-sweep fraction.

Master volume is applied by the processor **after** Color and the feedback tap
(`applyMasterGain`, ramped per block), so the feedback loop gain doesn't depend on
the output level.

### Grain spawning workflow

```
processBlock: sample clock reaches the next event (offset k in the block)
        ↓
Pattern fire gate (probability / Euclidean step at PPQ index / transient)
        ↓
Pick source (Spectral > Feedback ring > bank slot > 440 Hz sine),
position (+ loop sweep + mod + jitter), pitch (scale-quantized, then + Motion),
pan = spread · rand(-1..1), envelope, gain = 1/sqrt(expected overlap)
        ↓
Acquire Grain from GrainPool, startDelay = k, fit span to source,
spawn sub-grains (family gain 1/sqrt(1 + subs))
        ↓
Render in this same block from sample k onward
```

Spawn-to-audible latency is zero: a grain sounds at the exact sample it was
scheduled for.

---

## Parameter management

`juce::AudioProcessorValueTreeState` (APVTS) owns all exposed parameters. Layout defined once in `Source/Parameters.cpp::createLayout()`.

### Naming convention

Parameter IDs are stable strings: `grainSize`, `density`, `position`, `positionJitter`, `pitchShift`, `stereoSpread`, `masterVolume`, `loop`, `motionEnabled`, `wowDepth`, `flutterDepth`, etc. (see `Source/Parameters.h` for the canonical list). Use `juce::ParameterID` with version hint to allow future migrations.

### Skewing

- Time-domain knobs (grain size, LFO rate): `juce::NormalisableRange<float>` with `skewFactorFromMidPoint` for log-feel.
- Linear parameters (position, pitch shift in semitones): plain linear.
- Density: log skew so the low end has finer control.

### Reading on the audio thread

```cpp
// In GranularEngine::process(), called inside processBlock:
const float grainSize = grainSizeParam_.load(std::memory_order_relaxed);
const float density = densityParam_.load(std::memory_order_relaxed);
```

`grainSizeParam_` is `std::atomic<float>*` obtained once from `apvts.getRawParameterValue("grainSize")` during `prepareToPlay`. Cached as a member; not re-fetched per block.

### Snapshots

Snapshots A/B/C/D are `juce::ValueTree` subtrees stored as `apvts.state` children. Switching snapshots calls `apvts.replaceState(snapshotNode)`, optionally interpolated over 250 ms via a `juce::SmoothedValue` per parameter.

---

## UI architecture

### `GranoAudioProcessorEditor`

Owns:

- A custom `juce::LookAndFeel` instance (`GranoLAF`) set as the editor's LookAndFeel.
- `WaveformDisplay` child component (top third).
- Six core controls: `Knob`s for Grain Size, Density, Position, Jitter; `Slider`s for Pitch Shift, Stereo Spread.
- Three module panels: `MotionPanel`, `ColorPanel`, `PatternPanel`, each with its own children.
- One `LfoPanel` containing both LFO visualizations and edit controls.
- A header bar with snapshot buttons, loop toggle, master volume.
- A footer with telemetry (CPU, voices, active grains).

Layout uses `juce::FlexBox` (preferred for adaptive panels) and explicit `setBounds` for the major regions. Resizable within the limits set in `setResizeLimits`.

### `WaveformDisplay`

Renders three layers:

1. **Background grid** (faint divisions every 0.5s).
2. **Waveform** rendered by `juce::AudioThumbnail` in `#4A4946`.
3. **Playhead cursor** (1.5 px vertical line in `#7CF5C4`) at the engine's current position.
4. **Grain particles** overlay: for each active grain (read from a lock-free particle buffer the engine writes), a translucent dot in `#C77CFF` at the grain's source position. Particle alpha equals the grain's current envelope amplitude. Particles fade out over ~60 ms after the grain ends (motion blur trail effect).

Repaints at 30 Hz via a `juce::Timer`. Repaint is region-limited to the affected x-coordinate range to keep paint cost low.

### `Knob` and `Slider`

Custom components, not `juce::Slider` subclasses (which paint the JUCE default and resist deep customization). Each implements `paint`, `mouseDown`, `mouseDrag`, `mouseUp`, `mouseDoubleClick` (reset to default), `mouseWheelMove`. Value binding via `juce::AudioProcessorValueTreeState::SliderAttachment` (works with any `juce::Slider` subclass; we have a thin `juce::Slider` adapter).

Visual states: idle, hover, focused, modulating. Modulating state shows a ring around the knob in the modulating LFO's color.

### LookAndFeel (`GranoLAF`)

Subclasses `juce::LookAndFeel_V4`. Overrides:

- `drawRotarySlider` — replaced with a custom knob render.
- `drawLinearSlider` — replaced with the slider render.
- `drawButtonBackground`, `drawToggleButton`.
- `getLabelFont`, `getPopupMenuFont` — return `Inter`. Font embedding via `BinaryData` is planned for v1.0.0 stable; until then, the implementation falls back to `juce::Font::getDefaultSansSerifFontName()`.
- `drawPopupMenuBackground`, `drawComboBox` — match the carbon palette.

Halo glow on active controls: implemented via `Graphics::drawDropShadow` with the control's accent color, at low opacity, with small radius. Sutil, not loud.

---

## Modulation matrix (F5)

`ModMatrix` holds N=8 slots:

```cpp
struct SlotParams {                    // pointers into APVTS atomics
    std::atomic<float>* pSource;       // 0=None, 1=LFO1, 2=LFO2
    std::atomic<float>* pDest;         // index into the Dest enum (28 destinations)
    std::atomic<float>* pAmount;       // -1.0 to +1.0
};
```

Per audio block (`ModMatrix::processBlock`, driven by the processor — ModMatrix is
the sole LFO driver):

1. Advance LFO1 by the block size; apply any LFO1→LFO2-Rate cross-mod; advance LFO2.
2. Sum `lfoValue × amount` per destination into an atomic `modSums_` array,
   clamped to ±1 on read.
3. Consumers pull offsets via `getModOffset(dest)` and scale by their own parameter
   range: the engine consumes GrainSize, Density, Position, PositionJitter,
   PitchShift (±24 st), StereoSpread, and MasterVol (±66 dB). Module destinations
   (Motion/Color/Pattern) and LFO phase/depth destinations are **not yet consumed**
   (deferred post-v1.0 — see STATE.md deferred issues).

This avoids modulating APVTS directly (which would conflict with host automation and
create unstable visual feedback).

---

## Sub-grain recursion (F6)

When `subGrainDepth > 0`, each scheduled grain additionally spawns sub-grains
(the parent still plays — sub-grains are layered on top):

1. Depth 1: 2-4 sub-grains (`SubGrain::kMinSubs..kMaxSubs`), each at most half the
   parent's length, positioned inside the parent's window with jittered pitch/pan.
2. Depth 2: each depth-1 sub-grain spawns 2-3 sub-sub-grains.
3. There is no user-facing sub-grain *count* parameter — only `subGrainDepth` (0/1/2).

Implementation: `SubGrain::spawnInto` builds the children on the audio thread right
after the parent. Each sub-grain is placed in **time** inside the parent window
(`startDelay` = parent delay + offset) and reads the source region the parent reads
at that moment; its span is fitted to the source like any grain. The whole family is
scaled by `1/sqrt(1 + subs)` so sub-grains add texture, not level.

Limits: pool capacity 1024, audio-thread active cap 256, depth capped at 2. Worst-case
expansion per parent is 1 + 4 + 4×3 = 17 grains.

---

## Stochastic timing distributions (F6)

Six distributions for inter-grain timing intervals:

| Name | Formula (mean = 1/density seconds; shape constants are fixed, there is no user jitter parameter) | Character |
|------|---|---|
| Uniform | `U(0.5, 1.5) × mean` | Even, predictable. |
| Gaussian | `Normal(μ = mean, σ = 0.3 × mean)` | Soft randomness. |
| Poisson (clustered) | Two-phase hyperexponential: 50 % short gaps `Exp(0.2 × mean)`, 50 % long gaps `Exp(1.92 × mean)` (mean ≈ preserved under the 5× clamp) | Bursts of grains separated by pauses. |
| Exponential | `-mean × ln(U)` (memoryless Poisson-process spacing) | "Natural", irregular event timing. |
| Pareto | α = 1.5, clamped at 5× mean | Sparse clusters of rapid grains. |
| 1/f (fractal) | Low-pass-filtered noise modulating the interval | Self-similar, musical. |

All implemented in `Source/Engine/StochasticTiming.cpp`. Unit tests verify distribution
moments against theoretical values (Pareto at 20% tolerance — clamping shifts the mean;
see STATE.md decision).

Intervals are kept in fractional samples on the sample clock (no millisecond truncation).
The chosen distribution determines the next grain spawn time the engine schedules. The trigger module (Free / Sync / Euclidean / Audio-driven) determines the base rate; the distribution adds shape.

---

## Spectral processing (F6)

`SpectralProcessor` runs on the message thread (file load, spectral parameter change,
or state restore) — it is a **one-shot source transformation**, not a streaming STFT:

1. The first 2048 samples of bank slot 0 are FFT'd unwindowed (`kFFTSize = 2048`,
   `juce::dsp::FFT`) — the output is a looping grain source, so no window is needed
   and level is preserved.
2. Bins are modified per mode: `Freeze` keeps magnitudes and assigns seeded random
   phases (a stationary texture — zero phase would collapse the frame into a click);
   `Blur` offsets each bin's original phase by up to ±π·blur and jitters magnitudes by
   blur (blur 0 = the dry frame).
3. One IFFT back to time domain (JUCE normalises by 1/N internally — no manual
   division; see STATE.md decision) into a 2048-sample output buffer.
4. While spectral is enabled, the engine granulates this 2048-sample buffer instead
   of the sample bank (source priority: Spectral > Feedback > bank > sine).
5. After a load, the FFT waits (30 Hz retry) until slot 0's live load generation
   matches the new file, so it never transforms the previous sample.

Latency: **0**. The transform is offline on the source; it adds no delay to the audio
path, so nothing is reported to the host (reporting 2048 used to shift the track
42 ms early).

---

## Memory budget

| Component | Budget |
|---|---|
| 4 × sample slots, 60s each, 48 kHz stereo float | 4 × 23 MB = 92 MB |
| Grain pool (1024 grains × 64 B) | 64 KB |
| Worker stacks (4 × 512 KB) | 2 MB |
| FFT buffers (overlap-add windows × 4 workers) | ~16 KB |
| LookAndFeel cached glyphs and atlases | ~8 MB |
| JUCE internals, host integration | ~5 MB |
| Headroom and miscellaneous | ~30 MB |
| **Total nominal** | **≤ 140 MB** |

Hard cap of 200 MB per instance is enforced by capping per-slot sample length to 60s (longer files are truncated with a UI notice).

---

## Real-time guarantees on the audio thread

The audio thread must complete `processBlock` within the block deadline. At 48 kHz with a 512-sample buffer, the deadline is 10.67 ms. We target ≤ 5 ms typical, ≤ 8 ms worst case.

Inviolable rules:

1. **No allocation.** Use the grain pool, pre-allocated arrays, `std::array`. Any `new`/`malloc`/`std::vector::push_back` in audio-thread code is a bug.
2. **No locks.** Use atomics for cross-thread state. Use lock-free queues/atomic records for handoff.
3. **No syscalls.** No file I/O, no `printf`, no logging via OS-level handles. The audio thread does not log at all.
4. **No exceptions.** Audio-thread code paths are written exception-free (`noexcept` throughout Engine/ and Modulation/); the project does not use a per-TU `-fno-exceptions` flag because JUCE headers require exceptions enabled at compile time.
5. **No virtual dispatch through chains.** If polymorphism is needed (e.g., envelope shapes), prefer `enum + switch` or CRTP. Single virtual at the boundary is fine.
6. **Bounded loop counts.** Every loop has a known upper bound. No `while (condition_from_external_state)`.

Code review and PAUL guardrails enforce these.

---

## Testing strategy

| Layer | Tooling | What's tested |
|---|---|---|
| Pure utilities (EnvelopeShapes, StochasticTiming) | Catch2 unit tests | Numerical correctness, edge cases. |
| Engine classes (Grain, GrainPool, GranularEngine) | Catch2 + manual harness | Grain accumulation, pool overflow, sample-clock timing (offline density, block-size invariance, PPQ lock), span fitting, level bounds. |
| Real-time safety | TSAN + custom static analysis | Race conditions, allocation in audio thread. Known JUCE 8 false positives suppressed via `Tests/tsan_suppressions.txt`. |
| Plugin integration | pluginval, auval | Host compliance. |
| Sound output | Manual listening + RMS regression | Subjective quality + automated detection of catastrophic regressions. |
| UI | Visual inspection on Mac + Windows + Linux | Look-and-feel parity. |

CI runs Catch2 + pluginval + TSAN on every push to `main` or feature branch.

---

## Future work (post v1.0)

See `.paul/ROADMAP.md` for deferred features. Notable areas:

- **MPE input**. Channel-per-note routing into independent grain streams.
- **AAX format**.
- **Surround output** (5.1, 7.1.4 Atmos).
- **Cloud preset library** via a small REST API.
- **Built-in sample browser** with metadata indexing.
- **Pitch tracking and harmonic re-synthesis** as an alternative grain source.

Each of these is a multi-week effort and is deliberately excluded from v1.0.
