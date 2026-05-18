#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <array>
#include <cstdint>

class Pattern
{
public:
    Pattern() = default;

    void prepare(double sampleRate) noexcept;
    void reset()                    noexcept;

    void setParamPointers(
        std::atomic<float>* enabled,
        std::atomic<float>* triggerMode,
        std::atomic<float>* syncDivision,
        std::atomic<float>* euclidPulses,
        std::atomic<float>* euclidSteps,
        std::atomic<float>* euclidRotation,
        std::atomic<float>* transientSens,
        std::atomic<float>* probability,
        std::atomic<float>* reverseProb,
        std::atomic<float>* quantizeScale,
        std::atomic<float>* spray) noexcept;

    // Called from the audio thread in PluginProcessor::processBlock().
    // Handles Sync BPM tracking and Audio-driven transient detection.
    void processBlock(const juce::AudioBuffer<float>& inputBuffer, double bpm) noexcept;

    // ── Scheduler-thread API (called from GranularEngine::SchedulerThread) ────────

    // Returns false when patternEnabled=true and the grain should be skipped
    // (probability roll, Euclidean miss, or Audio-driven no transient).
    bool shouldFireGrain() noexcept;

    // Returns true when patternEnabled=true and reverseProb roll succeeds.
    bool shouldReverseGrain() noexcept;

    // Returns quantized+spray pitch in semitones.
    // basePitchSt = pitchShift + pitchMod already summed by the engine.
    float getPitchAdjustment(float basePitchSt) noexcept;

    // Returns spray-influenced duration multiplier (1.0 = unchanged).
    float getDurMultiplier() noexcept;

    // Returns the override inter-grain interval in seconds for Sync/Euclidean/Audio modes.
    // Returns -1.0f for Free mode (engine uses density param).
    float getNextIntervalSec() noexcept;

    // True when triggerMode is not Free and patternEnabled=true.
    bool isSchedulerOverrideActive() const noexcept;

private:
    // ── Scale tables ─────────────────────────────────────────────────────────────
    static constexpr int kMaxScaleNotes = 12;
    static constexpr int kNumScales     = 11;

    struct Scale { int8_t notes[kMaxScaleNotes]; int count; };
    static const Scale kScales[kNumScales];

    float quantizeToScale(float pitchSt, int scaleIdx) noexcept;

    // ── Euclidean rhythm ─────────────────────────────────────────────────────────
    using EuclidPattern = std::array<bool, 32>;
    static EuclidPattern buildEuclidean(int pulses, int steps, int rotation) noexcept;
    void rebuildEuclidIfNeeded() noexcept;

    EuclidPattern euclidPattern_{};
    int           euclidPatternLen_  { 8 };
    int           euclidStep_        { 0 };   // scheduler-thread only
    int           cachedPulses_      { -1 };
    int           cachedSteps_       { -1 };
    int           cachedRotation_    { -1 };

    // ── Sync ─────────────────────────────────────────────────────────────────────
    std::atomic<float> syncIntervalAtomic_ { 0.25f };  // written by audio thread, read by scheduler

    // ── Audio-driven transient detection ─────────────────────────────────────────
    float  energyFollower_    { 0.0f };
    double sampleRate_        { 44100.0 };
    int    transientCooldown_ { 0 };

    std::atomic<bool> transientFired_ { false };  // written by audio thread, cleared by scheduler

    // ── Constants ────────────────────────────────────────────────────────────────
    static constexpr float kMaxSprayPitchSt     = 12.0f;
    static constexpr float kMaxSprayDurMult     = 2.0f;
    static constexpr float kTransientCooldownMs = 80.0f;
    static constexpr float kPollingIntervalSec  = 0.010f;

    // ── Param pointers ────────────────────────────────────────────────────────────
    std::atomic<float>* pEnabled_          { nullptr };
    std::atomic<float>* pTriggerMode_      { nullptr };
    std::atomic<float>* pSyncDivision_     { nullptr };
    std::atomic<float>* pEuclidPulses_     { nullptr };
    std::atomic<float>* pEuclidSteps_      { nullptr };
    std::atomic<float>* pEuclidRotation_   { nullptr };
    std::atomic<float>* pTransientSens_    { nullptr };
    std::atomic<float>* pProbability_      { nullptr };
    std::atomic<float>* pReverseProb_      { nullptr };
    std::atomic<float>* pQuantizeScale_    { nullptr };
    std::atomic<float>* pSpray_            { nullptr };

    juce::Random rng_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Pattern)
};
