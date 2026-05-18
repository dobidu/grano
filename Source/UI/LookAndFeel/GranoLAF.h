#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class GranoLAF : public juce::LookAndFeel_V4
{
public:
    GranoLAF();

    // Semantic colour palette (DESIGN_SPEC ADR-009)
    static const juce::Colour kSurfaceBase;   // #0A0B0D
    static const juce::Colour kSurface1;      // #1A1D22
    static const juce::Colour kBorderMuted;   // #2A2E36
    static const juce::Colour kTextPrimary;   // #E8E6E1
    static const juce::Colour kTextSecond;    // #8B8985
    static const juce::Colour kVital;         // #7CF5C4  active/accent
    static const juce::Colour kGrain;         // #C77CFF  grain particles
    static const juce::Colour kClip;          // #FF5252  clipping only

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float startAngle,
                          float endAngle, juce::Slider&) override;

    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& bg,
                              bool isMouseOver, bool isButtonDown) override;

    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool isMouseOver, bool isButtonDown) override;

    juce::Font getLabelFont(juce::Label&) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranoLAF)
};
