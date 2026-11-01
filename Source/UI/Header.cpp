#include "Header.h"
#include "ParamFormat.h"

using namespace Theme;
using Console::textWidth;

// ── CommandLine ───────────────────────────────────────────────────────────────
CommandLine::CommandLine()
{
    setTitle("Command line");
    setAccessible(true);
    setInterceptsMouseClicks(false, false);
}

void CommandLine::show(const juce::String& name, const juce::String& sub, const juce::String& value)
{
    if (name == name_ && sub == sub_ && value == value_)
        return;
    name_ = name; sub_ = sub; value_ = value;
    setDescription(name + " " + value);
    if (auto* h = getAccessibilityHandler())
        h->notifyAccessibilityEvent(juce::AccessibilityEvent::valueChanged);
    repaint();
}

void CommandLine::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour(C::lcdBg);
    g.fillRoundedRectangle(b, kKeyRadius);
    g.setColour(C::groove.withAlpha(0.6f));
    g.fillRect(b.reduced(3.0f, 0.0f).withHeight(2.0f).translated(0.0f, 1.0f));
    g.setColour(C::groove);
    g.drawRoundedRectangle(b.reduced(0.5f), kKeyRadius, 1.0f);

    auto inner = b.reduced(12.0f, 0.0f);
    const auto valueArea = inner.removeFromRight(196.0f);
    inner.removeFromRight(14.0f);

    const auto nameFont = font(Face::silkBold, 15.0f, 0.12f);
    const auto subFont  = font(Face::lcd, 13.0f);
    g.setColour(C::lcdName);
    g.setFont(nameFont);
    g.drawText(name_.toUpperCase(), inner.withY(9.0f).withHeight(19.0f), juce::Justification::centredLeft, true);
    g.setColour(C::lcdDim);
    g.setFont(subFont);
    g.drawText(sub_, inner.withY(30.0f).withHeight(17.0f), juce::Justification::centredLeft, true);

    g.setColour(C::lcd);
    g.setFont(font(Face::lcd, 42.0f, -0.02f));
    g.drawFittedText(value_, valueArea.toNearestInt(), juce::Justification::centredRight, 1, 0.6f);
}

// ── SnapshotBar ───────────────────────────────────────────────────────────────
class SnapshotBar::SnapKey : public Console::Key
{
public:
    SnapKey(SnapshotBar& bar, int i)
        : Key(juce::String::charToString((juce::juce_wchar)('A' + i)), C::live), bar_(bar), i_(i)
    {
        setTextSize(20.0f, Face::wordmark);
        const auto L = getButtonText();
        setTitle("Snapshot " + L);
        setTooltip("Snapshot " + L + ": click to recall. Ctrl/Cmd-click or right-click to store, recall or clear.");
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { bar_.showMenu(i_); return; }
        Key::mouseDown(e);
    }
    void clicked(const juce::ModifierKeys& mods) override
    {
        if (mods.isCommandDown())        bar_.store(i_);
        else if (bar_.snaps_.isOccupied(i_)) bar_.recall(i_);
        else                             bar_.store(i_);
    }
private:
    SnapshotBar& bar_;
    int i_;
};

SnapshotBar::SnapshotBar(Snapshots& s, juce::AudioProcessorValueTreeState& apvts)
    : snaps_(s), apvts_(apvts)
{
    setTitle("Snapshots");
    setFocusContainerType(FocusContainerType::focusContainer);
    for (int i = 0; i < Snapshots::kNumSlots; ++i)
    {
        keys_[(size_t)i] = std::make_unique<SnapKey>(*this, i);
        addAndMakeVisible(*keys_[(size_t)i]);
    }
    refresh();
}

SnapshotBar::~SnapshotBar() = default;

void SnapshotBar::store(int i)
{
    snaps_.save(i, apvts_.copyState());
    refresh();
    if (onAction) onAction(i, "stored");
}

void SnapshotBar::recall(int i)
{
    if (!snaps_.isOccupied(i)) return;
    snaps_.recall(i, apvts_);
    refresh();
    if (onAction) onAction(i, "recalled");
}

void SnapshotBar::clear(int i)
{
    snaps_.clearSlot(i);
    refresh();
    if (onAction) onAction(i, "cleared");
}

void SnapshotBar::showMenu(int i)
{
    juce::PopupMenu m;
    const bool has = snaps_.isOccupied(i);
    m.addItem("Store current", [this, i] { store(i); });
    m.addItem("Recall", has, false, [this, i] { recall(i); });
    m.addItem("Clear", has, false, [this, i] { clear(i); });
    m.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(keys_[(size_t)i].get()));
}

void SnapshotBar::refresh()
{
    for (int i = 0; i < Snapshots::kNumSlots; ++i)
    {
        auto& k = *keys_[(size_t)i];
        k.setToggleState(snaps_.getLive() == i, juce::dontSendNotification);
        k.setUnlitInk(snaps_.isOccupied(i) ? C::lcd : C::silk);
        const auto state = snaps_.getLive() == i ? "live" : snaps_.isOccupied(i) ? "stored" : "empty";
        k.setDescription(state);
    }
    repaint();
}

