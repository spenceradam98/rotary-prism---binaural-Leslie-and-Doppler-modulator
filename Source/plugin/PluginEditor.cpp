#include "PluginEditor.h"

using namespace juce;

namespace
{
constexpr float kStart = MathConstants<float>::pi * -0.75f, kEnd = MathConstants<float>::pi * 0.75f;
constexpr int kNumSections = 6;

const Theme& themeOf (const Component& c) { return dynamic_cast<PrismLookAndFeel&> (c.getLookAndFeel()).theme(); }
} // namespace

// ============================================================================ Knob
Knob::Knob (RotaryPrismProcessor& p, const String& id, const String& lbl) : label (lbl)
{
    param = p.apvts.getParameter (id);
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (kStart, kEnd, true);
    addAndMakeVisible (slider);
    attach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.apvts, id, slider);
    if (param != nullptr)
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    slider.onValueChange = [this] { repaint(); };
    slider.setTooltip (param != nullptr ? param->getName (64) : lbl);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    r.removeFromBottom (28);
    slider.setBounds (r.withSizeKeepingCentre (jmin (r.getWidth(), r.getHeight()), r.getHeight()));
}

void Knob::paint (Graphics& g)
{
    const auto& t = themeOf (*this);
    auto r = getLocalBounds().removeFromBottom (28);
    g.setColour (t.text);
    g.setFont (FontOptions (11.0f, Font::bold));
    g.drawText (label.toUpperCase(), r.removeFromTop (14), Justification::centred, false);
    g.setColour (slider.isMouseOverOrDragging() ? accent : t.dim);
    g.setFont (FontOptions (11.0f));
    g.drawText (param != nullptr ? param->getCurrentValueAsText() : String(), r, Justification::centred, false);
}

// ============================================================================ Section
Section::Section (RotaryPrismProcessor& p, const String& t, int i) : index (i), proc (p), title (t) {}

Knob& Section::addKnob (const String& id, const String& label)
{
    knobs.push_back (std::make_unique<Knob> (proc, id, label));
    addAndMakeVisible (*knobs.back());
    return *knobs.back();
}

void Section::addChoice (const String& id)
{
    auto box = std::make_unique<ComboBox>();
    if (auto* prm = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter (id)))
        box->addItemList (prm->choices, 1);
    addAndMakeVisible (*box);
    comboAttach.push_back (std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, *box));
    combos.push_back (std::move (box));
}

void Section::addToggle (const String& id, const String& label)
{
    auto b = std::make_unique<ToggleButton> (label);
    addAndMakeVisible (*b);
    toggleAttach.push_back (std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, *b));
    toggles.push_back (std::move (b));
}

void Section::setAccent (Colour c)
{
    accent = c;
    for (auto& k : knobs) { k->accent = c; k->slider.setColour (Slider::rotarySliderFillColourId, c); }
    for (auto& t : toggles) t->setColour (ToggleButton::tickColourId, c);
    repaint();
}

void Section::resized()
{
    auto r = getLocalBounds().reduced (10, 6);
    r.removeFromTop (24);
    const int nCtl = (int) (combos.size() + toggles.size());
    if (nCtl > 0)
    {
        auto row = r.removeFromTop (24);
        const int w = (row.getWidth() - (nCtl - 1) * 6) / nCtl;
        for (auto& c : combos) { c->setBounds (row.removeFromLeft (w)); row.removeFromLeft (6); }
        for (auto& t : toggles) { t->setBounds (row.removeFromLeft (w).withSizeKeepingCentre (jmin (w, 130), row.getHeight())); row.removeFromLeft (6); }
        r.removeFromTop (4);
    }
    else
        r.removeFromTop (8);
    if (knobs.empty()) return;
    const int n = (int) knobs.size();
    const int cw = r.getWidth() / n, rh = jmin (r.getHeight(), 118);
    for (int i = 0; i < n; ++i)
        knobs[(size_t) i]->setBounds (r.getX() + i * cw, r.getY() + (r.getHeight() - rh) / 2, cw, rh);
}

