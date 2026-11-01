#include "Console.h"
#include "ParamFormat.h"
#include "LookAndFeel/GranoLAF.h"

using namespace Theme;

namespace Console
{
// ── Touch reporting ───────────────────────────────────────────────────────────
void notifyTouched(juce::Component& from, juce::RangedAudioParameter& p, float prev)
{
    if (auto* s = from.findParentComponentOfClass<TouchSink>())
        s->controlTouched(from, p, prev);
}

void notifyChanged(juce::Component& from, juce::RangedAudioParameter& p)
{
    if (auto* s = from.findParentComponentOfClass<TouchSink>())
        s->controlChanged(p);
}

// ── Drawing helpers ───────────────────────────────────────────────────────────
float textWidth(const juce::Font& f, const juce::String& t)
{
    return juce::GlyphArrangement::getStringWidth(f, t);
}

juce::String midTruncate(const juce::Font& f, const juce::String& text, float maxWidth)
{
    if (textWidth(f, text) <= maxWidth)
        return text;
    const int dot = text.lastIndexOfChar('.');
    const auto ext  = dot > 0 ? text.substring(dot) : juce::String();
    auto stem = dot > 0 ? text.substring(0, dot) : text;
    const auto ell = juce::String::fromUTF8("\xe2\x80\xa6");
    while (stem.isNotEmpty() && textWidth(f, stem + ell + ext) > maxWidth)
        stem = stem.dropLastCharacters(1);
    return stem + ell + ext;
}

void drawStrip(juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour(C::lcdBg);
    g.fillRoundedRectangle(r, kStripRadius);
    g.setColour(C::glint);
    g.fillRect(r.reduced(2.0f, 0.0f).withHeight(1.0f).translated(0.0f, 1.0f));
    g.setColour(C::groove);
    g.drawRoundedRectangle(r.reduced(0.5f), kStripRadius, 1.0f);
}

void drawValue(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
               float numberPt, juce::Justification just, juce::Colour colour)
{
    const auto parts = ParamFormat::split(text);
    const auto nf = font(Face::lcd, numberPt);
    const auto uf = font(Face::silk, 13.0f);
    const float gap = parts.unit.isEmpty() ? 0.0f : 3.0f;
    float nw = textWidth(nf, parts.number);
    const float uw = parts.unit.isEmpty() ? 0.0f : textWidth(uf, parts.unit);

    // Too wide: squeeze the number, keep the unit.
    float hscale = 1.0f;
    if (nw + gap + uw > area.getWidth() && nw > 0.0f)
    {
        hscale = juce::jmax(0.6f, (area.getWidth() - gap - uw) / nw);
        nw *= hscale;
    }
    const float total = nw + gap + uw;
    float x = area.getX();
    if (just.testFlags(juce::Justification::horizontallyCentred)) x = area.getCentreX() - total * 0.5f;
    else if (just.testFlags(juce::Justification::right))          x = area.getRight() - total;

    g.setColour(colour);
    g.setFont(nf.withHorizontalScale(hscale));
    g.drawText(parts.number, juce::Rectangle<float>(x, area.getY(), nw + 1.0f, area.getHeight()),
               juce::Justification::centredLeft, false);
    if (parts.unit.isNotEmpty())
    {
        g.setColour(colour.withMultipliedAlpha(0.8f));
        g.setFont(uf);
        g.drawText(parts.unit, juce::Rectangle<float>(x + nw + gap, area.getY(), uw + 1.0f, area.getHeight()),
                   juce::Justification::centredLeft, false);
    }
}

void drawSilk(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
              juce::Justification just, juce::Colour colour)
{
    g.setColour(colour);
    g.setFont(font(Face::silk, 13.0f, 0.08f));
    g.drawFittedText(text.toUpperCase(), area.toNearestInt(), just, 1, 0.7f);
}

void drawFocus(juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour(C::lcd);
    g.drawRoundedRectangle(r.reduced(1.0f), kKeyRadius, 2.0f);
}

void collectControls(juce::Component& c, std::vector<ParamControl*>& out)
{
    for (auto* child : c.getChildren())
    {
        if (auto* pc = dynamic_cast<ParamControl*>(child))
            out.push_back(pc);
        collectControls(*child, out);
    }
}

// ── ParamControl ──────────────────────────────────────────────────────────────
namespace
{
    class ParamValue final : public juce::AccessibilityValueInterface
    {
    public:
        explicit ParamValue(ParamControl& c) : c_(c) {}
        bool isReadOnly() const override { return false; }
        double getCurrentValue() const override { return c_.getValue(); }
        void setValue(double v) override { c_.setValueFromUser((float)v); }
        juce::String getCurrentValueAsString() const override { return c_.getValueText(); }
        void setValueAsString(const juce::String& t) override { setValue(t.retainCharacters("-0123456789.").getDoubleValue()); }
        AccessibleValueRange getRange() const override
        {
            const auto& r = c_.getParam().getNormalisableRange();
            return { { r.start, r.end }, r.interval > 0.0f ? r.interval : (r.end - r.start) / 100.0 };
        }
    private:
        ParamControl& c_;
    };
}

ParamControl::ParamControl(juce::RangedAudioParameter& p, juce::Colour edge, juce::String label)
    : param_(p), edge_(edge), label_(std::move(label)),
      attachment_(p, [this](float v) { value_ = v; repaint(); }, nullptr)
{
    setTitle(ParamFormat::fullName(p.getParameterID()));
    setDescription(ParamFormat::hint(p.getParameterID()));
    setTooltip(ParamFormat::hint(p.getParameterID()));
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(false);
    setRepaintsOnMouseActivity(false);
    attachment_.sendInitialUpdate();
}

ParamControl::~ParamControl() = default;

juce::String ParamControl::getValueText() const
{
    return ParamFormat::valueText(param_.getParameterID(), value_);
}

bool ParamControl::isBipolar() const
{
    const auto& r = param_.getNormalisableRange();
    return r.start < 0.0f && r.end > 0.0f;
}

void ParamControl::setModDepth(float d)
{
    if (std::abs(d - modDepth_) > 1.0e-4f)
    {
        modDepth_ = d;
        repaint();
    }
}

void ParamControl::setValueFromUser(float v)
{
    const float prev = value_;
    notifyTouched(*this, param_, prev);
    attachment_.setValueAsCompleteGesture(v);
    notifyChanged(*this, param_);
}

float ParamControl::stepNorm() const
{
    const auto& r = param_.getNormalisableRange();
    return r.interval > 0.0f ? r.interval / (r.end - r.start) : 0.0f;
}

void ParamControl::setNormInGesture(float n)
{
    dragNorm_ = juce::jlimit(0.0f, 1.0f, n);
    const float v = param_.convertFrom0to1(dragNorm_);
    if (v != value_)
    {
        attachment_.setValueAsPartOfGesture(v);
        notifyChanged(*this, param_);
    }
}

void ParamControl::mouseDown(const juce::MouseEvent& e)
{
    if (!e.mods.isLeftButtonDown())
        return;
    inGesture_ = true;
    notifyTouched(*this, param_, value_);
    attachment_.beginGesture();
    dragStart_ = e.position;
    dragNorm_ = dragStartNorm_ = getNorm();

    const auto span = jumpSpan();
    if (!span.isEmpty())
    {
        const float pos = axis_ == Axis::vertical ? e.position.y : e.position.x;
        float n = (pos - span.getStart()) / span.getLength();
        if (axis_ == Axis::vertical) n = 1.0f - n;
        if (std::abs(n - dragStartNorm_) > 0.06f)
        {
            setNormInGesture(n);
            dragStartNorm_ = dragNorm_;
        }
    }
}

void ParamControl::mouseDrag(const juce::MouseEvent& e)
{
    if (!inGesture_)
        return;
    const float d = axis_ == Axis::vertical ? dragStart_.y - e.position.y : e.position.x - dragStart_.x;
    const float fine = e.mods.isCommandDown() || e.mods.isCtrlDown() ? 0.1f : 1.0f;
    setNormInGesture(dragStartNorm_ + d / dragPx_ * fine);
}

void ParamControl::mouseUp(const juce::MouseEvent&)
{
    if (!inGesture_)
        return;
    inGesture_ = false;
    attachment_.endGesture();
}

void ParamControl::mouseDoubleClick(const juce::MouseEvent&)
{
    setValueFromUser(param_.convertFrom0to1(param_.getDefaultValue()));
}

void ParamControl::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    const float dir = (w.deltaY != 0.0f ? w.deltaY : w.deltaX) * (w.isReversed ? -1.0f : 1.0f);
    if (dir == 0.0f)
        return;
    const float step = juce::jmax(stepNorm(), 0.01f) * (dir > 0.0f ? 1.0f : -1.0f);
    setValueFromUser(param_.convertFrom0to1(juce::jlimit(0.0f, 1.0f, getNorm() + step)));
}

