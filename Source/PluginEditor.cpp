#include "PluginEditor.h"

// Design constants from DESIGN_SPEC.md
namespace
{
    constexpr int   kDefaultWidth  = 1000;
    constexpr int   kDefaultHeight = 620;
    constexpr int   kMinWidth      = 800;
    constexpr int   kMinHeight     = 500;
    constexpr int   kMaxWidth      = 2000;
    constexpr int   kMaxHeight     = 1240;

    // surface.base — the primary background colour
    const juce::Colour kColourSurfaceBase  { 0xff0a0b0d };

    // text.primary — used for the logo label
    const juce::Colour kColourTextPrimary  { 0xffe8e6e1 };

    // Logo: JetBrains Mono 18 px medium, UPPERCASE, letter-spacing 0.32 em
    constexpr float kLogoFontSize          = 18.0f;
    // Letter-spacing 0.32 em ≈ 0.32 × 18 = 5.76 px per character gap.
    // Approximated by drawing with extra tracking via juce::GlyphArrangement.
    constexpr float kLogoLetterSpacing     = 5.76f;
}

GranoAudioProcessorEditor::GranoAudioProcessorEditor(GranoAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(kDefaultWidth, kDefaultHeight);
    setResizable(true, true);
    setResizeLimits(kMinWidth, kMinHeight, kMaxWidth, kMaxHeight);

    errorLabel_.setJustificationType(juce::Justification::centred);
    errorLabel_.setColour(juce::Label::textColourId, juce::Colour{ 0xffff6b6b });
    errorLabel_.setVisible(false);
    addAndMakeVisible(errorLabel_);
}

void GranoAudioProcessorEditor::paint(juce::Graphics& g)
{
    // ── Background ──────────────────────────────────────────────────────────
    g.fillAll(kColourSurfaceBase);

    // ── Radial vignette — darkens corners ~6% ───────────────────────────────
    {
        const auto bounds = getLocalBounds().toFloat();
        juce::ColourGradient vignette(
            juce::Colours::transparentBlack,
            bounds.getCentreX(), bounds.getCentreY(),
            kColourSurfaceBase.darker(0.06f).withAlpha(1.0f),
            0.0f, 0.0f,
            true); // radial
        g.setGradientFill(vignette);
        g.fillRect(bounds);
    }

    // ── Logo: "GRANO" centred ───────────────────────────────────────────────
    // JetBrains Mono isn't bundled yet (added in F3 via BinaryData).
    // Fall back to the system monospaced font for F0.
    {
        const juce::String logoText = "GRANO";
        juce::Font logoFont(juce::Font::getDefaultMonospacedFontName(),
                            kLogoFontSize, juce::Font::bold);

        // Approximate the letter-spacing by distributing glyphs manually.
        juce::GlyphArrangement glyphs;
        glyphs.addFittedText(logoFont, logoText,
                             0.0f, 0.0f,
                             (float)getWidth(), (float)getHeight(),
                             juce::Justification::centred, 1);

        // Shift each glyph right by cumulative letter-spacing offset.
        // Total extra width = (N-1) * kLogoLetterSpacing.
        const int   n          = logoText.length();
        const float totalExtra = (float)(n - 1) * kLogoLetterSpacing;
        const float startShift = -totalExtra * 0.5f;

        juce::GlyphArrangement spaced;
        for (int i = 0; i < glyphs.getNumGlyphs(); ++i)
        {
            auto glyph = glyphs.getGlyph(i);
            glyph.moveBy(startShift + (float)i * kLogoLetterSpacing, 0.0f);
            spaced.addGlyph(glyph);
        }

        g.setColour(kColourTextPrimary);
        spaced.draw(g);
    }
}

void GranoAudioProcessorEditor::resized()
{
    // Error label docked to bottom, full width, 24 px height.
    errorLabel_.setBounds(0, getHeight() - 24, getWidth(), 24);
}

bool GranoAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& path : files)
    {
        const juce::String ext = juce::File(path).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aif" || ext == ".aiff" ||
            ext == ".flac" || ext == ".ogg" || ext == ".mp3")
            return true;
    }
    return false;
}

void GranoAudioProcessorEditor::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    if (files.isEmpty())
        return;

    processorRef.loadSampleFile(juce::File(files[0]));

    const auto& err = processorRef.getLastLoadError();
    if (err.isEmpty())
        clearError();
    else
        showError(err);
}

void GranoAudioProcessorEditor::showError(const juce::String& message)
{
    errorLabel_.setText(message, juce::dontSendNotification);
    errorLabel_.setVisible(true);
    repaint();
}

void GranoAudioProcessorEditor::clearError()
{
    errorLabel_.setVisible(false);
    repaint();
}
