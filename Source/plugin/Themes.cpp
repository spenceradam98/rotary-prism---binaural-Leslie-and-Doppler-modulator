#include "Themes.h"

using namespace juce;

namespace
{
constexpr float kStart = MathConstants<float>::pi * -0.75f, kEnd = MathConstants<float>::pi * 0.75f;

const Theme kThemes[] = {
    { "Spectrum",   Colour (0xff0b0b0d), Colour (0xff16161a), Colour (0xff121216), Colour (0xff2a2a31), Colour (0xfff2f2f2), Colour (0xff8a8a94),
                    Colour (0xffffffff), Colour (0xff1d1d22), Colour (0xff2c2c33), Colour (0xffffffff), 0, true, false },
    { "Console",    Colour (0xff383b40), Colour (0xff2c2e33), Colour (0xff474b52), Colour (0xff25272b), Colour (0xffefe8d8), Colour (0xffb8b2a4),
                    Colour (0xffe8a33a), Colour (0xff141414), Colour (0xff2a2a2a), Colour (0xffefe8d8), 1, false, false },
    { "Eclipse",    Colour (0xff06070d), Colour (0xff0d0f1d), Colour (0xff0e1020), Colour (0xff252a4a), Colour (0xffe8e6ff), Colour (0xff8a88b0),
                    Colour (0xffff9a3c), Colour (0xff0a0b16), Colour (0xff171a30), Colour (0xffffc88a), 2, false, false },
    { "Moon Dust",  Colour (0xffd5d6da), Colour (0xffc4c6cc), Colour (0xffededf0), Colour (0xffb0b2b9), Colour (0xff1d1e22), Colour (0xff666870),
                    Colour (0xff3a6df0), Colour (0xff1a1a1d), Colour (0xff303035), Colour (0xffffffff), 3, false, true },
    { "Tape Room",  Colour (0xff4a2e1c), Colour (0xff37220f), Colour (0xffefe4cc), Colour (0xffb59c74), Colour (0xff3a2a1a), Colour (0xff7a654a),
                    Colour (0xffb8401f), Colour (0xffd9dadc), Colour (0xff8d9096), Colour (0xff2a2a2a), 4, false, true },
    { "Kaleido",    Colour (0xff07060c), Colour (0xff120f1f), Colour (0xff100d1c), Colour (0xff2c2546), Colour (0xfff6f0ff), Colour (0xff9a90bb),
                    Colour (0xffc77dff), Colour (0xff15111f), Colour (0xff272038), Colour (0xffffffff), 5, true, false },
};

// spectral colours for the sections (Spectrum theme)
const Colour kSpectrum[8] = { Colour (0xffff4b4b), Colour (0xffff9a3c), Colour (0xffffd83c), Colour (0xff58d66a),
                              Colour (0xff3cc8e8), Colour (0xff4b7bff), Colour (0xff8a5bff), Colour (0xffd25bff) };
} // namespace

const Theme& themeAt (int i) { return kThemes[jlimit (0, numThemes() - 1, i)]; }
int numThemes() { return (int) (sizeof (kThemes) / sizeof (kThemes[0])); }

Colour sectionColour (const Theme& t, int section, int numSections)
{
    if (t.style == 5)   // Kaleido: hues spread evenly around the colour wheel
        return Colour::fromHSV ((float) section / (float) jmax (1, numSections) * 0.85f, 0.62f, 1.0f, 1.0f);
    if (t.spectrumAccents)
        return kSpectrum[jlimit (0, 7, (int) std::round ((double) section * 7.0 / jmax (1, numSections - 1)))];
    return t.accent;
}

