#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>

// Audio-rate-capable LFO.
// All waveform computation is lock-free. Drawable waveform uses atomic<float>
// per point so UI writes and audio reads don't need a lock.
class Lfo
{
public:
    static constexpr int kDrawablePoints = 64;

    enum class Waveform { Sine = 0, Triangle, Saw, Square, Random, SAndH, Drawable };

    Lfo() noexcept;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void setParamPointers(std::atomic<float>* rate,
                          std::atomic<float>* waveform,
                          std::atomic<float>* sync,
                          std::atomic<float>* phase,
                          std::atomic<float>* depth) noexcept;

    // Audio thread: advance phase by one sample, return value scaled by depth.
    float advanceSample() noexcept;

    // Audio thread: return last computed value (no phase advance).
    float currentValue() const noexcept { return lastValue_; }

    // Audio thread (block boundary): update sync rate from DAW bpm.
    void processBlock(int numSamples, double bpm) noexcept;

    // UI thread: edit drawable waveform table.
    void setDrawablePoint(int index, float value) noexcept;
    float getDrawablePoint(int index) const noexcept;

    // ModMatrix → LFO cross-mod: adds Hz offset to effective rate (clamped ≥ 0.01).
    // Safe to call from audio thread while advanceSample() runs on same thread.
    void setRateModOffset(float offsetHz) noexcept {
        rateModOffset_.store(offsetHz, std::memory_order_relaxed);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Lfo)

private:
    float computeSample(float normPhase, Waveform wf) noexcept;
    float sampleDrawable(float normPhase) const noexcept;

    double phase_{ 0.0 };
    double sampleRate_{ 44100.0 };
    float  lastValue_{ 0.0f };

    // S&H state
    float  sAndHHeld_{ 0.0f };
    float  prevPhaseForSH_{ 0.0f };

    // Smooth random state (interpolate between two random values each cycle)
    float  randCurrent_{ 0.0f };
    float  randNext_{ 0.0f };
    float  prevPhaseForRand_{ 0.0f };

    // Sync mode: rate computed from DAW bpm (updated in processBlock)
    float  syncRateHz_{ 1.0f };

    // Cross-mod offset in Hz — written by ModMatrix, read in advanceSample()
    std::atomic<float> rateModOffset_{ 0.0f };

    // 64-point drawable table — written from UI, read from audio thread
    std::array<std::atomic<float>, kDrawablePoints> drawable_{};

    juce::Random rng_;

    std::atomic<float>* pRate_     { nullptr };
    std::atomic<float>* pWaveform_ { nullptr };
    std::atomic<float>* pSync_     { nullptr };
    std::atomic<float>* pPhase_    { nullptr };
    std::atomic<float>* pDepth_    { nullptr };
};
