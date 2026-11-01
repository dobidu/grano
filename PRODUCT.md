# Product

<!-- impeccable:product-schema 1 -->

## Platform

web

(Design mocks are authored as HTML. The shipping product is a native desktop audio plugin — JUCE 8, VST3/AU/Standalone on Windows, macOS, Linux — so every mock must be translatable by hand into JUCE `Component`/`LookAndFeel` painting: flat vector shapes, paths, text, no browser-only effects.)

## Users

- **Primary: electronic musicians performing live.** They tweak Grano in real time during sets and jams, in a DAW or the standalone app, often under stage light or in a dark room, glancing between the plugin and other gear. They need the instrument's state readable at a glance and controls large enough to grab fast.
- **Secondary: studio sound designers.** Long exploratory sessions building textures in a DAW, usually on a large monitor, where depth and precise readouts matter.

## Product Purpose

Grano is a granular synthesis instrument for experimental sound design. It slices loaded samples into grains and re-sprays them, shaped by modulation, pattern and colour modules. Success: a performer can load a sample, find a texture, and steer it live without hunting for hidden state.

## Positioning

The engine goes past a stock granulator: sub-grain recursion (grains that spawn grain families), six stochastic timing distributions (Uniform, Gaussian, Poisson, Exponential, Pareto, 1/f), an internal feedback path that re-granulates its own output, spectral Freeze/Blur on the source, a 4-slot sample bank blended by weight, sample-accurate host-synced and Euclidean triggering, and audio-rate LFOs (up to 8 kHz) through an 8-slot matrix.

## Operating Context

- Lives inside a DAW plugin window or a standalone window; the host controls window chrome. Window is resizable; the redesign targets a fixed-ratio scalable canvas (base 1200 × 780, 0.75–1.6×).
- Samples arrive by drag-and-drop from the OS or a file dialog, into one of 4 slots.
- Four parameter snapshots (A/B/C/D) are the live-performance memory: store, recall, compare.
- Parameters are automated from the DAW; the UI must reflect host-driven changes.

## Capabilities and Constraints

- Parameter set is fixed (IDs, ranges, choices): Grain Size 5–500 ms, Density 1–100 g/s, Position 0–100 %, Position Jitter, Pitch Shift ±24 st, Stereo Spread, Master Volume −60…+6 dB, Loop; Sub-grain Depth 0–2; Stochastic Distribution (6); Feedback on/gain 0–0.95/damp; Spectral on/Freeze|Blur/blur amount; 4 slot weights; Envelope Shape (Hann, Tukey, Gaussian, Triangle, Rectangle); MOTION (wow depth/rate, flutter depth/rate, drift, crackle level/colour); COLOR (saturate, decimate, tilt EQ, verb mix); PATTERN (trigger Free/Sync/Euclidean/Audio, sync division, Euclid pulses/steps/rotation, transient sensitivity, probability, reverse prob, quantize scale, spray); LFO 1/2 (rate 0.01 Hz–8 kHz, waveform, sync, phase, depth); 8 matrix slots (source LFO1/LFO2 → destination, bipolar amount).
- Telemetry available to the UI: live grain snapshots (position, envelope amplitude), active-grain count, loop-sweep position, output level.
- Out of scope for v1.0: drawable LFO editor, spectrogram, pitch-curve editor, MIDI learn.
- Naming neutrality: no artist, band or commercial product names anywhere.
- Fonts must be embeddable under an open licence (OFL or similar).

## Brand Commitments

- Name: **Grano**. Module names MOTION, COLOR, PATTERN are product vocabulary.
- The previous "Lab Equipment Brutalist" look (DESIGN_SPEC.md) is **not** binding; the user opened the visual world for replacement (2026-11-01).

## Evidence on Hand

- 5 factory presets: Broken, Cloud, Drone, Rhythmic, Spectral (`Resources/Presets/`).
- Existing screenshot `screenshot.png` (old UI). No testimonials, users or benchmarks exist; none may be invented.

## Product Principles

1. **State is never hidden.** What is on, what is loaded, what is modulating and which snapshot is live must read at a glance.
2. **Performable first.** Live performance is the primary scene: big readable targets, no tab hunting, no tiny text.
3. **The sound leads.** Visuals show what the engine is actually doing (grains, sweep, modulation), not decoration.
4. **Not another generic plugin.** Avoid the stock dark-synth look of glowing neon knobs.

## Accessibility & Inclusion

Legible under stage lighting and from arm's length; no text below a size readable at 0.75× scale; colour never the only carrier of state; every control keyboard-reachable with an accessible name.