bool ParamControl::keyPressed(const juce::KeyPress& k)
{
    int dir = 0;
    if (k.isKeyCode(juce::KeyPress::upKey) || k.isKeyCode(juce::KeyPress::rightKey)) dir = 1;
    if (k.isKeyCode(juce::KeyPress::downKey) || k.isKeyCode(juce::KeyPress::leftKey)) dir = -1;
    if (dir == 0)
        return false;
    const float base = stepNorm() > 0.0f ? stepNorm() : 0.02f;
    const float step = k.getModifiers().isShiftDown() && stepNorm() == 0.0f ? 0.1f : base;
    setValueFromUser(param_.convertFrom0to1(juce::jlimit(0.0f, 1.0f, getNorm() + step * (float)dir)));
    return true;
}

std::unique_ptr<juce::AccessibilityHandler> ParamControl::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler>(
        *this, juce::AccessibilityRole::slider, juce::AccessibilityActions{},
        juce::AccessibilityHandler::Interfaces{ std::make_unique<ParamValue>(*this) });
}

// ── Fader ─────────────────────────────────────────────────────────────────────
namespace
{
    constexpr float kStripH  = 44.0f;
    constexpr float kTrackPad = 10.0f;

    void drawModStrip(juce::Graphics& g, juce::Rectangle<float> strip, bool markerBottom, float marker)
    {
        g.setColour(C::mMod);
        g.fillRect(strip.reduced(1.0f).removeFromBottom(3.0f));
        const float x = strip.getRight() - marker - (markerBottom ? 3.0f : 4.0f);
        const float y = markerBottom ? strip.getBottom() - marker - 6.0f : strip.getY() + (marker > 6.0f ? 4.0f : 3.0f);
        g.fillRoundedRectangle(x, y, marker, marker, 1.0f);
    }
}

