#include "Monitor.h"
#include "Console.h"
#include "../Parameters.h"

using namespace Theme;

namespace
{
    constexpr float kTop = 30.0f, kBot = 92.0f, kX0 = 14.0f; // at the 118 px base height
}

Monitor::Monitor(juce::AudioFormatManager& fm, juce::AudioProcessorValueTreeState& apvts, GranularEngine& engine)
    : thumb_(512, fm, thumbCache_),
      engine_(engine),
      positionParam_(*apvts.getParameter(ParamIDs::position)),
      positionAttach_(positionParam_, [this](float v) { position_ = v; repaint(); }, nullptr),
      grainSize_(apvts.getRawParameterValue(ParamIDs::grainSize)),
      jitter_   (apvts.getRawParameterValue(ParamIDs::positionJitter)),
      loop_     (apvts.getRawParameterValue(ParamIDs::loop)),
      pitch_    (apvts.getRawParameterValue(ParamIDs::pitchShift)),
      noSourceKeys_(*apvts.getParameter(ParamIDs::noSourceMode),
                    juce::StringArray{ "Test tone", "Silence" }, C::tape, 2)
{
    noSourceKeys_.setGap(4);
    noSourceKeys_.setVisible(false);
    addChildComponent(noSourceKeys_);
    setTitle("Monitor");
    setDescription("Waveform of the selected slot. Click or drag to set Position.");
    setTooltip("Click or drag on the waveform to set Position. Drop an audio file to load it into the selected slot.");
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
    setOpaque(true);
    positionAttach_.sendInitialUpdate();
}

Monitor::~Monitor() = default;

void Monitor::setSample(const juce::File& f, double sr, int frames, int channels)
{
    if (frames <= 0 || f == juce::File())
    {
        name_ = {};
        thumb_.clear();
        sampleRate_ = 0.0;
        numFrames_ = numChannels_ = 0;
    }
    else if (f.getFullPathName() != name_ || frames != numFrames_)
    {
        name_ = f.getFullPathName();
        sampleRate_ = sr;
        numFrames_ = frames;
        numChannels_ = channels;
        thumb_.setSource(new juce::FileInputSource(f));
    }
    repaint();
}

void Monitor::setDragHighlight(bool b)
{
    if (b != dragHighlight_) { dragHighlight_ = b; repaint(); }
}

void Monitor::tick()
{
    grainCount_ = engine_.getGrainSnapshots(snaps_.data(), GranularEngine::MaxActiveGrains);
    const bool noSource = engine_.isNoSourceActive();
    if (noSource != noSource_)
    {
        noSource_ = noSource;
        noSourceKeys_.setVisible(noSource);
    }
    repaint();
}

void Monitor::resized()
{
    // Bottom centre, where the grain count sits; label drawn to its left.
    const int w = 196, h = 22;
    noSourceKeys_.setBounds(getWidth() / 2 - w / 2 + 52, getHeight() - h - 5, w, h);
}

juce::Rectangle<float> Monitor::band() const
{
    return { kX0, kTop, (float)getWidth() - 2.0f * kX0, kBot - kTop };
}

void Monitor::setPositionFromX(float x)
{
    const auto b = band();
    positionAttach_.setValueAsPartOfGesture(juce::jlimit(0.0f, 1.0f, (x - b.getX()) / b.getWidth()));
    Console::notifyChanged(*this, positionParam_);
}

void Monitor::mouseDown(const juce::MouseEvent& e)
{
    if (numFrames_ <= 0 || !e.mods.isLeftButtonDown())
        return;
    dragging_ = true;
    Console::notifyTouched(*this, positionParam_, position_);
    positionAttach_.beginGesture();
    setPositionFromX(e.position.x);
}

void Monitor::mouseDrag(const juce::MouseEvent& e)
{
    if (dragging_) setPositionFromX(e.position.x);
}

void Monitor::mouseUp(const juce::MouseEvent&)
{
    if (!dragging_) return;
    dragging_ = false;
    positionAttach_.endGesture();
}

