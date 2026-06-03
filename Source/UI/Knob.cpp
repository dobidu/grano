#include "Knob.h"

Knob::Knob(const juce::String& name)
{
    slider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(slider_);

    nameLabel_.setText(name, juce::dontSendNotification);
    nameLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(nameLabel_);

    valueLabel_.setJustificationType(juce::Justification::centred);
    valueLabel_.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f)));
    valueLabel_.setColour(juce::Label::textColourId, juce::Colour{ 0xff6b7280u });
    addAndMakeVisible(valueLabel_);

    slider_.onValueChange = [this] {
        valueLabel_.setText(juce::String(slider_.getValue(), 2),
                            juce::dontSendNotification);
    };
}

void Knob::resized()
{
    constexpr int kLabelH = 16;
    constexpr int kValueH = 12;
    auto b = getLocalBounds();
    nameLabel_.setBounds(b.removeFromBottom(kLabelH));
    valueLabel_.setBounds(b.removeFromBottom(kValueH));
    slider_.setBounds(b);
}
