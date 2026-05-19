#pragma once
#include "Lfo.h"
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>

class ModMatrix
{
public:
    static constexpr int kNumSlots = 8;

    // Destination enum — order must match kModDests choice array in Parameters.cpp.
    enum Dest {
        kNone = 0,
        kGrainSize, kDensity, kPosition, kPositionJitter, kPitchShift,
        kStereoSpread, kMasterVol,
        kWowDepth, kWowRate, kFlutterDepth, kFlutterRate, kDrift,
        kCrackleLevel, kCrackleColor,
        kSaturate, kDecimate, kTiltEq, kVerbMix,
        kProbability, kReverseProb, kSpray, kTransientSens,
        kLfo1Rate, kLfo1Phase, kLfo1Depth,
        kLfo2Rate, kLfo2Phase, kLfo2Depth,
        kNumDests
    };

    ModMatrix() = default;

    void setLfos(Lfo* lfo1, Lfo* lfo2) noexcept;
    void setSlotParams(int slot,
                       std::atomic<float>* source,
                       std::atomic<float>* dest,
                       std::atomic<float>* amount) noexcept;
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    // Audio thread: advance LFOs and compute mod offsets for this block.
    void processBlock(int numSamples, double bpm) noexcept;

    // Scheduler/audio thread: current mod offset for destination d in [-1..+1].
    float getModOffset(Dest d) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMatrix)

private:
    Lfo* lfo1_{ nullptr };
    Lfo* lfo2_{ nullptr };

    struct SlotParams {
        std::atomic<float>* pSource{ nullptr }; // 0=None, 1=LFO1, 2=LFO2
        std::atomic<float>* pDest  { nullptr }; // index into Dest enum
        std::atomic<float>* pAmount{ nullptr }; // [-1..+1]
    };
    std::array<SlotParams, kNumSlots> slots_{};

    // Written by audio thread in processBlock, read by scheduler thread via getModOffset.
    // atomic<float> gives TSAN-clean cross-thread reads without a lock.
    std::array<std::atomic<float>, kNumDests> modSums_{};
};
