#include "ConsoleView.h"
#include "../Utils/DebugLog.h"
#include "ParamFormat.h"
#include "../PluginProcessor.h"
#include "../Parameters.h"

using namespace Theme;

namespace
{
    const juce::String kDot = juce::String::fromUTF8(" \xc2\xb7 ");
    const juce::String kDefaultHint =
        juce::String::fromUTF8("Drag a sample onto the monitor \xc2\xb7 Ctrl/Cmd-click a snapshot to store \xc2\xb7 double-click a control to reset");

    juce::String versionText()
    {
#ifdef JucePlugin_VersionString
        return juce::StringArray::fromTokens(JucePlugin_VersionString, ".", {}).joinIntoString(".", 0, 2);
#else
        return "1.0";
#endif
    }
}

ConsoleView::ConsoleView(GranoAudioProcessor& p)
    : proc_(p),
      snapshots_(p.getSnapshots(), p.getAPVTS()),
      loop_(*p.getAPVTS().getParameter(ParamIDs::loop)),
      master_(*p.getAPVTS().getParameter(ParamIDs::masterVolume)),
      monitor_(p.getFormatManager(), p.getAPVTS(), p.getEngine()),
      grain_(p.getAPVTS()), engine_(p.getAPVTS()), envelope_(p.getAPVTS()),
      motion_(p.getAPVTS()), color_(p.getAPVTS()), pattern_(p.getAPVTS()),
      lfo1_(p.getAPVTS(), 1), lfo2_(p.getAPVTS(), 2), matrix_(p.getAPVTS())
{
    setOpaque(true);
    setTitle("Grano");
    setFocusContainerType(FocusContainerType::keyboardFocusContainer);

    addAndMakeVisible(cmd_);
    addAndMakeVisible(snapshots_);
    addAndMakeVisible(loop_);
    addAndMakeVisible(master_);
    addAndMakeVisible(monitor_);

    auto& apvts = p.getAPVTS();
    const char* weights[] = { ParamIDs::slot0Weight, ParamIDs::slot1Weight, ParamIDs::slot2Weight, ParamIDs::slot3Weight };
    for (int i = 0; i < 4; ++i)
    {
        auto s = std::make_unique<SlotStrip>(i, *apvts.getParameter(weights[i]));
        s->onShow  = [this, i] { showSlot(i); };
        s->onLoad  = [this, i] { chooseFile(i); };
        s->onClear = [this, i]
        {
            proc_.unloadSlot(i);
            slotErrors_[(size_t)i] = {};
            message("Slot " + juce::String(i + 1), "cleared", "sample bank");
        };
        addAndMakeVisible(*s);
        slots_[(size_t)i] = std::move(s);
    }

    banks_ = { &grain_, &engine_, &envelope_, &motion_, &color_, &pattern_, &lfo1_, &lfo2_, &matrix_ };
    for (auto* b : banks_)
        addAndMakeVisible(*b);

    std::vector<Console::ParamControl*> all;
    Console::collectControls(*this, all);
    for (auto* c : all)
        controls_[c->getParam().getParameterID()] = c;

    snapshots_.onAction = [this](int i, const juce::String& what)
    {
        message("Snapshot " + juce::String::charToString((juce::juce_wchar)('A' + i)), what, "snapshot");
    };

    hint_ = kDefaultHint;
    addMouseListener(this, true);

    if (auto* gs = apvts.getParameter(ParamIDs::grainSize))
        showCommand(*gs);

    setSize(kBaseWidth, kBaseHeight);
    refreshSlots(true);
    refreshModulation();
    startTimerHz(30);
}

ConsoleView::~ConsoleView()
{
    stopTimer();
    removeMouseListener(this);
}

// ── Layout (base px; see design/mock/grano-ui.html) ───────────────────────────
void ConsoleView::resized()
{
    cmd_.setBounds(160, 8, 428, 56);
    snapshots_.setBounds(604, 8, 224, 56);
    loop_.setBounds(844, 8, 60, 56);
    master_.setBounds(918, 8, 266, 62);
    monitor_.setBounds(16, 72, 1168, 118);
    for (int i = 0; i < 4; ++i)
        slots_[(size_t)i]->setBounds(16 + i * 296, 198, i < 3 ? 288 : 280, 36);

    grain_   .setBounds(16, 244, 524, 200);
    engine_  .setBounds(548, 244, 404, 200);
    envelope_.setBounds(960, 244, 224, 200);
    motion_  .setBounds(16, 452, 412, 150);
    color_   .setBounds(436, 452, 258, 150);
    pattern_ .setBounds(702, 452, 482, 150);
    lfo1_    .setBounds(16, 610, 300, 140);
    lfo2_    .setBounds(324, 610, 300, 140);
    matrix_  .setBounds(632, 610, 552, 140);
}