void Section::paint (Graphics& g)
{
    const auto& t = themeOf (*this);
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    if (t.style == 1)   // console: brushed steel with screws
    {
        g.setGradientFill (ColourGradient (t.panel.brighter (0.06f), 0, b.getY(), t.panel.darker (0.08f), 0, b.getBottom(), false));
        g.fillRoundedRectangle (b, 3.0f);
        g.setColour (Colours::white.withAlpha (0.025f));
        for (float y = b.getY() + 2; y < b.getBottom(); y += 3.0f) g.drawHorizontalLine ((int) y, b.getX() + 2, b.getRight() - 2);
        for (auto p : { b.getTopLeft().translated (7, 7), b.getTopRight().translated (-7, 7), b.getBottomLeft().translated (7, -7), b.getBottomRight().translated (-7, -7) })
        {
            g.setColour (Colour (0xff8a8d93)); g.fillEllipse (Rectangle<float> (6, 6).withCentre (p));
            g.setColour (Colour (0xff3a3c40)); g.drawLine (p.x - 2.5f, p.y, p.x + 2.5f, p.y, 1.0f);
        }
    }
    else
    {
        g.setColour (t.panel); g.fillRoundedRectangle (b, 8.0f);
        if (t.style == 2 || t.style == 5)
        {
            g.setGradientFill (ColourGradient (accent.withAlpha (0.10f), b.getCentreX(), b.getY(), Colours::transparentBlack, b.getCentreX(), b.getY() + 90, false));
            g.fillRoundedRectangle (b, 8.0f);
        }
    }
    g.setColour (t.style == 0 || t.style == 5 ? accent.withAlpha (0.45f) : t.panelEdge);
    g.drawRoundedRectangle (b, 8.0f, 1.0f);
    if (t.style == 0 || t.style == 5)
    {
        g.setColour (accent);
        g.fillRoundedRectangle (b.getX() + 10, b.getY() + 1, b.getWidth() - 20, 2.0f, 1.0f);
    }
    g.setColour (t.text);
    g.setFont (FontOptions (14.0f, Font::bold));
    g.drawText (title.toUpperCase(), getLocalBounds().reduced (12, 6).removeFromTop (24), Justification::centredLeft, false);
}

// ============================================================================ CabinetView
CabinetView::CabinetView (RotaryPrismProcessor& p) : proc (p) { setRepaintsOnMouseActivity (true); }

Point<float> CabinetView::centre() const { return getLocalBounds().toFloat().getCentre().translated (0.0f, -6.0f); }
float CabinetView::cabRadius() const { return jmin ((float) getWidth(), (float) getHeight()) * 0.27f; }
float CabinetView::micRadius (float d) const { return cabRadius() * 1.22f + d * cabRadius() * 0.55f; }

float CabinetView::param (const char* id) const
{
    if (auto* p = proc.apvts.getParameter (id)) return p->convertFrom0to1 (p->getValue());
    return 0.0f;
}

void CabinetView::setParam (const char* id, float v)
{
    if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
}

Point<float> CabinetView::micPos (int which) const
{
    const float az = which == 2 ? 0.0f : (which == 0 ? -0.5f : 0.5f) * degreesToRadians (param ("micAngle"));
    const float r = micRadius (param ("micDistance"));
    const auto c = centre();
    return { c.x + r * std::sin (az), c.y + r * std::cos (az) };   // front of the cabinet faces down
}

int CabinetView::hitMic (Point<float> p) const
{
    const int mode = roundToInt (param ("outMode"));
    if (mode == 1) return -1;
    for (int m = 0; m < 3; ++m)
        if ((m == 2) == (mode == 2) && micPos (m).getDistanceFrom (p) < 18.0f) return m;
    return -1;
}

void CabinetView::mouseMove (const MouseEvent& e)
{
    const int h = hitMic (e.position);
    if (h != hover) { hover = h; repaint(); }
    setMouseCursor (h >= 0 ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor);
}

