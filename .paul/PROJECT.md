# PROJECT — Grano

## Identity

**Name**: Grano
**Type**: Audio plugin instrument — granular synthesis
**Tagline**: A granular instrument for experimental sound design.

## Purpose

Grano is an instrument plugin designed for producers, sound designers, and live performers who need a granular engine that goes beyond conventional textural synthesis. Existing granular plugins typically offer a polished but narrow palette. Grano targets the territory of advanced granular sound design: dense particle clouds, sub-grain recursion, stochastic distributions, internal feedback paths, spectral processing, and audio-rate parameter modulation — all under disciplined, performable controls.

The plugin should be technically rigorous (real-time safe, low-latency, multi-platform) and aesthetically distinctive (visually clear, performatively responsive).

## Goals

| ID | Goal | Success criterion |
|----|------|---|
| G1 | Stable, real-time granular engine | No xruns at 512-sample buffer @ 48 kHz with up to 200 concurrent grains on a 2020-era laptop. |
| G2 | Multi-platform plugin | Builds and validates (pluginval verbosity 5) for VST3 + AU + Standalone on Windows 10+, macOS 11+, and Linux Ubuntu 22.04+. |
| G3 | Distinctive sonic territory | The plugin produces sounds that conventional granular plugins cannot — verified by audio examples covering sub-grain recursion, stochastic timing, feedback, and spectral processing. |
| G4 | Performable interface | All MVP parameters reachable in one screen, no menu diving for core functions. MIDI learn on any parameter. 4 snapshots with instant switching. |
| G5 | Robust loading | Drag & drop WAV/AIFF/MP3 with no crash on malformed files. Graceful degradation. |
| G6 | Modulation depth | Two LFOs (free or DAW-sync) routable to any parameter, capable of audio-rate modulation up to 8 kHz. |

## Non-goals

- Real-time pitch tracking of the input or harmonic re-synthesis (out of scope for v1.0).
- ARA (Audio Random Access) integration — not relevant for a granular instrument that loads its own samples.
- AAX format — deferred. Avid SDK approval cost not justified for v1.0.
- Browser/cloud sample library — deferred. v1.0 ships local sample loading only.
- Built-in sequencer beyond the Euclidean and stochastic trigger modes — full MIDI sequencer is out of scope.
- Surround / Atmos output — stereo only for v1.0.

## Stack

| Component | Choice | Reasoning |
|-----------|--------|---|
| Language | C++20 | Modern features (concepts, ranges, `std::span`, `std::format`) without sacrificing real-time guarantees. |
| Framework | JUCE 8 | Industry standard for cross-platform audio plugins. JUCE 8 introduced CMake-first workflow and modernized `juce::dsp`. |
| Build system | CMake 3.22+ | First-class JUCE 8 support, multi-IDE, easier CI than Projucer. |
| DSP primitives | `juce::dsp` + custom | `juce::dsp::FFT`, `juce::dsp::ProcessorChain`, `juce::dsp::Convolution` for spectral and filtering. Custom for granular core. |
| Parameter management | `juce::AudioProcessorValueTreeState` (APVTS) | Standard JUCE pattern. Handles automation, undo, host integration. |
| Lock-free communication | `juce::AbstractFifo` | Single-producer single-consumer queue between worker threads and audio thread. |
| Worker threads | `juce::ThreadPool` | For grain generation off the audio thread. |
| Plugin formats | VST3, AU, Standalone | Primary VST3 (cross-platform), AU (macOS DAWs Logic / GarageBand), Standalone (debug, live use). |
| Plugin format excluded | ARA, AAX | See Non-goals. |
| Audio formats | WAV, AIFF, MP3 | Via `juce::AudioFormatManager`. MP3 requires the `JUCE_USE_MP3AUDIOFORMAT` flag. |
| Process orchestration | PAUL | Plan/Apply/Unify Loop for disciplined development. |
| Token compression | Caveman | ~65% token reduction in Claude Code sessions. |
| Code navigation | Graphify | Knowledge graph of the codebase consulted before file reads. |
| Testing | Catch2 v3 | Header-only-ish, modern, good JUCE integration patterns. |
| Plugin validation | pluginval (Tracktion), auval (Apple) | Industry-standard validators. |

## Architectural Decisions (ADRs)

### ADR-001: Multi-threaded grain generation with lock-free FIFO

