#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class LfoPanel : public juce::Component
{
public:
    LfoPanel();

    void init(juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics& g) override;
    void resized()               override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LfoPanel)

private:
    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void showLfo(int idx); // 0=LFO1, 1=LFO2

    juce::TextButton  lfo1Tab_{ "LFO1" }, lfo2Tab_{ "LFO2" };
    int               currentLfo_{ 0 };

    juce::ComboBox    waveformBox_;
    juce::Slider      rateSlider_, depthSlider_, phaseSlider_;
    juce::ToggleButton syncButton_{ "SYNC" };
    juce::Label        rateLabel_ { "", "RATE"  };
    juce::Label        depthLabel_{ "", "DEPTH" };
    juce::Label        phaseLabel_{ "", "PHASE" };

    std::unique_ptr<ComboAttach>  waveAttach_;
    std::unique_ptr<SliderAttach> rateAttach_, depthAttach_, phaseAttach_;
    std::unique_ptr<ButtonAttach> syncAttach_;

    juce::AudioProcessorValueTreeState* apvts_{ nullptr };
};
