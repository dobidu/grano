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

void GranoSlider::paintOverChildren(juce::Graphics& g)
{
    if (grainSizeParam_ == nullptr)
        return;

    const float grainMs  = grainSizeParam_->load(std::memory_order_relaxed);
    // Normalize [5, 500] ms → [0, 1]; maps to at most 40% of track width.
    const float normFrac = juce::jmap(juce::jlimit(5.0f, 500.0f, grainMs),
                                       5.0f, 500.0f, 0.0f, 1.0f);
    if (normFrac < 0.002f)
        return;

    const auto  sb      = slider_.getBoundsInParent().toFloat();
    const float thumbR  = 5.0f;
    const float trackX0 = sb.getX() + thumbR;
    const float trackW  = sb.getWidth() - thumbR * 2.0f;

    const double range  = slider_.getMaximum() - slider_.getMinimum();
    const float valFrac = range > 0.0
        ? (float)((slider_.getValue() - slider_.getMinimum()) / range) : 0.0f;
    const float thumbX  = trackX0 + valFrac * trackW;
    const float extW    = juce::jmin(normFrac * trackW * 0.4f,
                                      sb.getRight() - thumbR - thumbX);
    if (extW < 1.0f)
        return;

    const float trackY = sb.getCentreY();
    const float rectH  = 10.0f;
    g.setColour(juce::Colour{ 0xff7cf5c4u }.withAlpha(0.35f));
    g.fillRoundedRectangle(thumbX, trackY - rectH * 0.5f, extW, rectH, 3.0f);
}