We will use a dedicated `juce::ThreadPool` of 3-4 worker threads to generate grains (sample buffer copy, pitch shift, envelope application). Generated grains are pushed to a single-producer single-consumer (SPSC) lock-free FIFO. The audio thread (`processBlock`) consumes ready grains from the FIFO and mixes them into the output. Worker threads never touch real-time-allocated state.

**Why**: This allows densities of 500-1000 concurrent grains. A single-threaded implementation tops out around 100-200 grains depending on grain size. The complexity cost is well-managed by JUCE's existing FIFO primitives.

**Alternative rejected**: Single-threaded engine running everything on the audio thread. Simpler but caps the sonic territory. Rejected because density is one of the project's three differentiating axes (along with sub-grain recursion and stochastic distributions).

### ADR-002: Sub-grain recursion up to depth 2

Each grain may optionally generate child sub-grains, recursively, up to a configurable depth of 2. This is implemented in the worker threads as a recursive call during grain construction (not at audio rate).

**Why**: Sub-grain recursion is one of the project's signature sonic capabilities. Limiting depth to 2 prevents runaway grain explosions. Empirically, depth 2 with reasonable density already produces "pulverization" textures rarely heard in commercial granular plugins.

### ADR-003: APVTS as the single source of parameter truth

All exposed parameters (knobs, sliders, toggles) live in a single `juce::AudioProcessorValueTreeState`. The UI binds via `SliderAttachment`, `ButtonAttachment`, `ComboBoxAttachment`. Snapshots A/B/C/D are stored as `juce::ValueTree` subtrees on top of the APVTS state.

**Why**: APVTS is the standard JUCE pattern. It gives us automation, undo, preset save/load, and host integration with minimal custom code.

### ADR-004: Audio-rate-capable LFOs implemented as `juce::dsp::Oscillator` subclasses

The two LFOs are implemented as classes wrapping `juce::dsp::Oscillator<float>` with extended waveform options (drawable, S&H, random) added via custom `processSample` overrides. Rates range from 0.01 Hz to 8 kHz. When the rate exceeds 20 Hz, the LFO essentially becomes an audio-rate modulator (FM, AM, etc.), which is intentional.

**Why**: Audio-rate modulation is one of the project's signature features. Using `juce::dsp::Oscillator` as a base ensures SIMD-optimized rendering for common waveforms; we extend it for the unusual ones.

### ADR-005: Spectral processing via JUCE's FFT with overlap-add

Spectral Freeze and Spectral Blur are implemented using `juce::dsp::FFT` with overlap-add (Hann window, 75% overlap, FFT size 2048). This runs on the worker threads, not the audio thread; the frozen/blurred buffer is then available as a sample source for subsequent grain generation.

**Why**: FFT in real-time on the audio thread is risky (cache spikes, alloc on platforms). Doing it on workers and feeding results forward keeps the audio thread predictable.

### ADR-006: Stereo only for v1.0

Output channel count is fixed at 2 (stereo). No surround, no Ambisonics, no Atmos in v1.0.

**Why**: Scope discipline. Multichannel adds substantial complexity to grain panning, modulation routing, and visualization. Worth revisiting in v2.0.

### ADR-007: Naming neutrality

No code, comment, doc, or asset will name any reference artist, band, or commercial plugin product. Module names and parameter names are functional and technical. This applies to git history as well.

**Why**: Long-term project hygiene. Avoids any future legal-perception risk and keeps the engineering vocabulary clean.

### ADR-008: Linux as a secondary target

The codebase will compile and run on Linux (Ubuntu 22.04+), but CI prioritizes Windows and macOS builds. Linux packaging (VST3 only, no AU) is provided in F7 but is not part of the gating release criteria.

**Why**: Linux is valuable for the target audience (Bitwig, Reaper, Ardour users), and JUCE supports it natively, but treating it as primary would double CI cost and packaging complexity. Secondary tier matches the scale of the project.

### ADR-009: Visual identity "Lab Equipment Brutalist"

The plugin UI uses a dark theme with carbon backgrounds (`#0A0B0D` → `#1F2329`), warm off-white text (`#E8E6E1`), and a semantic accent palette: mint green (`#7CF5C4`) for vital signs / active values, violet (`#C77CFF`) for grain particles, magenta (`#FF6B9D`) for modulation, amber (`#FFB347`) for the Motion module, red (`#FF5252`) only for clipping. Typography is Inter (sans labels) + JetBrains Mono (numeric values). Brutalist geometry: 4px radius, 1px borders, no skeumorphism.

