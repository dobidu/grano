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
5. [User manual](#user-manual)
   - [First use walkthrough](#first-use-walkthrough)
   - [Sound design recipes](#sound-design-recipes)
   - [Understanding grain density vs. texture](#understanding-grain-density-vs-texture)
   - [Source priority explained](#source-priority-explained)
   - [Modulation routing examples](#modulation-routing-examples)
   - [Snapshot performance technique](#snapshot-performance-technique)
6. [MOTION module](#motion-module)
7. [COLOR module](#color-module)
8. [PATTERN module](#pattern-module)
9. [Modulation](#modulation)
   - [LFO 1 & 2](#lfo-1--2)
   - [Modulation matrix](#modulation-matrix)
   - [Snapshots A/B/C/D](#snapshots-abcd)
10. [Advanced engine](#advanced-engine)
    - [Sub-grain recursion](#sub-grain-recursion)
    - [Stochastic timing](#stochastic-timing)
    - [Feedback path](#feedback-path)
    - [Spectral processor](#spectral-processor)
11. [Performance notes](#performance-notes)
12. [Building from source](#building-from-source)
13. [Parameter reference](#parameter-reference)
14. [Troubleshooting](#troubleshooting)

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

## User manual

This section is a practical guide from first sound through advanced techniques. Read it top-to-bottom on first use; after that, treat it as a recipe book.

### First use walkthrough

Follow these steps to go from a silent plugin to a working grain cloud in under five minutes.

**Step 1 — Install and open**

Install the plugin as described in [Installing Grano](#installing-grano). Open your DAW, create an instrument or effect track, and load Grano. Set the track output to your speakers or headphones and confirm you can see the Grano UI.

**Step 2 — Load a sample**

Drag any WAV or AIFF file onto the waveform display. A spoken word recording, a short musical phrase, or a sustained instrument note all work well for a first session. The waveform renders in the display and the engine begins granulating immediately.

If you have no suitable file at hand, skip this step — the engine falls back to an internal 440 Hz sine tone, which is enough to understand all controls.

**Step 3 — Confirm you have output**

Check that `masterVolume` is at 0 dB (not −60 dB). You should hear a continuous texture. If the sample is very quiet, raise `masterVolume` by 3–6 dB.

**Step 4 — Explore Grain Size**

Turn `grainSize` slowly from its default of 100 ms down to 10 ms. The texture becomes more granular and diffuse — individual pitch content dissolves into a cloud. Bring it back up to 300–400 ms and the sample's pitch and timbre become more recognisable. This is the most fundamental trade-off in granular synthesis.

**Step 5 — Explore Density**

With `grainSize` back at 100 ms, lower `density` from 10 g/s to 2 g/s. You can now hear discrete grains fire individually. Raise it to 40–60 g/s and the cloud thickens until individual grains are no longer audible — only the aggregate texture remains.

**Step 6 — Explore Position Jitter**

Set `density` to 15 g/s and `grainSize` to 80 ms. Now raise `positionJitter` from 0% to 100%. At 0%, all grains read from the same spot in the sample (set by `position`). At 100%, each grain reads from a random position across the entire sample. The texture becomes maximally varied and unpredictable. Values between 20–50% are often the most musical.

At this point you understand the three core axes of control: grain duration, grain rate, and read-position scatter. Every other parameter in Grano modifies or layers on top of these three.

---

### Sound design recipes

Each recipe lists every parameter that differs from its default. Parameters not listed remain at their default value. All recipes assume a loaded sample — a sustained instrumental note or voice recording gives the clearest results.

#### Recipe 1 — Frozen cloud (static texture)

Dense, static granular wash. No rhythmic content, no pitch movement.

| Parameter | Value |
|-----------|-------|
| `grainSize` | 20 ms |
| `density` | 30 g/s |
| `position` | 40% |
| `positionJitter` | 80% |
| `pitchShift` | 0 st |
| `stereoSpread` | 80% |
| `stochasticDist` | Gaussian |

The short grains and high jitter eliminate any time-structure from the source. The Gaussian distribution keeps the texture even — no large timing gaps.

#### Recipe 2 — Rhythmic pulse (pattern-synced)

Sparse grain triggers locked to the DAW tempo for percussive, rhythmic textures.

| Parameter | Value |
|-----------|-------|
| `grainSize` | 60 ms |
| `density` | 4 g/s |
| `positionJitter` | 15% |
| `stereoSpread` | 60% |
| `patternEnabled` | On |
| `triggerMode` | Sync |
| `syncDivision` | 1/8 |
| `probability` | 75% |
| `reverseProb` | 20% |

`probability` at 75% drops a quarter of the synced triggers randomly, creating a human-feeling groove rather than a machine pattern. `reverseProb` at 20% flips occasional grains for variation.

#### Recipe 3 — Pitch-frozen drone

Long, harmonically stable sustained tone. Works best with a melodic or tonal source sample.

| Parameter | Value |
|-----------|-------|
| `grainSize` | 300 ms |
| `density` | 8 g/s |
| `position` | 25% |
| `positionJitter` | 5% |
| `pitchShift` | 0 st |
| `stereoSpread` | 30% |
| `spectralEnabled` | On |
| `spectralMode` | Freeze |
| `motionEnabled` | On |
| `driftAmount` | 15% |

The `spectralMode` Freeze crystallises the sample's harmonic content before granulation. The very low `positionJitter` keeps all grains reading from nearly the same spectral snapshot. `driftAmount` adds a slow, breath-like pitch wander to prevent stasis.

#### Recipe 4 — Glitch burst (Pareto distribution + low density)

Heavy-tailed inter-grain timing creates long silences punctuated by rapid bursts.

| Parameter | Value |
|-----------|-------|
| `grainSize` | 15 ms |
| `density` | 6 g/s |
| `positionJitter` | 60% |
| `stereoSpread` | 90% |
| `stochasticDist` | Pareto |
| `colorEnabled` | On |
| `decimate` | 40% |
| `tiltEq` | +0.4 |

The Pareto distribution's α = 1.5 tail means the engine occasionally fires five to ten grains in rapid succession, then goes silent for a second or more. `decimate` at 40% adds digital grit to the bursts. The high `stereoSpread` scatters the burst grains across the stereo field.

#### Recipe 5 — Self-eating feedback loop

The engine granulates its own output, accumulating and degrading over time.

| Parameter | Value |
|-----------|-------|
| `grainSize` | 80 ms |
| `density` | 12 g/s |
| `positionJitter` | 30% |
| `feedbackEnabled` | On |
| `feedbackGain` | 0.70 |
| `feedbackDamp` | 40% |
| `motionEnabled` | On |
| `wowDepth` | 25% |
| `wowRate` | 0.3 Hz |
| `colorEnabled` | On |
| `saturate` | 35% |

Start playing and wait 10–15 seconds. The texture progressively transforms as the engine re-granulates its previous output. `feedbackDamp` at 40% removes high frequencies on each pass, gradually darkening the texture. `saturate` adds warmth compression as the signal accumulates. Raise `feedbackGain` gradually — do not jump to 0.90+ without damping in place.

#### Recipe 6 — Spectral shimmer (blur mode)

Phase-smeared spectral texture derived from the source sample's spectral envelope.

| Parameter | Value |
|-----------|-------|
| `grainSize` | 120 ms |
| `density` | 20 g/s |
| `positionJitter` | 50% |
| `stereoSpread` | 70% |
| `spectralEnabled` | On |
| `spectralMode` | Blur |
| `spectralBlurAmount` | 45% |
| `colorEnabled` | On |
| `verbMix` | 30% |
| `tiltEq` | +0.2 |

`spectralBlurAmount` at 45% randomises phases enough to create shimmer without collapsing the harmonic content into pure noise (which happens at 100%). The added reverb (`verbMix`) extends the spatial tail of the shimmer. Use a bright source sample — strings, high piano notes, or vocals — for the clearest shimmer character.

#### Recipe 7 — Pulverized deep texture (sub-grain depth 2)

Each trigger spawns up to 16 grains, building a dense, internally-differentiated particle cloud.

| Parameter | Value |
|-----------|-------|
| `grainSize` | 50 ms |
| `density` | 8 g/s |
| `positionJitter` | 40% |
| `stereoSpread` | 100% |
| `subGrainDepth` | 2 |
| `stochasticDist` | 1/f |
| `colorEnabled` | On |
| `saturate` | 20% |
| `verbMix` | 20% |

`subGrainDepth` 2 means each primary grain spawns 2–4 sub-grains, each of which spawns 2–3 more. With `density` at 8 g/s you get an apparent grain count of 64–128 per second. The 1/f stochastic distribution gives the timing fractal structure — variation at all timescales. CPU usage will be noticeably higher; see [Performance notes](#performance-notes).

#### Recipe 8 — LFO-driven morphing cloud

A continuously evolving texture where LFO 1 sweeps the read position slowly across the sample.

| Parameter | Value |
|-----------|-------|
| `grainSize` | 90 ms |
| `density` | 18 g/s |
| `positionJitter` | 25% |
| `stereoSpread` | 65% |
| `lfo1Rate` | 0.05 Hz |
| `lfo1Waveform` | Sine |
| `lfo1Depth` | 70% |
| Mod slot 1 source | LFO 1 |
| Mod slot 1 dest | `position` |
| Mod slot 1 amount | +80% |
| `lfo2Rate` | 0.3 Hz |
| `lfo2Waveform` | Triangle |
| `lfo2Depth` | 40% |
| Mod slot 2 source | LFO 2 |
| Mod slot 2 dest | `grainSize` |
| Mod slot 2 amount | +50% |

LFO 1 at 0.05 Hz completes one full position sweep every 20 seconds, slowly cycling through the sample. LFO 2 simultaneously modulates `grainSize`, creating periodic texture transitions between fine (10 ms) and coarse (135 ms) grain structures. The two LFOs are unsynchronised at these settings, so their phase relationship drifts — the cloud never repeats exactly.

---

### Understanding grain density vs. texture

`density` sets the mean rate of grain triggers per second, but what you hear is determined jointly by density, grain size, and their product.

When `density` × `grainSize` (in seconds) exceeds approximately 1.0, grains overlap continuously and the output becomes a smooth, merged texture — the cloud. Individual grains are no longer audible as discrete events. Below that product, you hear articulated grains with silence between them. The transition zone (product ≈ 0.5–1.5) is where granular synthesis is most interesting: grains are dense enough to sustain a texture, but sparse enough that individual events contribute distinct timbral character.

Increasing `positionJitter` does not change density or overlap, but it randomises which part of the sample each grain reads. High jitter collapses temporal coherence: even a harmonically clear sample sounds diffuse because consecutive grains are drawn from unrelated moments. Low jitter preserves the harmonic character of whichever sample region `position` points at.

The practical takeaway: use `density` to control overlap and articulation, `grainSize` to control timbral resolution and pitch intelligibility, and `positionJitter` to control how much of the source material's inherent structure survives into the output.

---

### Source priority explained

Grano has four possible grain sources, applied in strict priority order: **Spectral > Feedback > Sample > Sine**.

When `spectralEnabled` is on, the spectral processor runs a 2048-point FFT on the loaded sample and produces a transformed buffer (Freeze or Blur). The engine takes its grain read positions from that transformed buffer. Nothing else matters as a source — not the feedback signal, not the raw sample.

When `spectralEnabled` is off but `feedbackEnabled` is on, the engine reads from the internal feedback buffer: the captured audio output of the previous block, re-injected as source material. The engine granulates its own recent output. The loaded sample contributes nothing to grain content in this state (though its presence affects the feedback signal's initial history).

When both Spectral and Feedback are off, the engine reads directly from the loaded `SampleBuffer`. This is the standard operation mode.

When no sample has been loaded at all, the engine falls back to an internal 440 Hz sine tone so the plugin produces output even without user content.

The practical implication: enabling Spectral while Feedback is also on does not create a combined source — Spectral wins unconditionally. To hear feedback-derived content, `spectralEnabled` must be off. To move between source types deliberately, use Snapshots to store configurations with different source combinations active.

---

### Modulation routing examples

The modulation matrix has eight slots. Each slot connects one source (LFO 1, LFO 2) to one destination parameter at a specified amount (−100% to +100%). The following three examples cover the range from slow timbral scanning to audio-rate synthesis.

#### Example 1 — LFO to Position (scanning)

Slowly sweeps the granular read position across the loaded sample, producing a continuously evolving timbre without manual automation.

| Slot field | Value |
|------------|-------|
| Source | LFO 1 |
| Dest | `position` |
| Amount | +70% |
| `lfo1Rate` | 0.02–0.1 Hz |
| `lfo1Waveform` | Sine or Triangle |

At 0.02 Hz the position completes a full sweep every 50 seconds — slow enough to sound organic, fast enough to hear clear timbral progression. Use a `loop`-enabled sample so the scan wraps smoothly. Raise Amount to +100% for a full-sample sweep; reduce it to +30% to confine scanning to a narrower region centred on the current `position` value.

#### Example 2 — LFO to Pitch Shift at audio rate (FM synthesis)

Sets LFO 1 into the audio-rate range and routes it to `pitchShift`, creating frequency modulation sidebands from the granular source.

| Slot field | Value |
|------------|-------|
| Source | LFO 1 |
| Dest | `pitchShift` |
| Amount | +15% to +60% |
| `lfo1Rate` | 80–440 Hz |
| `lfo1Waveform` | Sine |

At 80 Hz with Amount +15%, the FM ratio is low and the sidebands are close to the carrier — a subtle metallic shimmer. At 440 Hz with Amount +60%, the FM index is high enough to create inharmonic spectra and noise bands. Combine with `stochasticDist` Gaussian and `grainSize` 30 ms for dense FM granular clouds. Note: the `pitchShift` parameter range is ±24 semitones; the LFO modulation amount scales within that range, so +100% amount at `lfo1Depth` 50% applies ±12 semitones of FM deviation.

#### Example 3 — LFO 1 to LFO 2 Rate (meta-modulation)

Routes LFO 1 into the rate of LFO 2, which is itself routed to another destination. The result is a rate-modulated modulator — the speed of the effect changes over time.

| Slot field | Value |
|------------|-------|
| Source | LFO 1 |
| Dest | `lfo2Rate` |
| Amount | +50% |
| `lfo1Rate` | 0.07 Hz |
| `lfo1Waveform` | Sine |
| `lfo2Rate` (base) | 0.5 Hz |
| `lfo2Waveform` | Triangle |
| Slot 2 source | LFO 2 |
| Slot 2 dest | `positionJitter` |
| Slot 2 amount | +60% |

LFO 1 at 0.07 Hz modulates LFO 2's rate between roughly 0.25 Hz and 0.75 Hz on a 14-second cycle. LFO 2 — running at that variable rate — modulates `positionJitter`. The result: the scatter of grain read positions breathes in and out slowly, driven by a modulator whose own period is shifting. The texture never quite repeats.

---

### Snapshot performance technique

Snapshots A, B, C, and D each store a complete state of all 74 parameters. The intended live use is to prepare two or more contrasting configurations and switch between them at performance time with a single click or MIDI trigger.

**Setting up an A/B contrast for live switching:**

1. Dial in your primary texture — the sound you want for the verse or background state. Right-click **Snapshot A** and select "Save current state". Snapshot A is now locked to this configuration.

2. Without disturbing Snapshot A, modify the parameters to create a contrasting state. Useful contrasts: sparse vs. dense (`density` 3 vs. 40), dry vs. wet (`verbMix` 0% vs. 60%), frozen vs. moving (`positionJitter` 5% vs. 80%), or quiet vs. loud (`masterVolume` −12 dB vs. 0 dB). Right-click **Snapshot B** and select "Save current state".

3. Click Snapshot A, then Snapshot B, then A again. The switch is instantaneous — all parameters jump to the stored values simultaneously. There is no crossfade; the transition is a hard cut.

4. For a performance with more variety, set up C and D as intermediate states or entirely different textures (e.g., C = feedback loop active, D = spectral shimmer). You can cycle through all four during a set.

**Practical notes:**

- Snapshots persist with the DAW project file. They are available after the project is reopened.
- Right-click any Snapshot button to overwrite it with the current state. There is no undo for snapshot overwrite, so save your project first if you need to preserve a previous state.
- MIDI mapping: if your DAW supports MIDI learn on plugin controls, map each Snapshot button to a pad or key for hands-free switching during recording.
- Snapshots store the state of all modules including `feedbackEnabled` and `spectralEnabled`. Switching from a snapshot with Feedback on to one with Feedback off does not flush the feedback buffer — the tail may still be audible for a few seconds. This can be used intentionally for smooth exits from feedback textures.

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

**Caution:** Feedback Gain near 0.95 with no damping on a loud source can produce sustained loud output. Always start with low Gain and increase gradually.

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
