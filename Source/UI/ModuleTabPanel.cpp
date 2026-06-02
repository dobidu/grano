#include "ModuleTabPanel.h"
#include "../Parameters.h"

namespace
{
    static const char* kTabLabels[] = { "ENGINE", "MOTION", "COLOR", "PATTERN", "LFO", "MOD" };

    const juce::Colour kSurface { 0xff111418u };
    const juce::Colour kBorder  { 0xff2a2e36u };
    const juce::Colour kMint    { 0xff7cf5c4u };

    void populateSubGrainBox(juce::ComboBox& b)
    {
        b.addItem("Depth 0", 1);
        b.addItem("Depth 1", 2);
        b.addItem("Depth 2", 3);
    }

    void populateStochasticBox(juce::ComboBox& b)
    {
        b.addItem("Uniform",     1);
        b.addItem("Gaussian",    2);
        b.addItem("Poisson",     3);
        b.addItem("Exponential", 4);
        b.addItem("Pareto",      5);
        b.addItem("1/f",         6);
    }

    void populateSpectralModeBox(juce::ComboBox& b)
    {
        b.addItem("Freeze", 1);
        b.addItem("Blur",   2);
    }

    void populateTriggerBox(juce::ComboBox& b)
    {
        b.addItem("Free",         1);
        b.addItem("Sync",         2);
        b.addItem("Euclidean",    3);
        b.addItem("Audio-driven", 4);
    }

    void populateQuantizeBox(juce::ComboBox& b)
    {
        b.addItem("Chromatic",   1);
        b.addItem("Major",       2);
        b.addItem("Minor",       3);
        b.addItem("Dorian",      4);
        b.addItem("Phrygian",    5);
        b.addItem("Lydian",      6);
        b.addItem("Mixolydian",  7);
        b.addItem("Pent Major",  8);
        b.addItem("Pent Minor",  9);
        b.addItem("Whole Tone", 10);
        b.addItem("Octatonic",  11);
    }
}

ModuleTabPanel::ModuleTabPanel()
{
    for (int i = 0; i < kTabCount; ++i)
    {
        tabBtns_[i].setButtonText(kTabLabels[i]);
        tabBtns_[i].setClickingTogglesState(false);
        tabBtns_[i].onClick = [this, i] { showTab(i); };
        addAndMakeVisible(tabBtns_[i]);
    }

    populateSubGrainBox    (subGrainDepthBox_);
    populateStochasticBox  (stochasticDistBox_);
    populateSpectralModeBox(spectralModeBox_);
    populateTriggerBox     (triggerModeBox_);
    populateQuantizeBox    (quantizeScaleBox_);
}

void ModuleTabPanel::init(juce::AudioProcessorValueTreeState& apvts)
{
    buildEngineTab  (apvts);
    buildMotionTab  (apvts);
    buildColorTab   (apvts);
    buildPatternTab (apvts);

    lfoPanel_.init(apvts);
    addAndMakeVisible(lfoPanel_);
    lfoPanel_.setVisible(false);

    modMatrixView_.init(apvts);
    addAndMakeVisible(modMatrixView_);
    modMatrixView_.setVisible(false);

    showTab(0);
}

void ModuleTabPanel::buildEngineTab(juce::AudioProcessorValueTreeState& apvts)
{
    juce::Component* engineControls[] = {
        &subGrainDepthBox_,  &subGrainLabel_,
        &stochasticDistBox_, &stochasticLabel_,
        &feedbackEnableBtn_,
        &feedbackGainKnob_,  &feedbackDampKnob_,
        &spectralEnableBtn_, &spectralModeBox_,  &spectralModeLabel_,
        &spectralBlurKnob_
    };
    for (auto* c : engineControls) { addAndMakeVisible(c); c->setVisible(false); }

    subGrainAttach_       = std::make_unique<CA>(apvts, ParamIDs::subGrainDepth,     subGrainDepthBox_);
    stochasticAttach_     = std::make_unique<CA>(apvts, ParamIDs::stochasticDist,    stochasticDistBox_);
    feedbackEnableAttach_ = std::make_unique<BA>(apvts, ParamIDs::feedbackEnabled,   feedbackEnableBtn_);
    feedbackGainAttach_   = std::make_unique<SA>(apvts, ParamIDs::feedbackGain,      feedbackGainKnob_.getSlider());
    feedbackDampAttach_   = std::make_unique<SA>(apvts, ParamIDs::feedbackDamp,      feedbackDampKnob_.getSlider());
    spectralEnableAttach_ = std::make_unique<BA>(apvts, ParamIDs::spectralEnabled,   spectralEnableBtn_);
    spectralModeAttach_   = std::make_unique<CA>(apvts, ParamIDs::spectralMode,      spectralModeBox_);
    spectralBlurAttach_   = std::make_unique<SA>(apvts, ParamIDs::spectralBlurAmount, spectralBlurKnob_.getSlider());
}

