#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <cmath>

class Color
{
public:
    Color() = default;  // explicit — JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR suppresses implicit default ctor

    void prepare(double sampleRate, int samplesPerBlock) noexcept;
    void reset()                                         noexcept;

    void setParamPointers(std::atomic<float>* enabled,
                          std::atomic<float>* saturate,
                          std::atomic<float>* decimate,
                          std::atomic<float>* tiltEq,
                          std::atomic<float>* verbMix) noexcept;

    // Called from audio thread in processBlock. RT-safe.
    void processBlock(juce::AudioBuffer<float>& buffer) noexcept;

private:
    void applySaturate(float* L, float* R, int N, float amount)    noexcept;
    void applyDecimate(float* L, float* R, int N, float amount)    noexcept;
    void applyTilt    (float* L, float* R, int N, float tiltParam) noexcept;
    void applyVerb    (float* L, float* R, int N, float mix)       noexcept;

    double sampleRate_ { 44100.0 };

    // Tilt EQ: 1-pole LPF coefficient (fixed at 1 kHz pivot, computed in prepare())
    float tiltAlpha_    { 0.0f };
    float tiltState_[2] { 0.0f, 0.0f };

    // Decimate: sample-and-hold state per channel + phase counter
    float decimateHeld_[2] { 0.0f, 0.0f };
    float decimatePhase_   { 0.0f };

    // Verb: JUCE built-in plate reverb (Freeverb-style), no external IR
    juce::Reverb reverb_;

    std::atomic<float>* pEnabled_  { nullptr };
    std::atomic<float>* pSaturate_ { nullptr };
    std::atomic<float>* pDecimate_ { nullptr };
    std::atomic<float>* pTiltEq_   { nullptr };
    std::atomic<float>* pVerbMix_  { nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Color)
};
