#include "PluginEditor.h"
#include "Parameters.h"

#if JUCE_WINDOWS
 #include <shellapi.h>
#endif

// Design constants from DESIGN_SPEC.md
namespace
{
    constexpr int   kDefaultWidth  = 1000;
    constexpr int   kDefaultHeight = 820;
    constexpr int   kMinWidth      = 800;
    constexpr int   kMinHeight     = 650;
    constexpr int   kMaxWidth      = 2000;
    constexpr int   kMaxHeight     = 1640;

    // Shared between paint() and resized()
    constexpr int   kMargin  = 36;
    constexpr int   kHeaderH = 52;

    const juce::Colour kColourSurfaceBase { 0xff0a0b0du };
    const juce::Colour kColourTextPrimary { 0xffe8e6e1u };
    const juce::Colour kColourBorderMuted { 0xff2a2e36u };

    constexpr float kLogoFontSize = 18.0f;
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

    static const char* kSlotLabels[] = { "S0", "S1", "S2", "S3" };
    for (int i = 0; i < 4; ++i)
    {
        slotButtons_[i].setButtonText(kSlotLabels[i]);
        slotButtons_[i].onClick = [this, i] { openFileChooser(i); };
        addAndMakeVisible(slotButtons_[i]);
    }

    curveEditor_.setAPVTS(&p.getAPVTS());
    addAndMakeVisible(curveEditor_);

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

    static const char* kSlotWeightIDs[] = {
        ParamIDs::slot0Weight, ParamIDs::slot1Weight,
        ParamIDs::slot2Weight, ParamIDs::slot3Weight
    };
    for (int i = 0; i < 4; ++i)
    {
        addAndMakeVisible(slotWeightKnobs_[i]);
        slotWeightAttachments_[i] = std::make_unique<SliderAttachment>(
            apvts, kSlotWeightIDs[i], slotWeightKnobs_[i].getSlider());
    }

    positionSlider_.setGrainExtent(
        apvts.getRawParameterValue(ParamIDs::grainSize), 0.0f);

    // Module tab panel — ENGINE / MOTION / COLOR / PATTERN / LFO / MOD
    moduleTabPanel_.init(apvts);
    addAndMakeVisible(moduleTabPanel_);

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

    // Header — SNAP label
    snapLabel_.setText("SNAP", juce::dontSendNotification);
    snapLabel_.setJustificationType(juce::Justification::centredRight);
    snapLabel_.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f)));
    snapLabel_.setColour(juce::Label::textColourId, juce::Colour{ 0xff6b7280u });
    addAndMakeVisible(snapLabel_);

    // Header — MASTER dB readout
    masterDbLabel_.setJustificationType(juce::Justification::centredLeft);
    masterDbLabel_.setFont(juce::Font(juce::FontOptions{}.withName(
        juce::Font::getDefaultMonospacedFontName()).withHeight(9.0f)));
    masterDbLabel_.setColour(juce::Label::textColourId, juce::Colour{ 0xff7cf5c4u });
    addAndMakeVisible(masterDbLabel_);

    // Footer — grain count
    footerGrainLabel_.setJustificationType(juce::Justification::centredLeft);
    footerGrainLabel_.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    footerGrainLabel_.setColour(juce::Label::textColourId, juce::Colour{ 0xff6b7280u });
    addAndMakeVisible(footerGrainLabel_);

    startTimerHz(10);
}

GranoAudioProcessorEditor::~GranoAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void GranoAudioProcessorEditor::timerCallback()
{
    // Grain count
    std::array<GranularEngine::GrainSnapshot, GranularEngine::MaxActiveGrains> snaps;
    const int grains = processorRef.getEngine()
                           .getGrainSnapshots(snaps.data(), GranularEngine::MaxActiveGrains);
    footerGrainLabel_.setText(juce::String(grains) + " grains",
                               juce::dontSendNotification);

    // MASTER volume (param is already in dB, range -60..+6)
    const float dB = processorRef.getAPVTS()
                         .getRawParameterValue(ParamIDs::masterVolume)->load();
    masterDbLabel_.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
}

void GranoAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(kColourSurfaceBase);

    // Radial vignette
    {
        const auto bounds = getLocalBounds().toFloat();
        juce::ColourGradient vignette(
            juce::Colours::transparentBlack,
            bounds.getCentreX(), bounds.getCentreY(),
            kColourSurfaceBase.darker(0.06f).withAlpha(1.0f),
            0.0f, 0.0f,
            true);
        g.setGradientFill(vignette);
        g.fillRect(bounds);
    }

    // Logo in title bar — left-aligned, system mono until BinaryData fonts land
    {
        g.setColour(kColourTextPrimary);
        g.setFont(juce::Font(juce::FontOptions{}
            .withName(juce::Font::getDefaultMonospacedFontName())
            .withHeight(kLogoFontSize)
            .withStyle("Bold")));
        g.drawText("GRANO",
                   kMargin, 0, 120, kHeaderH,
                   juce::Justification::centredLeft, false);
    }

    // Header separator
    g.setColour(kColourBorderMuted);
    g.drawLine(0.0f, (float)kHeaderH, (float)getWidth(), (float)kHeaderH, 1.0f);
}

void GranoAudioProcessorEditor::resized()
{
    // kMargin and kHeaderH live in the anonymous namespace (shared with paint())
    constexpr int kSectionGap   = 12;
    constexpr int kItemGap      = 6;
    constexpr int kPosStripH    = 44;
    constexpr int kKnobH        = 100;
    constexpr int kSpreadH      = 44;
    constexpr int kSlotStripH   = 80;
    constexpr int kBottomPanelH = 240;
    constexpr int kFooterH      = 24;
    constexpr int kSlotBtnW     = 28;
    constexpr int kSlotBtnH     = 24;
    constexpr int kLoopBtnW     = 56;
    constexpr int kLoopBtnH     = 24;
    constexpr int kSnapBtnW     = 32;
    constexpr int kSnapBtnH     = 24;

    const int w        = getWidth();
    const int h        = getHeight();
    const int contentX = kMargin;
    const int contentW = w - 2 * kMargin;
    const int btnY     = (kHeaderH - kSnapBtnH) / 2;

    // SNAP label + A/B/C/D buttons — header left, after logo
    {
        constexpr int kSnapLabelW = 36;
        int sx = contentX + 128;
        snapLabel_.setBounds(sx, btnY, kSnapLabelW, kSnapBtnH);
        sx += kSnapLabelW + kItemGap;
        for (int i = 0; i < 4; ++i)
        {
            snapButtons_[i].setBounds(sx, btnY, kSnapBtnW, kSnapBtnH);
            sx += kSnapBtnW + kItemGap;
        }
    }

    // Slot load buttons S0-S3 — header far right
    {
        const int totalSlotW = 4 * kSlotBtnW + 3 * kItemGap;
        int sx = w - kMargin - totalSlotW;
        for (int i = 0; i < 4; ++i)
        {
            slotButtons_[i].setBounds(sx, btnY, kSlotBtnW, kSlotBtnH);
            sx += kSlotBtnW + kItemGap;
        }
    }

    // Loop toggle — header right, left of slot buttons
    {
        const int totalSlotW = 4 * kSlotBtnW + 3 * kItemGap;
        loopButton_.setBounds(w - kMargin - totalSlotW - kItemGap - kLoopBtnW,
                              btnY, kLoopBtnW, kLoopBtnH);
    }

    // Waveform — variable height filling space between header and controls
    const int waveformH = h - kHeaderH
                            - 6 * kSectionGap
                            - kPosStripH - kKnobH - kSpreadH - kSlotStripH - kBottomPanelH - kFooterH;
    int y = kHeaderH + kSectionGap;
    waveformDisplay_.setBounds(contentX, y, contentW, waveformH);
    y += waveformH + kSectionGap;

    // Position slider
    positionSlider_.setBounds(contentX, y, contentW, kPosStripH);
    y += kPosStripH + kSectionGap;

    // Knob row — 5 equal columns (LOOP moved to header)
    const int knobW = contentW / 5;
    grainSizeKnob_ .setBounds(contentX,             y, knobW,                kKnobH);
    densityKnob_   .setBounds(contentX + knobW,     y, knobW,                kKnobH);
    posJitterKnob_ .setBounds(contentX + knobW * 2, y, knobW,                kKnobH);
    pitchShiftKnob_.setBounds(contentX + knobW * 3, y, knobW,                kKnobH);
    masterVolKnob_ .setBounds(contentX + knobW * 4, y, knobW,                kKnobH);
    y += kKnobH + kSectionGap;

    // Spread slider
    spreadSlider_.setBounds(contentX, y, contentW, kSpreadH);
    y += kSpreadH + kSectionGap;

    // Slot strip — weight knobs (left) + CurveEditor envelope selector (right)
    {
        const int slotKnobW = contentW / 8;
        for (int i = 0; i < 4; ++i)
            slotWeightKnobs_[i].setBounds(contentX + i * slotKnobW, y, slotKnobW, kSlotStripH);
        curveEditor_.setBounds(contentX + 4 * slotKnobW, y,
                               contentW - 4 * slotKnobW, kSlotStripH);
    }
    y += kSlotStripH + kSectionGap;

    // Bottom panel — ModuleTabPanel (ENGINE|MOTION|COLOR|PATTERN|LFO|MOD)
    moduleTabPanel_.setBounds(contentX, y, contentW, kBottomPanelH);

    // Footer — grain count left, error centred, MASTER dB right
    footerGrainLabel_.setBounds(contentX, h - kFooterH, contentW / 3, kFooterH);
    errorLabel_.setBounds      (contentX + contentW / 3, h - kFooterH, contentW / 3, kFooterH);
    masterDbLabel_.setBounds   (contentX + contentW * 2 / 3, h - kFooterH, contentW / 3, kFooterH);
}

