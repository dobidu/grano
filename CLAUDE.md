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
│   ├── Grain.h                 Single grain descriptor, header-only (F1)
│   ├── GranularEngine.{h,cpp}  Engine, sample-clock scheduler, mixer (F1, 08-08)
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
├── UI/                         Lighting-console editor (08-09); see DESIGN_SPEC.md
│   ├── LookAndFeel/
│   │   ├── GranoTheme.{h,cpp}  Colour tokens + embedded fonts (single source)
│   │   └── GranoLAF.{h,cpp}    ComboBox / PopupMenu / tooltip / button styling
│   ├── ParamFormat.{h,cpp}     Display text, names, hints, mod-dest table
│   ├── Console.{h,cpp}         Widget kit: faders, encoders, strips, keys, banks
│   ├── ConsoleView.{h,cpp}     1200x780 root: layout, timer, command line, hints
│   ├── Header.{h,cpp}          Command line, snapshots, loop, master meter
│   ├── Monitor.{h,cpp}         Waveform + grains + loop playhead + click-to-position
│   ├── SlotStrip.{h,cpp}       Sample slot: show / load / weight / clear
│   └── Banks.{h,cpp}           GRAIN ENGINE ENVELOPE MOTION COLOR PATTERN LFO MATRIX
└── Utils/                      (empty — constants live in their owning headers;
                                 Spectrogram display deferred post-v1.0)

Tests/                          Catch2 unit tests
Resources/                      Presets/ (5 factory .gpreset files)
graphify-out/                   Knowledge graph (consult before reads)
.paul/                          PAUL state and plans
```

---

## Threading model

**Two classes of threads:**

1. **Audio thread** (host-owned). Runs `processBlock`: schedules grains on a sample clock (exact sample offsets, PPQ-locked Sync/Euclidean) and renders them. Modulation, scheduling and Motion run in 32-sample sub-blocks. Real-time safe.
2. **Message thread / UI thread** (JUCE-owned). Runs editor paint/update, sample loading, spectral FFT, freeing retired buffers. Can allocate freely.

(The F1 "GrainScheduler" thread + SPSC FIFO was removed in 08-08: wall-clock scheduling broke offline bounces and onset timing.)

**Communication:**

- UI → Audio: via APVTS atomics. UI sets parameter, audio reads atomically. Lock-free.
- Message thread → Audio: sample buffers via `SampleBuffer` pending record `{buffer, length, generation}`, swapped in one atomic exchange; old buffers retire through an `AbstractFifo` and are freed on the message thread. The audio thread never allocates or frees.
- Audio → UI: grain snapshot array + atomic count, loop-sweep fraction, sampled by UI `Timer` callbacks (WaveformDisplay 30 Hz, editor footer 10 Hz).

**What never happens:**

- The audio thread allocating, freeing, locking or waiting.
- The FFT reading a slot before the newly loaded buffer is live (load generation check).
- UI thread blocking on audio thread.

See `ARCHITECTURE.md` section "Threading model" for the diagram.

---

## Performance budget

- Nominal: ≤ 15% CPU on a 2020 i7 with 200 grains, all modules enabled, default settings.
- Worst case: ≤ 40% CPU on the same hardware at the 256 active-grain cap, sub-grain depth 2, feedback on, spectral on.
- Latency: 0 samples reported (spectral is offline on the source and adds no audio-path delay).
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
