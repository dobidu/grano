#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Rotary control with name label below. Styled by GranoLAF.
// Attach to APVTS externally via SliderAttachment on getSlider().
class Knob : public juce::Component
{
public:
    explicit Knob(const juce::String& name);

    juce::Slider& getSlider() noexcept { return slider_; }

    void resized() override;

private:
    juce::Slider slider_;
    juce::Label  nameLabel_;
    juce::Label  valueLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Knob)
};
