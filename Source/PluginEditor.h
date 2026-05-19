#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/WaveformDisplay.h"
#include "UI/LookAndFeel/GranoLAF.h"
#include "UI/Knob.h"
#include "UI/Slider.h"
#include "UI/LfoPanel.h"
#include "UI/ModulationMatrixView.h"
#include "UI/CurveEditor.h"

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
    ~GranoAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized()              override;

    // ── FileDragAndDropTarget ─────────────────────────────────────────────────
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files)               override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void showError(const juce::String& message);
    void clearError();
    void openFileChooser(int slot);

    juce::uint32 errorGen_{ 0 };

    GranoAudioProcessor& processorRef;

    GranoLAF laf_;

    WaveformDisplay                    waveformDisplay_;
    juce::TextButton                   slotButtons_[4];
    std::unique_ptr<juce::FileChooser> fileChooser_;
    int                                pendingSlot_{ 0 };
    juce::Label                        errorLabel_;
    CurveEditor                        curveEditor_;
    Knob                               slotWeightKnobs_[4] = { Knob{"W0"}, Knob{"W1"}, Knob{"W2"}, Knob{"W3"} };
    std::unique_ptr<SliderAttachment>  slotWeightAttachments_[4];

    // ── Core controls ─────────────────────────────────────────────────────────
    GranoSlider        positionSlider_  { "POSITION" };
    Knob               grainSizeKnob_   { "SIZE"     };
    Knob               densityKnob_     { "DENS"     };
    Knob               posJitterKnob_   { "JITTER"   };
    Knob               pitchShiftKnob_  { "PITCH"    };
    GranoSlider        spreadSlider_    { "SPREAD"   };
    Knob               masterVolKnob_   { "VOLUME"   };
    juce::ToggleButton loopButton_;

    // ── APVTS attachments (must outlive controls) ──────────────────────────────
    std::unique_ptr<SliderAttachment> positionAttach_;
    std::unique_ptr<SliderAttachment> grainSizeAttach_;
    std::unique_ptr<SliderAttachment> densityAttach_;
    std::unique_ptr<SliderAttachment> posJitterAttach_;
    std::unique_ptr<SliderAttachment> pitchShiftAttach_;
    std::unique_ptr<SliderAttachment> spreadAttach_;
    std::unique_ptr<SliderAttachment> masterVolAttach_;
    std::unique_ptr<ButtonAttachment> loopAttach_;

    LfoPanel             lfoPanel_;
    ModulationMatrixView modMatrixView_;
    juce::TextButton     snapButtons_[4];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranoAudioProcessorEditor)
};
