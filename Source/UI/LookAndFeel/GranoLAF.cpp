#include "GranoLAF.h"

using namespace Theme;

GranoLAF::GranoLAF()
{
    setColour(juce::ResizableWindow::backgroundColourId, C::desk);
    setColour(juce::ComboBox::textColourId,              C::lcd);
    setColour(juce::ComboBox::backgroundColourId,        C::lcdBg);
    setColour(juce::ComboBox::outlineColourId,           C::groove);
    setColour(juce::ComboBox::arrowColourId,             C::lcd);
    setColour(juce::ComboBox::focusedOutlineColourId,    C::lcd);
    setColour(juce::PopupMenu::backgroundColourId,            C::menu);
    setColour(juce::PopupMenu::textColourId,                  C::silk);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, C::tape);
    setColour(juce::PopupMenu::highlightedTextColourId,       C::tapeInk);
    setColour(juce::Label::textColourId,             C::silk);
    setColour(juce::TextButton::buttonColourId,      C::key);
    setColour(juce::TextButton::buttonOnColourId,    C::tape);
    setColour(juce::TextButton::textColourOffId,     C::silk);
    setColour(juce::TextButton::textColourOnId,      C::tapeInk);
    setColour(juce::TooltipWindow::backgroundColourId, C::menu);
    setColour(juce::TooltipWindow::textColourId,       C::silk);
    setColour(juce::TooltipWindow::outlineColourId,    C::groove);
}

void GranoLAF::paintKey(juce::Graphics& g, juce::Rectangle<float> r, bool lit,
                        juce::Colour litColour, bool hover, bool down)
{
    if (down)
        r = r.translated(0.0f, 1.0f);
    else
    {
        g.setColour(C::shadow);
        g.fillRoundedRectangle(r.translated(0.0f, 1.0f), kKeyRadius);
    }
    g.setColour(lit ? litColour : (hover ? C::keyHover : C::key));
    g.fillRoundedRectangle(r, kKeyRadius);
    g.setColour(lit ? C::groove.withAlpha(0.6f) : C::edge);
    g.drawRoundedRectangle(r.reduced(0.5f), kKeyRadius, 1.0f);
}

// ── ComboBox ──────────────────────────────────────────────────────────────────
void GranoLAF::drawComboBox(juce::Graphics& g, int width, int height, bool,
                            int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height);
    g.setColour(C::lcdBg);
    g.fillRoundedRectangle(r, kStripRadius);
    g.setColour(box.hasKeyboardFocus(true) ? C::lcd : (box.isMouseOver(true) ? C::line2 : C::groove));
    g.drawRoundedRectangle(r.reduced(0.5f), kStripRadius, 1.0f);

    // Chevron
    const float cx = (float)width - 12.0f, cy = (float)height * 0.5f;
    juce::Path p;
    p.startNewSubPath(cx - 4.0f, cy - 2.0f);
    p.lineTo(cx, cy + 2.0f);
    p.lineTo(cx + 4.0f, cy - 2.0f);
    g.setColour(C::lcd);
    g.strokePath(p, juce::PathStrokeType(1.5f));
}

juce::Font GranoLAF::getComboBoxFont(juce::ComboBox& box)
{
    // Matrix destination selects use silkscreen type (names), others values.
    if (box.getProperties()["silk"])
        return font(Face::silk, 13.0f, 0.02f);
    return font(Face::lcd, 14.0f);
}

void GranoLAF::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(1, 1, box.getWidth() - 22, box.getHeight() - 2);
    label.setFont(getComboBoxFont(box));
    label.setBorderSize({ 0, 6, 0, 0 });
    label.setMinimumHorizontalScale(0.8f);
}

juce::PopupMenu::Options GranoLAF::getOptionsForComboBoxPopupMenu(juce::ComboBox& box, juce::Label& label)
{
    return LookAndFeel_V4::getOptionsForComboBoxPopupMenu(box, label)
        .withMinimumWidth(juce::jmax(box.getWidth(), 120))
        .withStandardItemHeight(26);
}

// ── PopupMenu ─────────────────────────────────────────────────────────────────
void GranoLAF::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.fillAll(C::menu);
    g.setColour(C::groove);
    g.drawRect(0, 0, width, height, 1);
}

void GranoLAF::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                 bool isSeparator, bool isActive, bool isHighlighted,
                                 bool isTicked, bool, const juce::String& text,
                                 const juce::String&, const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour(C::line);
        g.fillRect(area.reduced(6, 0).withSizeKeepingCentre(area.getWidth() - 12, 1));
        return;
    }
    auto r = area.reduced(2, 1).toFloat();
    if (isHighlighted && isActive)
    {
        g.setColour(C::tape);
        g.fillRoundedRectangle(r, kStripRadius);
    }
    g.setColour(!isActive ? C::disabled : (isHighlighted ? C::tapeInk : (isTicked ? C::lcd : C::silk)));
    g.setFont(getPopupMenuFont());
    g.drawFittedText(text, area.reduced(10, 0), juce::Justification::centredLeft, 1, 0.85f);
}

juce::Font GranoLAF::getPopupMenuFont()
{
    return font(Face::silk, 14.0f, 0.04f);
}

// ── Tooltip ───────────────────────────────────────────────────────────────────
juce::Rectangle<int> GranoLAF::getTooltipBounds(const juce::String& tipText, juce::Point<int> screenPos,
                                                juce::Rectangle<int> parentArea)
{
    juce::TextLayout tl;
    juce::AttributedString s;
    s.append(tipText, font(Face::text, 13.0f), C::silk);
    tl.createLayoutWithBalancedLineLengths(s, 320.0f);
    const int w = (int)tl.getWidth() + 18, h = (int)tl.getHeight() + 12;
    return juce::Rectangle<int>(screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                                screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6,
                                w, h).constrainedWithin(parentArea);
}

void GranoLAF::drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height)
{
    g.fillAll(C::menu);
    g.setColour(C::groove);
    g.drawRect(0, 0, width, height, 1);
    juce::AttributedString s;
    s.append(text, font(Face::text, 13.0f), C::silk);
    juce::TextLayout tl;
    tl.createLayoutWithBalancedLineLengths(s, 320.0f);
    tl.draw(g, juce::Rectangle<float>(9.0f, 6.0f, (float)width - 18.0f, (float)height - 12.0f));
}

// ── TextButton ────────────────────────────────────────────────────────────────
void GranoLAF::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                    bool isMouseOver, bool isButtonDown)
{
    paintKey(g, b.getLocalBounds().toFloat().reduced(0.5f, 0.5f).withTrimmedBottom(1.0f),
             b.getToggleState(), b.findColour(juce::TextButton::buttonOnColourId),
             isMouseOver, isButtonDown);
}

void GranoLAF::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool isButtonDown)
{
    g.setFont(getTextButtonFont(b, b.getHeight()));
    g.setColour(b.findColour(b.getToggleState() ? juce::TextButton::textColourOnId
                                                : juce::TextButton::textColourOffId)
                 .withMultipliedAlpha(b.isEnabled() ? 1.0f : 0.4f));
    auto r = b.getLocalBounds().withTrimmedBottom(1);
    if (isButtonDown) r = r.translated(0, 1);
    g.drawFittedText(b.getButtonText().toUpperCase(), r.reduced(4, 0), juce::Justification::centred, 1, 0.8f);
}

juce::Font GranoLAF::getTextButtonFont(juce::TextButton&, int)
{
    return font(Face::silkBold, 13.0f, 0.1f);
}

juce::Font GranoLAF::getLabelFont(juce::Label&)
{
    return font(Face::silk, 13.0f, 0.12f);
}