void CabinetView::mouseDown (const MouseEvent& e)
{
    dragging = hitMic (e.position);
    if (dragging < 0) return;
    for (auto* id : { "micDistance", "micAngle" })
        if (auto* p = proc.apvts.getParameter (id)) p->beginChangeGesture();
}

void CabinetView::mouseDrag (const MouseEvent& e)
{
    if (dragging < 0) return;
    const auto v = e.position - centre();
    const float len = v.getDistanceFromOrigin();
    setParam ("micDistance", jlimit (0.0f, 1.0f, (len - cabRadius() * 1.22f) / (cabRadius() * 0.55f)));
    if (dragging != 2)
    {
        const float az = std::atan2 (std::abs (v.x), v.y);   // 0 = straight in front
        setParam ("micAngle", jlimit (0.0f, 180.0f, radiansToDegrees (2.0f * az)));
    }
    repaint();
}

void CabinetView::mouseUp (const MouseEvent&)
{
    if (dragging < 0) return;
    for (auto* id : { "micDistance", "micAngle" })
        if (auto* p = proc.apvts.getParameter (id)) p->endChangeGesture();
    dragging = -1;
}

void CabinetView::drawRotors (Graphics& g, const Theme& t, bool binaural)
{
    const auto& eng = proc.getEngine();
    const auto c = centre();
    const float R = cabRadius();
    const float twoPi = MathConstants<float>::twoPi;
    // screen angle for a model azimuth: front is down (mics) or up (binaural listener)
    auto at = [&] (float turns, float radius)
    {
        const float a = twoPi * turns;
        return binaural ? Point<float> (c.x + radius * std::sin (a), c.y - radius * std::cos (a))
                        : Point<float> (c.x + radius * std::sin (a), c.y + radius * std::cos (a));
    };
    const float hornTurns = eng.hornAngle(), drumTurns = eng.drumAngle();
    const float hornHz = eng.hornSpeed(), drumHz = eng.drumSpeed();
    const bool kaleido = t.style == 5;
    const Colour acc = kaleido ? Colour (0xffc77dff) : t.accent;

    // ---- drum: a disc with a scoop opening, light trail behind the opening ----
    const float rd = R * 0.86f;
    g.setColour (t.light ? t.knob.withAlpha (0.18f) : Colours::black.withAlpha (0.35f));
    g.fillEllipse (Rectangle<float> (2 * rd, 2 * rd).withCentre (c));
    {
        const float trail = jlimit (0.02f, 0.95f, drumHz * 0.16f);   // turns of trail
        const int steps = 28;
        for (int i = 0; i < steps; ++i)
        {
            const float f0 = (float) i / steps, f1 = (float) (i + 1) / steps;
            Path p;
            const float a0 = drumTurns - trail * f1, a1 = drumTurns - trail * f0;
            p.startNewSubPath (c);
            for (int k = 0; k <= 4; ++k) p.lineTo (at (a0 + (a1 - a0) * k / 4.0f, rd));
            p.closeSubPath();
            const Colour col = kaleido ? Colour::fromHSV (std::fmod (drumTurns * 0.5f + f0 * 0.6f, 1.0f), 0.7f, 1.0f, 1.0f) : acc;
            g.setColour (col.withAlpha (0.22f * (1.0f - f0)));
            g.fillPath (p);
        }
        Path scoop;   // the opening of the drum, 70 degrees wide
        scoop.startNewSubPath (c);
        for (int k = 0; k <= 10; ++k) scoop.lineTo (at (drumTurns - 0.1f + 0.2f * k / 10.0f, rd));
        scoop.closeSubPath();
        g.setColour (acc.withAlpha (0.55f)); g.fillPath (scoop);
    }
    g.setColour (t.dim.withAlpha (0.7f));
    g.drawEllipse (Rectangle<float> (2 * rd, 2 * rd).withCentre (c), 1.5f);

    // ---- horn: double horn (one real bell, one dummy), trail or blur at audio rate ----
    const float rh = R * 0.95f;
    const float blur = jlimit (0.0f, 1.0f, (hornHz - 9.0f) / 12.0f);
    {
        const float trail = jlimit (0.03f, 0.95f, hornHz * 0.12f);
        const int steps = 36;
        for (int i = 0; i < steps; ++i)
        {
            const float f0 = (float) i / steps, f1 = (float) (i + 1) / steps;
            Path p;
            const float a0 = hornTurns - trail * f1, a1 = hornTurns - trail * f0;
            const auto o0 = at (a0, rh), o1 = at (a1, rh), i0 = at (a0, rh * 0.62f), i1 = at (a1, rh * 0.62f);
            p.startNewSubPath (i0); p.lineTo (o0); p.lineTo (o1); p.lineTo (i1); p.closeSubPath();
            const Colour col = kaleido ? Colour::fromHSV (std::fmod (hornTurns * 0.2f + f0 * 0.85f, 1.0f), 0.75f, 1.0f, 1.0f) : acc;
            g.setColour (col.withAlpha ((0.55f - 0.25f * blur) * (1.0f - f0)));
            g.fillPath (p);
        }
    }
    if (blur > 0.0f)
    {
        // spinning too fast to see: a shimmering ring, rainbow in Kaleido
        for (int i = 0; i < 48; ++i)
        {
            const float a0 = (float) i / 48.0f;
            const Colour col = kaleido ? Colour::fromHSV (a0, 0.7f, 1.0f, 1.0f) : acc;
            Path p; p.addCentredArc (c.x, c.y, rh * 0.8f, rh * 0.8f, 0.0f, twoPi * a0, twoPi * (a0 + 1.0f / 48.0f) + 0.01f, true);
            g.setColour (col.withAlpha (0.65f * blur));
            g.strokePath (p, PathStrokeType (rh * 0.36f));
        }
    }
    const float hornAlpha = 1.0f - 0.7f * blur;
    auto bell = [&] (float turns, float len, float mouth, bool real)
    {
        const float a = twoPi * turns;
        const Point<float> dir = binaural ? Point<float> (std::sin (a), -std::cos (a)) : Point<float> (std::sin (a), std::cos (a));
        const Point<float> perp (-dir.y, dir.x);
        Path p;
        const auto tip = c + dir * len, root = c + dir * (R * 0.08f);
        p.startNewSubPath (root + perp * 4.0f);
        p.quadraticTo (c + dir * (len * 0.7f) + perp * 5.0f, tip + perp * mouth);
        p.lineTo (tip - perp * mouth);
        p.quadraticTo (c + dir * (len * 0.7f) - perp * 5.0f, root - perp * 4.0f);
        p.closeSubPath();
        g.setColour ((real ? t.knobCap.brighter (0.4f) : t.knobCap).withAlpha (hornAlpha));
        g.fillPath (p);
        g.setColour ((real ? acc : t.dim).withAlpha (hornAlpha));
        g.strokePath (p, PathStrokeType (1.5f));
        if (real)
        {
            g.setColour (acc.withAlpha (hornAlpha));
            g.drawLine (Line<float> (tip + perp * mouth, tip - perp * mouth), 3.0f);
        }
    };
    bell (hornTurns + 0.5f, rh * 0.82f, R * 0.12f, false);
    bell (hornTurns, rh, R * 0.2f, true);
    g.setColour (t.knob); g.fillEllipse (Rectangle<float> (R * 0.16f, R * 0.16f).withCentre (c));
    g.setColour (t.dim); g.drawEllipse (Rectangle<float> (R * 0.16f, R * 0.16f).withCentre (c), 1.0f);
}

