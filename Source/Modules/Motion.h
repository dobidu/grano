#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>

class Motion
{
public:
    Motion() = default;
    void prepare(double sampleRate) noexcept;
    void reset()                    noexcept;

    void setParamPointers(std::atomic<float>* enabled,
                          std::atomic<float>* wowDepth,  std::atomic<float>* wowRate,
                          std::atomic<float>* flutDepth, std::atomic<float>* flutRate,
                          std::atomic<float>* drift,
                          std::atomic<float>* crackLevel,
                          std::atomic<float>* crackColor) noexcept;

    // Called from audio thread in processBlock.
    // Updates pitchModSt_ and adds crackle to buffer.
    void processBlock(juce::AudioBuffer<float>& buffer) noexcept;

    // Read by GranularEngine::scheduleGrain() (scheduler thread).
    // Returns total pitch offset in semitones (Wow + Flutter + Drift).
    float getPitchModSemitones() const noexcept
    {
        return pitchModSt_.load(std::memory_order_relaxed);
    }

private:
    float nextPink()                    noexcept;
    void  updatePitchMod(int numSamples) noexcept;

    double sampleRate_ { 44100.0 };

    // LFO phases (audio thread only)
    float wowPhase_     { 0.0f };
    float flutterPhase_ { 0.0f };

    // Drift random walk (audio thread only)
    float driftCurrent_ { 0.0f };
    float driftTarget_  { 0.0f };
    int   driftCounter_ { 0 };

    // Pink noise — Paul Kellet 7-stage (audio thread only)
    float b0_ {}, b1_ {}, b2_ {}, b3_ {}, b4_ {}, b5_ {}, b6_ {};

    // Color filter state (1-pole IIR, audio thread only)
    float filterState_ { 0.0f };

    // Cross-thread: audio thread writes, scheduler thread reads
    std::atomic<float> pitchModSt_ { 0.0f };

    std::atomic<float>* pEnabled_    { nullptr };
    std::atomic<float>* pWowDepth_   { nullptr };
    std::atomic<float>* pWowRate_    { nullptr };
    std::atomic<float>* pFlutDepth_  { nullptr };
    std::atomic<float>* pFlutRate_   { nullptr };
    std::atomic<float>* pDrift_      { nullptr };
    std::atomic<float>* pCrackLevel_ { nullptr };
    std::atomic<float>* pCrackColor_ { nullptr };

    juce::Random rng_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Motion)
};
