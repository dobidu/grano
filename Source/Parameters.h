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

    // F5 — LFO 1
    inline constexpr const char* lfo1Rate     = "lfo1Rate";
    inline constexpr const char* lfo1Waveform = "lfo1Waveform";
    inline constexpr const char* lfo1Sync     = "lfo1Sync";
    inline constexpr const char* lfo1Phase    = "lfo1Phase";
    inline constexpr const char* lfo1Depth    = "lfo1Depth";

    // F5 — LFO 2
    inline constexpr const char* lfo2Rate     = "lfo2Rate";
    inline constexpr const char* lfo2Waveform = "lfo2Waveform";
    inline constexpr const char* lfo2Sync     = "lfo2Sync";
    inline constexpr const char* lfo2Phase    = "lfo2Phase";
    inline constexpr const char* lfo2Depth    = "lfo2Depth";

    // F6b — Sub-grain + Stochastic timing
    inline constexpr const char* subGrainDepth  = "subGrainDepth";
    inline constexpr const char* stochasticDist = "stochasticDist";

    // F6c — Feedback path
    inline constexpr const char* feedbackEnabled = "feedbackEnabled";
    inline constexpr const char* feedbackGain    = "feedbackGain";
    inline constexpr const char* feedbackDamp    = "feedbackDamp";

    // F6c — Spectral processor
    inline constexpr const char* spectralEnabled    = "spectralEnabled";
    inline constexpr const char* spectralMode       = "spectralMode";
    inline constexpr const char* spectralBlurAmount = "spectralBlurAmount";

    // F6d — Multi-sample bank slot weights
    inline constexpr const char* slot0Weight = "slot0Weight";
    inline constexpr const char* slot1Weight = "slot1Weight";
    inline constexpr const char* slot2Weight = "slot2Weight";
    inline constexpr const char* slot3Weight = "slot3Weight";

    // F6e — Envelope shape selector (0=Hann 1=Tukey 2=Gaussian 3=Triangle 4=Rectangle)
    inline constexpr const char* envelopeShape = "envelopeShape";

    // F5 — ModMatrix slots (8 slots × 3 params)
    inline constexpr const char* slot1Source = "slot1Source";
    inline constexpr const char* slot1Dest   = "slot1Dest";
    inline constexpr const char* slot1Amount = "slot1Amount";
    inline constexpr const char* slot2Source = "slot2Source";
    inline constexpr const char* slot2Dest   = "slot2Dest";
    inline constexpr const char* slot2Amount = "slot2Amount";
    inline constexpr const char* slot3Source = "slot3Source";
    inline constexpr const char* slot3Dest   = "slot3Dest";
    inline constexpr const char* slot3Amount = "slot3Amount";
    inline constexpr const char* slot4Source = "slot4Source";
    inline constexpr const char* slot4Dest   = "slot4Dest";
    inline constexpr const char* slot4Amount = "slot4Amount";
    inline constexpr const char* slot5Source = "slot5Source";
    inline constexpr const char* slot5Dest   = "slot5Dest";
    inline constexpr const char* slot5Amount = "slot5Amount";
    inline constexpr const char* slot6Source = "slot6Source";
    inline constexpr const char* slot6Dest   = "slot6Dest";
    inline constexpr const char* slot6Amount = "slot6Amount";
    inline constexpr const char* slot7Source = "slot7Source";
    inline constexpr const char* slot7Dest   = "slot7Dest";
    inline constexpr const char* slot7Amount = "slot7Amount";
    inline constexpr const char* slot8Source = "slot8Source";
    inline constexpr const char* slot8Dest   = "slot8Dest";
    inline constexpr const char* slot8Amount = "slot8Amount";
}

// Returns "slotNSource", "slotNDest", or "slotNAmount" for slot index 1..8.
juce::String slotParamID(int slotIndex, const char* suffix);

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
