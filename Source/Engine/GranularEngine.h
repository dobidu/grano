#pragma once

#include "GrainPool.h"
#include "SampleBuffer.h"
#include "SubGrain.h"
#include "StochasticTiming.h"
#include "FeedbackPath.h"
#include "SpectralProcessor.h"
#include "MultiSampleBank.h"
#include "../Modules/Motion.h"
#include "../Modules/Pattern.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <vector>

// GranularEngine — owns the grain pool, the sample-clock grain scheduler and
// the audio-thread mixer.
//
// Threading model (08-08, R-01):
//   Everything runs on the audio thread. processBlock() advances a sample
//   clock: grains are spawned at exact sample offsets inside the block (no
//   wall-clock thread, no FIFO), so offline renders, realtime playback and
//   any host block size produce identical grain timing. Sync/Euclidean steps
//   are anchored to the host PPQ position while the transport plays.
//   The UI reads grain snapshots (plain array + atomic count).
//
// RT contract: processBlock() performs no heap allocation, no locks,
//   no blocking calls. All storage is pre-allocated.
//
// Source data: MultiSampleBank set via setBank(); falls back to an internal
//   440 Hz sine if nothing is loaded.
class GranularEngine
{
public:
    static constexpr int MaxActiveGrains = 256;

    GranularEngine();
    ~GranularEngine();

    // Called from prepare-to-play (message thread, audio stopped). Allocates
    // the test sample. May be called multiple times (re-initialises cleanly).
    void prepare(double sampleRate, int samplesPerBlock);

    // Audio thread. Clears `buffer`, spawns due grains at their sample offsets,
    // renders all active grains. Does NOT apply master volume (see
    // applyMasterGain) so the feedback tap can sit pre-volume. RT-safe.
    void processBlock(juce::AudioBuffer<float>& buffer) noexcept;

    // Audio thread: host transport for the block about to be processed.
    // ppq = quarter-note position at the first sample of that block.
    void setTransport(double bpm, bool isPlaying, bool ppqValid, double ppq) noexcept
    {
        bpm_ = bpm > 0.0 ? bpm : 120.0;
        transportPlaying_ = isPlaying;
        ppqValid_ = ppqValid;
        ppq_ = ppq;
    }

    // Audio thread: master volume (+ mod), ramped from the previous block's
    // gain to avoid zipper noise (R-14).
    void applyMasterGain(juce::AudioBuffer<float>& buffer) noexcept;

    // Releases all active grains and resets the sample clock. Audio stopped.
    void reset();

    // Tests / telemetry: deterministic RNG seed and total grains spawned
    // (parents only, after pattern gating) since construction.
    void setRandomSeed(juce::int64 seed) noexcept { grainRng_.setSeed(seed); }
    long long getSpawnCount() const noexcept
    {
        return spawnCount_.load(std::memory_order_relaxed);
    }

    // Renders (adds) one grain into L/R for numSamples output samples.
    // Returns true when the grain has finished playback. Stateless apart
    // from the grain itself — public for unit tests.
    static bool renderGrain(Grain* g, float* L, float* R, int numSamples) noexcept;

    // UI: current loop-sweep offset in [0..1) (0 when loop is off).
    float getLoopSweepFraction() const noexcept
    {
        return loopSweepFrac_.load(std::memory_order_relaxed);
    }

    // Set the multi-sample bank as the grain source. Call before prepareToPlay.
    void setBank(MultiSampleBank* b) noexcept { bankSource_ = b; }
    void setBankParamPointers(std::atomic<float>* w0,
                              std::atomic<float>* w1,
                              std::atomic<float>* w2,
                              std::atomic<float>* w3) noexcept
    {
        pSlotWeight_[0] = w0;
        pSlotWeight_[1] = w1;
        pSlotWeight_[2] = w2;
        pSlotWeight_[3] = w3;
    }

    // Provide Motion module for per-grain pitch modulation. Call before audio starts.
    void setPitchModSource(Motion* m) noexcept { pitchMod_ = m; }

    // Provide Pattern module for grain scheduling. Call before audio starts.
    void setPatternSource(Pattern* p) noexcept { pattern_ = p; }

    // Provide ModMatrix for per-grain pitch and jitter modulation. Call before audio starts.
    void setModMatrixSource(class ModMatrix* mm) noexcept { modMatrix_ = mm; }

    // Wire APVTS parameter atomics. Call once from PluginProcessor constructor.
    void setParamPointers(std::atomic<float>* grainSize,
                          std::atomic<float>* density,
                          std::atomic<float>* position,
                          std::atomic<float>* positionJitter,
                          std::atomic<float>* pitchShift,
                          std::atomic<float>* stereoSpread,
                          std::atomic<float>* masterVolume,
                          std::atomic<float>* loop) noexcept
    {
        pGrainSize_      = grainSize;
        pDensity_        = density;
        pPosition_       = position;
        pPositionJitter_ = positionJitter;
        pPitchShift_     = pitchShift;
        pStereoSpread_   = stereoSpread;
        pMasterVolume_   = masterVolume;
        pLoop_           = loop;
    }

    // Wire F6b params. Call once from PluginProcessor constructor.
    void setAdvancedParamPointers(std::atomic<float>* subGrainDepth,
                                  std::atomic<float>* stochasticDist) noexcept
    {
        pSubGrainDepth_  = subGrainDepth;
        pStochasticDist_ = stochasticDist;
    }

    // Wire F6e envelope shape param. Call once from PluginProcessor constructor.
    void setEnvelopeShapeParam(std::atomic<float>* p) noexcept { pEnvelopeShape_ = p; }