juce::Rectangle<float> Fader::track() const
{
    return getLocalBounds().toFloat().withTrimmedTop(kStripH + 6.0f);
}

juce::Range<float> Fader::jumpSpan() const
{
    const auto t = track();
    return { t.getY() + kTrackPad, t.getBottom() - kTrackPad };
}

void Fader::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    auto strip = b.withHeight(kStripH);
    drawStrip(g, strip);
    const bool mod = modDepth_ > 0.0f;
    if (mod) drawModStrip(g, strip, false, 7.0f);

    const auto inner = strip.reduced(6.0f, 4.0f);
    g.setColour(C::lcdName);
    g.setFont(font(Face::silk, 13.0f, 0.06f));
    g.drawFittedText(label_.toUpperCase(), inner.withHeight(16.0f).withTrimmedRight(mod ? 9.0f : 0.0f).toNearestInt(),
                     juce::Justification::centredLeft, 1, 0.7f);
    drawValue(g, getValueText(), inner.withTrimmedTop(18.0f).withHeight(18.0f), 15.0f,
              juce::Justification::centredLeft, C::lcd);

    // Track well
    const auto t = track();
    g.setColour(C::well);
    g.fillRoundedRectangle(t, kKeyRadius);
    g.setColour(C::groove.withAlpha(0.5f));
    g.fillRect(t.reduced(2.0f, 0.0f).withHeight(1.0f));
    const float top = t.getY() + kTrackPad, bot = t.getBottom() - kTrackPad, cx = t.getCentreX();
    for (int i = 0; i <= 10; ++i)
    {
        const float y = std::round(top + (bot - top) * (float)i / 10.0f);
        g.setColour(C::tick);
        g.fillRect(t.getX() + 6.0f, y, 7.0f, 1.0f);
        g.fillRect(t.getRight() - 13.0f, y, 7.0f, 1.0f);
    }
    g.setColour(C::groove);
    g.fillRect(cx - 1.0f, top, 2.0f, bot - top);
    g.setColour(C::grooveHi);
    g.fillRect(cx + 1.0f, top, 1.0f, bot - top);

    const float n = getNorm();
    const float capY = top + (1.0f - n) * (bot - top);
    g.setColour(edge_.withAlpha(0.75f));
    if (isBipolar())
    {
        const float mid = (top + bot) * 0.5f;
        g.fillRect(cx - 2.0f, juce::jmin(mid, capY), 4.0f, std::abs(mid - capY));
    }
    else
        g.fillRect(cx - 2.0f, capY, 4.0f, bot - capY);

    if (mod)
    {
        const float a = modDepth_;
        const float y0 = top + (1.0f - juce::jmin(1.0f, n + a)) * (bot - top);
        const float y1 = top + (1.0f - juce::jmax(0.0f, n - a)) * (bot - top);
        g.setColour(C::mMod);
        g.fillRect(cx - 7.0f, y0, 14.0f, 1.5f);
        g.fillRect(cx - 7.0f, y1 - 1.5f, 14.0f, 1.5f);
    }

    const auto cap = juce::Rectangle<float>(38.0f, 22.0f).withCentre({ cx, capY });
    g.setColour(C::shadow);
    g.fillRoundedRectangle(cap.translated(0.0f, 3.0f), kKeyRadius);
    g.setColour(isMouseOver() ? C::capHi : C::cap);
    g.fillRoundedRectangle(cap, kKeyRadius);
    g.setColour(C::live);
    g.fillRoundedRectangle(cap.reduced(4.0f, 0.0f).withSizeKeepingCentre(cap.getWidth() - 8.0f, 2.0f), 1.0f);

    if (hasKeyboardFocus(false)) drawFocus(g, t.expanded(1.0f));
}