void CabinetView::drawMic (Graphics& g, const Theme& t, Point<float> p, bool hot)
{
    const auto c = centre();
    const auto dir = (c - p) / jmax (1.0f, c.getDistanceFrom (p));
    const Point<float> perp (-dir.y, dir.x);
    // stand line away from the cabinet, capsule pointing at it
    g.setColour (t.dim.withAlpha (0.6f));
    g.drawLine (Line<float> (p, p - dir * 22.0f), 2.0f);
    Path body;
    body.startNewSubPath (p + perp * 6.0f); body.lineTo (p + dir * 14.0f + perp * 4.0f);
    body.lineTo (p + dir * 14.0f - perp * 4.0f); body.lineTo (p - perp * 6.0f); body.closeSubPath();
    g.setColour (t.knob); g.fillPath (body);
    g.setColour (hot ? t.accent : t.dim); g.strokePath (body, PathStrokeType (1.5f));
    g.setColour (hot ? t.accent : t.text.withAlpha (0.8f));
    g.fillEllipse (Rectangle<float> (12.0f, 12.0f).withCentre (p + dir * 15.0f));
}

void CabinetView::drawHead (Graphics& g, const Theme& t)
{
    const auto c = centre();
    const float r = cabRadius() * 0.24f;
    g.setColour (t.light ? t.panel : t.bg2);
    g.fillEllipse (Rectangle<float> (2 * r, 2 * r * 1.1f).withCentre (c));
    g.setColour (t.text);
    g.drawEllipse (Rectangle<float> (2 * r, 2 * r * 1.1f).withCentre (c), 2.0f);
    Path nose; nose.addTriangle (c.x - r * 0.25f, c.y - r * 1.02f, c.x + r * 0.25f, c.y - r * 1.02f, c.x, c.y - r * 1.4f);
    g.fillPath (nose);
    g.fillRoundedRectangle (Rectangle<float> (r * 0.28f, r * 0.6f).withCentre ({ c.x - r * 1.05f, c.y }), 3.0f);
    g.fillRoundedRectangle (Rectangle<float> (r * 0.28f, r * 0.6f).withCentre ({ c.x + r * 1.05f, c.y }), 3.0f);
    g.setFont (FontOptions (10.0f, Font::bold));
    g.drawText ("L", Rectangle<float> (20, 14).withCentre ({ c.x - r * 1.6f, c.y }), Justification::centred);
    g.drawText ("R", Rectangle<float> (20, 14).withCentre ({ c.x + r * 1.6f, c.y }), Justification::centred);
}