void GranoAudioProcessorEditor::parentHierarchyChanged()
{
#if JUCE_WINDOWS
    // OLE IDropTarget registration fails silently when the DAW has initialised COM
    // with COINIT_MULTITHREADED (Audacity, some others). Enable WM_DROPFILES as a
    // fallback — JUCE's Win32ComponentPeer already handles that message and routes
    // it to FileDragAndDropTarget::filesDropped.
    if (auto* peer = getPeer())
        if (auto* hwnd = peer->getNativeHandle())
            ::DragAcceptFiles(static_cast<HWND>(hwnd), TRUE);
#endif
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

    processorRef.loadSampleFile(juce::File(files[0]), 0);

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

    // Auto-dismiss after 3 s; generation counter cancels stale callbacks.
    // Both here and the lambda run on the message thread — no race.
    const auto gen = ++errorGen_;
    juce::Timer::callAfterDelay(3000, [this, gen] {
        if (errorGen_ == gen) clearError();
    });
}

void GranoAudioProcessorEditor::clearError()
{
    errorLabel_.setVisible(false);
    repaint();
}

void GranoAudioProcessorEditor::openFileChooser(int slot)
{
    fileChooser_ = std::make_unique<juce::FileChooser>(
        "Select audio file",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");

    fileChooser_->launchAsync(juce::FileBrowserComponent::openMode |
                              juce::FileBrowserComponent::canSelectFiles,
        [this, slot](const juce::FileChooser& fc)
        {
            const auto results = fc.getResults();
            if (results.isEmpty())
                return;
            const auto file = results[0];
            processorRef.loadSampleFile(file, slot);
            if (processorRef.getLastLoadError().isEmpty())
            {
                if (slot == 0)
                {
                    const double sr = processorRef.getLastLoadedSampleRate();
                    const int    nf = processorRef.getLastLoadedNumFrames();
                    waveformDisplay_.setFile(file, sr, nf);
                    positionSlider_.setFileDurationMs((float)(nf / sr * 1000.0));
                }
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
