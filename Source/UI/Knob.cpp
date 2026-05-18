#include "Knob.h"

Knob::Knob(const juce::String& name)
{
    slider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(slider_);

    nameLabel_.setText(name, juce::dontSendNotification);
    nameLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(nameLabel_);
}

void Knob::resized()
{
    constexpr int kLabelH = 16;
    auto b = getLocalBounds();
    nameLabel_.setBounds(b.removeFromBottom(kLabelH));
    slider_.setBounds(b);
}