// ── Encoder ───────────────────────────────────────────────────────────────────
Encoder::Encoder(juce::RangedAudioParameter& p, juce::Colour edge, juce::String label)
    : ParamControl(p, edge, std::move(label))
{
    setDrag(Axis::vertical, 160.0f);
}

void Encoder::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const juce::Point<float> c(b.getCentreX(), 17.0f);
    constexpr float A0 = -0.75f * juce::MathConstants<float>::pi;
    constexpr float A1 =  0.75f * juce::MathConstants<float>::pi;
    const float a = A0 + (A1 - A0) * getNorm();

    if (modDepth_ > 0.0f)
    {
        juce::Path arc;
        const float span = modDepth_ * (A1 - A0);
        arc.addCentredArc(c.x, c.y, 21.0f, 21.0f, 0.0f, juce::jmax(A0, a - span), juce::jmin(A1, a + span), true);
        g.setColour(C::mMod);
        g.strokePath(arc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const auto knob = juce::Rectangle<float>(34.0f, 34.0f).withCentre(c);
    g.setColour(C::shadow);
    g.fillEllipse(knob.translated(0.0f, 3.0f));
    g.setColour(isMouseOver() ? C::knob.brighter(0.08f) : C::knob);
    g.fillEllipse(knob);
    g.setColour(C::edge);
    g.drawEllipse(knob.reduced(0.5f), 1.0f);

    juce::Path ind;
    ind.startNewSubPath(c.x, c.y - 3.0f);
    ind.lineTo(c.x, c.y - 15.0f);
    g.setColour(C::live);
    g.strokePath(ind, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                 juce::AffineTransform::rotation(a, c.x, c.y));

    const auto strip = b.withTrimmedTop(39.0f);
    drawStrip(g, strip);
    if (modDepth_ > 0.0f) drawModStrip(g, strip, true, 5.0f);
    const auto inner = strip.reduced(4.0f, 3.0f);
    g.setColour(C::lcdName);
    g.setFont(font(Face::silk, 13.0f, 0.01f));
    g.drawFittedText(label_.toUpperCase(), inner.withHeight(16.0f).toNearestInt(), juce::Justification::centred, 1, 0.7f);
    drawValue(g, getValueText(), inner.withTrimmedTop(17.0f).withHeight(17.0f), 14.0f,
              juce::Justification::centred, C::lcd);

    if (hasKeyboardFocus(false)) drawFocus(g, knob.expanded(6.0f));
}

// ── ValueStrip ────────────────────────────────────────────────────────────────
ValueStrip::ValueStrip(juce::RangedAudioParameter& p, juce::Colour edge, juce::String label)
    : ParamControl(p, edge, std::move(label))
{
    setDrag(Axis::horizontal, 120.0f);
}

void ValueStrip::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    drawStrip(g, b);
    if (isMouseOver()) { g.setColour(C::line2); g.drawRoundedRectangle(b.reduced(0.5f), kStripRadius, 1.0f); }
    g.setColour(edge_.withAlpha(0.8f));
    g.fillRect(b.getX(), b.getBottom() - 2.0f, b.getWidth() * getNorm(), 2.0f);
    if (modDepth_ > 0.0f) drawModStrip(g, b, false, 5.0f);
    const auto inner = b.reduced(6.0f, 0.0f);
    g.setColour(C::lcdName);
    g.setFont(font(Face::silk, 13.0f, 0.05f));
    g.drawText(label_.toUpperCase(), inner, juce::Justification::centredLeft, false);
    drawValue(g, getValueText(), inner.withTrimmedLeft(textWidth(font(Face::silk, 13.0f, 0.05f), label_.toUpperCase()) + 4.0f)
                                      .withTrimmedRight(modDepth_ > 0.0f ? 6.0f : 0.0f),
              14.0f, juce::Justification::centredRight, C::lcd);
    if (hasKeyboardFocus(false)) drawFocus(g, b.expanded(1.0f));
}

