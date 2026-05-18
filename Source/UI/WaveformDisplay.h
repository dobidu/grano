#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Engine/GranularEngine.h"

class WaveformDisplay : public juce::Component,
                        private juce::Timer
{
public:
    explicit WaveformDisplay(juce::AudioFormatManager& formatManager);
    ~WaveformDisplay() override;

    // Message thread: called after a sample is loaded successfully.
    void setFile(const juce::File& file, double sampleRate, int numFrames);

    // Connect to engine for particle data (call once, before first paint).
    void setEngine(GranularEngine* engine) noexcept { engine_ = engine; }

    // PluginEditor calls these to drive the drag-over highlight.
    void setDragHighlightActive(bool active);

    // Wire to APVTS position param so playhead follows the knob, not just grain averages.
    void setPositionParam(std::atomic<float>* p) noexcept { positionParam_ = p; }

    void paint(juce::Graphics&) override;
    void resized()              override;

private:
    void timerCallback() override { repaint(); }

    void paintBackground    (juce::Graphics&, juce::Rectangle<float> bounds);
    void paintWaveform      (juce::Graphics&, juce::Rectangle<float> bounds);
    void paintParticles     (juce::Graphics&, juce::Rectangle<float> bounds,
                             const GranularEngine::GrainSnapshot* snaps, int count);
    void paintPlayhead      (juce::Graphics&, juce::Rectangle<float> bounds,
                             const GranularEngine::GrainSnapshot* snaps, int count);
    void paintLabels        (juce::Graphics&, juce::Rectangle<float> bounds, int grainCount);
    void paintDragHighlight (juce::Graphics&, juce::Rectangle<float> bounds);

    juce::AudioFormatManager&  formatManager_;
    juce::AudioThumbnailCache  thumbnailCache_{ 5 };
    juce::AudioThumbnail       thumbnail_;

    GranularEngine* engine_{ nullptr };

    juce::String filename_;
    double       fileSampleRate_{ 0.0 };
    int          fileNumFrames_{ 0 };

    bool dragHighlightActive_{ false };
    std::atomic<float>* positionParam_{ nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};
