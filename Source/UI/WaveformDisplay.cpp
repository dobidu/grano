#include "WaveformDisplay.h"

namespace WD {
    const juce::Colour kBg       { 0xff050608u }; // surface.shaft
    const juce::Colour kBorder   { 0xff2a2e36u }; // stroke.default
    const juce::Colour kWaveform { 0xff4a4946u }; // text.tertiary
    const juce::Colour kVital    { 0xff7cf5c4u }; // accent.vital  (playhead)
    const juce::Colour kGrain    { 0xffc77cffu }; // accent.grain  (particles)
    const juce::Colour kTextSec  { 0xff8b8985u }; // text.secondary
    const juce::Colour kTextTert { 0xff4a4946u }; // text.tertiary
    constexpr float kRadius   = 4.0f;
    constexpr float kBorderPx = 1.0f;
    constexpr int   kTimerHz  = 30;
}

WaveformDisplay::WaveformDisplay(juce::AudioFormatManager& fm)
    : formatManager_(fm),
      thumbnail_(512, fm, thumbnailCache_)
{
    startTimerHz(WD::kTimerHz);
}

WaveformDisplay::~WaveformDisplay()
{
    stopTimer();
}

void WaveformDisplay::setFile(const juce::File& file, double sampleRate, int numFrames)
{
    filename_       = file.getFileName();
    fileSampleRate_ = sampleRate;
    fileNumFrames_  = numFrames;
    thumbnail_.setSource(new juce::FileInputSource(file));
    repaint();
}

void WaveformDisplay::setDragHighlightActive(bool active)
{
    dragHighlightActive_ = active;
    repaint();
}

void WaveformDisplay::resized() {}

void WaveformDisplay::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(WD::kBorderPx * 0.5f);

    paintBackground(g, bounds);

    const auto inner = bounds.reduced(8.0f);

    // Collect snapshots once — used by both particles and playhead.
    std::array<GranularEngine::GrainSnapshot, GranularEngine::MaxActiveGrains> snaps;
    int snapCount = 0;
    if (engine_ != nullptr)
        snapCount = engine_->getGrainSnapshots(snaps.data(), GranularEngine::MaxActiveGrains);

    paintWaveform(g, inner);
    paintParticles(g, inner, snaps.data(), snapCount);
    paintPlayhead(g, inner, snaps.data(), snapCount);
    paintLabels(g, inner, snapCount);

    if (dragHighlightActive_)
        paintDragHighlight(g, bounds);
}

void WaveformDisplay::paintBackground(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour(WD::kBg);
    g.fillRoundedRectangle(bounds, WD::kRadius);
    g.setColour(WD::kBorder);
    g.drawRoundedRectangle(bounds, WD::kRadius, WD::kBorderPx);
}

void WaveformDisplay::paintWaveform(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    if (thumbnail_.getTotalLength() > 0.0)
    {
        g.setColour(WD::kWaveform);
        thumbnail_.drawChannels(g, bounds.toNearestInt(), 0.0, thumbnail_.getTotalLength(), 1.0f);
    }
    else
    {
        g.setColour(WD::kTextTert);
        g.setFont(juce::Font(juce::FontOptions{}.withName(juce::Font::getDefaultSansSerifFontName()).withHeight(13.0f)));
        g.drawText("Drop audio file", bounds, juce::Justification::centred, false);
    }
}

void WaveformDisplay::paintParticles(juce::Graphics& g, juce::Rectangle<float> bounds,
                                     const GranularEngine::GrainSnapshot* snaps, int count)
{
    if (count <= 0 || fileNumFrames_ <= 0)
        return;

    for (int i = 0; i < count; ++i)
    {
        const float x = bounds.getX() + snaps[i].srcFraction * bounds.getWidth();
        // Deterministic vertical jitter per grain index, distributed across component height
        const float yFrac = static_cast<float>(std::hash<int>{}(i) % 1000) / 1000.0f;
        const float y = bounds.getY() + yFrac * bounds.getHeight();

        g.setColour(WD::kGrain.withAlpha(snaps[i].envelopeAmp));
        g.fillEllipse(x - 1.5f, y - 1.5f, 3.0f, 3.0f);
    }
}