void ConsoleView::paint(juce::Graphics& g)
{
    g.fillAll(C::desk);

    // Wordmark
    g.setColour(C::silk);
    g.setFont(font(Face::wordmark, 24.0f, 0.1f));
    g.drawText("GRANO", juce::Rectangle<float>(18.0f, 12.0f, 140.0f, 28.0f), juce::Justification::centredLeft, false);
    Console::drawSilk(g, "Granular" + kDot + versionText(), { 18.0f, 42.0f, 140.0f, 14.0f });

    // Plate shadows under banks
    g.setColour(C::shadow.withMultipliedAlpha(0.6f));
    for (auto* b : banks_)
        g.fillRoundedRectangle(b->getBounds().toFloat().translated(0.0f, 2.0f).reduced(1.0f, 0.0f), kPlateRadius);

    // Status strip
    const auto row = juce::Rectangle<float>(18.0f, 756.0f, 1164.0f, 16.0f);
    const auto lf = font(Face::lcd, 13.0f);
    float x = row.getX();
    auto seg = [&](const juce::String& t, juce::Colour c)
    {
        g.setColour(c);
        g.setFont(lf);
        const float w = Console::textWidth(lf, t);
        g.drawText(t, juce::Rectangle<float>(x, row.getY(), w + 1.0f, row.getHeight()), juce::Justification::centredLeft, false);
        x += w;
    };
    seg(juce::String(grains_), C::lcd);
    seg(grains_ == 1 ? " grain" + kDot + "cpu " : " grains" + kDot + "cpu ", C::silkDim);
    seg(juce::String(cpu_) + " %", C::lcd);
    seg(kDot + juce::String(loaded_) + " of 4 slots loaded", C::silkDim);

    g.setColour(C::silk);
    g.setFont(font(Face::text, 13.0f));
    g.drawText(hint_, row.withTrimmedLeft(x - row.getX() + 24.0f), juce::Justification::centredRight, true);
}

// ── Command line ──────────────────────────────────────────────────────────────
void ConsoleView::controlTouched(juce::Component& from, juce::RangedAudioParameter& p, float prev)
{
    current_ = &p;
    gestureStart_ = prev;
    auto* bank = from.findParentComponentOfClass<Console::Bank>();
    if (bank != touchedBank_)
    {
        if (touchedBank_) touchedBank_->setTouched(false);
        touchedBank_ = bank;
        if (touchedBank_) touchedBank_->setTouched(true);
    }
    showCommand(p);
}

void ConsoleView::controlChanged(juce::RangedAudioParameter& p)
{
    const float v = p.convertFrom0to1(p.getValue());
    if (&p == current_ && v != gestureStart_)
        previous_[p.getParameterID()] = gestureStart_;
    showCommand(p);
}

void ConsoleView::showCommand(juce::RangedAudioParameter& p)
{
    const auto id = p.getParameterID();
    const float v = p.convertFrom0to1(p.getValue());
    juce::String sub;
    if (auto it = previous_.find(id); it != previous_.end())
        sub << juce::String::fromUTF8("\xe2\x86\x90 ") << ParamFormat::valueText(id, it->second) << kDot;
    sub << "default " << ParamFormat::valueText(id, p.convertFrom0to1(p.getDefaultValue()));
    juce::String value = ParamFormat::valueText(id, v);
    if (id.endsWith("Enabled"))
        value = v > 0.5f ? "on" : "bypassed";
    cmd_.show(ParamFormat::fullName(id), sub, value);
}

void ConsoleView::message(const juce::String& name, const juce::String& value, const juce::String& sub)
{
    current_ = nullptr;
    cmd_.show(name, sub, value);
}

// ── Hover hints ───────────────────────────────────────────────────────────────
void ConsoleView::setHint(const juce::String& h)
{
    const auto next = h.isEmpty() ? kDefaultHint : h;
    if (next != hint_)
    {
        hint_ = next;
        repaint(0, 750, kBaseWidth, 30);
    }
}

void ConsoleView::mouseMove(const juce::MouseEvent& e)
{
    for (auto* c = e.originalComponent; c != nullptr && c != this; c = c->getParentComponent())
        if (auto* t = dynamic_cast<juce::TooltipClient*>(c))
        {
            const auto tip = t->getTooltip();
            if (tip.isNotEmpty()) { setHint(tip); return; }
        }
    setHint({});
}

void ConsoleView::mouseExit(const juce::MouseEvent& e)
{
    if (!getLocalBounds().contains(e.getEventRelativeTo(this).getPosition()))
        setHint({});
}

// ── Samples ───────────────────────────────────────────────────────────────────
void ConsoleView::showSlot(int i)
{
    shownSlot_ = i;
    refreshSlots(true);
}

