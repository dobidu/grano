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

    // F4b — COLOR module
    inline constexpr const char* colorEnabled = "colorEnabled";
    inline constexpr const char* saturate     = "saturate";
    inline constexpr const char* decimate     = "decimate";
    inline constexpr const char* tiltEq       = "tiltEq";
    inline constexpr const char* verbMix      = "verbMix";

    // F4c — PATTERN module
    inline constexpr const char* patternEnabled       = "patternEnabled";
    inline constexpr const char* triggerMode          = "triggerMode";
    inline constexpr const char* syncDivision         = "syncDivision";
    inline constexpr const char* euclidPulses         = "euclidPulses";
    inline constexpr const char* euclidSteps          = "euclidSteps";
    inline constexpr const char* euclidRotation       = "euclidRotation";
    inline constexpr const char* transientSensitivity = "transientSensitivity";
    inline constexpr const char* probability          = "probability";
    inline constexpr const char* reverseProb          = "reverseProb";
    inline constexpr const char* quantizeScale        = "quantizeScale";
    inline constexpr const char* spray                = "spray";
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
