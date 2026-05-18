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

    // F4a — MOTION module
    inline constexpr const char* motionEnabled  = "motionEnabled";
    inline constexpr const char* wowDepth       = "wowDepth";
    inline constexpr const char* wowRate        = "wowRate";
    inline constexpr const char* flutterDepth   = "flutterDepth";
    inline constexpr const char* flutterRate    = "flutterRate";
    inline constexpr const char* driftAmount    = "driftAmount";
    inline constexpr const char* crackleLevel   = "crackleLevel";
    inline constexpr const char* crackleColor   = "crackleColor";
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
