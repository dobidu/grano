#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/WaveformDisplay.h"

// GranoAudioProcessorEditor is the AudioProcessorEditor entry point.
//
// F0: Carbon background (#0A0B0D) + centred "GRANO" label. No controls.
// F2: FileDragAndDropTarget — drop audio files to load samples.
// F3: Custom LookAndFeel applied; knobs and sliders added.
// F6: Visual polish — halo glows, radial vignette, particle trails.
//
// See DESIGN_SPEC.md for the complete visual identity specification.

class GranoAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   public juce::FileDragAndDropTarget
{
public:
    explicit GranoAudioProcessorEditor(GranoAudioProcessor&);
    ~GranoAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized()              override;

    // ── FileDragAndDropTarget ─────────────────────────────────────────────────
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files)               override;

private:
    void showError(const juce::String& message);
    void clearError();
    void openFileChooser();

    GranoAudioProcessor& processorRef;

    WaveformDisplay              waveformDisplay_;
    juce::TextButton             loadButton_;
    std::unique_ptr<juce::FileChooser> fileChooser_;
    juce::Label                  errorLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranoAudioProcessorEditor)
};
