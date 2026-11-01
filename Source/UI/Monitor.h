#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Engine/GranularEngine.h"

// Monitor — the waveform well across the top of the console.
//
// Shows the sample in the selected slot, the read position (Position, plus
// the engine's loop-sweep offset when Loop is on), the Position Jitter span
// and grain-size shadow, and live grains from the engine snapshot (heads sized
// and warmed by envelope amplitude). Click / drag sets Position as one host
// gesture. Files dropped on the editor load into the selected slot.

class Monitor : public juce::Component,
                public juce::SettableTooltipClient
{
public:
    Monitor(juce::AudioFormatManager&, juce::AudioProcessorValueTreeState&, GranularEngine&);
    ~Monitor() override;

    // Message thread: sample shown (empty file / 0 frames = empty slot).
    void setSample(const juce::File&, double sampleRate, int numFrames, int numChannels);
    void setDragHighlight(bool);

    // Editor timer (30 Hz): capture grains, repaint.
    void tick();
    int  getGrainCount() const noexcept { return grainCount_; }

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    void setPositionFromX(float x);
    juce::Rectangle<float> band() const;

    juce::AudioThumbnailCache thumbCache_{ 4 };
    juce::AudioThumbnail      thumb_;
    GranularEngine&           engine_;
    juce::RangedAudioParameter& positionParam_;
    juce::ParameterAttachment   positionAttach_;
    std::atomic<float>* grainSize_;
    std::atomic<float>* jitter_;
    std::atomic<float>* loop_;
    std::atomic<float>* pitch_;

    juce::String name_;
    double sampleRate_ { 0.0 };
    int    numFrames_ { 0 }, numChannels_ { 0 };
    bool   dragHighlight_ { false }, dragging_ { false };
    float  position_ { 0.0f };

    std::array<GranularEngine::GrainSnapshot, GranularEngine::MaxActiveGrains> snaps_{};
    int grainCount_ { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Monitor)
};
