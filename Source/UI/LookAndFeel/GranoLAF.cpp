#include "GranoLAF.h"

const juce::Colour GranoLAF::kSurfaceBase { 0xff0a0b0du };
const juce::Colour GranoLAF::kSurface1   { 0xff1a1d22u };
const juce::Colour GranoLAF::kBorderMuted{ 0xff2a2e36u };
const juce::Colour GranoLAF::kTextPrimary{ 0xffe8e6e1u };
const juce::Colour GranoLAF::kTextSecond { 0xff8b8985u };
const juce::Colour GranoLAF::kVital      { 0xff7cf5c4u };
const juce::Colour GranoLAF::kGrain      { 0xffc77cffu };
const juce::Colour GranoLAF::kClip       { 0xffff5252u };

GranoLAF::GranoLAF()
{
    setColour(juce::ResizableWindow::backgroundColourId,    kSurfaceBase);
    setColour(juce::Slider::rotarySliderFillColourId,       kVital);
    setColour(juce::Slider::rotarySliderOutlineColourId,    kBorderMuted);
    setColour(juce::Slider::trackColourId,                  kVital);
    setColour(juce::Slider::thumbColourId,                  kVital);
    setColour(juce::Label::textColourId,                    kTextSecond);
    setColour(juce::TextButton::buttonColourId,             kSurface1);
    setColour(juce::TextButton::buttonOnColourId,           kVital.withAlpha(0.15f));
    setColour(juce::TextButton::textColourOffId,            kTextSecond);
    setColour(juce::TextButton::textColourOnId,             kVital);
    setColour(juce::ToggleButton::textColourId,             kTextSecond);
    setColour(juce::ToggleButton::tickColourId,             kVital);
    setColour(juce::ToggleButton::tickDisabledColourId,     kBorderMuted);
}

void GranoLAF::drawRotarySlider(juce::Graphics& g,
                                 int x, int y, int width, int height,
                                 float sliderPos,
                                 float startAngle, float endAngle,
                                 juce::Slider& /*slider*/)
{
    const float radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
    const float cx     = (float)x + (float)width  * 0.5f;
    const float cy     = (float)y + (float)height * 0.5f;
    const float rr     = radius * 2.0f;
    const float rx     = cx - radius;
    const float ry     = cy - radius;
    const float angle  = startAngle + sliderPos * (endAngle - startAngle);

    // Track arc (grey)
    {
        juce::Path arc;
        arc.addArc(rx, ry, rr, rr, startAngle, endAngle, true);
        g.setColour(kBorderMuted);
        g.strokePath(arc, juce::PathStrokeType(2.0f));
    }
    // Halo passes — wider arcs at low alpha before solid arc renders on top
    {
        const float halos[][2] = { {6.0f, 0.04f}, {4.5f, 0.09f}, {3.0f, 0.18f} };
        for (auto& [hw, ha] : halos)
        {
            juce::Path halo;
            halo.addArc(rx, ry, rr, rr, startAngle, angle, true);
            g.setColour(kVital.withAlpha(ha));
            g.strokePath(halo, juce::PathStrokeType(hw,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
    // Value arc (mint)
    {
        juce::Path arc;
        arc.addArc(rx, ry, rr, rr, startAngle, angle, true);
        g.setColour(kVital);
        g.strokePath(arc, juce::PathStrokeType(
            2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    // Knob body
    const float ir = radius * 0.68f;
    g.setColour(kSurface1);
    g.fillEllipse(cx - ir, cy - ir, ir * 2.0f, ir * 2.0f);
    g.setColour(kBorderMuted);
    g.drawEllipse(cx - ir, cy - ir, ir * 2.0f, ir * 2.0f, 1.0f);
    // Needle
    const float nl = ir * 0.65f;
    g.setColour(kVital);
    g.drawLine(cx, cy, cx + nl * std::sin(angle), cy - nl * std::cos(angle), 1.5f);
}

void GranoLAF::drawLinearSlider(juce::Graphics& g,
                                  int x, int y, int width, int height,
                                  float sliderPos, float /*minPos*/, float /*maxPos*/,
                                  juce::Slider::SliderStyle style,
                                  juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height,
                                          sliderPos, 0.0f, 0.0f, style, slider);
        return;
    }
    const float trackY = (float)y + (float)height * 0.5f;
    const float thick  = 2.0f;
    const float thumbR = 5.0f;

    g.setColour(kBorderMuted);
    g.fillRoundedRectangle((float)x, trackY - thick * 0.5f,
                            (float)width, thick, 1.0f);

    const float fillW = sliderPos - (float)x;
    if (fillW > 0.0f)
    {
        g.setColour(kVital);
        g.fillRoundedRectangle((float)x, trackY - thick * 0.5f, fillW, thick, 1.0f);
    }

    g.setColour(kVital);
    g.fillEllipse(sliderPos - thumbR, trackY - thumbR, thumbR * 2.0f, thumbR * 2.0f);
    g.setColour(kSurfaceBase);
    g.drawEllipse(sliderPos - thumbR, trackY - thumbR, thumbR * 2.0f, thumbR * 2.0f, 1.0f);
}

void GranoLAF::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                     const juce::Colour& /*bg*/,
                                     bool isMouseOver, bool isDown)
{
    const auto b = button.getLocalBounds().toFloat().reduced(0.5f);
    juce::Colour fill = kSurface1;
    if (button.getToggleState())  fill = kVital.withAlpha(0.12f);
    else if (isDown)              fill = kSurface1.brighter(0.15f);
    else if (isMouseOver)         fill = kSurface1.brighter(0.06f);

    g.setColour(fill);
    g.fillRoundedRectangle(b, 4.0f);

    const juce::Colour border = button.getToggleState() ? kVital : kBorderMuted;
    g.setColour(border);
    g.drawRoundedRectangle(b, 4.0f, 1.0f);

    if (button.getToggleState())
    {
        g.setColour(kVital.withAlpha(0.40f));
        g.drawRoundedRectangle(b.expanded(1.0f), 4.5f, 1.5f);
    }
}

void GranoLAF::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                               bool /*isMouseOver*/, bool /*isDown*/)
{
    g.setColour(button.getToggleState() ? kVital : kTextSecond);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
    g.drawText(button.getButtonText(), button.getLocalBounds(),
               juce::Justification::centred, true);
}

juce::Font GranoLAF::getLabelFont(juce::Label& /*label*/)
{
    return juce::Font(juce::FontOptions{}.withHeight(10.0f));
}
