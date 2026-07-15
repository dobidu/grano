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
        │ Grain   │ │ Sample  │ │  GrainPool       │
        │ Pool    │ │ Buffer  │ │  SPSC FIFO       │
        │         │ │         │ │  WorkerThread    │
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

Three classes of threads cooperate. None ever blocks another.

### Audio thread (host-owned)

- Runs `GranoAudioProcessor::processBlock(buffer, midi)` once per audio block.
- Calls `GranularEngine::process(buffer)`.
- Reads APVTS parameter atomics via `getRawParameterValue()`.
- Drains the **incoming grain FIFO** (worker → audio) for any grains ready to mix.
- Mixes active grains into the output buffer.
- Updates lock-free telemetry (grain snapshot array + atomic count for the UI particles).
- **Forbidden**: allocation (`new`, `malloc`, container resize), locks, system calls, exceptions, virtual calls through unbounded chains.

### Message thread / UI thread (JUCE-owned)

- Runs editor paint, mouse, keyboard, MIDI control events.
- Reads APVTS atomics and engine telemetry counters for display.
- Updates controls via `APVTS::SliderAttachment` and similar (which post parameter changes to APVTS internally).
- Drives a `juce::Timer` at 30 Hz that triggers `WaveformDisplay::repaint()` and updates particle positions.
- Allocates freely. Slow operations (sample loading) are dispatched to worker threads, never run inline.

### Scheduler thread (single `juce::Thread`, "GrainScheduler")

As shipped, there is **one** scheduler thread, not a worker pool. (A `juce::ThreadPool`
upgrade was considered in F1 and deferred; the SPSC FIFO requires a single producer,
and one scheduler comfortably meets the grain-rate budget.)

- Runs **grain scheduling**: waits out the stochastic/pattern-derived interval, then
  fills a lightweight `Grain` descriptor (source pointer, position, length, pitch,
  pan, envelope shape) and pushes it onto the SPSC FIFO. Grains are *rendered* on the
  audio thread, not here — the descriptor is just bookkeeping, so scheduling is cheap.
- **Sample loading** runs inline on the message thread (`loadSampleFile`), which
  decodes via `juce::AudioFormatManager` and hands the buffer to `MultiSampleBank`
  for an atomic pending→current swap on the audio thread.
- **Spectral FFT** runs on the message thread (file load, parameter change, or state
  restore via the 30 Hz timer), writing a 2048-sample source buffer.
- May allocate (loading paths), may call into the OS (scheduler `wait`). Never blocks
  the audio thread.

### Communication primitives

| Direction | Mechanism | Reasoning |
|---|---|---|
| UI → Audio (parameter changes) | APVTS atomic load | Lock-free, host automation works. |
| Message thread → Audio (sample swap) | `SampleBuffer` pending/current/retired pointers + `AbstractFifo` of retired buffers | Atomic swap on the audio thread; retired buffers freed by the 30 Hz timer. |
| Scheduler → Audio (scheduled grains) | SPSC `juce::AbstractFifo` carrying `Grain*` descriptors | Lock-free, predictable. Audio drains; never blocks. |
| Audio → UI (telemetry) | Grain snapshot array + `std::atomic<int>` count | UI reads opportunistically at 30 Hz; visual tearing accepted. |

---

## The granular engine in detail

### `Grain` (POD-like, ~ 64 bytes)

Represents one in-flight grain. Fields:

```cpp
struct Grain
{
    const float* source;        // mono source pointer (+2 guard samples past sourceLen)
    int   sourceLen;            // playable length, excluding guards
    int   startPos;             // start index into source
    int   lengthSamples;        // grain length in output samples
    float pitchRatio;           // read-position multiplier; output duration is constant
    float pan;                  // -1.0 (L) to +1.0 (R)
    EnvelopeShape shape;        // enum
    float currentPhase;         // normalized 0..1 across the grain's lifetime
    bool  reversed;
};
```

No virtuals. No constructors that allocate. Filled by the scheduler thread, rendered
by the audio thread. Note: phase is normalized and advances by `1/lengthSamples` per
output sample — pitch changes the *read position* slope, not the grain duration.

### `GrainPool`

Fixed-size array of `Grain` slots, pre-allocated. Capacity 1024 in the default build.
Slot allocation is a lock-free CAS scan with a rotating hint index — no free-list,
no lock.

```cpp
class GrainPool {
    std::array<Grain, 1024>             slots_;
    std::array<std::atomic<bool>, 1024> inUse_;
    std::atomic<int>                    hint_;
public:
    Grain* acquire();        // scheduler side (CAS on inUse_ flags, hint-guided)
    void   release(Grain*);  // audio side, marks slot free
};
```

The audio thread's *active* list is separately capped at `MaxActiveGrains = 256`;
grains beyond that are dropped at the FIFO drain.

### Incoming grain FIFO

