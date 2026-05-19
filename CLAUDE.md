# CLAUDE.md

Persistent context for Claude Code working on Grano. Read this at the start of every session.

After reading, run `/caveman-compress CLAUDE.md` once per project lifetime to shrink this file by ~50%. Code blocks, paths, and identifiers are preserved byte-for-byte.

---

## Project at a glance

**Grano** is a granular synthesis audio plugin (VST3 + AU + Standalone) built in C++20 with JUCE 8 and CMake. It targets experimental sound design with a multi-threaded engine, sub-grain recursion, stochastic timing distributions, internal feedback, spectral processing, and audio-rate parameter modulation.

Three plugin formats. Three primary platforms (Windows, macOS) plus Linux as a secondary target. Stereo output only in v1.0.

---

## What's loaded

You have these tools active in this project:

- **PAUL** — Plan/Apply/Unify Loop. Every unit of work goes through `/paul:plan` → `/paul:apply` → `/paul:unify`. See `.paul/` for state.
- **Caveman** — output compression. Default to `/caveman full`. Cuts ~65% of output tokens. Use `/caveman-commit` for commit messages.
- **Graphify** — knowledge graph of the codebase at `graphify-out/`. Hook is installed, so the graph is consulted before file reads. Use `/graphify query "..."` to navigate.
- **frontend-design** — built-in Anthropic skill for UI work. Loads automatically when editing `Source/UI/`.

---

## The hard rules

These are non-negotiable. Violating any of these requires a `consider-issues` entry, not a workaround.

1. **Real-time safety on the audio thread.** No allocation, no locks, no system calls, no exceptions, no virtual dispatch through unbounded chains. Anything in `Source/Engine/` that runs in `processBlock` is real-time-critical. Pre-allocate; use lock-free FIFOs (`juce::AbstractFifo`); pass primitives or `std::span` not container references.

2. **APVTS is the single source of truth for parameters.** Never add raw `float` or `std::atomic<float>` members on `GranoAudioProcessor` that look like parameters. Add them to `Source/Parameters.cpp` and bind via APVTS.

3. **Naming neutrality.** No reference artist, band, or commercial plugin product name in any code, comment, identifier, asset, doc, or commit message. The blocklist is in `.paul/SPECIAL-FLOWS.md`. Module and parameter names are functional and technical.

4. **CMake-first.** Every new source file must be added to `CMakeLists.txt`. Don't rely on globbing; explicit `target_sources` keeps generated IDE projects honest.

5. **Tests follow code.** Every public class in `Source/Engine/` or `Source/Modulation/` needs at least one Catch2 test under `Tests/`. Tests are required for unify, not for apply.

6. **JUCE 8 idioms.** Use `juce::dsp::ProcessorChain`, `juce::dsp::Oscillator`, `juce::AudioProcessorValueTreeState`, `juce::AbstractFifo`, `juce::ThreadPool`, `juce::dsp::FFT`. Don't reinvent what JUCE 8 provides.

7. **C++20, not later, not earlier.** Concepts, ranges, `std::span`, `std::format`, `consteval` are fair game. No C++23 features yet (compiler support uneven).

---

## How to work

### Starting a session

1. `/caveman full` (cuts tokens for the session).
2. `/paul:progress` (tells you the current phase, current plan, and one next action).
3. If the next action is "apply", first read the plan file at `.paul/phases/<NN>-<name>/<NN>-<KK>-PLAN.md`.
4. Use `/graphify query` to orient yourself in the codebase if you've been away.

### During a plan's apply

- Tasks are sequential. Each has `files`, `action`, `verify`, `done`. Complete one before starting the next.
- If verification fails, fix in place and re-verify. Do not move on with a broken task.
- For audio-thread code, prefer reading existing `Source/Engine/` patterns first. Don't invent new patterns when an established one exists.
- Commit at the end of each task or at logical groupings. Use `/caveman-commit`.

### Ending a session

- If a plan is mid-apply: `/paul:pause "<reason>"` to create a handoff.
- If a plan is done: `/paul:unify` to close the loop. **Required.**
- `git push` so the other machine has the state.

### Resuming on the other machine

```
git pull
claude
/paul:resume
```

PAUL reads `.paul/STATE.md` and tells you exactly where to pick up. No decision fatigue.

---

## Codebase orientation (after F0+)