// ── HFader ────────────────────────────────────────────────────────────────────
HFader::HFader(juce::RangedAudioParameter& p, juce::Colour edge, juce::String label)
    : ParamControl(p, edge, std::move(label))
{
    setDrag(Axis::horizontal, 200.0f);
}

juce::Range<float> HFader::jumpSpan() const { return { 8.0f, (float)getWidth() - 8.0f }; }

void HFader::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour(C::well);
    g.fillRoundedRectangle(b, kKeyRadius);
    const float cy = b.getCentreY(), x0 = 8.0f, x1 = b.getWidth() - 8.0f;
    g.setColour(C::groove);
    g.fillRect(x0, cy - 1.0f, x1 - x0, 2.0f);
    g.setColour(C::grooveHi);
    g.fillRect(x0, cy + 1.0f, x1 - x0, 1.0f);
    const float capX = x0 + getNorm() * (x1 - x0);
    g.setColour(edge_.withAlpha(0.75f));
    g.fillRoundedRectangle(x0, cy - 2.0f, capX - x0, 4.0f, 1.0f);
    const auto cap = juce::Rectangle<float>(14.0f, juce::jmin(18.0f, b.getHeight() - 2.0f)).withCentre({ capX, cy });
    g.setColour(C::shadow);
    g.fillRoundedRectangle(cap.translated(0.0f, 2.0f), 2.0f);
    g.setColour(isMouseOver() ? C::capHi : C::cap);
    g.fillRoundedRectangle(cap, 2.0f);
    g.setColour(C::live);
    g.fillRect(capX - 1.0f, cap.getY() + 3.0f, 2.0f, cap.getHeight() - 6.0f);
    if (hasKeyboardFocus(false)) drawFocus(g, b.expanded(1.0f));
}

