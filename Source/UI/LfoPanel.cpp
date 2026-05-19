#include "LfoPanel.h"
#include "../Parameters.h"

LfoPanel::LfoPanel()
{
    for (auto* s : { &rateSlider_, &depthSlider_, &phaseSlider_ })
    {
        s->setSliderStyle(juce::Slider::LinearHorizontal);
        s->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    }

    for (auto* lbl : { &rateLabel_, &depthLabel_, &phaseLabel_ })
    {
        lbl->setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
        lbl->setJustificationType(juce::Justification::centredRight);
    }

    syncButton_.setClickingTogglesState(true);

    addAndMakeVisible(lfo1Tab_);
    addAndMakeVisible(lfo2Tab_);
    addAndMakeVisible(waveformBox_);
    addAndMakeVisible(rateSlider_);
    addAndMakeVisible(depthSlider_);
    addAndMakeVisible(phaseSlider_);
    addAndMakeVisible(syncButton_);
    addAndMakeVisible(rateLabel_);
    addAndMakeVisible(depthLabel_);
    addAndMakeVisible(phaseLabel_);

    rateReadout_.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    rateReadout_.setJustificationType(juce::Justification::centredLeft);
    rateReadout_.setColour(juce::Label::textColourId, juce::Colour(0xff8b8985u));
    addAndMakeVisible(rateReadout_);
}

void LfoPanel::init(juce::AudioProcessorValueTreeState& apvts)
{
    apvts_ = &apvts;

    waveformBox_.addItem("Sine",     1);
    waveformBox_.addItem("Triangle", 2);
    waveformBox_.addItem("Saw",      3);
    waveformBox_.addItem("Square",   4);
    waveformBox_.addItem("Random",   5);
    waveformBox_.addItem("S&H",      6);
    waveformBox_.addItem("Drawable", 7);

    lfo1Tab_.onClick = [this] { showLfo(0); };
    lfo2Tab_.onClick = [this] { showLfo(1); };

    showLfo(0);
}

void LfoPanel::showLfo(int idx)
{
    currentLfo_ = idx;

    waveAttach_.reset();
    rateAttach_.reset();
    depthAttach_.reset();
    phaseAttach_.reset();
    syncAttach_.reset();

    if (apvts_ == nullptr)
        return;

    const char* rateID  = (idx == 0) ? ParamIDs::lfo1Rate     : ParamIDs::lfo2Rate;
    const char* wfID    = (idx == 0) ? ParamIDs::lfo1Waveform : ParamIDs::lfo2Waveform;
    const char* syncID  = (idx == 0) ? ParamIDs::lfo1Sync     : ParamIDs::lfo2Sync;
    const char* phaseID = (idx == 0) ? ParamIDs::lfo1Phase    : ParamIDs::lfo2Phase;
    const char* depthID = (idx == 0) ? ParamIDs::lfo1Depth    : ParamIDs::lfo2Depth;

    waveAttach_  = std::make_unique<ComboAttach> (*apvts_, wfID,    waveformBox_);
    rateAttach_  = std::make_unique<SliderAttach>(*apvts_, rateID,  rateSlider_);
    depthAttach_ = std::make_unique<SliderAttach>(*apvts_, depthID, depthSlider_);
    phaseAttach_ = std::make_unique<SliderAttach>(*apvts_, phaseID, phaseSlider_);
    syncAttach_  = std::make_unique<ButtonAttach>(*apvts_, syncID,  syncButton_);

    rateSlider_.onValueChange = [this] {
        const double hz = rateSlider_.getValue();
        juce::String text;
        if      (hz < 1.0)    text = juce::String(hz, 2) + " Hz";
        else if (hz < 1000.0) text = juce::String(hz, 1) + " Hz";
        else                  text = juce::String(hz / 1000.0, 2) + " kHz";
        rateReadout_.setText(text, juce::dontSendNotification);
    };
    rateSlider_.onValueChange(); // populate immediately on tab switch
}

void LfoPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1d24));
    g.setColour(juce::Colour(0xff2e3340));
    g.drawRect(getLocalBounds(), 1);
    g.setColour(juce::Colour(0xff7cf5c4));
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    g.drawText("LFO", 4, 2, 30, 12, juce::Justification::centredLeft, false);
}

void LfoPanel::resized()
{
    const int w   = getWidth();
    const int h   = getHeight();
    const int pad = 4;

    int y = 16;

    const int tabH = 24;
    const int half = (w - pad * 2) / 2;
    lfo1Tab_.setBounds(pad,          y, half, tabH);
    lfo2Tab_.setBounds(pad + half,   y, half, tabH);
    y += tabH + pad;

    waveformBox_.setBounds(pad, y, w - pad * 2, 22);
    y += 22 + pad;

    const int labelW   = 44;
    const int sliderH  = 22;
    const int sliderW  = w - pad * 2 - labelW;
    const int readoutW = 50;

    rateLabel_  .setBounds(pad,                            y, labelW,                         sliderH);
    rateSlider_ .setBounds(pad + labelW,                   y, sliderW - readoutW - pad,        sliderH);
    rateReadout_.setBounds(w - pad - readoutW,             y, readoutW,                        sliderH);
    y += sliderH + pad;

    depthLabel_.setBounds (pad,           y, labelW,  sliderH);
    depthSlider_.setBounds(pad + labelW,  y, sliderW, sliderH);
    y += sliderH + pad;

    phaseLabel_.setBounds (pad,           y, labelW,  sliderH);
    phaseSlider_.setBounds(pad + labelW,  y, sliderW, sliderH);
    y += sliderH + pad;

    syncButton_.setBounds(w - pad - 60, h - pad - 22, 60, 22);

    juce::ignoreUnused(h);
}
