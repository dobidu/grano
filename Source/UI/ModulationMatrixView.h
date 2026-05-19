#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

class ModulationMatrixView : public juce::Component
{
public:
    ModulationMatrixView() = default;

    void init(juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics& g) override;
    void resized()               override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationMatrixView)

private:
    static constexpr int kNumSlots = 8;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct SlotRow
    {
        juce::Label    indexLabel;
        juce::ComboBox sourceBox, destBox;
        juce::Slider   amountSlider;
        std::unique_ptr<ComboAttach>  sourceAttach, destAttach;
        std::unique_ptr<SliderAttach> amountAttach;
    };

    std::array<SlotRow, kNumSlots> rows_;
};
