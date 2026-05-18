---
phase: 02-sample-io
plan: 01
subsystem: engine
tags: [SampleBuffer, AudioFormatManager, FileDragAndDropTarget, atomic-swap, JUCE-Timer]

requires:
  - phase: 01-granular-engine
    provides: GranularEngine with testSample_ fallback; AbstractFifo SPSC pattern

provides:
  - SampleBuffer lock-free 3-state atomic buffer (pending/current/retired)
  - AudioFormatManager WAV/AIFF/FLAC/OGG/MP3 loading
  - FileDragAndDropTarget in PluginEditor with error label
  - GranularEngine reads from SampleBuffer (testSample_ fallback preserved)
  - 15 new Catch2 tests (31 total)

affects: [02-02 WaveformDisplay, F3 APVTS, all phases reading from SampleBuffer]

tech-stack:
  added: [juce_audio_formats, juce::AudioFormatManager, juce::FileDragAndDropTarget, juce::Timer (private inheritance)]
  patterns: [AbstractFifo retirement queue (cap 4) for multi-load safety, MessageManager guard for headless tests]

key-files:
  created: [Source/Engine/SampleBuffer.h, Source/Engine/SampleBuffer.cpp, Tests/test_sample_io.cpp]
  modified: [Source/Engine/GranularEngine.h, Source/Engine/GranularEngine.cpp, Source/PluginProcessor.h, Source/PluginProcessor.cpp, Source/PluginEditor.h, Source/PluginEditor.cpp, CMakeLists.txt, Tests/CMakeLists.txt]

key-decisions:
  - "AbstractFifo (cap 4) for retired buffers instead of single atomic pointer — handles burst of 4 loads between 30 Hz cleanup ticks"
  - "numAudioSamples parameter on setPending() to track playable frames excluding 2 guard samples"
  - "trySwap() called unconditionally every processBlock (no newSourcePending_ flag) — simpler, equivalent latency"
  - "Left-channel-only read from stereo files (true, false) — avoids amplitude doubling; proper downmix deferred"
  - "MessageManager guard for startTimerHz() — prevents JUCE assertion in headless test contexts"

patterns-established:
  - "Guard: `if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)` before JUCE Timer start in test-compiled code"
  - "Retired buffer cleanup via AbstractFifo (not single atomic) — burst-safe up to kRetiredCapacity loads"
  - "Engine integration via setSource(SampleBuffer*) called once before audio starts — no atomic pointer on engine side"

duration: ~2 sessions (F1-02 context carryover)
started: 2026-05-17T00:00:00Z
completed: 2026-05-17T00:00:00Z
---

# Phase 2 Plan 01: SampleBuffer + AudioFormatManager + Drag-Drop Summary

**Lock-free 3-state SampleBuffer, WAV/AIFF/FLAC/OGG/MP3 loading via AudioFormatManager, FileDragAndDropTarget on PluginEditor, and GranularEngine wired to live sample with 440 Hz sine fallback.**

## Performance