```
Source/
├── PluginProcessor.{h,cpp}    AudioProcessor entry point
├── PluginEditor.{h,cpp}        AudioProcessorEditor entry point
├── Parameters.{h,cpp}          APVTS parameter layout (added in F3)
├── Engine/
│   ├── Grain.{h,cpp}           Single grain (added in F1)
│   ├── GranularEngine.{h,cpp}  Engine, FIFO, worker pool (F1)
│   ├── GrainPool.{h,cpp}       Pre-allocated grain slots (F1)
│   ├── EnvelopeShapes.{h,cpp}  Hann/Tukey/Gauss/etc (F1)
│   ├── SampleBuffer.{h,cpp}    Atomic sample swap (F2)
│   ├── SubGrain.{h,cpp}        Recursion (F6)
│   ├── StochasticTiming.{h,cpp} 6 distributions (F6)
│   ├── FeedbackPath.{h,cpp}    Internal feedback (F6)
│   ├── SpectralProcessor.{h,cpp} FFT freeze/blur (F6)
│   └── MultiSampleBank.{h,cpp} 4-slot blending (F6)
├── Modules/
│   ├── Motion.{h,cpp}          Wow, Flutter, Drift, Crackle (F4a)
│   ├── Color.{h,cpp}           Saturate, Decimate, Tilt, Verb (F4b)
│   └── Pattern.{h,cpp}         Trigger, Probability, Reverse, Quantize, Spray (F4c)
├── Modulation/
│   ├── Lfo.{h,cpp}             2 LFOs, audio-rate capable (F5)
│   ├── ModMatrix.{h,cpp}       8-slot routing matrix (F5)
│   └── Snapshots.{h,cpp}       A/B/C/D parameter snapshots (F5)
├── UI/
│   ├── LookAndFeel/
│   │   └── GranoLAF.{h,cpp}    Custom LookAndFeel (F3+, polished F6)
│   ├── Knob.{h,cpp}            Custom knob component (F3)
│   ├── Slider.{h,cpp}          Custom slider component (F3)
│   ├── WaveformDisplay.{h,cpp} Waveform + playhead + particles (F2+)
│   ├── LfoPanel.{h,cpp}        LFO visualization + edit (F5)
│   ├── ModulationMatrixView.{h,cpp} Matrix editor (F5)
│   ├── CurveEditor.{h,cpp}     Drawable pitch/envelope curves (F6)
│   └── Spectrogram.{h,cpp}     Optional spectral display (F6)
└── Utils/
    ├── Constants.h             Magic numbers in one place
    └── DSPMath.h               DSP helpers, conversions, lookup tables

Tests/                          Catch2 unit tests
Resources/                      Fonts, presets, demo audio
graphify-out/                   Knowledge graph (consult before reads)
.paul/                          PAUL state and plans
```

---

## Threading model

**Three classes of threads:**

1. **Audio thread** (host-owned). Runs `processBlock`. Real-time safe.
2. **Message thread / UI thread** (JUCE-owned). Runs editor paint/update. Can allocate freely.
3. **Worker threads** (our pool, `juce::ThreadPool`, 3-4 workers). Generate grains, run spectral FFT, anything heavy and non-real-time-deadline.

**Communication:**

- UI → Audio: via APVTS atomics. UI sets parameter, audio reads atomically. Lock-free.
- UI → Workers: via direct method calls (workers are thread-safe-by-construction internally).
- Workers → Audio: via SPSC `juce::AbstractFifo` carrying ready grain descriptors. Audio drains the FIFO each block.
- Audio → UI: via lock-free counters and atomics (grain count, CPU load) sampled by the UI's `Timer` callback at 30 Hz.

**What never happens:**

- Workers writing to memory the audio thread reads concurrently.
- Audio thread waiting on workers (the FIFO is its only contact, drained non-blocking).
- UI thread blocking on audio thread.

See `ARCHITECTURE.md` section "Threading model" for the diagram.

---

## Performance budget

- Nominal: ≤ 15% CPU on a 2020 i7 with 200 grains, all modules enabled, default settings.
- Worst case: ≤ 40% CPU on the same hardware with 1000 grains, sub-grain depth 2, feedback on, spectral on.
- Latency: ≤ 64 samples @ 48 kHz (≈ 1.3 ms) excluding spectral. Spectral adds 2048 samples ≈ 42 ms, reported via `setLatencySamples`.
- Memory: ≤ 200 MB resident with all 4 sample slots loaded.

---

## Quick commands

```
/paul:progress            where am I, what's next
/paul:plan <phase>        create the next plan
/paul:apply <path>        execute approved plan
/paul:unify <path>        close the loop
/paul:pause <reason>      handoff for break
/paul:resume              pick up after break
/paul:discuss <phase>     decide before planning
/paul:consider-issues     triage deferred work

/caveman full             default compression
/caveman ultra            extreme density
/caveman-commit           commit message
/caveman-stats            token savings
/caveman-compress <file>  shrink memory file

/graphify .               build graph
/graphify . --update      re-extract changed files
/graphify query "..."     ask the graph
/graphify path A B        shortest path between nodes
/graphify explain X       summarize node X

cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build --output-on-failure
pluginval --strictness-level 5 path/to/Grano.vst3
```

---

## When confused

1. `/paul:progress` — what phase are we in, what plan is current.
2. `/graphify query "<topic>"` — let the graph orient you instead of grepping.
3. Read `.paul/phases/<current>/...PLAN.md` — the plan is the contract.
4. Read `ARCHITECTURE.md` for design rationale.
5. Read `PROJECT.md` for ADRs and constraints.

If after all that you're still confused, `/paul:pause "stuck on <X>"` and create a `/paul:discover` plan to explore the issue. Don't guess; explore explicitly.

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

Rules:
- ALWAYS read graphify-out/GRAPH_REPORT.md before reading any source files, running grep/glob searches, or answering codebase questions. The graph is your primary map of the codebase.
- IF graphify-out/wiki/index.md EXISTS, navigate it instead of reading raw files
- For cross-module "how does X relate to Y" questions, prefer `graphify query "<question>"`, `graphify path "<A>" "<B>"`, or `graphify explain "<concept>"` over grep — these traverse the graph's EXTRACTED + INFERRED edges instead of scanning files
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
