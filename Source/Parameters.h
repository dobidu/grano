#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs {
    inline constexpr const char* grainSize      = "grainSize";
    inline constexpr const char* density        = "density";
    inline constexpr const char* position       = "position";
    inline constexpr const char* positionJitter = "positionJitter";
    inline constexpr const char* pitchShift     = "pitchShift";
    inline constexpr const char* stereoSpread   = "stereoSpread";
    inline constexpr const char* masterVolume   = "masterVolume";
    inline constexpr const char* loop           = "loop";
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
