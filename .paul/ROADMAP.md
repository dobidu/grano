# ROADMAP — Grano

Eight phases from bare repo to validated release. Each phase has explicit entry/exit criteria. PAUL enforces single-phase focus: only one phase is `current` at a time.

---

## F0 — Foundation

**Goal**: A buildable empty plugin shell on Windows, macOS, and Linux. CI green. No DSP yet.

**Entry**: bootstrap pack unzipped, git initialized, PAUL initialized.

**Exit criteria**:
- `cmake -B build && cmake --build build` succeeds on all three platforms.
- Produces VST3, AU (macOS only), and Standalone artifacts.
- The Standalone opens, shows a window with the plugin name. No controls yet.
- `pluginval --strictness-level 5` passes on the VST3 with no errors.
- GitHub Actions CI matrix (Windows + macOS) passes on `main` branch.
- `Tests/` contains at least one passing Catch2 test asserting the plugin's name string.

**Deliverables**: `CMakeLists.txt`, `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.{h,cpp}`, `Tests/test_main.cpp`, `.github/workflows/ci.yml`.

**Effort estimate**: 1-2 days.

---

## F1 — Granular Engine Core

**Goal**: Working granular engine that produces audio from a hard-coded loaded sample. No UI controls yet — values are constants in code.

**Entry**: F0 complete and unified.

**Exit criteria**:
- Class `Grain` with: source pointer, start position, length in samples, pitch ratio, pan, envelope shape, current phase. Real-time-safe construction (no allocation post-init).
- Class `GranularEngine` owning a pre-allocated grain pool (1024 slots), an SPSC `juce::AbstractFifo` for spawn requests, and a `juce::ThreadPool` for grain generation workers.
- `GranularEngine::processBlock` consumes ready grains from the FIFO and mixes them into the output. Audio thread allocates nothing.
- A hard-coded test sample plays back as a granular cloud at 50 grains/sec, 100ms grain size, with Hann window. Verified by ear and by inspecting `processBlock` output via a debug tap.
- Unit tests: grain pool overflow handling, envelope shape correctness (Hann/Tukey/Gauss/Triangle/Rectangle), pitch ratio integer/fractional cases.
- TSAN run in CI passes.

**Deliverables**: `Source/Engine/Grain.{h,cpp}`, `Source/Engine/GranularEngine.{h,cpp}`, `Source/Engine/GrainPool.{h,cpp}`, `Source/Engine/EnvelopeShapes.{h,cpp}`, `Tests/test_engine.cpp`.

**Effort estimate**: 4-6 days.

---

## F2 — Sample I/O and Waveform Display

**Goal**: User can drag a sample file onto the plugin, it loads, and the waveform appears. Playhead cursor follows the grain position. Particles for active grains appear on the waveform.

**Entry**: F1 complete and unified.