void ModuleTabPanel::buildMotionTab(juce::AudioProcessorValueTreeState& apvts)
{
    juce::Component* motionControls[] = {
        &motionEnableBtn_,
        &wowDepthKnob_,    &wowRateKnob_,
        &flutterDepthKnob_,&flutterRateKnob_,
        &driftAmountKnob_,
        &crackleLevelKnob_,&crackleColorKnob_
    };
    for (auto* c : motionControls) { addAndMakeVisible(c); c->setVisible(false); }

    motionEnableAttach_ = std::make_unique<BA>(apvts, ParamIDs::motionEnabled,  motionEnableBtn_);
    wowDepthAttach_     = std::make_unique<SA>(apvts, ParamIDs::wowDepth,       wowDepthKnob_.getSlider());
    wowRateAttach_      = std::make_unique<SA>(apvts, ParamIDs::wowRate,        wowRateKnob_.getSlider());
    flutterDepthAttach_ = std::make_unique<SA>(apvts, ParamIDs::flutterDepth,   flutterDepthKnob_.getSlider());
    flutterRateAttach_  = std::make_unique<SA>(apvts, ParamIDs::flutterRate,    flutterRateKnob_.getSlider());
    driftAmountAttach_  = std::make_unique<SA>(apvts, ParamIDs::driftAmount,    driftAmountKnob_.getSlider());
    crackleLevelAttach_ = std::make_unique<SA>(apvts, ParamIDs::crackleLevel,   crackleLevelKnob_.getSlider());
    crackleColorAttach_ = std::make_unique<SA>(apvts, ParamIDs::crackleColor,   crackleColorKnob_.getSlider());
}

void ModuleTabPanel::buildColorTab(juce::AudioProcessorValueTreeState& apvts)
{
    juce::Component* colorControls[] = {
        &colorEnableBtn_,
        &saturateKnob_, &decimateKnob_,
        &tiltEqKnob_,   &verbMixKnob_
    };
    for (auto* c : colorControls) { addAndMakeVisible(c); c->setVisible(false); }

    colorEnableAttach_ = std::make_unique<BA>(apvts, ParamIDs::colorEnabled, colorEnableBtn_);
    saturateAttach_    = std::make_unique<SA>(apvts, ParamIDs::saturate,     saturateKnob_.getSlider());
    decimateAttach_    = std::make_unique<SA>(apvts, ParamIDs::decimate,     decimateKnob_.getSlider());
    tiltEqAttach_      = std::make_unique<SA>(apvts, ParamIDs::tiltEq,       tiltEqKnob_.getSlider());
    verbMixAttach_     = std::make_unique<SA>(apvts, ParamIDs::verbMix,      verbMixKnob_.getSlider());
}

void ModuleTabPanel::buildPatternTab(juce::AudioProcessorValueTreeState& apvts)
{
    juce::Component* patternControls[] = {
        &patternEnableBtn_,
        &triggerModeBox_,  &triggerModeLabel_,
        &quantizeScaleBox_,&quantizeLabel_,
        &probabilityKnob_, &reverseProbKnob_,  &sprayKnob_,
        &euclidPulsesKnob_,&euclidStepsKnob_,  &euclidRotKnob_
    };
    for (auto* c : patternControls) { addAndMakeVisible(c); c->setVisible(false); }

    patternEnableAttach_ = std::make_unique<BA>(apvts, ParamIDs::patternEnabled,  patternEnableBtn_);
    triggerModeAttach_   = std::make_unique<CA>(apvts, ParamIDs::triggerMode,     triggerModeBox_);
    quantizeScaleAttach_ = std::make_unique<CA>(apvts, ParamIDs::quantizeScale,   quantizeScaleBox_);
    probabilityAttach_   = std::make_unique<SA>(apvts, ParamIDs::probability,     probabilityKnob_.getSlider());
    reverseProbAttach_   = std::make_unique<SA>(apvts, ParamIDs::reverseProb,     reverseProbKnob_.getSlider());
    sprayAttach_         = std::make_unique<SA>(apvts, ParamIDs::spray,           sprayKnob_.getSlider());
    euclidPulsesAttach_  = std::make_unique<SA>(apvts, ParamIDs::euclidPulses,    euclidPulsesKnob_.getSlider());
    euclidStepsAttach_   = std::make_unique<SA>(apvts, ParamIDs::euclidSteps,     euclidStepsKnob_.getSlider());
    euclidRotAttach_     = std::make_unique<SA>(apvts, ParamIDs::euclidRotation,  euclidRotKnob_.getSlider());
}