    // Wire feedback source + params. Call once from PluginProcessor constructor.
    void setFeedbackSource(FeedbackPath* fb) noexcept { feedbackSource_ = fb; }
    void setFeedbackParamPointers(std::atomic<float>* enabled,
                                  std::atomic<float>* gain,
                                  std::atomic<float>* damp) noexcept
    {
        pFeedbackEnabled_ = enabled;
        pFeedbackGain_    = gain;
        pFeedbackDamp_    = damp;
    }

    // Wire spectral source + params. Call once from PluginProcessor constructor.
    void setSpectralSource(SpectralProcessor* sp) noexcept { spectralSource_ = sp; }
    void setSpectralParamPointers(std::atomic<float>* enabled,
                                  std::atomic<float>* mode,
                                  std::atomic<float>* blurAmount) noexcept
    {
        pSpectralEnabled_    = enabled;
        pSpectralMode_       = mode;
        pSpectralBlurAmount_ = blurAmount;
    }

    struct GrainSnapshot
    {
        float srcFraction;  // grain start position as [0..1] of loaded sample length
        float envelopeAmp;  // current envelope amplitude [0..1] for particle opacity
        float yFraction;    // stable per-grain vertical scatter [0..1) for particles
    };

    // UI thread (30 Hz Timer): copy current active grain snapshot data.
    // Returns the number of grains written into `out` (≤ maxCount).
    // Technically racy on the snapshot array (audio writes, UI reads) but
    // aligned-float writes are atomic on x86 and visual tearing is acceptable.
    int getGrainSnapshots(GrainSnapshot* out, int maxCount) const noexcept;

private:
    // Sample-clock scheduling for the next numSamples (audio thread).
    void runScheduler(int numSamples) noexcept;
    // Pattern gate + spawn. step >= 0: PPQ-anchored grid index.
    void fireEvent(long long step, int offset) noexcept;
    // Interval to the next free-running event, in samples (>= 1).
    double nextIntervalSamples(int mode) noexcept;
    // Builds one grain (+ sub-grains) starting `offset` samples into the block.
    void scheduleGrain(int offset) noexcept;
    void addActive(Grain* g) noexcept;


    // --- data ---
    GrainPool pool_;

    // Active grains — audio thread only, no sharing
    std::array<Grain*, MaxActiveGrains> activeGrains_{};
    int                                 activeGrainCount_{ 0 };

    // Hard-coded 440 Hz sine, 1 s @ sampleRate_ + 2 guard samples.
    // Written once in prepare(), read-only thereafter.
    // +2 guard samples prevent srcInt+1 OOB at pitchRatio ≤ 2.
    std::vector<float> testSample_;
    bool               sampleReady_{ false };

    // Multi-sample bank — set once before audio starts, never changed while running.
    MultiSampleBank*    bankSource_{ nullptr };
    std::atomic<float>* pSlotWeight_[4]{};

    double sampleRate_{ 48000.0 };

    // APVTS param pointers — set once before audio starts, read only in scheduler/audio threads.
    std::atomic<float>* pGrainSize_     { nullptr };
    std::atomic<float>* pDensity_       { nullptr };
    std::atomic<float>* pPosition_      { nullptr };
    std::atomic<float>* pPositionJitter_{ nullptr };
    std::atomic<float>* pPitchShift_    { nullptr };
    std::atomic<float>* pStereoSpread_  { nullptr };
    std::atomic<float>* pMasterVolume_  { nullptr };
    std::atomic<float>* pLoop_          { nullptr };
    std::atomic<float>* pSubGrainDepth_ { nullptr };
    std::atomic<float>* pStochasticDist_{ nullptr };
    std::atomic<float>* pEnvelopeShape_ { nullptr };

    // Feedback source + params — set once before audio starts.
    FeedbackPath*       feedbackSource_  { nullptr };
    std::atomic<float>* pFeedbackEnabled_{ nullptr };
    std::atomic<float>* pFeedbackGain_   { nullptr };
    std::atomic<float>* pFeedbackDamp_   { nullptr };

    // Spectral source + params — set once before audio starts.
    SpectralProcessor*  spectralSource_     { nullptr };
    std::atomic<float>* pSpectralEnabled_   { nullptr };
    std::atomic<float>* pSpectralMode_      { nullptr };
    std::atomic<float>* pSpectralBlurAmount_{ nullptr };

    // Pitch modulation source — set once before audio starts.
    Motion*  pitchMod_ { nullptr };

    // Pattern module — set once before audio starts.
    Pattern* pattern_  { nullptr };

    // ModMatrix — set once before audio starts.
    class ModMatrix* modMatrix_{ nullptr };

    // Audio thread only.
    juce::Random grainRng_;

    // Sample clock: samples from the start of the current block to the next
    // free-running event (may be fractional — no ms truncation).
    double samplesToNext_{ 0.0 };
    double bpm_{ 120.0 };
    bool   transportPlaying_{ false };
    bool   ppqValid_{ false };
    double ppq_{ 0.0 };
    static constexpr long long kNoStep = -(1LL << 62);
    long long lastAnchoredStep_{ kNoStep };

    // Loop position sweep (audio thread). When the loop param is on, the
    // effective grain position advances with rendered samples and wraps,
    // sweeping the source once per source duration.
    double loopTimeSec_{ 0.0 };
    bool   prevLoopOn_ { false };
    std::atomic<float> loopSweepFrac_{ 0.0f };

    std::atomic<long long> spawnCount_{ 0 };

    // Master gain ramp state (audio thread).
    float lastMasterGain_{ -1.0f };

    std::array<GrainSnapshot, MaxActiveGrains> grainSnapshots_{};
    std::atomic<int>                           grainSnapshotCount_{ 0 };
};