// ── BipolarBar ────────────────────────────────────────────────────────────────
BipolarBar::BipolarBar(juce::RangedAudioParameter& p, juce::Colour edge, juce::String label)
    : ParamControl(p, edge, std::move(label))
{
    setDrag(Axis::horizontal, 90.0f);
}

juce::Range<float> BipolarBar::jumpSpan() const { return { 0.0f, (float)getWidth() }; }

void BipolarBar::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour(C::well);
    g.fillRoundedRectangle(b, kStripRadius);
    const float mid = b.getCentreX();
    g.setColour(C::midTick);
    g.fillRect(mid - 0.5f, b.getBottom() - 7.0f, 1.0f, 6.0f);
    const float n = getNorm();
    const float x = b.getWidth() * n;
    g.setColour(edge_.withAlpha(0.85f));
    g.fillRoundedRectangle(juce::jmin(mid, x), b.getBottom() - 6.0f, std::abs(x - mid), 4.0f, 1.0f);
    drawValue(g, getValueText(), b.withTrimmedBottom(3.0f), 13.0f, juce::Justification::centred, C::silk);
    if (hasKeyboardFocus(false)) drawFocus(g, b.expanded(1.0f));
}

// ── Key ───────────────────────────────────────────────────────────────────────
Key::Key(const juce::String& caption, juce::Colour litColour)
    : juce::Button(caption), lit_(litColour)
{
    setMouseClickGrabsKeyboardFocus(false);
    setWantsKeyboardFocus(true);
    setTitle(caption);
}

void Key::paintButton(juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced(0.5f).withTrimmedBottom(1.0f);
    const bool lit = getToggleState();
    GranoLAF::paintKey(g, r, lit, lit_, over && isEnabled(), down);
    if (down) r = r.translated(0.0f, 1.0f);
    const auto ink = (lit ? C::tapeInk : inkOff_).withMultipliedAlpha(isEnabled() ? 1.0f : 0.35f);
    const auto caption = getButtonText().toUpperCase();

    if (icon_)
    {
        if (iconAbove_ && caption.isNotEmpty())
        {
            auto ir = r.reduced(4.0f, 6.0f);
            const auto tr = ir.removeFromBottom(15.0f);
            icon_(g, ir, ink);
            g.setColour(ink);
            g.setFont(font(face_, pt_, 0.03f));
            g.drawFittedText(caption, tr.toNearestInt(), juce::Justification::centred, 1, 0.75f);
        }
        else
            icon_(g, r, ink);
    }
    else
    {
        g.setColour(ink);
        g.setFont(font(face_, pt_, 0.1f));
        g.drawFittedText(caption, r.toNearestInt().reduced(3, 0), juce::Justification::centred, 1, 0.75f);
    }
    if (hasKeyboardFocus(false)) drawFocus(g, getLocalBounds().toFloat());
}

// ── ChoiceKeys ────────────────────────────────────────────────────────────────
ChoiceKeys::ChoiceKeys(juce::RangedAudioParameter& p, juce::StringArray labels, juce::Colour lit,
                       int columns, std::vector<int> values, bool clearable, int clearValue)
    : param_(p),
      attachment_(p, [this](float) { refresh(); }, nullptr),
      values_(std::move(values)), columns_(juce::jmax(1, columns)), clearValue_(clearValue), clearable_(clearable)
{
    const auto id = p.getParameterID();
    for (int i = 0; i < labels.size(); ++i)
    {
        if ((int)values_.size() <= i)
            values_.push_back(i);
        auto k = std::make_unique<Key>(labels[i], lit);
        const int v = values_[(size_t)i];
        const auto choice = ParamFormat::valueText(id, (float)v);
        k->setTitle(ParamFormat::fullName(id) + " " + choice);
        k->setTooltip(ParamFormat::fullName(id) + ": " + choice);
        k->onClick = [this, v]
        {
            const float prev = param_.convertFrom0to1(param_.getValue());
            const int cur = juce::roundToInt(prev);
            const float next = (clearable_ && cur == v) ? (float)clearValue_ : (float)v;
            notifyTouched(*this, param_, prev);
            attachment_.setValueAsCompleteGesture(next);
            notifyChanged(*this, param_);
            if (onSelect) onSelect(juce::roundToInt(next));
        };
        addAndMakeVisible(*k);
        keys_.push_back(std::move(k));
    }
    attachment_.sendInitialUpdate();
}

