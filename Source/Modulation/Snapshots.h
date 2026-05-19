#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

// 4-slot parameter state snapshot (A=0, B=1, C=2, D=3).
// Message-thread only — never called from audio thread.
class Snapshots
{
public:
    Snapshots() = default;
    static constexpr int kNumSlots = 4;

    void save   (int slot, const juce::ValueTree& state) noexcept;
    void recall (int slot, juce::AudioProcessorValueTreeState& apvts) const noexcept;
    bool isOccupied (int slot) const noexcept;

    juce::ValueTree serialise() const;
    void deserialise(const juce::ValueTree& data);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Snapshots)

private:
    std::array<juce::ValueTree, kNumSlots> slots_{};
};