void SnapshotBar::resized()
{
    for (int i = 0; i < Snapshots::kNumSlots; ++i)
        keys_[(size_t)i]->setBounds(i * 58, 0, 50, 38);
}

void SnapshotBar::paint(juce::Graphics& g)
{
    for (int i = 0; i < Snapshots::kNumSlots; ++i)
    {
        const bool live = snaps_.getLive() == i;
        const auto txt = live ? "live" : snaps_.isOccupied(i) ? "stored" : "empty";
        Console::drawSilk(g, txt, juce::Rectangle<float>((float)i * 58.0f, 41.0f, 50.0f, 14.0f),
                          juce::Justification::centred, live ? C::live : C::silkDim);
    }
}

// ── LoopSection ───────────────────────────────────────────────────────────────
LoopSection::LoopSection(juce::RangedAudioParameter& loop)
    : key_(loop, C::lcd, {}, {})
{
    key_.setIcon([](juce::Graphics& g, juce::Rectangle<float> r, juce::Colour ink)
    {
        // Two arcs with arrowheads (24-unit icon grid).
        juce::Path p;
        p.startNewSubPath(4, 12);  p.cubicTo(4, 6, 12, 2.5f, 16, 7); p.lineTo(18, 9);
        p.startNewSubPath(18, 4);  p.lineTo(18, 9); p.lineTo(13, 9);
        p.startNewSubPath(20, 12); p.cubicTo(20, 18, 12, 21.5f, 8, 17); p.lineTo(6, 15);
        p.startNewSubPath(6, 20);  p.lineTo(6, 15); p.lineTo(11, 15);
        const auto box = juce::Rectangle<float>(18.0f, 18.0f).withCentre(r.getCentre());
        g.setColour(ink);
        g.strokePath(p, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                     p.getTransformToScaleToFit(box, true));
    });
    key_.onState = [this](bool) { repaint(); };
    addAndMakeVisible(key_);
}

void LoopSection::resized() { key_.setBounds(0, 0, 60, 38); }

void LoopSection::paint(juce::Graphics& g)
{
    Console::drawSilk(g, key_.getToggleState() ? "Loop on" : "Loop off",
                      juce::Rectangle<float>(-10.0f, 41.0f, 80.0f, 14.0f), juce::Justification::centred,
                      key_.getToggleState() ? C::lcd : C::silkDim);
}

// ── MasterSection ─────────────────────────────────────────────────────────────
MasterSection::MasterSection(juce::RangedAudioParameter& volume)
    : fader_(volume, C::mGrain, "Master"), volume_(volume)
{
    addAndMakeVisible(fader_);
    setTitle("Master");
}

void MasterSection::pushPeaks(float l, float r)
{
    const std::array<float, 2> in { l, r };
    bool changed = false;
    for (size_t i = 0; i < 2; ++i)
    {
        const float next = juce::jmax(in[i], level_[i] * 0.85f);
        if (std::abs(next - level_[i]) > 1.0e-4f) changed = true;
        level_[i] = next < 1.0e-4f ? 0.0f : next;
    }
    if (l > 1.0f || r > 1.0f) { clipHold_ = 45; changed = true; }   // 1.5 s at 30 Hz
    else if (clipHold_ > 0 && --clipHold_ == 0) changed = true;
    if (changed) repaint();
}

void MasterSection::resized()
{
    fader_.setBounds(0, 44, getWidth(), 18);
}

void MasterSection::paintMeter(juce::Graphics& g, juce::Rectangle<float> r, float level) const
{
    g.setColour(C::well);
    g.fillRoundedRectangle(r, 1.0f);
    const float db = juce::Decibels::gainToDecibels(level, -60.0f);
    const float frac = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 66.0f);
    for (float x = 0.0f; x < r.getWidth(); x += 7.0f)
    {
        const float f = x / r.getWidth();
        const bool lit = f < frac;
        const auto col = f >= 0.88f ? C::clip : f >= 0.70f ? C::meterMid : C::meterLo;
        g.setColour(lit ? col : C::meterOff);
        g.fillRect(r.getX() + x, r.getY(), juce::jmin(5.0f, r.getWidth() - x), r.getHeight());
    }
}

void MasterSection::paint(juce::Graphics& g)
{
    const auto w = (float)getWidth();
    Console::drawSilk(g, "Master", { 0.0f, 0.0f, 100.0f, 16.0f });
    // Clip LED
    const auto led = juce::Rectangle<float>(w - 10.0f, 3.0f, 10.0f, 10.0f);
    g.setColour(clipHold_ > 0 ? C::clip : C::clipOff);
    g.fillRoundedRectangle(led, 2.0f);
    g.setColour(C::groove);
    g.drawRoundedRectangle(led.reduced(0.5f), 2.0f, 1.0f);
    Console::drawValue(g, ParamFormat::valueText(volume_.getParameterID(), volume_.convertFrom0to1(volume_.getValue())),
                       { w - 130.0f, 0.0f, 112.0f, 16.0f }, 15.0f, juce::Justification::centredRight, C::lcd);
    paintMeter(g, { 0.0f, 20.0f, w, 9.0f }, level_[0]);
    paintMeter(g, { 0.0f, 31.0f, w, 9.0f }, level_[1]);
}
