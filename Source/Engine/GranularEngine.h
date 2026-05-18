#pragma once

#include "GrainPool.h"
#include "SampleBuffer.h"
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

    // Set the live sample source. Call before prepareToPlay (before audio starts).
    // Pass nullptr to revert to the 440 Hz sine fallback.
    void setSource(SampleBuffer* sb) noexcept { sampleSource_ = sb; }

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

    // Live sample source — set once before audio starts, never changed while running.
    SampleBuffer* sampleSource_{ nullptr };

    double sampleRate_{ 48000.0 };

    std::array<GrainSnapshot, MaxActiveGrains> grainSnapshots_{};
    std::atomic<int>                           grainSnapshotCount_{ 0 };

    SchedulerThread schedulerThread_{ *this };
};
