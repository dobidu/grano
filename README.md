# Grano

A granular instrument plugin for experimental sound design.

`VST3` · `AU` · `Standalone` · Windows · macOS · Linux

---

## What it is

Grano is a granular synthesis instrument built in C++ with the JUCE framework. It loads audio samples (WAV, AIFF, MP3) and granulates them through a multi-threaded engine with sub-grain recursion, stochastic timing distributions, internal feedback paths, spectral processing, and per-grain audio-rate modulation.

The design target is experimental sound design — drones, evolving textures, broken rhythms, glitch artifacts, granular clouds, and unstable timbral worlds.

## Core capabilities

**Sample I/O**: Drag & drop WAV, AIFF, MP3. Multi-sample blending across 4 slots with weighted stochastic selection per grain.

**Granular engine**: 5–500 ms grain size, 1–100 grains/sec density (or sync-locked, or Euclidean, or audio-driven), position 0–100%, position jitter 0–100%, pitch shift ±24 semitones (or scale-locked to 12+ scales including microtonal), stereo spread 0–100%. Up to 1000 concurrent grains via lock-free FIFO + worker thread pool. Sub-grain recursion to depth 2. Six stochastic timing distributions (Uniform, Gaussian, Poisson, Exponential, Pareto, 1/f).

**Three processing modules** (each with bypass):
- `MOTION` — Wow, Flutter, Drift, Crackle. Organic instability.
- `COLOR` — Saturate, Decimate, Tilt, Verb. Spectral coloration.
- `PATTERN` — Trigger Mode, Probability, Reverse Probability, Quantize, Spray. Algorithmic structure.

**Modulation**: Two LFOs (sine / triangle / saw / square / random / S&H / drawable custom), rates up to audio-rate (8 kHz), free or DAW-sync, assignable to any parameter via a routing matrix. Snapshots A/B/C/D with persistence.

**Visualization**: Waveform with cursor, live grain particles overlaid on the waveform, optional spectrogram.

## Tech stack

| Component | Technology |
|-----------|------------|
| Language | C++20 |
| Framework | JUCE 8 |
| Build | CMake 3.22+ |
| Process orchestration | [PAUL](https://github.com/ChristopherKahler/paul) |
| Token compression | [Caveman](https://github.com/JuliusBrussee/caveman) |
| Code navigation | [Graphify](https://github.com/safishamsi/graphify) |
| Plugin formats | VST3, AU, Standalone |
| Targets | Windows 10+, macOS 11+, Linux (Ubuntu 22.04+) |

## Status

In active development. See `.paul/ROADMAP.md` for the current phase and `.paul/STATE.md` for the current loop position.

## Getting started

For development setup, read `SETUP_GUIDE.md`. For build instructions, read `BUILD.md`.

## License

To be determined.
