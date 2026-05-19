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

    // Wire to APVTS grainSize param (ms) to draw grain extent shadow on waveform.
    void setGrainSizeParam(std::atomic<float>* p) noexcept { grainSizeParam_ = p; }

    void paint(juce::Graphics&) override;
    void resized()              override;

private:
    void timerCallback() override;

    void paintBackground    (juce::Graphics&, juce::Rectangle<float> bounds);
    void paintWaveform      (juce::Graphics&, juce::Rectangle<float> bounds);
    void paintParticles     (juce::Graphics&, juce::Rectangle<float> bounds,
                             const GranularEngine::GrainSnapshot* snaps,  int count,
                             const GranularEngine::GrainSnapshot* trail1, int count1,
                             const GranularEngine::GrainSnapshot* trail2, int count2);
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
    std::atomic<float>* positionParam_  { nullptr };
    std::atomic<float>* grainSizeParam_ { nullptr };

    // Trail ring: [0]=current, [1]=1 frame ago, [2]=2 frames ago
    static constexpr int kTrailFrames = 3;
    std::array<std::array<GranularEngine::GrainSnapshot,
        GranularEngine::MaxActiveGrains>, kTrailFrames> trailSnaps_{};
    std::array<int, kTrailFrames> trailCounts_{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};