void CabinetView::paint (Graphics& g)
{
    const auto& t = themeOf (*this);
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (t.light ? t.panel : t.bg2);
    g.fillRoundedRectangle (b, 10.0f);
    g.setColour (t.panelEdge); g.drawRoundedRectangle (b, 10.0f, 1.0f);

    const int mode = roundToInt (param ("outMode"));
    const bool binaural = mode == 1;
    const auto c = centre();
    const float R = cabRadius();

    // floor: room rings that widen with Room and Distance
    const float room = param ("room");
    for (int i = 1; i <= 4; ++i)
    {
        const float rr = R * (1.3f + 0.28f * i);
        g.setColour (t.dim.withAlpha (0.05f + 0.12f * room));
        g.drawEllipse (Rectangle<float> (2 * rr, 2 * rr).withCentre (c), 1.0f);
    }

    // cabinet: square body with the top cut away
    const float cab = R * (1.08f + 0.1f * param ("cabSize"));
    auto box = Rectangle<float> (2 * cab, 2 * cab).withCentre (c);
    const Colour wood = t.style == 4 ? Colour (0xff6b4226) : (t.style == 3 ? Colour (0xff8a8d96) : t.knob.brighter (0.15f));
    g.setColour (Colours::black.withAlpha (0.3f)); g.fillRoundedRectangle (box.translated (3, 5), 12.0f);
    g.setGradientFill (ColourGradient (wood.brighter (0.15f), box.getX(), box.getY(), wood.darker (0.3f), box.getRight(), box.getBottom(), false));
    g.fillRoundedRectangle (box, 12.0f);
    g.setColour (t.light ? Colours::black.withAlpha (0.55f) : Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (box.reduced (R * 0.08f), 8.0f);

    drawRotors (g, t, binaural);

    if (binaural)
        drawHead (g, t);
    else
    {
        // front grille marker
        g.setColour (t.dim);
        g.setFont (FontOptions (10.0f, Font::bold));
        g.drawText ("FRONT", Rectangle<float> (60, 14).withCentre ({ c.x, box.getBottom() + 10 }), Justification::centred);
        for (int m = 0; m < 3; ++m)
            if ((m == 2) == (mode == 2)) drawMic (g, t, micPos (m), m == hover || m == dragging);
    }

    // caption
    g.setColour (t.dim);
    g.setFont (FontOptions (11.5f));
    const String hint = binaural ? String ("Binaural: listen on headphones. The rotors circle your head.")
                                 : String ("Drag a mic: in/out = distance, around = angle");
    g.drawText (hint, b.removeFromBottom (24).reduced (12, 0), Justification::centred);
}

// ============================================================================ Editor
RotaryPrismEditor::RotaryPrismEditor (RotaryPrismProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    content.addAndMakeVisible (cabinet);

    auto add = [this] (const char* title) -> Section&
    {
        sections.push_back (std::make_unique<Section> (proc, title, (int) sections.size()));
        content.addAndMakeVisible (*sections.back());
        return *sections.back();
    };
    auto& horn = add ("Horn");
    horn.addToggle ("hornAudio", "AUDIO RATE");
    horn.addToggle ("noteTrack", "RATE FROM NOTE");
    horn.addKnob ("hornSlow", "Slow");
    horn.addKnob ("hornFast", "Fast");
    horn.addKnob ("hornAccel", "Accel");
    horn.addKnob ("hornDecel", "Decel");
    horn.addKnob ("hornLevel", "Level");
    horn.addKnob ("hornAudioHz", "Rate");
    horn.addKnob ("ringFm", "Ring / FM");

    auto& drum = add ("Drum");
    drum.addKnob ("drumSlow", "Slow");
    drum.addKnob ("drumFast", "Fast");
    drum.addKnob ("drumAccel", "Accel");
    drum.addKnob ("drumDecel", "Decel");
    drum.addKnob ("drumLevel", "Level");

    auto& mics = add ("Mics");
    mics.addKnob ("micDistance", "Distance");
    mics.addKnob ("micAngle", "Angle");
    mics.addKnob ("micBalance", "Horn/Drum");

    auto& space = add ("Space");
    space.addChoice ("outMode");
    space.addKnob ("cabSize", "Cabinet");
    space.addKnob ("room", "Room");

    auto& pre = add ("Pre");
    pre.addKnob ("drive", "Drive");
    pre.addKnob ("crossover", "Crossover");

    auto& global = add ("Output");
    global.addKnob ("mix", "Mix");
    global.addKnob ("output", "Output");

    for (int i = 0; i < 3; ++i)
    {
        speedBtn[i].setClickingTogglesState (false);
        speedBtn[i].onClick = [this, i] { setSpeed (i); };
        content.addAndMakeVisible (speedBtn[i]);
    }
    if (auto* prm = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter ("midiSpeed")))
        for (int i = 0; i < prm->choices.size(); ++i) midiBox.addItem ("MIDI: " + prm->choices[i], i + 1);
    midiBox.setTooltip ("MIDI speed control: sustain pedal (CC 64) or held notes flip Slow/Fast");
    content.addAndMakeVisible (midiBox);
    midiAttach = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "midiSpeed", midiBox);

    content.addAndMakeVisible (presetBox);
    content.addAndMakeVisible (prevPreset);
    content.addAndMakeVisible (nextPreset);
    content.addAndMakeVisible (themeBox);
    refreshPresetBox();
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && idx != proc.getCurrentProgram()) proc.setCurrentProgram (idx);
    };
    auto step = [this] (int d)
    {
        const int n = proc.getNumPrograms();
        proc.setCurrentProgram ((proc.getCurrentProgram() + d + n) % n);
    };
    prevPreset.onClick = [step] { step (-1); };
    nextPreset.onClick = [step] { step (1); };

    for (int i = 0; i < numThemes(); ++i) themeBox.addItem (themeAt (i).name, i + 1);
    themeBox.setSelectedId (proc.getTheme() + 1, dontSendNotification);
    themeBox.onChange = [this] { applyTheme (themeBox.getSelectedId() - 1); };

    applyTheme (proc.getTheme());
    syncSpeedButtons();

    constrainer.setFixedAspectRatio ((double) kDesignW / kDesignH);
    constrainer.setSizeLimits (kDesignW * 6 / 10, kDesignH * 6 / 10, kDesignW * 2, kDesignH * 2);
    setConstrainer (&constrainer);
    setResizable (true, true);
    const float s = jlimit (0.6f, 2.0f, proc.getUiScale());
    setSize (roundToInt (kDesignW * s), roundToInt (kDesignH * s));
    startTimerHz (60);
}