void ModuleTabPanel::showTab(int idx)
{
    activeTab_ = idx;

    for (int i = 0; i < kTabCount; ++i)
        tabBtns_[i].setToggleState(i == idx, juce::dontSendNotification);

    juce::Component* engineControls[] = {
        &subGrainDepthBox_,  &subGrainLabel_,
        &stochasticDistBox_, &stochasticLabel_,
        &feedbackEnableBtn_, &feedbackGainKnob_, &feedbackDampKnob_,
        &spectralEnableBtn_, &spectralModeBox_,  &spectralModeLabel_,
        &spectralBlurKnob_
    };
    for (auto* c : engineControls) c->setVisible(idx == 0);

    juce::Component* motionControls[] = {
        &motionEnableBtn_,
        &wowDepthKnob_,    &wowRateKnob_,
        &flutterDepthKnob_,&flutterRateKnob_,
        &driftAmountKnob_,
        &crackleLevelKnob_,&crackleColorKnob_
    };
    for (auto* c : motionControls) c->setVisible(idx == 1);

    juce::Component* colorControls[] = {
        &colorEnableBtn_,
        &saturateKnob_, &decimateKnob_,
        &tiltEqKnob_,   &verbMixKnob_
    };
    for (auto* c : colorControls) c->setVisible(idx == 2);

    juce::Component* patternControls[] = {
        &patternEnableBtn_,
        &triggerModeBox_,  &triggerModeLabel_,
        &quantizeScaleBox_,&quantizeLabel_,
        &probabilityKnob_, &reverseProbKnob_,  &sprayKnob_,
        &euclidPulsesKnob_,&euclidStepsKnob_,  &euclidRotKnob_
    };
    for (auto* c : patternControls) c->setVisible(idx == 3);

    lfoPanel_.setVisible     (idx == 4);
    modMatrixView_.setVisible(idx == 5);

    resized();
    repaint();
}

void ModuleTabPanel::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds();

    g.setColour(kSurface);
    g.fillRect(b);

    // Top border
    g.setColour(kBorder);
    g.drawLine(0.f, 0.f, (float)b.getWidth(), 0.f, 1.f);

    // Active tab mint accent — 2px line at bottom of tab strip
    const int tabW = b.getWidth() / kTabCount;
    g.setColour(kMint);
    g.fillRect(activeTab_ * tabW, kTabBarH - 2, tabW, 2);
}

