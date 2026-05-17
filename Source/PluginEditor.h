#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// GranoAudioProcessorEditor is the AudioProcessorEditor entry point.
//
// F0: Carbon background (#0A0B0D) + centred "GRANO" label. No controls.
// F3: Custom LookAndFeel applied; knobs and sliders added.
// F6: Visual polish — halo glows, radial vignette, particle trails.
//
// See DESIGN_SPEC.md for the complete visual identity specification.

class GranoAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit GranoAudioProcessorEditor(GranoAudioProcessor&);
    ~GranoAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized()              override;

private:
    GranoAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranoAudioProcessorEditor)
};
