# Grano — Release Notes

## v1.0.0-beta.4 (unreleased)

### New

- **No Source setting.** When no slot can play (nothing loaded, or every slot
  weight at 0, with Feedback and Spectral off), you can now choose what Grano
  does: **Test tone** (the 440 Hz sine, as before; the default) or **Silence**.
  The selector appears at the bottom of the monitor whenever it applies. It is
  saved with the project and is not automatable. Snapshot recall leaves it
  unchanged. Existing projects keep the test tone.

---

## v1.0.0-beta.3 (2026-11-01)

A new editor and a correctness release. A full code review (08-08) found that
several features produced wrong audio. **Existing projects and presets will sound
different**, mostly closer to what the controls always claimed to do.

### New editor

The tabbed editor is replaced by a single-screen console. Every parameter is
visible at once.

- **One screen, no tabs.** The window scales from 0.75× to 1.6× at a fixed aspect
  ratio, and text stays sharp.
- **Every control shows its value and unit** on a scribble strip. Double-click resets
  to default; Ctrl/Cmd-drag gives fine control; the mouse wheel and arrow keys step.
- **Command line** in the header names the control you touch, with its previous and
  default values. The status strip shows a hint for whatever is under the pointer,
  plus the grain count, CPU load and number of loaded slots.
- **Monitor** shows the selected slot's waveform with the live grains, the
  loop-swept read head, the jitter span and the grain-size shadow. Click or drag on
  it to set Position.
- **Slot strips** show the filename, weight, and loaded / empty / failed state for
  each of the 4 slots. Click a strip to load a file; drop a file onto the editor to
  load it into the selected slot.
- **Snapshots A–D:** click to recall (or to store into an empty slot), Ctrl/Cmd-click
  to store, right-click for Store / Recall / Clear. The live snapshot is lit.
- **Master section** has a peak meter (L/R), a clip LED and the volume fader.
- **Contextual Pattern controls:** Sync division appears in Sync and Euclid, the
  Euclid ring with Pulses / Steps / Rotation appears in Euclid, and Transient
  Sensitivity appears in Audio mode.
- Turning a module off dims its bank and strikes through its title. Modulated
  parameters carry a blue marker and range bracket.
- Fonts: Archivo and Martian Mono (SIL OFL), embedded.
- Reopening the editor or reloading a project shows the loaded waveforms and slot
  names.

### Sound changes you will hear

- **Stereo Spread works as labelled.** 0% is now mono and 100% fully wide. Before,
  the knob was V-shaped and its default (50%) was **mono**; the default is now a
  half-width stereo image.
- **Dense settings are no longer much louder.** Grain level is normalised by the
  expected overlap (density × grain size) and sub-grain families are normalised
  too. High-density patches are quieter than before; turn Volume up if needed.
- **Spectral Freeze is a sustained texture,** not a click. **Blur 0% is the dry
  source** and smears smoothly towards noise.
- **Verb no longer adds +6 dB** to the dry signal the moment it is engaged; the mix
  knob is an equal-power dry/wet crossfade.
- **Crackle** stays audible across the whole Color range and no longer buzzes at the
  host block rate. **Drift** wanders the same way at any buffer size.
- **Saturate** fades in smoothly with the knob instead of jumping +2–24 dB.
- **Poisson** timing is now a clustered (bursty) process — it used to be identical
  to Exponential.
- **Spray** randomises grain length both shorter and longer (½×–2×).
- **Scale quantize** no longer removes Motion's wow/flutter/drift.

### Fixes

- **Offline bounces match playback.** Grains are now scheduled on a sample-accurate
  clock on the audio thread (the old wall-clock scheduler thread rendered fast
  offline bounces almost silent). Onsets land on their exact sample; Sync and
  Euclidean patterns lock to the host timeline.
- **No more clicks at the end of the sample** when pitched up — grains are fitted
  to the source and always play their full envelope. Long samples (> 1 min) keep
  sub-sample precision.
- **Feedback** can no longer run away to inf/NaN, never reads across its own write
  head (periodic clicks), still passes signal at 100% Damp, and is cleared on
  transport restart / sample-rate change.
- **Spectral** latency is 0 (the reported 2048 samples shifted the track 42 ms early)
  and a newly loaded sample is always the one transformed.
- **Master volume** changes and LFO→Volume are ramped (no zipper noise).
- Loading a preset/state now clears samples and snapshots that are not in it.
- Two rapid loads into one slot can no longer pair a buffer with the wrong length;
  the audio thread never frees memory.
- Transient (Audio) trigger is independent of buffer size and listens to both
  channels.

### Compatibility

Parameters and the saved-state format are unchanged; the sound of existing patches
changes as described above.

---


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
  thread running at configurable density (1–100 grains/sec)
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
