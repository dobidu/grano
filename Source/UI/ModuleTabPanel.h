#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "Knob.h"
#include "LfoPanel.h"
#include "ModulationMatrixView.h"

// ModuleTabPanel hosts all F4/F6 module controls in 6 tabs:
//   ENGINE | MOTION | COLOR | PATTERN | LFO | MOD
//
// Replaces the flat lfoPanel_ + modMatrixView_ pair in PluginEditor.

class ModuleTabPanel : public juce::Component
{
public:
    ModuleTabPanel();

    void init(juce::AudioProcessorValueTreeState& apvts);

    void paint  (juce::Graphics& g) override;
    void resized()                  override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModuleTabPanel)

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    static constexpr int kTabCount = 6;
    static constexpr int kTabBarH  = 28;

    void showTab        (int idx);
    void buildEngineTab (juce::AudioProcessorValueTreeState& apvts);
    void buildMotionTab (juce::AudioProcessorValueTreeState& apvts);
    void buildColorTab  (juce::AudioProcessorValueTreeState& apvts);
    void buildPatternTab(juce::AudioProcessorValueTreeState& apvts);

    int activeTab_{ 0 };
    juce::TextButton tabBtns_[kTabCount];

    // ── ENGINE tab ──────────────────────────────────────────────────────────────
    juce::ComboBox     subGrainDepthBox_;
    juce::Label        subGrainLabel_    { "", "SUB-GRAIN" };
    juce::ComboBox     stochasticDistBox_;
    juce::Label        stochasticLabel_  { "", "TIMING" };

    juce::ToggleButton feedbackEnableBtn_ { "FEEDBACK" };
    Knob               feedbackGainKnob_  { "GAIN" };
    Knob               feedbackDampKnob_  { "DAMP" };

    juce::ToggleButton spectralEnableBtn_ { "SPECTRAL" };
    juce::ComboBox     spectralModeBox_;
    juce::Label        spectralModeLabel_ { "", "MODE" };
    Knob               spectralBlurKnob_  { "BLUR" };

    std::unique_ptr<CA> subGrainAttach_, stochasticAttach_, spectralModeAttach_;
    std::unique_ptr<BA> feedbackEnableAttach_, spectralEnableAttach_;
    std::unique_ptr<SA> feedbackGainAttach_, feedbackDampAttach_, spectralBlurAttach_;

    // ── MOTION tab ──────────────────────────────────────────────────────────────
    juce::ToggleButton motionEnableBtn_  { "MOTION ON" };
    Knob               wowDepthKnob_     { "WOW DEP"  };
    Knob               wowRateKnob_      { "WOW RATE" };
    Knob               flutterDepthKnob_ { "FLT DEP"  };
    Knob               flutterRateKnob_  { "FLT RATE" };
    Knob               driftAmountKnob_  { "DRIFT"    };
    Knob               crackleLevelKnob_ { "CRACKLE"  };
    Knob               crackleColorKnob_ { "CR COLOR" };

    std::unique_ptr<BA> motionEnableAttach_;
    std::unique_ptr<SA> wowDepthAttach_, wowRateAttach_;
    std::unique_ptr<SA> flutterDepthAttach_, flutterRateAttach_;
    std::unique_ptr<SA> driftAmountAttach_;
    std::unique_ptr<SA> crackleLevelAttach_, crackleColorAttach_;

    // ── COLOR tab ───────────────────────────────────────────────────────────────
    juce::ToggleButton colorEnableBtn_ { "COLOR ON" };
    Knob               saturateKnob_   { "SAT"  };
    Knob               decimateKnob_   { "DECI" };
    Knob               tiltEqKnob_     { "TILT" };
    Knob               verbMixKnob_    { "VERB" };

    std::unique_ptr<BA> colorEnableAttach_;
    std::unique_ptr<SA> saturateAttach_, decimateAttach_, tiltEqAttach_, verbMixAttach_;

    // ── PATTERN tab ─────────────────────────────────────────────────────────────
    juce::ToggleButton patternEnableBtn_ { "PATTERN ON" };
    juce::ComboBox     triggerModeBox_;
    juce::Label        triggerModeLabel_ { "", "TRIGGER" };
    juce::ComboBox     quantizeScaleBox_;
    juce::Label        quantizeLabel_    { "", "SCALE"   };
    Knob               probabilityKnob_  { "PROB"    };
    Knob               reverseProbKnob_  { "REV PROB" };
    Knob               sprayKnob_        { "SPRAY"   };
    Knob               euclidPulsesKnob_ { "PULSES"  };
    Knob               euclidStepsKnob_  { "STEPS"   };
    Knob               euclidRotKnob_    { "ROT"     };

    std::unique_ptr<BA> patternEnableAttach_;
    std::unique_ptr<CA> triggerModeAttach_, quantizeScaleAttach_;
    std::unique_ptr<SA> probabilityAttach_, reverseProbAttach_, sprayAttach_;
    std::unique_ptr<SA> euclidPulsesAttach_, euclidStepsAttach_, euclidRotAttach_;

    // ── LFO + MOD tabs ──────────────────────────────────────────────────────────
    LfoPanel             lfoPanel_;
    ModulationMatrixView modMatrixView_;
};