// ============================================================================ LookAndFeel
void PrismLookAndFeel::setTheme (int index)
{
    th = &themeAt (index);
    const auto& t = *th;
    const Colour boxBg = t.light ? t.panel.darker (0.06f) : t.bg.brighter (0.06f);
    setColour (ComboBox::backgroundColourId, boxBg);
    setColour (ComboBox::textColourId, t.text);
    setColour (ComboBox::outlineColourId, t.panelEdge);
    setColour (ComboBox::arrowColourId, t.dim);
    setColour (PopupMenu::backgroundColourId, t.light ? t.panel : t.bg2);
    setColour (PopupMenu::textColourId, t.text);
    setColour (PopupMenu::headerTextColourId, t.accent);
    setColour (PopupMenu::highlightedBackgroundColourId, t.accent.withAlpha (0.3f));
    setColour (PopupMenu::highlightedTextColourId, t.text);
    setColour (TextButton::textColourOffId, t.text);
    setColour (TextButton::textColourOnId, t.text);
    setColour (Slider::textBoxTextColourId, t.text);
}

void PrismLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float start, float end, Slider& s)
{
    const auto& t = *th;
    const Colour acc = s.findColour (Slider::rotarySliderFillColourId);
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float d = jmin (b.getWidth(), b.getHeight());
    const auto c = b.getCentre();
    const float r = d * 0.5f;
    const float angle = start + pos * (end - start);
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float zeroAngle = bipolar ? start + (float) s.valueToProportionOfLength (0.0) * (end - start) : start;

    auto arc = [&] (float radius, float a0, float a1, Colour col, float thick)
    {
        Path p; p.addCentredArc (c.x, c.y, radius, radius, 0.0f, a0, a1, true);
        g.setColour (col); g.strokePath (p, PathStrokeType (thick, PathStrokeType::curved, PathStrokeType::rounded));
    };
    auto pointer = [&] (float r0, float r1, Colour col, float thick)
    {
        g.setColour (col);
        g.drawLine (Line<float> (c.getPointOnCircumference (r0, angle), c.getPointOnCircumference (r1, angle)), thick);
    };

    switch (t.style)
    {
        case 0:   // Spectrum: flat disc, coloured value arc
        {
            arc (r - 2.0f, start, end, t.panelEdge, 3.0f);
            arc (r - 2.0f, jmin (zeroAngle, angle), jmax (zeroAngle, angle), acc, 3.0f);
            auto body = Rectangle<float> (d * 0.68f, d * 0.68f).withCentre (c);
            g.setColour (t.knob); g.fillEllipse (body);
            g.setColour (t.knobCap); g.drawEllipse (body, 1.0f);
            pointer (d * 0.08f, d * 0.32f, t.pointer, 2.2f);
            break;
        }
        case 1:   // Console: black bakelite with a coloured cap
        {
            g.setColour (Colours::black.withAlpha (0.45f)); g.fillEllipse (Rectangle<float> (d * 0.86f, d * 0.86f).withCentre (c.translated (1.5f, 3.0f)));
            for (int i = 0; i <= 10; ++i)
            {
                const float a = start + (end - start) * i / 10.0f;
                g.setColour (t.dim.withAlpha (0.8f));
                g.drawLine (Line<float> (c.getPointOnCircumference (r, a), c.getPointOnCircumference (r - 3.5f, a)), 1.0f);
            }
            auto skirt = Rectangle<float> (d * 0.82f, d * 0.82f).withCentre (c);
            g.setGradientFill (ColourGradient (Colour (0xff3a3a3a), skirt.getX(), skirt.getY(), Colour (0xff050505), skirt.getRight(), skirt.getBottom(), false));
            g.fillEllipse (skirt);
            auto cap = skirt.reduced (d * 0.17f);
            g.setGradientFill (ColourGradient (acc.brighter (0.3f), cap.getX(), cap.getY(), acc.darker (0.6f), cap.getRight(), cap.getBottom(), false));
            g.fillEllipse (cap);
            pointer (d * 0.12f, d * 0.40f, t.pointer, 2.5f);
            break;
        }
        case 2:   // Eclipse: dark disc, glowing corona arc
        {
            for (int i = 3; i >= 1; --i) arc (r - 3.0f, jmin (zeroAngle, angle), jmax (zeroAngle, angle), acc.withAlpha (0.12f), 3.0f + i * 3.0f);
            arc (r - 3.0f, start, end, t.panelEdge, 2.0f);
            arc (r - 3.0f, jmin (zeroAngle, angle), jmax (zeroAngle, angle), acc, 2.5f);
            auto body = Rectangle<float> (d * 0.64f, d * 0.64f).withCentre (c);
            g.setGradientFill (ColourGradient (t.knobCap, body.getCentreX(), body.getY(), t.knob, body.getCentreX(), body.getBottom(), false));
            g.fillEllipse (body);
            g.setColour (acc.withAlpha (0.35f)); g.drawEllipse (body, 1.0f);
            g.setColour (t.pointer); g.fillEllipse (Rectangle<float> (4.0f, 4.0f).withCentre (c.getPointOnCircumference (d * 0.24f, angle)));
            break;
        }
        case 3:   // Moon Dust: knurled black knob, white line, blue value arc
        {
            arc (r - 1.5f, start, end, t.panelEdge, 2.0f);
            arc (r - 1.5f, jmin (zeroAngle, angle), jmax (zeroAngle, angle), acc, 2.0f);
            auto body = Rectangle<float> (d * 0.76f, d * 0.76f).withCentre (c);
            g.setColour (Colours::black.withAlpha (0.25f)); g.fillEllipse (body.translated (0.0f, 2.0f));
            g.setColour (t.knob); g.fillEllipse (body);
            g.setColour (Colours::white.withAlpha (0.08f));
            for (int i = 0; i < 24; ++i)
            {
                const float a = angle + MathConstants<float>::twoPi * i / 24.0f;
                g.drawLine (Line<float> (c.getPointOnCircumference (d * 0.38f, a), c.getPointOnCircumference (d * 0.32f, a)), 1.2f);
            }
            pointer (d * 0.06f, d * 0.36f, t.pointer, 2.2f);
            break;
        }
        case 5:   // Kaleido: prism disc, rainbow value arc
        {
            const int segs = 24;
            const float a0 = jmin (zeroAngle, angle), a1 = jmax (zeroAngle, angle);
            arc (r - 2.5f, start, end, t.panelEdge, 3.0f);
            for (int i = 0; i < segs; ++i)
            {
                const float s0 = a0 + (a1 - a0) * i / segs, s1 = a0 + (a1 - a0) * (i + 1) / segs;
                if (s1 - s0 < 1e-4f) continue;
                const Colour hue = Colour::fromHSV ((s0 - start) / (end - start) * 0.85f, 0.65f, 1.0f, 1.0f);
                arc (r - 2.5f, s0, s1 + 0.01f, hue.withAlpha (0.18f), 8.0f);
                arc (r - 2.5f, s0, s1 + 0.01f, hue, 3.0f);
            }
            auto body = Rectangle<float> (d * 0.64f, d * 0.64f).withCentre (c);
            g.setGradientFill (ColourGradient (t.knobCap.brighter (0.2f), body.getX(), body.getY(), t.knob, body.getRight(), body.getBottom(), false));
            g.fillEllipse (body);
            Path facet;   // hexagonal prism cap
            for (int i = 0; i < 6; ++i)
            {
                const auto pt = c.getPointOnCircumference (d * 0.22f, angle + MathConstants<float>::twoPi * i / 6.0f);
                if (i == 0) facet.startNewSubPath (pt); else facet.lineTo (pt);
            }
            facet.closeSubPath();
            g.setColour (Colours::white.withAlpha (0.07f)); g.fillPath (facet);
            g.setColour (acc.withAlpha (0.5f)); g.strokePath (facet, PathStrokeType (1.0f));
            pointer (d * 0.08f, d * 0.30f, t.pointer, 2.2f);
            break;
        }
        default:  // Tape Room: chrome skirt knob with black centre
        {
            for (int i = 0; i <= 10; ++i)
            {
                const float a = start + (end - start) * i / 10.0f;
                g.setColour (t.dim);
                g.drawLine (Line<float> (c.getPointOnCircumference (r, a), c.getPointOnCircumference (r - 3.0f, a)), 1.0f);
            }
            auto skirt = Rectangle<float> (d * 0.80f, d * 0.80f).withCentre (c);
            g.setColour (Colours::black.withAlpha (0.35f)); g.fillEllipse (skirt.translated (1.0f, 2.5f));
            g.setGradientFill (ColourGradient (Colour (0xffffffff), skirt.getX(), skirt.getY(), Colour (0xff6a6c70), skirt.getRight(), skirt.getBottom(), false));
            g.fillEllipse (skirt);
            g.setColour (Colour (0xff1a1a1a));
            g.drawLine (Line<float> (c.getPointOnCircumference (d * 0.40f, angle), c.getPointOnCircumference (d * 0.29f, angle)), 2.5f);
            auto cap = skirt.reduced (d * 0.16f);
            g.setGradientFill (ColourGradient (Colour (0xff3a3a3a), cap.getX(), cap.getY(), Colour (0xff0a0a0a), cap.getRight(), cap.getBottom(), false));
            g.fillEllipse (cap);
            pointer (d * 0.04f, d * 0.22f, Colour (0xffefe4cc), 2.0f);
            break;
        }
    }
}

void PrismLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool)
{
    const auto& t = *th;
    const Colour acc = b.findColour (ToggleButton::tickColourId);
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    const bool isPower = b.getProperties()["power"];
    if (isPower)
    {
        // round LED power button
        auto led = Rectangle<float> (14.0f, 14.0f).withCentre (r.getCentre());
        if (on)
        {
            g.setColour (acc.withAlpha (0.25f)); g.fillEllipse (led.expanded (4.0f));
            g.setColour (acc); g.fillEllipse (led);
            g.setColour (Colours::white.withAlpha (0.5f)); g.fillEllipse (led.reduced (4.0f).translated (-1.0f, -1.0f));
        }
        else
        {
            g.setColour (t.light ? t.panelEdge : t.bg); g.fillEllipse (led);
            g.setColour (t.dim.withAlpha (highlighted ? 0.9f : 0.5f)); g.drawEllipse (led, 1.2f);
        }
        return;
    }
    // pill switch with label
    g.setColour (on ? acc.withAlpha (0.22f) : (t.light ? t.panel.darker (0.05f) : t.bg.withAlpha (0.6f)));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (on ? acc : t.panelEdge.brighter (highlighted ? 0.3f : 0.0f));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    auto dot = Rectangle<float> (7.0f, 7.0f).withCentre ({ r.getX() + 9.0f, r.getCentreY() });
    g.setColour (on ? acc : t.dim.withAlpha (0.5f)); g.fillEllipse (dot);
    g.setColour (on ? t.text : t.dim);
    g.setFont (FontOptions (11.5f, Font::bold));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (16.0f).withTrimmedRight (3.0f), Justification::centred);
}

void PrismLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool highlighted, bool down)
{
    const auto& t = *th;
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const Colour acc = b.findColour (TextButton::buttonOnColourId);
    const bool on = b.getToggleState();
    g.setColour ((t.light ? t.panel.darker (0.05f) : t.bg.brighter (0.06f)).brighter (down ? 0.15f : 0.0f));
    g.fillRoundedRectangle (r, 4.0f);
    if (on) { g.setColour (acc.withAlpha (0.28f)); g.fillRoundedRectangle (r, 4.0f); }
    g.setColour (on ? acc : (highlighted ? t.accent : t.panelEdge));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

void PrismLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    const auto& t = *th;
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (ComboBox::backgroundColourId)); g.fillRoundedRectangle (r, 4.0f);
    g.setColour (box.hasKeyboardFocus (false) || box.isMouseOver (true) ? t.accent.withAlpha (0.8f) : t.panelEdge);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    Path arrow;
    const float ax = (float) w - 12.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (t.dim); g.fillPath (arrow);
}

Font PrismLookAndFeel::getComboBoxFont (ComboBox&) { return FontOptions (13.0f); }
Font PrismLookAndFeel::getTextButtonFont (TextButton&, int) { return FontOptions (13.0f, Font::bold); }
Font PrismLookAndFeel::getPopupMenuFont() { return FontOptions (14.0f); }