void ConsoleView::chooseFile(int slot)
{
    const auto start = proc_.getSlotFile(slot).existsAsFile()
        ? proc_.getSlotFile(slot).getParentDirectory()
        : juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    chooser_ = std::make_unique<juce::FileChooser>("Load sample into slot " + juce::String(slot + 1), start,
                                                   "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
    juce::Component::SafePointer<ConsoleView> safe(this);
    chooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe, slot](const juce::FileChooser& fc)
        {
            if (auto* v = safe.getComponent())
                if (fc.getResults().size() > 0)
                    v->loadSlot(slot, fc.getResult());
        });
}

void ConsoleView::loadIntoShownSlot(const juce::File& f)
{
    loadSlot(shownSlot_, f);
}

void ConsoleView::loadSlot(int slot, const juce::File& f)
{
    proc_.loadSampleFile(f, slot);
    const auto& err = proc_.getLastLoadError();
    slotErrors_[(size_t)slot] = err;
    slots_[(size_t)slot]->setError(err);
    if (err.isEmpty())
    {
        shownSlot_ = slot;
        // A freshly loaded slot with zero weight would never be heard.
        if (auto* w = proc_.getAPVTS().getParameter("slot" + juce::String(slot) + "Weight"))
            if (w->getValue() <= 0.0f)
                w->setValueNotifyingHost(w->convertTo0to1(0.5f));
        message("Slot " + juce::String(slot + 1), "loaded", f.getFileName());
    }
    else
        message("Slot " + juce::String(slot + 1), "failed", err);
    refreshSlots(true);
}

juce::String ConsoleView::describeShownSlots() const
{
    juce::String s;
    for (int i = 0; i < 4; ++i)
        s << " [" << (i + 1) << ":" << (slots_[(size_t)i]->isLoaded() ? "loaded" : "empty") << "]";
    return s;
}

void ConsoleView::refreshSlots(bool force)
{
    const auto gen = proc_.getSlotStateGeneration();
    if (!force && gen == slotGen_)
        return;
    slotGen_ = gen;
    loaded_ = 0;
    for (int i = 0; i < 4; ++i)
    {
        const auto info = proc_.getSlotInfo(i);
        auto& s = *slots_[(size_t)i];
        const bool wasLoaded = s.isLoaded();
        s.setFile(info.numFrames > 0 ? info.file : juce::File());
        if (wasLoaded && !s.isLoaded())
            GranoLog::write(proc_.logId() + " editor: slot " + juce::String(i + 1)
                            + " strip emptied (slot state gen " + juce::String(gen) + ")");
        if (info.numFrames > 0) ++loaded_;
        s.setShown(i == shownSlot_);
    }
    const auto info = proc_.getSlotInfo(shownSlot_);
    monitor_.setSample(info.file, info.sampleRate, info.numFrames, info.numChannels);
    repaint(0, 750, kBaseWidth, 30);
}

// ── Modulation marks ──────────────────────────────────────────────────────────
void ConsoleView::refreshModulation()
{
    auto& apvts = proc_.getAPVTS();
    std::map<juce::String, float> depth;
    for (int i = 1; i <= 8; ++i)
    {
        const int src  = juce::roundToInt(apvts.getRawParameterValue(slotParamID(i, "Source"))->load());
        const int dest = juce::roundToInt(apvts.getRawParameterValue(slotParamID(i, "Dest"))->load());
        const float amt = apvts.getRawParameterValue(slotParamID(i, "Amount"))->load();
        if (src <= 0 || dest <= 0 || amt == 0.0f || !ParamFormat::modDestActive(dest, src))
            continue;   // reserved destinations get no mark: they change nothing
        const float lfoDepth = apvts.getRawParameterValue(src == 1 ? ParamIDs::lfo1Depth : ParamIDs::lfo2Depth)->load();
        depth[ParamFormat::modDestParamID(dest)] += std::abs(amt) * lfoDepth * 0.5f;
    }
    for (auto& [id, c] : controls_)
    {
        const auto it = depth.find(id);
        c->setModDepth(it != depth.end() ? juce::jmin(1.0f, it->second) : 0.0f);
    }
    matrix_.refresh();
}

// ── Timer ─────────────────────────────────────────────────────────────────────
void ConsoleView::timerCallback()
{
    monitor_.tick();
    master_.pushPeaks(proc_.consumeOutputPeak(0), proc_.consumeOutputPeak(1));

    if (++tickCount_ % 3 != 0)
        return;

    // 10 Hz
    refreshSlots(false);
    refreshModulation();
    snapshots_.refresh();
    const int g = monitor_.getGrainCount();
    const int cpu = juce::roundToInt(proc_.getCpuLoad() * 100.0);
    if (g != grains_ || cpu != cpu_)
    {
        grains_ = g;
        cpu_ = cpu;
        repaint(0, 750, kBaseWidth, 30);
    }
}