RotaryPrismEditor::~RotaryPrismEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void RotaryPrismEditor::refreshPresetBox()
{
    presetBox.clear (dontSendNotification);
    int n = 0; auto* ps = rp::factoryPresets (n);
    String lastCat;
    for (int i = 0; i < n; ++i)
    {
        if (lastCat != ps[i].category) { presetBox.addSectionHeading (ps[i].category); lastCat = ps[i].category; }
        presetBox.addItem (ps[i].name, i + 1);
    }
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    lastProgram = proc.getCurrentProgram();
}

void RotaryPrismEditor::applyTheme (int index)
{
    proc.setTheme (index);
    lnf.setTheme (index);
    const auto& t = lnf.theme();
    for (auto& s : sections) s->setAccent (sectionColour (t, s->index, kNumSections));
    for (auto& b : speedBtn) b.setColour (TextButton::buttonOnColourId, t.accent);
    sendLookAndFeelChange();
    repaint();
}

void RotaryPrismEditor::setSpeed (int s)
{
    if (auto* p = proc.apvts.getParameter ("speed"))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) s));
        p->endChangeGesture();
    }
}

void RotaryPrismEditor::resized()
{
    const float s = (float) getWidth() / (float) kDesignW;
    proc.setUiScale (s);
    content.setBounds (0, 0, kDesignW, kDesignH);
    content.setTransform (AffineTransform::scale (s));
}

