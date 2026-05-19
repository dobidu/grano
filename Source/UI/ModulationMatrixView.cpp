#include "ModulationMatrixView.h"
#include "../Parameters.h"

static const char* kDestNames[] = {
    "None",          "Grain Size",    "Density",       "Position",
    "Pos Jitter",    "Pitch Shift",   "Stereo Spread", "Master Vol",
    "Wow Depth",     "Wow Rate",      "Flutter Depth", "Flutter Rate",
    "Drift",         "Crackle Lvl",   "Crackle Color", "Saturate",
    "Decimate",      "Tilt EQ",       "Verb Mix",      "Probability",
    "Reverse Prob",  "Spray",         "Transient Sens","LFO1 Rate",
    "LFO1 Phase",    "LFO1 Depth",    "LFO2 Rate",     "LFO2 Phase",
    "LFO2 Depth"
};
static constexpr int kNumDests = 29;

void ModulationMatrixView::init(juce::AudioProcessorValueTreeState& apvts)
{
    for (int i = 0; i < kNumSlots; ++i)
    {
        auto& row = rows_[i];

        row.indexLabel.setText(juce::String(i + 1) + ":", juce::dontSendNotification);
        row.indexLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
        row.indexLabel.setJustificationType(juce::Justification::centredRight);

        row.sourceBox.addItem("None", 1);
        row.sourceBox.addItem("LFO1", 2);
        row.sourceBox.addItem("LFO2", 3);

        for (int d = 0; d < kNumDests; ++d)
            row.destBox.addItem(kDestNames[d], d + 1);

        row.amountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        row.amountSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 18);

        addAndMakeVisible(row.indexLabel);
        addAndMakeVisible(row.sourceBox);
        addAndMakeVisible(row.destBox);
        addAndMakeVisible(row.amountSlider);

        const juce::String srcID = slotParamID(i + 1, "Source");
        const juce::String dstID = slotParamID(i + 1, "Dest");
        const juce::String amtID = slotParamID(i + 1, "Amount");

        row.sourceAttach = std::make_unique<ComboAttach>(apvts, srcID, row.sourceBox);
        row.destAttach   = std::make_unique<ComboAttach>(apvts, dstID, row.destBox);
        row.amountAttach = std::make_unique<SliderAttach>(apvts, amtID, row.amountSlider);
    }
}

void ModulationMatrixView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1d24));
    g.setColour(juce::Colour(0xff2e3340));
    g.drawRect(getLocalBounds(), 1);

    g.setColour(juce::Colour(0xffff6b9d));
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    g.drawText("MOD MATRIX", 4, 2, 100, 12, juce::Justification::centredLeft, false);

    g.setColour(juce::Colour(0xff8090a0));
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f)));
    constexpr int hY = 16;
    g.drawText("SRC",    26,  hY, 60, 12, juce::Justification::centred, false);
    g.drawText("DEST",   88,  hY, 90, 12, juce::Justification::centred, false);
    g.drawText("AMOUNT", 180, hY, 70, 12, juce::Justification::centred, false);
}

void ModulationMatrixView::resized()
{
    const int w    = getWidth();
    const int pad  = 4;
    const int rowH = 22;
    const int gap  = 2;

    int y = 30; // header area (title + column labels)

    for (int i = 0; i < kNumSlots; ++i)
    {
        auto& row = rows_[i];

        row.indexLabel  .setBounds(pad,       y, 20,             rowH);
        row.sourceBox   .setBounds(pad + 22,  y, 60,             rowH);
        row.destBox     .setBounds(pad + 84,  y, 90,             rowH);
        row.amountSlider.setBounds(pad + 176, y, w - 180 - pad,  rowH);

        y += rowH + gap;
    }
}