void WaveformDisplay::paintPlayhead(juce::Graphics& g, juce::Rectangle<float> bounds,
                                    const GranularEngine::GrainSnapshot* snaps, int count)
{
    if (fileNumFrames_ <= 0)
        return;

    float meanFrac = 0.0f;
    if (positionParam_ != nullptr)
    {
        meanFrac = positionParam_->load(std::memory_order_relaxed);
    }
    else if (count > 0)
    {
        for (int i = 0; i < count; ++i)
            meanFrac += snaps[i].srcFraction;
        meanFrac /= static_cast<float>(count);
    }
    else
    {
        return;
    }

    const float x     = bounds.getX() + meanFrac * bounds.getWidth();
    const float top   = bounds.getY();
    const float bot   = bounds.getBottom();
    const float midY  = (top + bot) * 0.5f;

    g.setColour(WD::kVital);
    g.drawLine(x, top, x, bot, 1.5f);
    g.fillEllipse(x - 1.5f, midY - 1.5f, 3.0f, 3.0f);
}

void WaveformDisplay::paintLabels(juce::Graphics& g, juce::Rectangle<float> bounds, int grainCount)
{
    const float inset   = 4.0f;
    const float labelH  = 14.0f;
    const auto sansFont = juce::Font(juce::FontOptions{}.withName(juce::Font::getDefaultSansSerifFontName()).withHeight(10.0f));
    const auto monoFont = juce::Font(juce::FontOptions{}.withName(juce::Font::getDefaultMonospacedFontName()).withHeight(11.0f));

    const float innerL = bounds.getX()      + inset;
    const float innerR = bounds.getRight()  - inset;
    const float topY   = bounds.getY()      + inset;
    const float botY   = bounds.getBottom() - inset - labelH;
    const float w      = innerR - innerL;

    // Top-left: filename
    if (filename_.isNotEmpty())
    {
        g.setFont(sansFont);
        g.setColour(WD::kTextSec);
        g.drawText(filename_, juce::Rectangle<float>(innerL, topY, w * 0.6f, labelH),
                   juce::Justification::centredLeft, true);
    }

    // Top-right: duration · sample rate · channels
    if (fileSampleRate_ > 0.0 && fileNumFrames_ > 0)
    {
        const double durationSec = fileNumFrames_ / fileSampleRate_;
        const juce::String info = juce::String(durationSec, 2) + "s · "
                                + juce::String(fileSampleRate_ / 1000.0, 1) + " kHz · mono";
        g.setFont(monoFont);
        g.setColour(WD::kTextSec);
        g.drawText(info, juce::Rectangle<float>(innerL + w * 0.4f, topY, w * 0.6f, labelH),
                   juce::Justification::centredRight, true);
    }

    // Bottom-left: 0.0s
    g.setFont(monoFont);
    g.setColour(WD::kTextTert);
    g.drawText("0.0s", juce::Rectangle<float>(innerL, botY, w * 0.25f, labelH),
               juce::Justification::centredLeft, false);

    // Bottom-center: grain count
    const juce::String grainLabel = juce::String::fromUTF8("\xe2\x96\xaa")
                                  + " " + juce::String(grainCount) + " grains active";
    g.setColour(WD::kGrain);
    g.drawText(grainLabel, juce::Rectangle<float>(innerL + w * 0.25f, botY, w * 0.5f, labelH),
               juce::Justification::centred, false);

    // Bottom-right: total duration
    if (fileSampleRate_ > 0.0 && fileNumFrames_ > 0)
    {
        const double durationSec = fileNumFrames_ / fileSampleRate_;
        const juce::String durStr = juce::String(durationSec, 2) + "s";
        g.setColour(WD::kTextTert);
        g.drawText(durStr, juce::Rectangle<float>(innerL + w * 0.75f, botY, w * 0.25f, labelH),
                   juce::Justification::centredRight, false);
    }
}

void WaveformDisplay::paintDragHighlight(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour(WD::kVital.withAlpha(0.15f));
    g.fillRoundedRectangle(bounds, WD::kRadius);
}