ChoiceKeys::~ChoiceKeys() = default;

void ChoiceKeys::refresh()
{
    selected_ = juce::roundToInt(param_.convertFrom0to1(param_.getValue()));
    for (size_t i = 0; i < keys_.size(); ++i)
        keys_[i]->setToggleState(values_[i] == selected_, juce::dontSendNotification);
}

void ChoiceKeys::resized()
{
    const int n = (int)keys_.size();
    const int rows = (n + columns_ - 1) / columns_;
    const float kw = ((float)getWidth() - (float)(gap_ * (columns_ - 1))) / (float)columns_;
    const float kh = ((float)getHeight() - (float)(gap_ * (rows - 1))) / (float)rows;
    for (int i = 0; i < n; ++i)
    {
        const int r = i / columns_, c = i % columns_;
        keys_[(size_t)i]->setBounds(juce::Rectangle<float>((kw + (float)gap_) * (float)c, (kh + (float)gap_) * (float)r, kw, kh)
                                        .toNearestInt());
    }
}

// ── ToggleKey ─────────────────────────────────────────────────────────────────
ToggleKey::ToggleKey(juce::RangedAudioParameter& p, juce::Colour lit, juce::String textOn, juce::String textOff)
    : Key(textOff, lit), param_(p),
      attachment_(p, [this](float v)
      {
          const bool on = v > 0.5f;
          setToggleState(on, juce::dontSendNotification);
          setButtonText(on ? on_ : off_);
          if (onState) onState(on);
      }, nullptr),
      on_(std::move(textOn)), off_(std::move(textOff))
{
    setTitle(ParamFormat::fullName(p.getParameterID()));
    setTooltip(ParamFormat::hint(p.getParameterID()));
    setClickingTogglesState(false);
    attachment_.sendInitialUpdate();
}

ToggleKey::~ToggleKey() = default;

void ToggleKey::clicked()
{
    const float prev = getToggleState() ? 1.0f : 0.0f;
    notifyTouched(*this, param_, prev);
    attachment_.setValueAsCompleteGesture(1.0f - prev);
    notifyChanged(*this, param_);
}

// ── ChoiceSelect ──────────────────────────────────────────────────────────────
ChoiceSelect::ChoiceSelect(juce::RangedAudioParameter& p, juce::StringArray labels, bool silkType)
    : param_(p),
      attachment_(p, [this](float v)
      {
          const juce::ScopedValueSetter<bool> svs(updating_, true);
          setSelectedItemIndex(juce::roundToInt(v), juce::dontSendNotification);
      }, nullptr)
{
    if (labels.isEmpty())
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(&p))
            labels = c->choices;
    addItemList(labels, 1);
    getProperties().set("silk", silkType);
    setTitle(ParamFormat::fullName(p.getParameterID()));
    setTooltip(ParamFormat::hint(p.getParameterID()));
    setMouseClickGrabsKeyboardFocus(false);
    onChange = [this]
    {
        if (updating_) return;
        const float prev = param_.convertFrom0to1(param_.getValue());
        notifyTouched(*this, param_, prev);
        attachment_.setValueAsCompleteGesture((float)getSelectedItemIndex());
        notifyChanged(*this, param_);
    };
    attachment_.sendInitialUpdate();
}

ChoiceSelect::~ChoiceSelect() = default;