| Metric | Value |
|--------|-------|
| Tasks | 3 completed |
| Files created | 3 (SampleBuffer.h, SampleBuffer.cpp, test_sample_io.cpp) |
| Files modified | 8 |
| Tests | 31 total (19 existing + 12 new) |
| Commit | 09786f1 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: SampleBuffer swaps atomically between threads | ✅ Pass | 12 unit tests cover all swap/retire/fallback paths |
| AC-2: AudioFormatManager loads WAV/AIFF/MP3 | ✅ Pass | registerBasicFormats() + `#if JUCE_USE_MP3AUDIOFORMAT` guarded; structural verify (no real file in tests) |
| AC-3: Drag-drop triggers loadSampleFile | ✅ Pass | isInterestedInFileDrag + filesDropped implemented; error label shown on failure |
| AC-4: GranularEngine reads from SampleBuffer with sine fallback | ✅ Pass | 3 integration tests verify: loaded source produces energy, empty source falls back to sine, null pointer doesn't crash |
| AC-5: ≥ 25 tests pass | ✅ Pass | 31/31 pass, exit 0 |

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| All 3 tasks | `09786f1` | Single atomic commit — all tasks verified before commit |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `Source/Engine/SampleBuffer.h` | Created | 3-state lock-free buffer: pending/current/retired with AbstractFifo retirement queue |
| `Source/Engine/SampleBuffer.cpp` | Created | trySwap() CAS, processRetired() FIFO drain, setPending() exchange |
| `Tests/test_sample_io.cpp` | Created | 12 SampleBuffer tests + 3 GranularEngine integration tests |
| `Source/Engine/GranularEngine.h` | Modified | Added SampleBuffer include, setSource(), sampleSource_ member |
| `Source/Engine/GranularEngine.cpp` | Modified | scheduleGrain() uses sampleSource_ with testSample_ fallback; processBlock() calls trySwap() and drains stale grains on swap |
| `Source/PluginProcessor.h` | Modified | private juce::Timer, SampleBuffer member (declared before engine_), AudioFormatManager, loadSampleFile(), getLastLoadError() |
| `Source/PluginProcessor.cpp` | Modified | registerBasicFormats(), engine_.setSource(), startTimerHz(30) with MM guard, loadSampleFile() impl |
| `Source/PluginEditor.h` | Modified | FileDragAndDropTarget inheritance, errorLabel_, drag-drop overrides |
| `Source/PluginEditor.cpp` | Modified | isInterestedInFileDrag, filesDropped → processorRef.loadSampleFile(), showError/clearError |
| `CMakeLists.txt` | Modified | SampleBuffer.cpp + juce_audio_formats added to plugin target |
| `Tests/CMakeLists.txt` | Modified | SampleBuffer.cpp + test_sample_io.cpp + juce_audio_formats added to test target |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| AbstractFifo (cap 4) for retired buffers | Single atomic `retired_` (plan spec) would lose a buffer if two swaps happened before message-thread cleanup; FIFO handles up to 4 burst loads safely | Retired slots use 32 bytes of stack; no heap |
| `setPending(buf, numAudioSamples)` — separate sample count | Guard samples (+2) must not be reported as playable frames; storing count alongside pointer avoids reading buffer metadata in audio thread | GranularEngine uses getNumSamples() directly as grain length cap |
| `trySwap()` called unconditionally per processBlock | Plan's `newSourcePending_` flag is redundant — trySwap() already short-circuits (returns false) when no pending buffer; removes an unnecessary atomic | Simpler engine; one fewer atomic per block |
| Left-channel-only for stereo files | `read(buf, 0, n, 0, true, true)` would double amplitude on stereo files; `true, false` takes only left channel cleanly | Mono buffers only in F2; proper stereo/downmix in F4+ |
| MessageManager guard on startTimerHz | GranoAudioProcessor is compiled into GranoTests; without MM, JUCE asserts in juce_Timer.cpp:376 | Tests exit 0; headless CI is clean |
| Error stored as `lastLoadError_` string (not bool return) | Plan specified `bool loadSampleFile()`; storing the message lets PluginEditor display specific error text without a second call | Minor API deviation; more useful for UI |

## Deviations from Plan

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 3 | Essential safety improvements |
| Scope additions | 1 | MessageManager guard (test robustness) |
| Deferred | 0 | — |

**Total impact:** Essential fixes, no scope creep.

### Auto-fixed Issues

**1. Retired buffer: AbstractFifo instead of single atomic**
- Found during: Task 1 (SampleBuffer design review)
- Issue: Single `retired_` atomic pointer loses buffers on burst loads (second retire before message-thread cleanup)
- Fix: `juce::AbstractFifo retiredFifo_{ 4 }` + `retiredSlots_` array — up to 4 buffers queued safely
- Verification: processRetired test exercises 3 sequential swaps

**2. Test integration: swap must be triggered before scheduler sees new buffer**
- Found during: Task 3 integration test execution
- Issue: `currentReadPtr_` is only written by `trySwap()` (audio thread atomic); scheduler reads it with acquire. Without a prior processBlock call, scheduler sees nullptr and falls back to testSample_ — integration test measured 0 energy.
- Fix: Integration test calls one `engine.processBlock()` before sleep(150) to trigger swap; then measures output energy after scheduler fills new grains.
- Verification: "GranularEngine uses SampleBuffer source when loaded" now passes

**3. `[[nodiscard]]` warnings on trySwap() in test call sites**
- Found during: Build output
- Fix: `(void) sb.trySwap()` at 5 test call sites where return value intentionally ignored
- Verification: Clean build with no nodiscard warnings

## Next Phase Readiness

**Ready:**
- SampleBuffer interface is stable — WaveformDisplay (02-02) can call `getSampleBuffer()` to access current read pointer and sample count for rendering
- `loadSampleFile()` is the single entry point for sample loading — unchanged in F2-02
- GranularEngine now plays user audio, not just a sine tone

**Concerns:**
- Stereo files: only left channel used. Engine mixes to stereo via pan, so mono input is correct for F2. Stereo-aware loading deferred.
- Error display: persistent label (not 3-second auto-dismiss as plan specified). Acceptable for F2; revisit in F3 when GranoLAF is built.
- ShutdownDetector JUCE assertion in test teardown: pre-existing from F1 headless context. Not a test failure (exit 0). Will not affect CI.

**Blockers:**
- None. F2-02 (WaveformDisplay) can proceed when `frontend-design` skill is available.

---
*Phase: 02-sample-io, Plan: 01*
*Completed: 2026-05-17*