**Exit criteria**:
- `juce::AudioFormatManager` registered with WAV, AIFF, MP3 readers. MP3 is gated by `JUCE_USE_MP3AUDIOFORMAT`.
- Drag & drop from OS file manager into the editor loads the sample, replaces the engine's source buffer atomically.
- Malformed files (truncated, wrong format, zero-length) are rejected gracefully with a visible error message; no crash.
- `WaveformDisplay` component renders the waveform with `juce::AudioThumbnail`. Playhead cursor (1.5 px vertical line, `#7CF5C4`) at current position.
- Active grain particles overlaid on the waveform: each particle a translucent dot (`#C77CFF`, opacity proportional to grain envelope amplitude, position = grain's source position, vertical jitter optional for visual depth).
- Particle rendering capped at 60 fps regardless of grain count. Profile to confirm no jank above 200 grains.

**Deliverables**: `Source/UI/WaveformDisplay.{h,cpp}`, `Source/Engine/SampleBuffer.{h,cpp}`, `Tests/test_sample_io.cpp`.

**Effort estimate**: 3-4 days.

---

## F3 — Core Controls and APVTS

**Goal**: All 6 core controls (4 knobs + 2 sliders) plus Master Volume and Loop toggle are wired to the engine via APVTS. State persists across plugin instances (preset save/load works).

**Entry**: F2 complete and unified.

**Exit criteria**:
- Single `juce::AudioProcessorValueTreeState` instance owns all parameters.
- Parameters defined: `grainSize` (5-500 ms, log-skewed), `density` (1-100 grains/s, log-skewed), `position` (0-100%), `positionJitter` (0-100%), `pitchShift` (-24 to +24 semitones, linear), `stereoSpread` (0-100%), `masterVolume` (-inf to +6 dB), `loop` (bool).
- UI binds via `SliderAttachment`, `ButtonAttachment`. Custom `LookAndFeel` applied (per `DESIGN_SPEC.md`).
- Engine reads parameters via `APVTS::getRawParameterValue` (atomic load on audio thread, no locks).
- Host automation works: tested in Reaper and Ableton, both sweep all parameters.
- Preset save/load via `juce::AudioProcessor::getStateInformation` / `setStateInformation` round-trips correctly.

**Deliverables**: `Source/Parameters.{h,cpp}` (parameter layout and IDs), updated `PluginProcessor` and `PluginEditor`, `Source/UI/LookAndFeel/GranoLAF.{h,cpp}` (skeleton), `Source/UI/Knob.{h,cpp}`, `Source/UI/Slider.{h,cpp}`, `Tests/test_parameters.cpp`.

**Effort estimate**: 3-4 days.

---

## F4 — Processing Modules

**Goal**: Three optional processing modules (MOTION, COLOR, PATTERN) each independently bypassable. UI sections per `DESIGN_SPEC.md`.

This phase is split into three sub-plans, executed sequentially.

### F4a — MOTION

**Exit criteria**:
- Wow LFO (0.1-2 Hz, sine) modulating grain pitch.
- Flutter LFO (3-20 Hz, sine + small noise) modulating grain pitch.
- Drift: very slow random walk on master pitch, ±20 cents over minutes.
- Crackle: filtered pink noise overlay, -60 to 0 dB, with color (HPF/LPF) control.
- Module bypass toggle: zero CPU when off.

### F4b — COLOR

**Exit criteria**:
- Saturate: continuous knob from soft tape sat through harmonic exciter into bitcrush. Internal modeled as a single waveshaper with curve interpolation.
- Decimate: bit depth (4-24 bits) and sample rate reduction (8 kHz to host SR) combined under a single control.
- Tilt EQ: ±12 dB tilt around 1 kHz.
- Verb: short plate/spring reverb, mix 0-100%. Built on `juce::dsp::Convolution` or a simple FDN.
- Module bypass toggle: zero CPU when off.

### F4c — PATTERN

**Exit criteria**:
- Trigger Mode selector: `Free`, `Sync`, `Euclidean`, `Audio-driven`.
- Free mode uses density knob in Hz.
- Sync mode uses host BPM and divisions: 1/4 .. 1/64, T (triplet) and D (dotted) modifiers.
- Euclidean: pulses (1-32), steps (1-32), rotation (0 to steps-1).
- Audio-driven: input transient detector with sensitivity and minimum-interval controls.
- Probability: 0-100% chance per scheduled grain to actually fire.
- Reverse Probability: 0-100% chance per fired grain to play reversed.
- Quantize: pitch shift snapped to selected scale (Chromatic, Major, Minor, Dorian, Phrygian, Lydian, Mixolydian, Pentatonic Major, Pentatonic Minor, Whole Tone, Octatonic, Custom-Scala).
- Spray: macro knob increasing randomness on per-grain pitch, duration, and envelope.

**Effort estimate**: 7-9 days for the full F4 (3 sub-plans).

---

## F5 — Modulation Matrix and Snapshots

**Goal**: Two LFOs assignable to any parameter. Four parameter snapshots A/B/C/D with instant switching.

**Entry**: F4 complete and unified (all three sub-phases).

**Exit criteria**:
- Two LFO instances: rate (0.01 Hz - 8 kHz, log-skewed), waveform (Sine, Triangle, Saw, Square, Random, S&H, Drawable), free or DAW-sync, phase offset, depth.
- Drawable waveform: stored as 64-point breakpoint envelope, edited via click-and-drag on the LFO's waveform display.
- Modulation matrix: minimum 8 slots, each routing source (LFO1, LFO2) → destination (any APVTS parameter) with amount (-100% to +100%).
- LFO updates run at audio rate when destination is a sample-accurate parameter (pitch, position-jitter), at control rate (block) otherwise.
- UI: LFO panel with waveform preview, rate display, destination dropdown, depth slider. Cross-modulation supported (LFO1 modulating LFO2's rate).
- Snapshots A/B/C/D: 4 buttons in the header, instant recall, optional 250 ms ramp on switch (configurable).

**Deliverables**: `Source/Modulation/Lfo.{h,cpp}`, `Source/Modulation/ModMatrix.{h,cpp}`, `Source/Modulation/Snapshots.{h,cpp}`, `Source/UI/LfoPanel.{h,cpp}`, `Source/UI/ModulationMatrixView.{h,cpp}`, `Tests/test_modulation.cpp`.

**Effort estimate**: 5-7 days.

---

## F6 — UI Polish and Sub-grain + Spectral + Stochastic

**Goal**: Visual polish per `DESIGN_SPEC.md`, plus the advanced "loud-and-loose" features (sub-grain recursion, stochastic distributions, internal feedback, spectral freeze/blur, 4-sample multi-blend, pitch/envelope curves drawable).

**Entry**: F5 complete and unified.

**Exit criteria**:
- LookAndFeel finalized: halo glows via `Graphics::drawDropShadow`, radial vignette background, particle animation with trails (motion blur ≈ 60 ms decay).
- Sub-grain recursion: depth 0/1/2 selector. Verified by listening tests: depth 2 produces noticeably more "pulverized" texture than depth 0.
- Stochastic distribution selector for inter-grain timing: Uniform, Gaussian, Poisson, Exponential, Pareto, 1/f (fractal). Each implemented mathematically correctly; verified by histograms of inter-arrival times in unit tests.
- Internal feedback: optional feedback path, audio output rolling back into a secondary sample buffer with dampening filter and gain. Stability verified by stress test (24h with full feedback) — must not diverge.
- Spectral Freeze and Spectral Blur: FFT-based, overlap-add, 2048 FFT size, Hann window, 75% overlap. Latency reported to host. Off by default.
- Multi-sample slots: 4 slots, each loadable independently. Per-grain weighted random selection.
- Pitch and envelope curves drawable: a panel showing the current grain's pitch trajectory and amplitude envelope, both editable via control points.
- Spectrogram secondary display toggleable.

**Deliverables**: `Source/Engine/SubGrain.{h,cpp}`, `Source/Engine/StochasticTiming.{h,cpp}`, `Source/Engine/FeedbackPath.{h,cpp}`, `Source/Engine/SpectralProcessor.{h,cpp}`, `Source/Engine/MultiSampleBank.{h,cpp}`, `Source/UI/CurveEditor.{h,cpp}`, `Source/UI/Spectrogram.{h,cpp}`, full polish pass on `GranoLAF`.

**Effort estimate**: 10-14 days.

---

## F7 — Release

**Goal**: Validated builds, code signing, installers, beta distribution.

**Entry**: F6 complete and unified.

**Exit criteria**:
- `pluginval --strictness-level 10` passes on Windows and macOS VST3.
- `auval -v aumu Gran Bidu` passes on macOS AU.
- Code signing and notarization on macOS, including standalone and AU/VST3 bundles.
- Windows: code signing certificate applied (or instructions documented for self-signing during beta).
- Installers: macOS `.pkg` (with notarization), Windows `.exe` via Inno Setup or similar, Linux `.tar.gz`.
- 5 demo presets ship with the plugin: at least one each of `Cloud`, `Drone`, `Rhythmic`, `Spectral`, `Broken`.
- Audio demo set (5-10 short examples) recorded and posted in the project's wiki / repo.
- README updated for end users with download links.
- Beta release tagged `v1.0.0-beta.1` on GitHub.

**Deliverables**: signed and notarized installers per OS, `Resources/Presets/*.preset`, demo audio in `Resources/Audio/Demos/`, `RELEASE_NOTES.md`.

**Effort estimate**: 5-7 days.

---

## Total estimated effort

35-50 days of focused work, depending on iteration speed and whether you take Linux fully through CI from day one.

## Phase dependencies graph

```
F0 → F1 → F2 → F3 → F4 (a → b → c) → F5 → F6 → F7
```

No parallel branches in v1.0. After F7, v1.1 work can branch (XY pad/morph, additional modules, etc.).

## Deferred features (post v1.0)

- XY pad macro control with 4-parameter morphing.
- Continuous A↔B↔C↔D snapshot morph.
- MPE input support.
- AAX format.
- Surround / multichannel output.
- Built-in sample browser with metadata.
- Cloud preset library.
- Pitch tracking and harmonic re-synthesis.

These go into `consider-issues` and are revisited only after v1.0 ships.