// ── Bank ──────────────────────────────────────────────────────────────────────
namespace
{
    juce::Font tapeFont() { return font(Face::silkBold, 13.0f, 0.14f); }
}

Bank::Bank(juce::String title, juce::Colour edge)
    : title_(std::move(title)), edge_(edge)
{
    setTitle(title_);
    setFocusContainerType(FocusContainerType::focusContainer);
    addAndMakeVisible(body_);
}

Bank::~Bank() = default;

void Bank::setOnParam(juce::RangedAudioParameter& p)
{
    onKey_ = std::make_unique<ToggleKey>(p, edge_);
    onKey_->onState = [this](bool on)
    {
        off_ = !on;
        body_.setAlpha(on ? 1.0f : kOffAlpha);
        repaint();
    };
    onKey_->setTitle(title_ + " on/off");
    addAndMakeVisible(*onKey_);
    off_ = !onKey_->getToggleState();
    body_.setAlpha(off_ ? kOffAlpha : 1.0f);
    resized();
}

void Bank::setTouched(bool t)
{
    if (t != touched_) { touched_ = t; repaint(); }
}

void Bank::resized()
{
    body_.setBounds(0, kBodyTop, getWidth(), getHeight() - kBodyTop);
    if (onKey_)
    {
        const int tx = 10 + 16 + (int)std::ceil(textWidth(tapeFont(), title_.toUpperCase())) + 8;
        onKey_->setBounds(tx, 2, 38, 18);
    }
}

void Bank::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour(C::plate);
    g.fillRoundedRectangle(b, kPlateRadius);
    g.setColour(touched_ ? C::line2 : C::line);
    g.drawRoundedRectangle(b.reduced(0.5f), kPlateRadius, 1.0f);

    const auto tf = tapeFont();
    const auto text = title_.toUpperCase();
    const float tw = textWidth(tf, text);
    const float w = 16.0f + tw + 10.0f + (onKey_ ? 8.0f + 38.0f : 0.0f);
    juce::Path tape;
    tape.addRoundedRectangle(10.0f, 0.0f, w, (float)kTapeH, 2.0f, 2.0f, false, false, true, true);
    g.setColour(C::tape);
    g.fillPath(tape);
    g.setColour(edge_);
    g.fillRect(10.0f, 0.0f, 7.0f, (float)kTapeH);
    g.setColour(C::tapeInk);
    g.setFont(tf);
    g.drawText(text, juce::Rectangle<float>(26.0f, 0.0f, tw + 2.0f, (float)kTapeH), juce::Justification::centredLeft, false);
    if (off_)
        g.fillRect(25.0f, (float)kTapeH * 0.5f - 1.0f, tw + 2.0f, 2.0f);
}

// ── SubPanel ──────────────────────────────────────────────────────────────────
SubPanel::SubPanel(juce::String title, juce::RangedAudioParameter& onParam, juce::Colour edge, bool leftSeam)
    : title_(std::move(title)), leftSeam_(leftSeam), onKey_(onParam, edge)
{
    addAndMakeVisible(onKey_);
    addAndMakeVisible(panel_);
    onKey_.onState = [this](bool on) { panel_.setAlpha(on ? 1.0f : kOffAlpha); };
    panel_.setAlpha(onKey_.getToggleState() ? 1.0f : kOffAlpha);
}

void SubPanel::resized()
{
    const int x0 = leftSeam_ ? 8 : 0;
    onKey_.setBounds(x0, 30, 40, 18);
    panel_.setBounds(x0 + 76, 6, getWidth() - x0 - 76, getHeight() - 6);
}

void SubPanel::paint(juce::Graphics& g)
{
    g.setColour(C::line);
    g.fillRect(0, 0, getWidth(), 1);
    if (leftSeam_)
        g.fillRect(0, 0, 1, getHeight());
    drawSilk(g, title_, juce::Rectangle<float>(leftSeam_ ? 8.0f : 0.0f, 10.0f, 72.0f, 14.0f));
}
}
