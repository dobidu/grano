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
- Updates lock-free telemetry counters (active grain count, current position, CPU sample).
- **Forbidden**: allocation (`new`, `malloc`, container resize), locks, system calls, exceptions, virtual calls through unbounded chains.

### Message thread / UI thread (JUCE-owned)

- Runs editor paint, mouse, keyboard, MIDI control events.
- Reads APVTS atomics and engine telemetry counters for display.
- Updates controls via `APVTS::SliderAttachment` and similar (which post parameter changes to APVTS internally).
- Drives a `juce::Timer` at 30 Hz that triggers `WaveformDisplay::repaint()` and updates particle positions.
- Allocates freely. Slow operations (sample loading) are dispatched to worker threads, never run inline.

### Worker thread pool (our `juce::ThreadPool`, 3-4 workers)

- Runs **grain generation tasks**: given a grain request descriptor (source pointer, position, length, pitch, envelope), produces a fully-rendered grain buffer (pitch-shifted, windowed, panned) and pushes it onto the incoming-grain FIFO.
- Runs **sample loading**: when the user drops a file, a worker reads it via `juce::AudioFormatManager`, decodes into a `juce::AudioBuffer<float>`, and atomically swaps the engine's `SampleBuffer` pointer.
- Runs **spectral FFT** (F6): when Spectral Freeze/Blur is engaged, a worker computes FFT, modifies bins, IFFTs back, and writes to a buffer that future grains can sample from.
- May allocate, may take ms-to-tens-of-ms, may call into the OS. Cannot block on the audio thread.

### Communication primitives

| Direction | Mechanism | Reasoning |
|---|---|---|
| UI → Audio (parameter changes) | APVTS atomic load | Lock-free, sample-accurate, host automation works. |
| UI → Workers (sample loading, grain spawn requests) | `juce::ThreadPool::addJob` | Standard JUCE pattern. |
| Workers → Audio (ready grains) | SPSC `juce::AbstractFifo` carrying grain descriptors | Lock-free, predictable. Audio drains; never blocks. |
| Audio → UI (telemetry) | `std::atomic<int/float>` counters | UI reads opportunistically at 30 Hz; staleness is fine. |
| Workers → Workers | Independent; each worker writes to a private buffer pool then publishes via atomic exchange | No shared mutable state. |

---

## The granular engine in detail

### `Grain` (POD-like, ~ 64 bytes)

Represents one in-flight grain. Fields:

```cpp
struct Grain
{
    const float* sourceLeft;        // pointer into SampleBuffer
    const float* sourceRight;       // ditto, may equal sourceLeft for mono
    int sourceLengthSamples;
    float currentPhase;             // 0.0 to lengthSamples, accumulates
    float pitchRatio;               // 1.0 = original speed
    float pan;                      // -1.0 (L) to +1.0 (R)
    int envelopeShape;              // enum index
    float envelopeParam;            // shape-dependent (e.g. Tukey plateau)
    int totalLengthSamples;         // grain length, not source length
    bool reverse;
    bool active;
};
```

No virtuals. No constructors that allocate. Pre-filled by workers, consumed by audio.

### `GrainPool`

Fixed-size array of `Grain` slots, pre-allocated. Capacity 1024 in default build. Slot allocation is via atomic index increment with modulo — no free-list, no lock.

```cpp
class GrainPool {
    std::array<Grain, 1024> slots_;
    std::atomic<int> writeIndex_{0};
public:
    Grain* acquire();   // worker side
    void release(Grain* g);  // audio side, marks inactive
};
```

### Incoming grain FIFO

`juce::AbstractFifo` configured for SPSC. Capacity matches `GrainPool`. Workers push fully-rendered grain descriptors; the audio thread pops them at the start of each `processBlock`.

### `GranularEngine`

The heart of the audio thread side. Each `process(buffer)`:

1. Drains the incoming-grain FIFO into a local "active grains" linked list (intrusive, allocation-free).
2. For each sample in the output buffer:
   - For each active grain: read source at `currentPhase` (with fractional interpolation), apply envelope at `currentPhase / totalLengthSamples`, scale by pan, accumulate into L and R.
   - Advance `currentPhase += pitchRatio`.
   - If `currentPhase >= totalLengthSamples`: mark grain inactive, recycle to pool.
3. Update telemetry (active grain count atomic).
4. Schedule next grain spawn via the **trigger module** (see Pattern section below): when it's time to spawn, push a request onto the worker queue.

For grain counts up to ~200, this loop is single-pass per sample. For higher counts, we batch by SIMD-vectorizing the envelope and source-read steps (`juce::dsp::SIMDRegister<float>`).

### Grain spawning workflow

```
Audio thread says "spawn now"  →  push GrainRequest to worker queue
        ↓
Worker picks up GrainRequest
        ↓
Worker reads from SampleBuffer at requested position
Worker computes pitch ratio, applies pitch shift if needed
Worker fills Grain slot from GrainPool
        ↓
Worker pushes ready Grain* onto SPSC FIFO to audio thread
        ↓
Audio thread drains FIFO at start of next processBlock
Audio thread renders the grain into the output
```

