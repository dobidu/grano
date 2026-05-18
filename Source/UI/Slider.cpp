#include "Slider.h"

GranoSlider::GranoSlider(const juce::String& name)
{
    slider_.setSliderStyle(juce::Slider::LinearHorizontal);
    slider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(slider_);

    nameLabel_.setText(name, juce::dontSendNotification);
    nameLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(nameLabel_);
}

void GranoSlider::resized()
{
    constexpr int kLabelH = 14;
    auto b = getLocalBounds();
    nameLabel_.setBounds(b.removeFromTop(kLabelH));
    slider_.setBounds(b);
}
