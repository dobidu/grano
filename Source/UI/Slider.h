#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Horizontal linear slider with name label above. Styled by GranoLAF.
// Attach to APVTS externally via SliderAttachment on getSlider().
class GranoSlider : public juce::Component
{
public:
    explicit GranoSlider(const juce::String& name);

    juce::Slider& getSlider() noexcept { return slider_; }

    void resized() override;

private:
    juce::Slider slider_;
    juce::Label  nameLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranoSlider)
};
