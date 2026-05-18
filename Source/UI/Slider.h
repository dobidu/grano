#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>

class GranoSlider : public juce::Component
{
public:
    explicit GranoSlider(const juce::String& name);

    juce::Slider& getSlider() noexcept { return slider_; }

    void setGrainExtent(std::atomic<float>* grainParam, float durationMs) noexcept
    {
        grainSizeParam_ = grainParam;
        fileDurationMs_ = durationMs;
    }
    void setFileDurationMs(float ms) noexcept { fileDurationMs_ = ms; }

    void resized()                       override;
    void paintOverChildren(juce::Graphics&) override;

private:
    juce::Slider slider_;
    juce::Label  nameLabel_;

    std::atomic<float>* grainSizeParam_ { nullptr };
    float               fileDurationMs_ { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranoSlider)
};
