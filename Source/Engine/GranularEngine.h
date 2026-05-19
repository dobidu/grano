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

// GranularEngine — owns the grain pool, scheduler thread, lock-free FIFO,
// and audio-thread mixer.
//
// Threading model:
//   SchedulerThread (single producer) — acquires grain slots, fills them,
//     pushes pointers into the AbstractFifo. Runs at ~50 grains/sec.
//   Audio thread (consumer) — drains the FIFO in processBlock(), mixes
//     active grains into the output buffer, releases finished grains.
//
// RT contract: processBlock() performs no heap allocation, no locks,
//   no blocking calls. All storage is pre-allocated.
//
// Source data: SampleBuffer set via setSource() provides live audio data.
//   Falls back to internal 440 Hz sine tone if no SampleBuffer is loaded.
//
// setSource() must be called before prepareToPlay() (before audio thread starts).
class GranularEngine
{
public:
    static constexpr int FifoCapacity    = 512;
    static constexpr int MaxActiveGrains = 256;

    GranularEngine();
    ~GranularEngine();

    // Called from prepare-to-play (message thread). Allocates test sample,
    // starts scheduler. May be called multiple times (re-initialises cleanly).
    void prepare(double sampleRate, int samplesPerBlock);

    // Called from processBlock (audio thread). RT-safe.
    void processBlock(juce::AudioBuffer<float>& buffer) noexcept;

    // Stops scheduler, releases all active grains, drains FIFO.
    void reset();

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
    };

    // UI thread (30 Hz Timer): copy current active grain snapshot data.
    // Returns the number of grains written into `out` (≤ maxCount).
    // Technically racy on the snapshot array (audio writes, UI reads) but
    // aligned-float writes are atomic on x86 and visual tearing is acceptable.
    int getGrainSnapshots(GrainSnapshot* out, int maxCount) const noexcept;

private:
    // --- scheduler thread (sole FIFO producer) ---
    class SchedulerThread : public juce::Thread
    {
    public:
        explicit SchedulerThread(GranularEngine& e)
            : juce::Thread("GrainScheduler"), engine_(e) {}
        void run() override;
    private:
        GranularEngine& engine_;
    };

    // Called only from SchedulerThread.
    void scheduleGrain() noexcept;

    // Renders one grain for numSamples output samples.
    // Returns true when the grain has finished playback.
    bool renderGrain(Grain* g, float* L, float* R, int numSamples) noexcept;

    // --- data ---
    GrainPool pool_;

    // SPSC FIFO: SchedulerThread → audio thread
    juce::AbstractFifo               fifo_{ FifoCapacity };
    std::array<Grain*, FifoCapacity> fifoSlots_{};

    // Active grains — audio thread only, no sharing
    std::array<Grain*, MaxActiveGrains> activeGrains_{};
    int                                 activeGrainCount_{ 0 };

    // Hard-coded 440 Hz sine, 1 s @ sampleRate_ + 2 guard samples.
    // Written once in prepare(), read-only thereafter.
    // +2 guard samples prevent srcInt+1 OOB at pitchRatio ≤ 2.
    std::vector<float> testSample_;
    std::atomic<bool>  sampleReady_{ false };

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

    // Pitch modulation source — set once before audio starts, read in scheduler thread.
    Motion*  pitchMod_ { nullptr };

    // Pattern module — set once before audio starts, consulted in scheduler thread.
    Pattern* pattern_  { nullptr };

    // ModMatrix — set once before audio starts, read in scheduler thread.
    class ModMatrix* modMatrix_{ nullptr };

    // Used only from SchedulerThread — no synchronization needed.
    juce::Random grainRng_;

    std::array<GrainSnapshot, MaxActiveGrains> grainSnapshots_{};
    std::atomic<int>                           grainSnapshotCount_{ 0 };

    SchedulerThread schedulerThread_{ *this };
};