Latency from "spawn now" to "audible" is typically 1-2 audio blocks (≤ 22 ms at 1024 samples / 48 kHz). For real-time-deadline grain spawning (the trigger module's pulse), this is acceptable; the FIFO size absorbs jitter.

---

## Parameter management

`juce::AudioProcessorValueTreeState` (APVTS) owns all exposed parameters. Layout defined once in `Source/Parameters.cpp::createLayout()`.

### Naming convention

Parameter IDs are stable strings: `grainSize`, `density`, `position`, `positionJitter`, `pitchShift`, `stereoSpread`, `masterVolume`, `loopEnabled`, `motionEnabled`, `motionWow`, `motionFlutter`, etc. Use `juce::ParameterID` with version hint to allow future migrations.

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
struct ModSlot {
    int sourceIndex;       // 0=LFO1, 1=LFO2, -1=disabled
    juce::String destParamId;
    float amount;          // -1.0 to +1.0
};
```

Per audio block:

1. For each LFO, compute its current value (at audio rate or block rate depending on destination).
2. For each active slot: read the destination parameter's base value from APVTS, add `lfo * amount * range`, write to a per-parameter "effective value" cache. Audio thread reads this cache, not the raw APVTS, for modulated parameters.

This avoids modulating APVTS directly (which would conflict with host automation and create unstable visual feedback).

---

## Sub-grain recursion (F6)

When `subGrainDepth > 0`, each scheduled grain is replaced by 2-8 sub-grains:

1. The parent grain's `totalLengthSamples` is divided by N (N = 2..8, configurable as `subGrainCount`).
2. Each sub-grain has a smaller `totalLengthSamples` and a slightly jittered position, pitch, and pan from the parent.
3. If `subGrainDepth > 1`, each sub-grain may itself recurse.

Implementation: in the worker, after building a `Grain`, if recursion is requested, the worker builds the children and pushes each onto the FIFO. The parent grain is **not** pushed (it's effectively replaced by its children).

Limits: hard cap on total grain count regardless of recursion (1024 active). Depth capped at 2. Count per parent capped at 8. This bounds the recursion expansion to 64× and the hard pool cap prevents pathological cases.

---

## Stochastic timing distributions (F6)

Six distributions for inter-grain timing intervals:

| Name | Formula (delta in seconds) | Character |
|------|---|---|
| Uniform | `1.0 / density` ± `jitter` | Even, predictable. |
| Gaussian | `Normal(μ = 1/density, σ = jitter)` | Soft randomness. |
| Poisson | `-ln(U) / density` | Memory-less, "natural" event timing. |
| Exponential | `-ln(U) * jitter + 1/density` | Heavy-tailed clustering. |
| Pareto | `(1 + jitter / U)^α` | Sparse clusters of rapid grains. |
| 1/f (fractal) | Spectral synthesis via low-pass-filtered noise | Self-similar, musical. |

All implemented in `Source/Engine/StochasticTiming.cpp`. Unit tests verify the distribution moments match the theoretical values within 5% over 10,000 samples.

The chosen distribution determines the next grain spawn time the engine schedules. The trigger module (Free / Sync / Euclidean / Audio-driven) determines the base rate; the distribution adds shape.

---

## Spectral processing (F6)

`SpectralProcessor` runs on a worker thread:

1. Buffer accumulates input until 2048 samples are available (`FFT_SIZE = 2048`, `HOP_SIZE = 512`, 75% overlap).
2. Windowed (Hann), FFT'd via `juce::dsp::FFT`.
3. Bins are modified per mode: `Freeze` holds the spectrum, `Blur` averages adjacent bins.
4. IFFT, windowed again, overlap-added into the output buffer.
5. Output buffer is exposed as a `SampleBuffer` source that grain workers can read from.

Latency: 2048 samples ≈ 42 ms at 48 kHz. Reported to host via `setLatencySamples(2048)` when spectral mode is engaged. Bypass returns latency to 0.

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
3. **No syscalls.** No file I/O, no `printf`, no logging via OS-level handles. (We have a lock-free SPSC log buffer the UI drains; the audio thread writes to it non-blocking.)
4. **No exceptions.** Compile with `-fno-exceptions` for audio-thread translation units (Engine/, Modulation/).
5. **No virtual dispatch through chains.** If polymorphism is needed (e.g., envelope shapes), prefer `enum + switch` or CRTP. Single virtual at the boundary is fine.
6. **Bounded loop counts.** Every loop has a known upper bound. No `while (condition_from_external_state)`.

Code review and PAUL guardrails enforce these.

---

## Testing strategy

| Layer | Tooling | What's tested |
|---|---|---|
| Pure utilities (DSPMath, EnvelopeShapes) | Catch2 unit tests | Numerical correctness, edge cases. |
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