void Monitor::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float W = b.getWidth();
    g.setColour(C::well);
    g.fillRoundedRectangle(b, kPlateRadius);

    // Grid
    g.setColour(C::grid);
    for (int i = 1; i < 10; ++i)
        g.fillRect(std::round((float)i * W / 10.0f), kTop - 6.0f, 1.0f, kBot - kTop + 12.0f);

    const auto band = this->band();
    const auto lcdFont = font(Face::lcd, 13.0f);

    if (numFrames_ > 0 && sampleRate_ > 0.0)
    {
        const double dur = (double)numFrames_ / sampleRate_;
        const bool loopOn = loop_->load(std::memory_order_relaxed) > 0.5f;
        float pos = position_;
        if (loopOn)
            pos = std::fmod(pos + engine_.getLoopSweepFraction(), 1.0f);
        const float jit = jitter_->load(std::memory_order_relaxed);
        const float sizeFrac = (float)((grainSize_->load(std::memory_order_relaxed) / 1000.0) / dur);

        // Jitter span + grain-size shadow
        g.setColour(C::tape.withAlpha(0.06f));
        g.fillRect(band.getX() + juce::jmax(0.0f, pos - jit * 0.5f) * band.getWidth(), kTop - 6.0f,
                   juce::jmin(1.0f, jit) * band.getWidth(), kBot - kTop + 12.0f);
        g.setColour(C::lcd.withAlpha(0.10f));
        g.fillRect(band.getX() + pos * band.getWidth(), kTop - 6.0f,
                   juce::jmax(2.0f, sizeFrac * band.getWidth()), kBot - kTop + 12.0f);

        // Waveform (first channel; the engine plays a mono downmix)
        if (thumb_.getTotalLength() > 0.0)
        {
            g.setColour(C::waveform);
            thumb_.drawChannel(g, band.toNearestInt(), 0.0, thumb_.getTotalLength(), 0, 1.0f);
        }

        // Grains: read-extent trace + head, warmer and larger with envelope.
        const float pitchScale = 1.0f + pitch_->load(std::memory_order_relaxed) / 24.0f;
        const float span = juce::jlimit(1.5f, 60.0f, sizeFrac * band.getWidth() * pitchScale);
        for (int i = 0; i < grainCount_; ++i)
        {
            const auto& s = snaps_[(size_t)i];
            const float a = juce::jlimit(0.0f, 1.0f, s.envelopeAmp);
            const float x = band.getX() + s.srcFraction * band.getWidth();
            const float y = band.getY() + (0.125f + 0.75f * s.yFraction) * band.getHeight();
            const auto col = C::grainLo.interpolatedWith(C::grainHi, a);
            g.setColour(col.withAlpha(a * 0.45f));
            g.fillRect(x, y - 1.5f, span * a, 3.0f);
            const float r = 2.0f + a * 2.6f;
            g.setColour(col.withAlpha(0.2f + 0.8f * a));
            g.fillEllipse(x + span * a - r, y - r, r * 2.0f, r * 2.0f);
        }

        // Playhead
        const float phx = band.getX() + pos * band.getWidth();
        g.setColour(C::live);
        g.fillRect(std::round(phx), kTop - 8.0f, 1.5f, kBot - kTop + 16.0f);
        g.fillEllipse(phx + 0.75f - 3.0f, kTop - 13.0f, 6.0f, 6.0f);
        if (loopOn)
        {
            g.setColour(C::lcd);
            g.setFont(font(Face::silkBold, 13.0f, 0.1f));
            g.drawText("LOOP", juce::Rectangle<float>(juce::jmin(W - 50.0f, phx + 7.0f), kTop - 18.0f, 44.0f, 14.0f),
                       juce::Justification::centredLeft, false);
        }

        // Labels
        const auto file = juce::File(name_);
        g.setColour(C::silk);
        g.setFont(font(Face::silk, 14.0f, 0.04f));
        g.drawText(file.getFileName(), juce::Rectangle<float>(24.0f, 6.0f, W * 0.5f, 16.0f),
                   juce::Justification::centredLeft, true);
        const auto meta = juce::String(dur, 2) + " s  " + juce::String::fromUTF8("\xc2\xb7")
                        + "  " + juce::String(sampleRate_ / 1000.0, sampleRate_ == 44100.0 ? 1 : 0) + " kHz  "
                        + juce::String::fromUTF8("\xc2\xb7") + "  "
                        + (numChannels_ == 1 ? "mono" : numChannels_ == 2 ? "stereo" : juce::String(numChannels_) + " ch");
        g.setColour(C::silkDim);
        g.setFont(lcdFont);
        g.drawText(meta, juce::Rectangle<float>(W * 0.5f, 6.0f, W * 0.5f - 12.0f, 16.0f), juce::Justification::centredRight, true);
        g.drawText("0.00 s", juce::Rectangle<float>(12.0f, b.getBottom() - 22.0f, 120.0f, 14.0f), juce::Justification::centredLeft, false);
        g.drawText(juce::String(dur, 2) + " s", juce::Rectangle<float>(W - 132.0f, b.getBottom() - 22.0f, 120.0f, 14.0f),
                   juce::Justification::centredRight, false);
        if (!noSource_)
        {
            g.setColour(C::lcd);
            g.drawText(juce::String(grainCount_) + (grainCount_ == 1 ? " grain" : " grains"),
                       juce::Rectangle<float>(W * 0.5f - 80.0f, b.getBottom() - 22.0f, 160.0f, 14.0f), juce::Justification::centred, false);
        }
    }
    else
    {
        g.setColour(C::silkDim);
        g.setFont(font(Face::silkBold, 14.0f, 0.16f));
        g.drawText("DROP A SAMPLE TO START", b, juce::Justification::centred, false);
    }

    if (noSource_)
    {
        // "NO SOURCE" label left of the Test tone / Silence keys.
        const auto k = noSourceKeys_.getBounds().toFloat();
        g.setColour(C::silkDim);
        g.setFont(font(Face::silk, 13.0f, 0.12f));
        g.drawText("NO SOURCE", juce::Rectangle<float>(k.getX() - 104.0f, k.getY(), 96.0f, k.getHeight()),
                   juce::Justification::centredRight, false);
    }

    if (dragHighlight_)
    {
        g.setColour(C::lcd.withAlpha(0.12f));
        g.fillRoundedRectangle(b, kPlateRadius);
        g.setColour(C::lcd);
        g.drawRoundedRectangle(b.reduced(1.0f), kPlateRadius, 2.0f);
    }
}
