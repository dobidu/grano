#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/LookAndFeel/GranoLAF.h"
#include "UI/ConsoleView.h"

// GranoAudioProcessorEditor — hosts the console view (fixed 1200 x 780 base
// canvas, scaled uniformly 0.75x-1.6x with a fixed aspect ratio) and owns
// file drag-and-drop (incl. the Win32 OLE drop target).
//
// See DESIGN_SPEC.md for the visual system and design/mock/grano-ui.html for
// the approved layout.

class GranoAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   public juce::FileDragAndDropTarget,
                                   private juce::Timer
{
public:
    explicit GranoAudioProcessorEditor(GranoAudioProcessor&);
    ~GranoAudioProcessorEditor() override;

    void paint(juce::Graphics&)      override;
    void resized()                   override;
    void parentHierarchyChanged()    override;

    // ── FileDragAndDropTarget ─────────────────────────────────────────────────
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files)               override;

private:
    void timerCallback() override;
#if JUCE_WINDOWS
    void tryRegisterWindowDrop();
#endif

    GranoLAF             laf_;
    ConsoleView          view_;
    juce::TooltipWindow  tooltips_ { this, 900 };

    bool  dragAcceptRegistered_{ false };
    void* droppedHwnd_{ nullptr }; // HWND with our OLE drop target (Win32)
    void* dropTarget_{ nullptr };  // IDropTarget* we registered (Win32)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranoAudioProcessorEditor)
};
