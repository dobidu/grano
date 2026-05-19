# Grano

[![CI](https://github.com/dobidu/grano/actions/workflows/ci.yml/badge.svg)](https://github.com/dobidu/grano/actions/workflows/ci.yml)

A granular synthesis instrument for experimental sound design.

`VST3` · `AU` · `Standalone` &nbsp;·&nbsp; Windows · macOS · Linux

---

## Contents

1. [What it is](#what-it-is)
2. [Installing Grano](#installing-grano)
   - [Windows](#windows)
   - [macOS](#macos)
   - [Linux](#linux)
3. [Loading a sample](#loading-a-sample)
4. [Core parameters](#core-parameters)
5. [MOTION module](#motion-module)
6. [COLOR module](#color-module)
7. [PATTERN module](#pattern-module)
8. [Modulation](#modulation)
   - [LFO 1 & 2](#lfo-1--2)
   - [Modulation matrix](#modulation-matrix)
   - [Snapshots A/B/C/D](#snapshots-abcd)
9. [Advanced engine](#advanced-engine)
   - [Sub-grain recursion](#sub-grain-recursion)
   - [Stochastic timing](#stochastic-timing)
   - [Feedback path](#feedback-path)
   - [Spectral processor](#spectral-processor)
10. [Performance notes](#performance-notes)
11. [Building from source](#building-from-source)
12. [Parameter reference](#parameter-reference)
13. [Troubleshooting](#troubleshooting)

---

## What it is

Grano is a granular synthesis instrument that loads your own audio files and turns them into particle clouds, frozen textures, broken rhythms, and unstable timbral worlds. It is built for experimental sound design, not conventional sample playback.

The design goes beyond a standard granular engine with:

- **Sub-grain recursion** — each grain can spawn child grains of itself, recursively, creating fractal-density cloud structures no conventional granular plugin produces.
- **Six stochastic timing distributions** — Uniform, Gaussian, Poisson, Exponential, Pareto, 1/f (fractal). The inter-grain interval is sampled from the chosen distribution, giving you everything from metronomic precision to heavy-tailed bursts.
- **Internal feedback path** — the engine's own audio output is re-injected as a grain source. Sustained tones accumulate, degrade, and self-modulate over time.
- **Spectral processor** — Freeze and Blur modes transform the loaded sample in the frequency domain before granulation begins. Freeze crystallises the spectrum; Blur smears phases to create evolving shimmer textures.
- **Audio-rate LFOs** — two LFOs running up to 8 kHz, routeable to any parameter, capable of FM, AM, tremolo, and noise-spectrum modulation.
- **Four parameter snapshots** — instant morph between four complete states during performance.

---

## Installing Grano

### Windows

**VST3:**

1. Copy `Grano.vst3` to `C:\Program Files\Common Files\VST3\` (system-wide) or `%APPDATA%\VST3\` (per-user).
2. Restart your DAW and rescan plugins.

**Standalone:**

Run `Grano.exe` directly. On first launch, Windows may show a SmartScreen warning because the binary is not yet commercially signed — click "More info → Run anyway".

### macOS

**VST3:**

Copy `Grano.vst3` to `~/Library/Audio/Plug-Ins/VST3/` (per-user) or `/Library/Audio/Plug-Ins/VST3/` (system-wide).

**AU (Audio Unit):**

Copy `Grano.component` to `~/Library/Audio/Plug-Ins/Components/`.

After copying, run:
```bash
auval -v aumu Gran Bidu
```
A passing result (exit 0) means the AU is registered correctly.

**Gatekeeper:**

If macOS blocks the plugin, run:
```bash
sudo xattr -rd com.apple.quarantine /Library/Audio/Plug-Ins/VST3/Grano.vst3
sudo xattr -rd com.apple.quarantine /Library/Audio/Plug-Ins/Components/Grano.component
```

### Linux

**VST3:**

Copy `Grano.vst3` to `~/.vst3/` or `/usr/lib/vst3/`. Most DAWs scan `~/.vst3` by default.

```bash
mkdir -p ~/.vst3
cp -R Grano.vst3 ~/.vst3/
```

Linux builds are VST3 + Standalone only. AU is macOS-exclusive.

---

## Loading a sample

Grano does not synthesize audio from scratch — it granulates source audio you provide.

**To load a sample:**
- Drag a WAV, AIFF, or MP3 file onto the waveform display area.
- Or click the **Load** button in the top bar and browse to a file.

Supported formats: WAV, AIFF, FLAC, OGG, MP3.

Once loaded, the waveform appears in the display. The engine starts granulating immediately. If no sample is loaded, Grano falls back to an internal 440 Hz sine tone so you can still hear the engine parameters in action.

---

## Core parameters

These are the fundamental granular engine controls. All are automatable and modulatable.

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| **Grain Size** | 5–500 ms | 100 ms | Duration of each grain. Shorter grains = more textural, longer = more melodic. |
| **Density** | 1–100 g/s | 10 g/s | Grains triggered per second (when Pattern mode is Free). At 100 g/s you get dense clouds; at 1–3 g/s you hear individual grains distinctly. |
| **Position** | 0–100% | 0% | Read position within the loaded sample. 0% = start, 100% = end. |
| **Position Jitter** | 0–100% | 0% | Random offset applied to Position per grain. At 100% grains scatter across the entire sample. |
| **Pitch Shift** | ±24 st | 0 st | Transpose all grains by up to two octaves. |
| **Stereo Spread** | 0–100% | 50% | Width of grain panning. 0% = mono center, 100% = full random L/R. |
| **Master Volume** | −60 to +6 dB | 0 dB | Output level. |
| **Loop** | On / Off | Off | When on, the engine continuously loops the Position sweep across the sample. |

### Tips

- **Frozen textures:** small Grain Size (10–30 ms) + low Density (2–5) + high Position Jitter (50–100%) → granular clouds with no rhythmic structure.
- **Melodic lines:** large Grain Size (150–400 ms) + moderate Density (5–15) + Position Jitter 0% → sustained tones following the sample's pitch.
- **Rhythmic textures:** moderate Grain Size + low Density + PATTERN module enabled (see below).

---

## MOTION module

MOTION introduces organic instability to the granular stream. Enable with the **MOTION** toggle.

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| **Wow Depth** | 0–100% | 0% | Low-frequency pitch drift. Simulates tape wow. |
| **Wow Rate** | 0.1–2.0 Hz | 0.5 Hz | Speed of the wow oscillation. |
| **Flutter Depth** | 0–100% | 0% | High-frequency pitch flutter. Simulates tape flutter. |
| **Flutter Rate** | 3–20 Hz | 8 Hz | Speed of the flutter oscillation. |
| **Drift Amount** | 0–100% | 0% | Slow random walk of the pitch center. Makes sustained tones gradually migrate. |
| **Crackle Level** | −60 to 0 dB | −60 dB | Adds broadband noise impulses (vinyl/tape crackle). −60 dB = off. |
| **Crackle Color** | 0–100% | 50% | Spectral tilt of the crackle noise. 0% = white, 100% = dark/brown. |

MOTION is bypassed by default — at 0% depth all parameters have no effect even when enabled.

---

## COLOR module

COLOR applies spectral processing to the engine output. Enable with the **COLOR** toggle.

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| **Saturate** | 0–100% | 0% | Harmonic saturation / soft-clip. Adds warmth and compression. |
| **Decimate** | 0–100% | 0% | Bit-crush and sample-rate reduction. Adds digital grit. |
| **Tilt EQ** | −1 to +1 | 0 | Spectral tilt. Negative = low-pass emphasis, positive = high-pass. |
| **Verb Mix** | 0–100% | 0% | Reverb wet/dry. Adds spatial depth. |

---

## PATTERN module

PATTERN controls how grains are triggered over time. It replaces or modifies the engine's timing logic. Enable with the **PATTERN** toggle.

### Trigger Mode

| Mode | Description |
|------|-------------|
| **Free** | Density-based: Density grains per second, jittered by the Stochastic distribution. |
| **Sync** | Tempo-synced to the DAW BPM at the chosen Sync Division. |
| **Euclidean** | Distributes k Pulses across n Steps with optional rotation. Creates irregular polyrhythmic patterns. |
| **Audio** | Trigger grains on incoming transients from the audio input. Set Transient Sensitivity to tune the threshold. |

### Euclidean parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| **Eucl Pulses** | 1–32 | 4 | Number of triggers in the pattern. |
| **Eucl Steps** | 2–32 | 8 | Total steps in the pattern. |
| **Eucl Rotation** | 0–31 | 0 | Rotates the pattern by N steps. |

### Gate and randomization

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| **Sync Division** | 1/4 to 1/64, triplets, dotted | 1/8 | Note division for Sync mode. |
| **Transient Sensitivity** | 0–100% | 50% | Audio transient detection threshold for Audio mode. |
| **Probability** | 0–100% | 100% | Per-trigger probability. At 100% every trigger fires; lower values drop random triggers. |
| **Reverse Prob** | 0–100% | 0% | Per-grain probability of reversing playback direction. |
| **Quantize Scale** | Chromatic / Major / Minor / Dorian / Phrygian / Lydian / Mixolydian / Pent Maj / Pent Min / Whole Tone / Octatonic | Chromatic | Locks Pitch Shift to scale degrees. |
| **Spray** | 0–100% | 0% | Random pitch scatter per grain, within the selected scale. |

---

## Modulation

### LFO 1 & 2

Two LFOs, freely routable to any parameter via the Modulation Matrix.

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| **Rate** | 0.01–8000 Hz | 1 Hz | LFO frequency. Above ~20 Hz enters audio-rate territory (FM/AM). |
| **Waveform** | Sine / Triangle / Saw / Square / Random / S&H / Drawable | Sine | LFO waveform. *Drawable* lets you sketch a custom one-cycle shape. |
| **Sync** | On / Off | Off | When on, Rate locks to DAW BPM subdivisions. |
| **Phase** | 0–360° | 0° | Start phase offset. Useful for stereo LFO pairs. |
| **Depth** | 0–100% | 50% | LFO output amplitude (before the modulation matrix amount scaling). |

### Modulation matrix

Eight routing slots, each with three settings:

| Column | Values | Description |
|--------|--------|-------------|
| **Source** | None / LFO 1 / LFO 2 | Modulation source. |
| **Dest** | Any automatable parameter | Modulation destination. |
| **Amount** | −100% to +100% | Modulation depth. Negative inverts polarity. |

Available destinations include all core parameters, all module parameters, and LFO rates/phases/depths themselves (LFO → LFO creates complex modulation shapes).

### Snapshots A/B/C/D

Four complete parameter states that can be stored and recalled instantly.

- **Save:** right-click a Snapshot button and select "Save current state".
- **Recall:** click a Snapshot button to instantly switch all parameters to that stored state.
- **Persist:** snapshots are saved with the plugin state and reload with your DAW project.

Snapshots are useful for creating contrasting sections in a performance (e.g., A = sparse/dry, B = dense/wet) or for A/B comparison while sound designing.

---

## Advanced engine

These features extend the granular engine beyond conventional operation. All are inactive at their default settings — they add no CPU cost when turned off.

### Sub-grain recursion

**Parameter:** Sub-grain Depth (0 / 1 / 2)

When Depth > 0, each triggered grain spawns additional child grains derived from itself before playback:

| Depth | Behavior |
|-------|----------|
| **0** | Normal operation. One grain per trigger. |
| **1** | Each grain spawns 2–4 sub-grains. Sub-grains have shorter length, slightly offset position, and small pitch deviations (±10%). Pan offsets ±0.2. |
| **2** | Each depth-1 sub-grain spawns 2–3 more. Up to 16 grains from a single trigger. |

Depth 2 with moderate density (5–15 g/s) creates dense particle clouds with organic internal motion — a texture rarely achievable with conventional granular engines.

Sub-grain generation runs on the scheduler thread, not the audio thread. It is real-time safe.

### Stochastic timing

**Parameter:** Stochastic Distribution

Controls the probability distribution of inter-grain time intervals. The mean interval is always `1000 / Density` ms; the distribution shapes how that interval varies.

| Distribution | Character |
|-------------|-----------|
| **Uniform** | Grains evenly jittered within a symmetric window. Predictable, slight humanisation. |
| **Gaussian** | Bell-curve jitter around the mean. Natural-feeling variation. |
| **Poisson** | Exponential inter-arrival (correct Poisson process). Bursts separated by longer silences. |
| **Exponential** | Heavier-tailed than Gaussian. More extreme timing deviations, occasional large gaps. |
| **Pareto** | Very heavy tail (α = 1.5). Long quiet stretches punctuated by rapid grain bursts. Good for glitchy, intermittent textures. |
| **1/f** | Fractal / pink-noise timing. Variation at all timescales simultaneously. Feels both organised and unpredictable. |

All samples are clamped to [1 ms, 5 × mean interval] to prevent runaway silences or impossibly dense bursts.

### Feedback path

**Parameters:** Feedback (on/off), Feedback Gain (0–0.95), Feedback Damp (0–100%)

The audio output of the engine is captured after each audio block and re-injected as a grain source for subsequent grains. This creates a self-referential feedback loop.

| Parameter | Effect |
|-----------|--------|
| **Feedback Gain** | How much of the previous output is fed back. Hard-clamped to 0.95 to prevent runaway divergence. At 0.95, sustained tones accumulate over many seconds. |
| **Feedback Damp** | Low-pass filter on the feedback signal before re-injection. 0% = no filtering, 100% = heavy LP filter (retains only low frequencies). |

**Source priority:** when Feedback is enabled, it takes precedence over the loaded SampleBuffer as the grain source. The engine granulates its own recent output instead of the original sample. Enable MOTION and COLOR after enabling Feedback for evolving, self-modifying textures.

**Caution:** Feedback Gain near 0.95 with no damping can produce sustained loud output. Always start with low Gain and increase gradually.

### Spectral processor

**Parameters:** Spectral (on/off), Spectral Mode (Freeze / Blur), Spectral Blur Amount (0–100%)

The spectral processor applies a 2048-point FFT transform to the loaded sample before granulation. The output is a 2048-sample transformed buffer used as the grain source in place of the original sample.

| Mode | Description |
|------|-------------|
| **Freeze** | Analyses the sample, takes magnitude information, sets all phases to zero. IFFT produces a static, phaseless reconstruction — a harmonically preserved but time-frozen texture. Suitable for sustained drone generation. |
| **Blur** | Randomises per-bin phases proportionally to Spectral Blur Amount. At Blur Amount = 0% no blurring occurs (equivalent to Freeze). At 100%, all phases are fully random — the output is noise-coloured by the sample's spectral envelope. Intermediate values create shimmering, evolving textures between the two extremes. |

**Spectral Blur Amount:** controls how far phases deviate from the original. 0% = Freeze-like, 100% = full phase noise. A setting of 30–60% often produces the most musical shimmer.

**Latency:** when spectral processing is enabled, Grano reports 2048 samples (≈ 42 ms at 48 kHz) of additional latency to the host. DAWs with delay compensation will align Grano's output correctly. Disable spectral processing to reduce latency to near-zero.

**Source priority:** Spectral takes highest priority as a grain source — above Feedback and the loaded SampleBuffer. Enable Feedback while Spectral is also on: the engine will use the spectral output as its source regardless.

---

## Performance notes

| Setting | Approx. CPU (2020 i7) |
|---------|----------------------|
| Default (200 grains, modules off) | ~8% |
| 500 grains, all modules on | ~20% |
| 1000 grains, Sub-grain depth 2, Feedback + Spectral on | ~35% |

The engine's grain generation runs on background worker threads, not the audio thread. Increasing Density or Sub-grain Depth adds CPU on those threads, not on the audio thread. You will rarely cause an audio dropout by increasing grain density — you may hit CPU wall first.

**Buffer size:** tested at 128 to 1024 samples at 44.1 and 48 kHz. 64-sample buffers may produce xruns on slower machines with high grain counts.

**Memory:** approximately 8 MB per second of loaded audio (mono). A 60-second sample uses ~480 MB. The plugin pool pre-allocates grain slots at startup and performs no heap allocation on the audio thread.

---

## Building from source

### Prerequisites

| Tool | Version | Where |
|------|---------|--------|
| JUCE 8 | 8.0.12 (pinned) | `git clone --branch 8.0.12 --depth 1 https://github.com/juce-framework/JUCE.git ~/JUCE` |
| CMake | ≥ 3.22 | https://cmake.org/download/ |
| C++ compiler | C++20 | Visual Studio 2022 (Win), Xcode 15+ (macOS), GCC 11+ / Clang 14+ (Linux) |
| Catch2 | v3 (auto-fetched) | Downloaded at configure time |

### Quick build

```bash
# Clone
git clone https://github.com/dobidu/grano.git
cd grano

# Configure (uses ~/JUCE by default; override with -DJUCE_DIR=/your/path)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build all targets
cmake --build build --config Debug

# Run tests
ctest --test-dir build --output-on-failure
```

### Windows (Visual Studio)

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Artifacts in `build\Grano_artefacts\Release\VST3\Grano.vst3` and `build\Grano_artefacts\Release\Standalone\Grano.exe`.

To install system-wide (run as Administrator):
```powershell
copy /Y build\Grano_artefacts\Release\VST3\Grano.vst3 "C:\Program Files\Common Files\VST3\"
```

### macOS

```bash
cmake -B build -G Xcode
cmake --build build --config Release

# Universal binary (Apple Silicon + Intel)
cmake -B build-universal -G Xcode \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build-universal --config Release
```

Install:
```bash
cp -R build/Grano_artefacts/Release/VST3/Grano.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/Grano_artefacts/Release/AU/Grano.component ~/Library/Audio/Plug-Ins/Components/
```

### Linux

```bash
sudo apt install -y build-essential cmake pkg-config \
  libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev \
  libfreetype-dev libx11-dev libxcomposite-dev libxcursor-dev \
  libxinerama-dev libxrandr-dev libgtk-3-dev \
  libwebkit2gtk-4.1-dev libfontconfig1-dev libgl1-mesa-dev

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

cp -R build/Grano_artefacts/Release/VST3/Grano.vst3 ~/.vst3/
```

### CMake options

| Option | Default | Purpose |
|--------|---------|---------|
| `JUCE_DIR` | `~/JUCE` | Path to the JUCE 8 clone. |
| `GRANO_BUILD_TESTS` | `ON` | Build Catch2 test suite. |
| `GRANO_ENABLE_TSAN` | `OFF` | ThreadSanitizer (Linux/macOS Debug only). |
| `GRANO_ENABLE_ASAN` | `OFF` | AddressSanitizer. |
| `GRANO_USE_MP3` | `ON` | Enable MP3 loading. Set `OFF` to avoid MP3 patent considerations. |

### Plugin validation

```bash
# Install pluginval from https://github.com/Tracktion/pluginval/releases
pluginval --strictness-level 5 build/Grano_artefacts/Debug/VST3/Grano.vst3

# macOS AU
auval -v aumu Gran Bidu
```

---

## Parameter reference

Complete list of all 74 automatable parameters.

### Granular engine

| ID | Name | Type | Range | Default |
|----|------|------|-------|---------|
| `grainSize` | Grain Size | Float | 5–500 ms | 100 ms |
| `density` | Density | Float | 1–100 g/s | 10 g/s |
| `position` | Position | Float | 0–1 | 0 |
| `positionJitter` | Position Jitter | Float | 0–1 | 0 |
| `pitchShift` | Pitch Shift | Float | −24–+24 st | 0 |
| `stereoSpread` | Stereo Spread | Float | 0–1 | 0.5 |
| `masterVolume` | Master Volume | Float | −60–+6 dB | 0 |
| `loop` | Loop | Bool | — | Off |

### MOTION module

| ID | Name | Range | Default |
|----|------|-------|---------|
| `motionEnabled` | Motion | Bool | Off |
| `wowDepth` | Wow Depth | 0–1 | 0 |
| `wowRate` | Wow Rate | 0.1–2.0 Hz | 0.5 Hz |
| `flutterDepth` | Flutter Depth | 0–1 | 0 |
| `flutterRate` | Flutter Rate | 3–20 Hz | 8 Hz |
| `driftAmount` | Drift | 0–1 | 0 |
| `crackleLevel` | Crackle Level | −60–0 dB | −60 dB |
| `crackleColor` | Crackle Color | 0–1 | 0.5 |

### COLOR module

| ID | Name | Range | Default |
|----|------|-------|---------|
| `colorEnabled` | Color | Bool | Off |
| `saturate` | Saturate | 0–1 | 0 |
| `decimate` | Decimate | 0–1 | 0 |
| `tiltEq` | Tilt EQ | −1–+1 | 0 |
| `verbMix` | Verb Mix | 0–1 | 0 |

### PATTERN module

| ID | Name | Range | Default |
|----|------|-------|---------|
| `patternEnabled` | Pattern | Bool | Off |
| `triggerMode` | Trigger Mode | Choice (Free / Sync / Euclidean / Audio) | Free |
| `syncDivision` | Sync Division | Choice | 1/8 |
| `euclidPulses` | Eucl Pulses | Int 1–32 | 4 |
| `euclidSteps` | Eucl Steps | Int 2–32 | 8 |
| `euclidRotation` | Eucl Rotation | Int 0–31 | 0 |
| `transientSensitivity` | Transient Sensitivity | 0–1 | 0.5 |
| `probability` | Probability | 0–1 | 1 |
| `reverseProb` | Reverse Prob | 0–1 | 0 |
| `quantizeScale` | Quantize Scale | Choice (11 scales) | Chromatic |
| `spray` | Spray | 0–1 | 0 |

### LFO 1 & 2

| ID | Name | Range | Default |
|----|------|-------|---------|
| `lfo1Rate` | LFO1 Rate | 0.01–8000 Hz | 1 Hz |
| `lfo1Waveform` | LFO1 Waveform | Choice (7) | Sine |
| `lfo1Sync` | LFO1 Sync | Bool | Off |
| `lfo1Phase` | LFO1 Phase | 0–360° | 0° |
| `lfo1Depth` | LFO1 Depth | 0–1 | 0.5 |
| `lfo2Rate` | LFO2 Rate | 0.01–8000 Hz | 1 Hz |
| `lfo2Waveform` | LFO2 Waveform | Choice (7) | Sine |
| `lfo2Sync` | LFO2 Sync | Bool | Off |
| `lfo2Phase` | LFO2 Phase | 0–360° | 0° |
| `lfo2Depth` | LFO2 Depth | 0–1 | 0.5 |

### Advanced engine

| ID | Name | Range | Default |
|----|------|-------|---------|
| `subGrainDepth` | Sub-grain Depth | Int 0–2 | 0 |
| `stochasticDist` | Stochastic Distribution | Choice (Uniform / Gaussian / Poisson / Exponential / Pareto / 1/f) | Uniform |
| `feedbackEnabled` | Feedback | Bool | Off |
| `feedbackGain` | Feedback Gain | 0–0.95 | 0.5 |
| `feedbackDamp` | Feedback Damp | 0–1 | 0.3 |
| `spectralEnabled` | Spectral | Bool | Off |
| `spectralMode` | Spectral Mode | Choice (Freeze / Blur) | Freeze |
| `spectralBlurAmount` | Spectral Blur | 0–1 | 0.5 |

### Modulation matrix (8 slots × 3 params = 24)

Each slot: `slotNSource` (Choice: None/LFO1/LFO2), `slotNDest` (Choice: 29 destinations), `slotNAmount` (Float −1 to +1).

---

## Troubleshooting

**No sound after loading a sample:**
- Check Master Volume is not at −60 dB.
- Confirm the sample format is supported (WAV, AIFF, FLAC, OGG, MP3). Re-save as WAV if unsure.
- Verify the sample has non-zero length. A corrupt file may load silently.

**Grano crashes when loading certain MP3 files:**
- JUCE's MP3 decoder is limited. Convert to WAV first: `ffmpeg -i input.mp3 output.wav`.
- Build with `GRANO_USE_MP3=OFF` and use WAV/AIFF only if persistent.

**Audio output is very quiet with Spectral enabled:**
- Spectral output amplitude depends on the source sample level. If the loaded sample is very quiet, the spectral source is too. Load a louder sample or raise Master Volume.
- Ensure the spectral buffer is initialised: load a sample and re-enable Spectral, or verify that spectral processing re-runs after each file load.

**Feedback diverges to silence or noise:**
- Feedback Gain near 0.95 with no damping on a loud source can ring. Reduce Gain to 0.5–0.7 first, then add Damp.
- The hard ceiling at 0.95 prevents mathematical divergence, but perceptual saturation can still occur with the COLOR Saturate module engaged.

**High CPU usage:**
- Reduce Density.
- Set Sub-grain Depth to 0 — Depth 2 multiplies grain count up to 16×.
- Disable Spectral (saves FFT compute on load and on each spectral trigger).
- Use a larger audio buffer (256 or 512 samples) in your DAW settings.

**JUCE assertion failure on Linux (MIDI / ALSA):**
- This is a non-critical warning when no MIDI device is present. Audio still works. Install ALSA MIDI support (`libasound2-dev`) or run with `JUCE_MIDI_IO=0`.

**Plugin not appearing in DAW scan (Windows):**
- Confirm the `.vst3` file was copied to `C:\Program Files\Common Files\VST3\`, not just the folder that contains it.
- Force a plugin rescan in your DAW.

**Plugin not appearing on macOS:**
- Run `auval -v aumu Gran Bidu` to check AU registration.
- Clear the component cache: `sudo killall -9 AudioComponentRegistrar` and relaunch your DAW.
- Remove the quarantine attribute: `sudo xattr -rd com.apple.quarantine /Library/Audio/Plug-Ins/VST3/Grano.vst3`

**DAW reports non-zero latency even with Spectral disabled:**
- If you enabled and then disabled Spectral, some DAWs cache the reported latency. Reload the plugin instance to reset.

---

## Status

In active development — F6 (UI Polish + Advanced Engine) in progress.

| Phase | Status |
|-------|--------|
| F0 Foundation | ✅ Complete |
| F1 Granular Engine | ✅ Complete |
| F2 Sample I/O + Waveform | ✅ Complete |
| F3 Core Controls + APVTS | ✅ Complete |
| F4 MOTION + COLOR + PATTERN | ✅ Complete |
| F5 LFOs + Modulation Matrix + Snapshots | ✅ Complete |
| F6a UI Polish | ✅ Complete |
| F6b Sub-grain + Stochastic Timing | ✅ Complete |
| F6c Feedback + Spectral | ✅ Complete |
| F6d Multi-sample bank | Pending |
| F6e CurveEditor + Spectrogram + integration | Pending |
| F7 Release | Pending |

---

## License

To be determined.