**Why**: See `DESIGN_SPEC.md` for the full rationale. The short version: the palette is semantically functional (each color carries meaning), the typography is technical-instrument-grade, and the visual identity is distinct from competitor plugins.

## Glossary

- **Grain**: A short snippet of audio (5–500 ms) extracted from a sample buffer, windowed by an envelope, and played back at a chosen pitch and stereo position.
- **Density**: Number of grains triggered per second.
- **Position**: Index into the source sample buffer from which the next grain starts.
- **Jitter**: Random deviation applied per-grain (typically to position).
- **Sub-grain**: A grain that is itself granulated into smaller children before playback.
- **Stochastic distribution**: A probability distribution governing the inter-grain time interval. Six are supported: Uniform, Gaussian, Poisson, Exponential, Pareto, 1/f (fractal).
- **Trigger Mode**: How grains are scheduled: `Free` (Hz), `Sync` (DAW divisions), `Euclidean` (k pulses in n steps, rotation r), `Audio-driven` (input transient detection).
- **MOTION / COLOR / PATTERN**: The three optional processing modules.
- **APVTS**: `juce::AudioProcessorValueTreeState` — the parameter management container.
- **SPSC FIFO**: Single-producer single-consumer first-in-first-out lock-free queue, used for inter-thread communication.
- **Snapshot**: A complete parameter state saved into one of four slots (A/B/C/D).
- **LookAndFeel**: JUCE's mechanism for customizing the rendering of UI controls.

## Constraints

- Real-time safety: no allocation, no locks, no system calls, no exceptions on the audio thread.
- Latency: total plugin latency ≤ 64 samples @ 48 kHz (≈ 1.3 ms) excluding spectral processing modules (which advertise their own latency to the host via `setLatencySamples`).
- CPU budget: nominal load ≤ 15% on a 2020 i7 with default settings (200 grains, all modules enabled). Worst case ≤ 40%.
- Memory: ≤ 200 MB resident per instance with all 4 sample slots loaded at typical lengths (≤ 60s each).
- Sample rate range: 44.1 kHz to 192 kHz. Internal DSP works at the host's rate (no internal resampling).

## Glossary of file conventions

| Path | Purpose |
|------|---------|
| `Source/` | Plugin source code. |
| `Source/PluginProcessor.{h,cpp}` | The `AudioProcessor` subclass. |
| `Source/PluginEditor.{h,cpp}` | The `AudioProcessorEditor` subclass. |
| `Source/Engine/` | Granular engine, grain pool, FIFOs. |
| `Source/Modules/` | MOTION, COLOR, PATTERN modules. |
| `Source/Modulation/` | LFOs, modulation matrix, snapshots. |
| `Source/UI/` | LookAndFeel, custom components, WaveformDisplay. |
| `Source/Utils/` | DSP utilities, math helpers, constants. |
| `Resources/` | Embedded fonts, default samples (if any), preset library. |
| `Tests/` | Catch2 unit tests. |
| `Builds/` | Generated by CMake — gitignored. |
| `JuceLibraryCode/` | Generated — gitignored. |
| `graphify-out/` | Graphify graph output — committed except `manifest.json` and `cost.json`. |

## Risks and mitigations

| Risk | Mitigation |
|------|-----------|
| Multi-threading bugs (race conditions, false sharing) | Use only proven JUCE FIFOs. Property-based tests on grain pool with TSAN runs in CI. |
| FFT latency exceeds buffer | Limit spectral processing to worker threads and report latency to host via `setLatencySamples`. |
| MP3 loading inconsistency across platforms | `JUCE_USE_MP3AUDIOFORMAT` is platform-specific in licensing. Document fallback (WAV/AIFF only) if MP3 patent licensing is a concern for distribution. |
| UI performance with many grains | Cap particle rendering at 60 fps; throttle waveform redraws via `juce::ComponentAnimator` and `Timer` at 30 Hz; render particles via `Graphics::fillPath` not per-particle draws. |
| Cross-platform font availability | Embed Inter and JetBrains Mono as resources via `BinaryData`. |
| Scope creep | PAUL loop enforces phase boundaries; `consider-issues` is the only place to add deferred work. |