void RotaryPrismEditor::syncSpeedButtons()
{
    const int speed = roundToInt (proc.apvts.getRawParameterValue ("speed")->load());
    for (int i = 0; i < 3; ++i) speedBtn[i].setToggleState (i == speed, dontSendNotification);
}

void RotaryPrismEditor::timerCallback()
{
    if (proc.getCurrentProgram() != lastProgram)
    {
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
        lastProgram = proc.getCurrentProgram();
    }
    syncSpeedButtons();
    limiterShown = jmin (proc.getEngine().limiterGain(), limiterShown * 0.9f + 0.1f);
    cabinet.repaint();
    content.repaint (0, 600, 480, 80);
}

void RotaryPrismEditor::Content::paint (Graphics& g)
{
    const auto& t = ed.lnf.theme();
    g.setGradientFill (ColourGradient (t.bg, 0, 0, t.bg2, 0, (float) getHeight(), false));
    g.fillAll();
    if (t.style == 5)   // Kaleido: faint prism rays behind everything
    {
        for (int i = 0; i < 7; ++i)
        {
            Path ray;
            const float x0 = 250.0f, y0 = 330.0f, a = -0.55f + i * 0.17f;
            ray.addTriangle (x0, y0, x0 + 1400 * std::cos (a), y0 + 1400 * std::sin (a), x0 + 1400 * std::cos (a + 0.12f), y0 + 1400 * std::sin (a + 0.12f));
            g.setColour (Colour::fromHSV (i / 7.0f, 0.7f, 1.0f, 0.035f));
            g.fillPath (ray);
        }
    }

    // header
    auto header = getLocalBounds().removeFromTop (56).reduced (18, 0);
    if (t.style == 5)
    {
        String title ("ROTARY PRISM");
        float x = (float) header.getX();
        g.setFont (FontOptions (24.0f, Font::bold));
        for (int i = 0; i < title.length(); ++i)
        {
            const String ch = title.substring (i, i + 1);
            g.setColour (Colour::fromHSV (i / (float) title.length() * 0.85f, 0.6f, 1.0f, 1.0f));
            g.drawText (ch, Rectangle<float> (x, (float) header.getY(), 24.0f, (float) header.getHeight()), Justification::centredLeft);
            x += GlyphArrangement::getStringWidth (g.getCurrentFont(), ch) + 1.5f;
        }
    }
    else
    {
        g.setColour (t.text);
        g.setFont (FontOptions (24.0f, Font::bold));
        g.drawText ("ROTARY PRISM", header, Justification::centredLeft);
    }
    g.setColour (t.dim);
    g.setFont (FontOptions (12.0f));
    g.drawText ("binaural rotary speaker & Doppler modulator", header.withTrimmedLeft (230), Justification::centredLeft);

    // readouts under the speed switch
    const auto& e = ed.proc.getEngine();
    auto r = Rectangle<int> (16, 640, 460, 28);
    auto readout = [&] (Rectangle<int> a, const String& label, const String& value, Colour col)
    {
        g.setColour (t.dim); g.setFont (FontOptions (10.5f, Font::bold));
        g.drawText (label, a.removeFromLeft (a.getWidth() / 2), Justification::centredRight);
        g.setColour (col); g.setFont (FontOptions (13.0f, Font::bold));
        g.drawText (value, a.withTrimmedLeft (6), Justification::centredLeft);
    };
    auto hz = [] (float v) { return v >= 100.0f ? String (roundToInt (v)) + " Hz" : String (v, 2) + " Hz"; };
    const int w = r.getWidth() / 3;
    readout (r.removeFromLeft (w), "HORN", hz (e.hornSpeed()), t.text);
    readout (r.removeFromLeft (w), "DRUM", hz (e.drumSpeed()), t.text);
    const float lim = ed.limiterShown;
    readout (r, "LIMIT", lim < 0.995f ? String (Decibels::gainToDecibels (lim), 1) + " dB" : String ("--"), lim < 0.995f ? t.accent : t.dim);
}