void ModuleTabPanel::resized()
{
    const int w        = getWidth();
    const int tabW     = w / kTabCount;
    const int contentY = kTabBarH;
    const int contentH = getHeight() - kTabBarH;
    constexpr int pad  = 8;

    for (int i = 0; i < kTabCount; ++i)
        tabBtns_[i].setBounds(i * tabW, 0, tabW, kTabBarH);

    const juce::Rectangle<int> content { pad, contentY + pad,
                                          w - 2 * pad, contentH - 2 * pad };

    if (activeTab_ == 0)  // ENGINE — 3-column layout
    {
        constexpr int comboH = 22;
        constexpr int labelH = 14;
        constexpr int knobH  = 80;
        constexpr int gap    = 8;
        const int cw   = content.getWidth();
        const int colW = cw / 3 - pad;   // usable width per column
        const int cx   = content.getX();
        const int cy   = content.getY();
        const int c1   = cx;
        const int c2   = cx + cw / 3;
        const int c3   = cx + cw * 2 / 3;

        // Column 1: Selectors (sub-grain depth + stochastic distribution)
        subGrainLabel_.setBounds   (c1, cy,                       colW, labelH);
        subGrainDepthBox_.setBounds(c1, cy + labelH,              colW, comboH);
        const int stocY = cy + labelH + comboH + gap;
        stochasticLabel_.setBounds   (c1, stocY,          colW, labelH);
        stochasticDistBox_.setBounds (c1, stocY + labelH, colW, comboH);

        // Column 2: Feedback (toggle + gain/damp knobs side by side)
        feedbackEnableBtn_.setBounds(c2, cy, colW, comboH);
        const int halfCol = colW / 2;
        feedbackGainKnob_.setBounds(c2,            cy + comboH + gap, halfCol,          knobH);
        feedbackDampKnob_.setBounds(c2 + halfCol,  cy + comboH + gap, colW - halfCol,   knobH);

        // Column 3: Spectral (toggle + mode combo + blur knob stacked)
        spectralEnableBtn_.setBounds(c3, cy,                       colW, comboH);
        spectralModeLabel_.setBounds(c3, cy + comboH + gap,        colW, labelH);
        spectralModeBox_.setBounds  (c3, cy + comboH + gap + labelH, colW, comboH);
        const int blurY = cy + comboH + gap + labelH + comboH + gap;
        spectralBlurKnob_.setBounds (c3, blurY, colW, knobH);
    }
    else if (activeTab_ == 1)  // MOTION
    {
        const int cx    = content.getX();
        const int cy    = content.getY();
        motionEnableBtn_.setBounds(cx, cy, 100, 22);

        // Two rows with gap between them
        const int availH = content.getHeight() - 22 - pad;
        const int knobH  = (availH - pad) / 2;
        const int knobW  = content.getWidth() / 4;
        const int row1Y  = cy + 22 + pad;
        const int row2Y  = row1Y + knobH + pad;

        wowDepthKnob_.setBounds    (cx + knobW * 0, row1Y, knobW, knobH);
        wowRateKnob_.setBounds     (cx + knobW * 1, row1Y, knobW, knobH);
        flutterDepthKnob_.setBounds(cx + knobW * 2, row1Y, knobW, knobH);
        flutterRateKnob_.setBounds (cx + knobW * 3, row1Y, knobW, knobH);

        driftAmountKnob_.setBounds (cx + knobW * 0, row2Y, knobW, knobH);
        crackleLevelKnob_.setBounds(cx + knobW * 1, row2Y, knobW, knobH);
        crackleColorKnob_.setBounds(cx + knobW * 2, row2Y, knobW, knobH);
    }
    else if (activeTab_ == 2)  // COLOR
    {
        const int cx    = content.getX();
        const int cy    = content.getY();
        colorEnableBtn_.setBounds(cx, cy, 100, 22);

        const int knobH = content.getHeight() - 22 - pad;
        const int knobW = content.getWidth() / 4;
        const int ky    = cy + 22 + pad;

        saturateKnob_.setBounds(cx + knobW * 0, ky, knobW, knobH);
        decimateKnob_.setBounds(cx + knobW * 1, ky, knobW, knobH);
        tiltEqKnob_.setBounds  (cx + knobW * 2, ky, knobW, knobH);
        verbMixKnob_.setBounds (cx + knobW * 3, ky, knobW, knobH);
    }
    else if (activeTab_ == 3)  // PATTERN
    {
        constexpr int comboH = 22;
        constexpr int labelH = 14;
        const int cx  = content.getX();
        const int cy  = content.getY();
        const int cw  = content.getWidth();
        patternEnableBtn_.setBounds(cx, cy, 120, 22);

        const int cmbY  = cy + 22 + pad;
        const int knobH = content.getHeight() - 22 - pad - comboH - pad;
        const int knobW = cw / 6;

        triggerModeLabel_.setBounds (cx,           cmbY,           cw / 4,       labelH);
        triggerModeBox_.setBounds   (cx,           cmbY + labelH,  cw / 4 - pad, comboH);
        quantizeLabel_.setBounds    (cx + cw / 4,  cmbY,           cw / 4,       labelH);
        quantizeScaleBox_.setBounds (cx + cw / 4,  cmbY + labelH,  cw / 4 - pad, comboH);

        const int knobY = cmbY + comboH + pad;
        probabilityKnob_.setBounds  (cx + knobW * 0, knobY, knobW, knobH);
        reverseProbKnob_.setBounds  (cx + knobW * 1, knobY, knobW, knobH);
        sprayKnob_.setBounds        (cx + knobW * 2, knobY, knobW, knobH);
        euclidPulsesKnob_.setBounds (cx + knobW * 3, knobY, knobW, knobH);
        euclidStepsKnob_.setBounds  (cx + knobW * 4, knobY, knobW, knobH);
        euclidRotKnob_.setBounds    (cx + knobW * 5, knobY, knobW, knobH);
    }
    else if (activeTab_ == 4)  // LFO
    {
        lfoPanel_.setBounds(content);
    }
    else if (activeTab_ == 5)  // MOD
    {
        modMatrixView_.setBounds(content);
    }
}
