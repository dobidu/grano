# Grano — Release Notes

## v1.0.0-beta.2 (2026-06-07)

Second public beta, focused on Windows fixes and exposing every module control in the UI.
Now in wider testing — please file issues for anything rough.

### Fixes

- **Crash at +24 st pitch** — extreme upward pitch shifts pushed the grain read position past
  the source buffer guard samples and crashed. The renderer now bounds-checks and ends the
  grain cleanly instead of reading out of bounds.
- **Windows file drag-and-drop** — dropping audio files onto the window now works in both the
  Standalone and DAW (VST3) hosts. The plugin registers its own OLE `IDropTarget`, so the
  cursor shows the copy affordance and the drop is delivered regardless of the host's COM
  initialisation.
- **VST3 / AU category** — plugin now reports as an effect (VST3 `Fx`, AU `Effect`) so hosts
  list and load it in the expected place.

### Additions

- **LOAD button** in the header — reliable file-chooser fallback for loading a sample without
  drag-and-drop.
- **Module tab panel** — a 6-tab rack (Engine, Motion, Color, Pattern, LFO, Mod) exposes every
  module parameter directly in the UI; previously many were automation-only.
- UI polish — footer grain count, SNAP label, master level in dB, coloured tab indicators, and
  per-knob value readouts.

### Other

- Added audio-output smoke tests (RMS, finite-sample, no-clip) to the suite.

> Upgrading from beta.1: settings and DAW projects remain compatible — all 79 parameters and the
> saved-state format are unchanged.

---

## v1.0.0-beta.1 (2026-05-19)

This is the first public beta release of Grano. All core features are implemented and validated.
Expect rough edges; filing issues is welcome.

### What's in this release

- **Granular engine** — GranularEngine with SPSC FIFO, 1024-slot GrainPool, grain scheduler
  thread running at configurable density (0.1–200 grains/sec)
- **Sub-grain recursion** — depth 0/1/2 selector; each grain spawns up to 4 child grains
- **Six stochastic timing distributions** — Uniform, Gaussian, Poisson, Exponential, Pareto, 1/f
- **Internal feedback path** — audio output re-injected as grain source with gain + dampening
- **Spectral Freeze and Blur** — 2048-point FFT, Hann window, 75% overlap; latency reported to host
- **Multi-sample bank** — 4 independently loadable slots with weighted random selection
- **Envelope shape selector** — Hann / Tukey / Gaussian / Triangle / Rectangle
- **MOTION module** — Wow, Flutter, Drift, Crackle
- **COLOR module** — Saturate, Decimate, Tilt EQ, Verb
- **PATTERN module** — Euclidean triggers, Probability, Reverse, Quantize (11 scales), Spray
- **Two LFOs** — 0.01 Hz – 8 kHz, 7 waveforms, audio-rate capable
- **Modulation matrix** — 8 slots, LFO1/LFO2 → 29 destinations, cross-modulation
- **Parameter snapshots A/B/C/D** — instant recall, DAW-automation compatible
- **79 APVTS parameters** — all automatable; state persists across DAW project save/load
- 117 Catch2 unit tests passing on Windows and macOS
- pluginval strictness-level 10 passing on VST3 (Win + macOS)

### Plugin formats

| Format | Windows | macOS | Linux |
|--------|---------|-------|-------|
| VST3 | ✅ | ✅ | ✅ (best-effort) |
| AU | — | ✅ (unsigned) | — |
| Standalone | ✅ | ✅ | ✅ (best-effort) |

### Factory presets

Five starter presets ship in `Resources/Presets/`:

| Preset | Character |
|--------|-----------|
| `Cloud.gpreset` | Dense particle cloud, long grains, wide stereo spread |
| `Drone.gpreset` | Slow, sustained, feedback-saturated |
| `Rhythmic.gpreset` | Euclidean 3-in-8 triggers, tight density |
| `Spectral.gpreset` | Spectral Freeze source, sub-grain depth 2 |
| `Broken.gpreset` | Pareto timing, high crackle, glitchy bursts |

Each file lists parameter names and values that can be applied manually to Grano's controls.
A preset browser UI is planned for a future update.

### Known issues

- **auval (macOS Audio Unit validation)** fails on macOS Sonoma / Sequoia (14/15) without a
  code-signing certificate. The AU installs and loads correctly in DAWs (Logic Pro, GarageBand,
  Reaper); the validator tool requires an Apple Developer ID. Code signing is planned for
  v1.0.0 stable.
- **Windows SmartScreen** may warn on first launch of the Standalone (unsigned binary).
  Click "More info → Run anyway."
- **Font embedding** — Inter and JetBrains Mono are not yet embedded via BinaryData; the UI
  falls back to system fonts. Embedded fonts are planned for v1.0.0 stable.
- **Snapshot recall ramp** — the 250 ms crossfade on snapshot A/B/C/D switch is not yet
  implemented; switching is instant. Planned for v1.0.0 stable.
- **Linux builds** are best-effort. VST3 and Standalone build on Ubuntu 22.04+; no pluginval
  validation on Linux CI.

### Installation

See [README.md](README.md) for full per-platform installation instructions and the
[GitHub Releases page](https://github.com/dobidu/grano/releases) for pre-built binaries.

### Building from source

See [BUILD.md](BUILD.md) for full instructions.

Quick start (macOS/Linux):

```bash
git clone https://github.com/dobidu/grano.git
cd grano
cmake -B build -DJUCE_DIR="$HOME/JUCE"
cmake --build build --config Release
```

---

## Planned for v1.0.0 stable

- Code signing + notarization (macOS), code signing (Windows)
- macOS `.pkg` installer, Windows installer
- Embedded Inter + JetBrains Mono fonts
- Preset browser UI (load `.gpreset` files from disk)
- 250 ms snapshot recall ramp
- auval validation in CI (requires Developer ID)