void RotaryPrismEditor::Content::resized()
{
    auto& e = ed;
    auto header = getLocalBounds().removeFromTop (56).reduced (18, 12);
    e.themeBox.setBounds (header.removeFromRight (130));
    header.removeFromRight (16);
    e.nextPreset.setBounds (header.removeFromRight (30));
    header.removeFromRight (4);
    e.presetBox.setBounds (header.removeFromRight (220));
    header.removeFromRight (4);
    e.prevPreset.setBounds (header.removeFromRight (30));

    e.cabinet.setBounds (16, 64, 460, 528);
    auto speedRow = Rectangle<int> (16, 600, 460, 34);
    for (auto& b : e.speedBtn) { b.setBounds (speedRow.removeFromLeft (82)); speedRow.removeFromLeft (6); }
    e.midiBox.setBounds (speedRow.withTrimmedLeft (4).reduced (0, 3));

    const int x = 488, w = 616;
    e.sections[0]->setBounds (x, 64, w, 200);                       // horn
    e.sections[1]->setBounds (x, 272, 384, 196);                    // drum
    e.sections[4]->setBounds (x + 392, 272, w - 392, 196);          // pre
    e.sections[2]->setBounds (x, 476, 270, 196);                    // mics
    e.sections[3]->setBounds (x + 278, 476, 194, 196);              // space
    e.sections[5]->setBounds (x + 480, 476, w - 480, 196);          // output
}
