#include "PluginEditor.h"
#include "Parameters.h"

// Design constants from DESIGN_SPEC.md
namespace
{
    constexpr int   kDefaultWidth  = 1000;
    constexpr int   kDefaultHeight = 820;
    constexpr int   kMinWidth      = 800;
    constexpr int   kMinHeight     = 650;
    constexpr int   kMaxWidth      = 2000;
    constexpr int   kMaxHeight     = 1640;

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
    : AudioProcessorEditor(&p), processorRef(p),
      waveformDisplay_(p.getFormatManager())
{
    setSize(kDefaultWidth, kDefaultHeight);
    setResizable(true, true);
    setResizeLimits(kMinWidth, kMinHeight, kMaxWidth, kMaxHeight);
    setLookAndFeel(&laf_);

    waveformDisplay_.setEngine(&p.getEngine());
    waveformDisplay_.setPositionParam(
        p.getAPVTS().getRawParameterValue(ParamIDs::position));
    waveformDisplay_.setGrainSizeParam(
        p.getAPVTS().getRawParameterValue(ParamIDs::grainSize));
    addAndMakeVisible(waveformDisplay_);

    loadButton_.setButtonText("Load");
    loadButton_.onClick = [this] { openFileChooser(); };
    addAndMakeVisible(loadButton_);

    errorLabel_.setJustificationType(juce::Justification::centred);
    errorLabel_.setColour(juce::Label::textColourId, juce::Colour{ 0xffff6b6b });
    errorLabel_.setVisible(false);
    addAndMakeVisible(errorLabel_);

    // Core controls
    addAndMakeVisible(positionSlider_);
    addAndMakeVisible(grainSizeKnob_);
    addAndMakeVisible(densityKnob_);
    addAndMakeVisible(posJitterKnob_);
    addAndMakeVisible(pitchShiftKnob_);
    addAndMakeVisible(spreadSlider_);
    addAndMakeVisible(masterVolKnob_);
    loopButton_.setButtonText("LOOP");
    loopButton_.setClickingTogglesState(true);
    addAndMakeVisible(loopButton_);

    // APVTS attachments
    auto& apvts = p.getAPVTS();
    positionAttach_  = std::make_unique<SliderAttachment>(apvts, ParamIDs::position,
                                                           positionSlider_.getSlider());
    grainSizeAttach_ = std::make_unique<SliderAttachment>(apvts, ParamIDs::grainSize,
                                                           grainSizeKnob_.getSlider());
    densityAttach_   = std::make_unique<SliderAttachment>(apvts, ParamIDs::density,
                                                           densityKnob_.getSlider());
    posJitterAttach_ = std::make_unique<SliderAttachment>(apvts, ParamIDs::positionJitter,
                                                           posJitterKnob_.getSlider());
    pitchShiftAttach_= std::make_unique<SliderAttachment>(apvts, ParamIDs::pitchShift,
                                                           pitchShiftKnob_.getSlider());
    spreadAttach_    = std::make_unique<SliderAttachment>(apvts, ParamIDs::stereoSpread,
                                                           spreadSlider_.getSlider());
    masterVolAttach_ = std::make_unique<SliderAttachment>(apvts, ParamIDs::masterVolume,
                                                           masterVolKnob_.getSlider());
    loopAttach_      = std::make_unique<ButtonAttachment>(apvts, ParamIDs::loop, loopButton_);

    positionSlider_.setGrainExtent(
        apvts.getRawParameterValue(ParamIDs::grainSize), 0.0f);

    // F5 panels
    lfoPanel_.init(apvts);
    addAndMakeVisible(lfoPanel_);

    modMatrixView_.init(apvts);
    addAndMakeVisible(modMatrixView_);

    // Snapshot buttons A/B/C/D
    static const char* kSnapLabels[] = { "A", "B", "C", "D" };
    for (int i = 0; i < 4; ++i)
    {
        snapButtons_[i].setButtonText(kSnapLabels[i]);
        snapButtons_[i].onClick = [this, i]
        {
            auto& snaps = processorRef.getSnapshots();
            auto& apvts_ = processorRef.getAPVTS();
            if (juce::ModifierKeys::currentModifiers.isCtrlDown())
                snaps.save(i, apvts_.copyState());
            else
                snaps.recall(i, apvts_);
        };
        addAndMakeVisible(snapButtons_[i]);
    }
}

GranoAudioProcessorEditor::~GranoAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
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
        juce::Font logoFont(juce::FontOptions{}
            .withName(juce::Font::getDefaultMonospacedFontName())
            .withHeight(kLogoFontSize)
            .withStyle("Bold"));

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
    constexpr int kMargin       = 40;
    constexpr int kHeaderH      = 48;
    constexpr int kPosStripH    = 44;   // position slider strip (label + slider)
    constexpr int kKnobH        = 100;  // knob area (rotary + label)
    constexpr int kSpreadH      = 44;   // spread slider strip
    constexpr int kBottomPanelH = 200;  // LfoPanel + ModulationMatrixView
    constexpr int kFooterH      = 24;   // error label
    constexpr int kLoadBtnW     = 72;
    constexpr int kLoadBtnH     = 24;
    constexpr int kLoopBtnW     = 56;
    constexpr int kLoopBtnH     = 24;
    constexpr int kSnapBtnW     = 36;
    constexpr int kSnapBtnH     = 24;
    constexpr int kGap          = 4;