`juce::AbstractFifo` configured for SPSC. Capacity matches `GrainPool`. Workers push fully-rendered grain descriptors; the audio thread pops them at the start of each `processBlock`.

### `GranularEngine`

The heart of the audio thread side. Each `processBlock(buffer)`:

1. `trySwapAll()` on the sample bank; if a new buffer swapped in, drains all active
   grains (their source pointers are stale).
2. Drains the incoming-grain FIFO into a fixed active-grain array (cap 256).
3. For each active grain, renders the whole block: read source at
   `startPos + phase * lengthSamples * pitchRatio` (fractional interpolation),
   apply envelope at the normalized phase, scale by pan, accumulate into L and R.
   Phase advances by `1/lengthSamples` per output sample; at phase ≥ 1 the grain
   is released back to the pool.
4. Applies master volume (with its mod-matrix offset) and writes the grain
   snapshot telemetry.

Grain *rendering* happens here, on the audio thread — the scheduler only produces
descriptors. The per-grain loop is scalar; at the 256-grain cap it fits the CPU
budget without SIMD batching.

### Grain spawning workflow

```
Scheduler thread wakes (stochastic interval or Pattern override)
        ↓
Pattern fire gate (probability / Euclidean step / transient) — may skip
        ↓
Scheduler picks source (Spectral > Feedback > bank slot > 440 Hz sine),
computes position (+ loop sweep + mod offsets + jitter), pitch, pan, envelope
        ↓
Scheduler acquires a Grain slot from GrainPool, fills the descriptor,
spawns sub-grains if subGrainDepth > 0
        ↓
Scheduler pushes Grain* onto the SPSC FIFO
        ↓
Audio thread drains FIFO at the start of the next processBlock
Audio thread renders the grain into the output
```

Latency from "spawn" to "audible" is typically ≤ 1 audio block plus the scheduler's
wake granularity. The FIFO (512 slots) absorbs jitter.

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

Implementation: the scheduler builds the parent, pushes it, then builds the children
from a pre-push copy of the parent and pushes those (`SubGrain::spawnInto`). Sub-grains
that don't fit the FIFO's first contiguous region are dropped (acceptable at normal
densities).

Limits: pool capacity 1024, audio-thread active cap 256, depth capped at 2. Worst-case
expansion per parent is 1 + 4 + 4×3 = 17 grains.

---

## Stochastic timing distributions (F6)

Six distributions for inter-grain timing intervals:

| Name | Formula (mean = 1/density seconds; shape constants are fixed, there is no user jitter parameter) | Character |
|------|---|---|
| Uniform | `U(0.5, 1.5) × mean` | Even, predictable. |
| Gaussian | `Normal(μ = mean, σ = 0.3 × mean)` | Soft randomness. |
| Poisson | `-mean × ln(U)` (exponential inter-arrival — the mathematically correct Poisson-process spacing) | Memory-less, "natural" event timing. |
| Exponential | `-mean × ln(U)` — **identical to Poisson as shipped**; the menu keeps both names for user familiarity | Memory-less clustering. |
| Pareto | α = 1.5, clamped at 5× mean | Sparse clusters of rapid grains. |
| 1/f (fractal) | Low-pass-filtered noise modulating the interval | Self-similar, musical. |

All implemented in `Source/Engine/StochasticTiming.cpp`. Unit tests verify distribution
moments against theoretical values (Pareto at 20% tolerance — clamping shifts the mean;
see STATE.md decision).

The chosen distribution determines the next grain spawn time the engine schedules. The trigger module (Free / Sync / Euclidean / Audio-driven) determines the base rate; the distribution adds shape.

---

## Spectral processing (F6)

`SpectralProcessor` runs on the message thread (file load, spectral parameter change,
or state restore) — it is a **one-shot source transformation**, not a streaming STFT:

1. The first 2048 samples of bank slot 0 are Hann-windowed and FFT'd
   (`kFFTSize = 2048`, `juce::dsp::FFT`).
2. Bins are modified per mode: `Freeze` zeroes all phases (static spectral texture);
   `Blur` randomizes phases and jitters magnitudes by `blurAmount`.
3. One IFFT back to time domain (JUCE normalises by 1/N internally — no manual
   division; see STATE.md decision) into a 2048-sample output buffer.
4. While spectral is enabled, the engine granulates this 2048-sample buffer instead
   of the sample bank (source priority: Spectral > Feedback > bank > sine).

Latency: `setLatencySamples(2048)` is reported while enabled and returned to 0 when
disabled (updated at runtime via the parameter listener + 30 Hz timer, and again in
`prepareToPlay` / state restore).

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
2. **No locks.** Use atomics for cross-thread state. Use lock-free FIFOs for handoff.
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
| Engine classes (Grain, GrainPool, GranularEngine) | Catch2 + manual harness | Correctness of grain accumulation, pool overflow handling, FIFO behavior. |
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
