#include "CurveEditor.h"

const char* CurveEditor::shapeLabels_[CurveEditor::kNumShapes] = {
    "Hann", "Tukey", "Gauss", "Tri", "Rect"
};

CurveEditor::CurveEditor()
{
    startTimerHz(30);
}

CurveEditor::~CurveEditor()
{
    stopTimer();
}

void CurveEditor::resized()
{
    const int w = getWidth() / kNumShapes;
    for (int i = 0; i < kNumShapes; ++i)
        thumbnailBounds_[i] = juce::Rectangle<int>(i * w, 0, w, getHeight());
}

void CurveEditor::paint(juce::Graphics& g)
{
    int selectedShape = 0;
    if (apvts_ != nullptr)
    {
        auto* p = apvts_->getRawParameterValue(ParamIDs::envelopeShape);
        if (p != nullptr)
            selectedShape = (int)std::clamp(p->load(std::memory_order_relaxed), 0.0f, 4.0f);
    }

    g.fillAll(juce::Colour(0xff1a1a2e));

    for (int i = 0; i < kNumShapes; ++i)
    {
        const bool selected = (i == selectedShape);
        const auto bounds = thumbnailBounds_[i].toFloat();

        if (selected)
            g.setColour(juce::Colour(0xff002e22));
        else
            g.setColour(juce::Colour(0xff111120));
        g.fillRect(bounds);

        drawEnvelopeCurve(g, static_cast<EnvelopeShape>(i),
                          bounds.reduced(4.0f, 6.0f), selected);

        g.setColour(selected ? juce::Colour(0xff62ffc8) : juce::Colour(0xff444466));
        g.setFont(10.0f);
        g.drawText(shapeLabels_[i],
                   thumbnailBounds_[i].removeFromBottom(14).toFloat(),
                   juce::Justification::centred);

        if (selected)
        {
            g.setColour(juce::Colour(0xff62ffc8));
            g.drawRect(bounds, 1.5f);
        }
        else
        {
            g.setColour(juce::Colour(0xff2a2a44));
            g.drawRect(bounds, 1.0f);
        }
    }
}

void CurveEditor::mouseDown(const juce::MouseEvent& e)
{
    if (apvts_ == nullptr)
        return;

    auto* param = apvts_->getParameter(ParamIDs::envelopeShape);
    if (param == nullptr)
        return;

    for (int i = 0; i < kNumShapes; ++i)
    {
        if (thumbnailBounds_[i].contains(e.getPosition()))
        {
            param->setValueNotifyingHost(param->convertTo0to1((float)i));
            break;
        }
    }
}

void CurveEditor::drawEnvelopeCurve(juce::Graphics& g, EnvelopeShape shape,
                                    juce::Rectangle<float> bounds, bool selected)
{
    constexpr int kPoints = 64;
    juce::Path path;

    const float x0 = bounds.getX();
    const float y0 = bounds.getBottom();
    const float w  = bounds.getWidth();
    const float h  = bounds.getHeight();

    auto shapeToY = [&](float t) -> float
    {
        // t in [0,1]; returns amplitude in [0,1]
        float amp = 0.0f;
        switch (shape)
        {
            case EnvelopeShape::Hann:
                amp = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * t));
                break;
            case EnvelopeShape::Tukey:
            {
                const float alpha = 0.5f;
                if (t < alpha / 2.0f)
                    amp = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * t / alpha));
                else if (t > 1.0f - alpha / 2.0f)
                    amp = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (1.0f - t) / alpha));
                else
                    amp = 1.0f;
                break;
            }
            case EnvelopeShape::Gaussian:
            {
                const float sigma = 0.4f;
                const float x = (t - 0.5f) / sigma;
                amp = std::exp(-0.5f * x * x);
                break;
            }
            case EnvelopeShape::Triangle:
                amp = 1.0f - std::abs(2.0f * t - 1.0f);
                break;
            case EnvelopeShape::Rectangle:
                amp = (t > 0.0f && t < 1.0f) ? 1.0f : 0.0f;
                break;
        }
        return amp;
    };

    path.startNewSubPath(x0, y0);
    for (int i = 0; i <= kPoints; ++i)
    {
        const float t = (float)i / (float)kPoints;
        const float amp = shapeToY(t);
        path.lineTo(x0 + t * w, y0 - amp * h);
    }
    path.lineTo(x0 + w, y0);

    g.setColour(selected ? juce::Colour(0x4062ffc8) : juce::Colour(0x20888888));
    g.fillPath(path);

    g.setColour(selected ? juce::Colour(0xff62ffc8) : juce::Colour(0xff666688));
    g.strokePath(path, juce::PathStrokeType(1.5f));
}