    const int w        = getWidth();
    const int h        = getHeight();
    const int contentX = kMargin;
    const int contentW = w - 2 * kMargin;

    // Snapshot buttons A/B/C/D — header left side
    {
        int sx = contentX;
        for (int i = 0; i < 4; ++i)
        {
            snapButtons_[i].setBounds(sx, (kHeaderH - kSnapBtnH) / 2, kSnapBtnW, kSnapBtnH);
            sx += kSnapBtnW + kGap;
        }
    }

    // Load button — header top-right
    loadButton_.setBounds(w - kMargin - kLoadBtnW,
                          (kHeaderH - kLoadBtnH) / 2,
                          kLoadBtnW, kLoadBtnH);

    // Waveform — fills space between header and controls + bottom panel
    const int controlsH = kPosStripH + kGap + kKnobH + kGap + kSpreadH;
    const int waveformH = h - kHeaderH - kGap - controlsH - kGap - kBottomPanelH - kGap - kFooterH;
    waveformDisplay_.setBounds(contentX, kHeaderH, contentW, waveformH);

    // Position slider — directly below waveform
    int y = kHeaderH + waveformH + kGap;
    positionSlider_.setBounds(contentX, y, contentW, kPosStripH);
    y += kPosStripH + kGap;

    // Knob row — 5 knobs + loop button
    const int knobZoneW = contentW - kLoopBtnW - kGap;
    const int knobW     = knobZoneW / 5;
    grainSizeKnob_ .setBounds(contentX,             y, knobW, kKnobH);
    densityKnob_   .setBounds(contentX + knobW,     y, knobW, kKnobH);
    posJitterKnob_ .setBounds(contentX + knobW * 2, y, knobW, kKnobH);
    pitchShiftKnob_.setBounds(contentX + knobW * 3, y, knobW, kKnobH);
    masterVolKnob_ .setBounds(contentX + knobW * 4, y, knobW, kKnobH);
    loopButton_    .setBounds(contentX + knobZoneW + kGap,
                              y + (kKnobH - kLoopBtnH) / 2,
                              kLoopBtnW, kLoopBtnH);
    y += kKnobH + kGap;

    // Spread slider — below knob row
    spreadSlider_.setBounds(contentX, y, contentW, kSpreadH);
    y += kSpreadH + kGap;

    // Bottom panel — LfoPanel left half, ModulationMatrixView right half
    const int halfW = contentW / 2;
    lfoPanel_     .setBounds(contentX,           y, halfW,            kBottomPanelH);
    modMatrixView_.setBounds(contentX + halfW,   y, contentW - halfW, kBottomPanelH);

    // Error label — docked to bottom
    errorLabel_.setBounds(0, h - kFooterH, w, kFooterH);
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
    {
        const double sr  = processorRef.getLastLoadedSampleRate();
        const int    nf  = processorRef.getLastLoadedNumFrames();
        waveformDisplay_.setFile(juce::File(files[0]), sr, nf);
        positionSlider_.setFileDurationMs((float)(nf / sr * 1000.0));
        clearError();
    }
    else
    {
        showError(err);
    }
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

void GranoAudioProcessorEditor::openFileChooser()
{
    fileChooser_ = std::make_unique<juce::FileChooser>(
        "Select audio file",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");

    fileChooser_->launchAsync(juce::FileBrowserComponent::openMode |
                              juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const auto results = fc.getResults();
            if (results.isEmpty())
                return;
            const auto file = results[0];
            processorRef.loadSampleFile(file);
            if (processorRef.getLastLoadError().isEmpty())
            {
                const double sr = processorRef.getLastLoadedSampleRate();
                const int    nf = processorRef.getLastLoadedNumFrames();
                waveformDisplay_.setFile(file, sr, nf);
                positionSlider_.setFileDurationMs((float)(nf / sr * 1000.0));
                clearError();
            }
            else
            {
                showError(processorRef.getLastLoadError());
            }
        });
}

void GranoAudioProcessorEditor::fileDragEnter(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    if (isInterestedInFileDrag(files))
        waveformDisplay_.setDragHighlightActive(true);
}

void GranoAudioProcessorEditor::fileDragExit(const juce::StringArray& /*files*/)
{
    waveformDisplay_.setDragHighlightActive(false);
}
