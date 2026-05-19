#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Engine/EnvelopeShapes.h"
#include "../Parameters.h"

class CurveEditor : public juce::Component, private juce::Timer
{
public:
    CurveEditor();
    ~CurveEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    void setAPVTS(juce::AudioProcessorValueTreeState* apvts) noexcept { apvts_ = apvts; }

private:
    void timerCallback() override { repaint(); }

    void drawEnvelopeCurve(juce::Graphics& g, EnvelopeShape shape,
                           juce::Rectangle<float> bounds, bool selected);

    juce::AudioProcessorValueTreeState* apvts_{ nullptr };

    static constexpr int kNumShapes = 5;
    static const char* shapeLabels_[kNumShapes];

    juce::Rectangle<int> thumbnailBounds_[kNumShapes];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CurveEditor)
};
