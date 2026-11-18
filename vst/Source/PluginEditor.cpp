#include "PluginEditor.h"

using namespace shtuner;

//==============================================================================
TunerLookAndFeel::TunerLookAndFeel()
{
    setColour (juce::Label::textColourId, tcol::text);
    setColour (juce::TextButton::buttonColourId, tcol::panel2);
    setColour (juce::TextButton::buttonOnColourId, tcol::teal);
    setColour (juce::TextButton::textColourOffId, tcol::muted);
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xff06201d));
    setColour (juce::ComboBox::backgroundColourId, tcol::panel2);
    setColour (juce::ComboBox::textColourId, tcol::text);
    setColour (juce::ComboBox::outlineColourId, tcol::line);
    setColour (juce::ComboBox::arrowColourId, tcol::muted);
    setColour (juce::PopupMenu::backgroundColourId, tcol::panel2);
    setColour (juce::PopupMenu::textColourId, tcol::text);
    setColour (juce::PopupMenu::headerTextColourId, tcol::teal);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, tcol::teal);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff06201d));
    setColour (juce::Slider::textBoxTextColourId, tcol::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void TunerLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& base, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    auto c = on ? b.findColour (juce::TextButton::buttonOnColourId) : base;
    if (over && ! on) c = c.brighter (0.12f);
    if (down) c = c.darker (0.12f);
    g.setColour (c); g.fillRoundedRectangle (r, 8.0f);
    if (! on) { g.setColour (tcol::line); g.drawRoundedRectangle (r, 8.0f, 1.0f); }
    else if (b.getProperties()["glow"]) { g.setColour (tcol::teal.withAlpha (0.35f)); g.drawRoundedRectangle (r.expanded (1.0f), 9.0f, 2.0f); }
}

void TunerLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId)); g.fillRoundedRectangle (r, 8.0f);
    g.setColour (tcol::line); g.drawRoundedRectangle (r, 8.0f, 1.0f);
    juce::Path arrow; const float ax = (float) w - 18.0f, ay = h * 0.5f;
    arrow.addTriangle (ax - 5, ay - 2.5f, ax + 5, ay - 2.5f, ax, ay + 3.5f);
    g.setColour (tcol::muted); g.fillPath (arrow);
}

void TunerLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    const float cy = y + h * 0.5f;
    g.setColour (tcol::line); g.fillRoundedRectangle ((float) x, cy - 3, (float) w, 6, 3);
    g.setColour (tcol::teal); g.fillRoundedRectangle ((float) x, cy - 3, pos - x, 6, 3);
    g.setColour (tcol::text); g.fillEllipse (pos - 8, cy - 8, 16, 16);
}

//==============================================================================
void NeedleMeter::set (bool a, double c, int t, const juce::String& n, const juce::String& o, const juce::String& s, double f, float lv, float gt)
{
    active = a; cents = c; tol = t; note = n; octave = o; sub = s; hz = f; level = lv; gate = gt;
    shown += ((active ? juce::jlimit (-50.0, 50.0, cents) : 0.0) - shown) * 0.45;
    repaint();
}

void NeedleMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (tcol::panel); g.fillRoundedRectangle (r, 14.0f);
    g.setColour (tcol::line); g.drawRoundedRectangle (r.reduced (0.5f), 14.0f, 1.0f);

    const bool inTune = active && std::abs (cents) <= tol;
    const auto accent = ! active ? tcol::line : (inTune ? tcol::teal : (std::abs (cents) <= 15 ? tcol::warn : tcol::bad));
    // nota arriba; arco debajo con el pivote cerca del borde inferior
    const float cx = r.getCentreX(), cy = r.getBottom() - 62.0f, rad = juce::jmin (r.getWidth() * 0.40f, cy - r.getY() - 124.0f);
    const float span = juce::MathConstants<float>::pi * 0.36f;   // ±65°
    auto angOf = [span] (double c) { return (float) (c / 50.0) * span; };

    juce::Path arc; arc.addCentredArc (cx, cy, rad, rad, 0, -span, span, true);
    g.setColour (tcol::panel2.brighter (0.15f)); g.strokePath (arc, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path zone; zone.addCentredArc (cx, cy, rad, rad, 0, angOf (-tol), angOf (tol), true);
    g.setColour (tcol::teal.withAlpha (active ? 0.9f : 0.45f)); g.strokePath (zone, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setFont (juce::FontOptions (12.0f));
    for (int c = -50; c <= 50; c += 10)
    {
        const float a = angOf (c), r1 = rad + 12, r2 = rad + (c % 50 == 0 ? 24.0f : 18.0f);
        g.setColour (tcol::muted.withAlpha (c == 0 ? 0.9f : 0.5f));
        g.drawLine (cx + r1 * std::sin (a), cy - r1 * std::cos (a), cx + r2 * std::sin (a), cy - r2 * std::cos (a), c % 50 == 0 ? 2.0f : 1.2f);
        if (c % 50 == 0 && c != 0)
            g.drawText ((c > 0 ? "+" : "") + juce::String (c), juce::Rectangle<float> (cx + (rad + 40) * std::sin (a) - 20, cy - (rad + 40) * std::cos (a) - 9, 40, 18), juce::Justification::centred);
    }
    // aguja
    const float a = angOf (shown);
    g.setColour (active ? accent : tcol::muted.withAlpha (0.4f));
    juce::Path needle; needle.addRoundedRectangle (-2.0f, -(rad + 6), 4.0f, rad + 6, 2.0f);
    g.fillPath (needle, juce::AffineTransform::rotation (a).translated (cx, cy));
    g.fillEllipse (cx - 7, cy - 7, 14, 14);

    // nota
    auto top = r.withTrimmedTop (6);
    g.setColour (active ? (inTune ? tcol::teal : tcol::text) : tcol::muted.withAlpha (0.5f));
    g.setFont (juce::FontOptions (58.0f, juce::Font::bold));
    juce::GlyphArrangement ga; ga.addLineOfText (g.getCurrentFont(), note, 0, 0);
    const float nw = ga.getBoundingBox (0, -1, true).getWidth();
    g.drawText (note, juce::Rectangle<float> (cx - nw * 0.5f - 6, top.getY(), nw + 12, 64), juce::Justification::centred);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText (octave, juce::Rectangle<float> (cx + nw * 0.5f + 4, top.getY() + 32, 40, 26), juce::Justification::centredLeft);

    g.setFont (juce::FontOptions (15.0f));
    g.setColour (tcol::muted);
    juce::String line2 = active ? ((cents >= 0 ? "+" : "") + juce::String (cents, 1) + juce::String (juce::CharPointer_UTF8 (" cents  ·  ")) + juce::String (hz, 2) + " Hz") : juce::String (juce::CharPointer_UTF8 ("Toca una nota"));
    g.drawText (line2, juce::Rectangle<float> (r.getX(), top.getY() + 64, r.getWidth(), 20), juce::Justification::centred);
    if (active)
    {
        const juce::String hint = inTune ? juce::String (juce::CharPointer_UTF8 ("✓ Afinada")) : (cents < 0 ? juce::String (juce::CharPointer_UTF8 ("▲ Sube (tensa)")) : juce::String (juce::CharPointer_UTF8 ("▼ Baja (afloja)")));
        g.setColour (accent); g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
        g.drawText (hint + (sub.isNotEmpty() ? juce::String (juce::CharPointer_UTF8 ("  ·  ")) + sub : juce::String()), juce::Rectangle<float> (r.getX(), cy + 14, r.getWidth(), 22), juce::Justification::centred);
    }
    // nivel de entrada (con marca de la puerta)
    auto lv = juce::Rectangle<float> (r.getX() + 16, r.getBottom() - 16, r.getWidth() - 32, 5);
    g.setColour (tcol::line); g.fillRoundedRectangle (lv, 2.5f);
    auto norm = [] (float db) { return juce::jlimit (0.0f, 1.0f, (db + 80.0f) / 80.0f); };
    g.setColour (level > gate ? tcol::teal : tcol::muted); g.fillRoundedRectangle (lv.withWidth (lv.getWidth() * norm (level)), 2.5f);
    g.setColour (tcol::text.withAlpha (0.6f)); g.fillRect (lv.getX() + lv.getWidth() * norm (gate) - 1, lv.getY() - 3, 2.0f, 11.0f);
}

//==============================================================================
static const char* kInstNames[] = { "Guitarra", "Bajo", "Ukelele", "Banjo", "Cromático" };

TunerEditor::TunerEditor (TunerProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    for (int i = 0; i < NumInstruments; ++i)
    {
        auto* b = instButtons.add (new juce::TextButton (u8 (kInstNames[i])));
        b->onClick = [this, i] { setParam (pid::instrument, (float) i); };
        addAndMakeVisible (b);
    }
    addAndMakeVisible (tuningBox);
    tuningBox.onChange = [this] { if (tuningBox.getSelectedId() > 0) setParam (pid::tuning, (float) (tuningBox.getSelectedId() - 1)); };
    for (auto* b : { &capoMinus, &capoPlus, &a4Minus, &a4Plus }) addAndMakeVisible (b);
    capoMinus.onClick = [this] { setParam (pid::capo, juce::jmax (0.0f, par (pid::capo) - 1)); };
    capoPlus.onClick  = [this] { setParam (pid::capo, juce::jmin (12.0f, par (pid::capo) + 1)); };
    a4Minus.onClick   = [this] { setParam (pid::a4, juce::jmax (415.0f, par (pid::a4) - 1)); };
    a4Plus.onClick    = [this] { setParam (pid::a4, juce::jmin (466.0f, par (pid::a4) + 1)); };
    for (auto* l : { &capoLabel, &a4Label }) { l->setJustificationType (juce::Justification::centred); l->setFont (juce::FontOptions (14.0f, juce::Font::bold)); addAndMakeVisible (l); }

    addAndMakeVisible (meter);
    autoB.onClick = [this] { setParam (pid::autoString, 1); };
    manualB.onClick = [this] { setParam (pid::autoString, 0); };
    refB.setButtonText (u8 ("Tono de referencia"));
    refB.onClick = [this] { setParam (pid::refTone, par (pid::refTone) > 0.5f ? 0.f : 1.f); };
    muteB.setButtonText ("Silenciar salida");
    muteB.onClick = [this] { setParam (pid::mute, par (pid::mute) > 0.5f ? 0.f : 1.f); };
    reportB.setButtonText ("Guardar informe (.txt)");
    reportB.onClick = [this]
    {
        auto f = proc.saveReport (u8 ("informe manual (botón)"));
        f.revealToUser();
        juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::InfoIcon).withTitle ("SampledHead Tuner")
                                        .withMessage (u8 ("Informe guardado en:\n") + f.getFullPathName()).withButton ("Aceptar", 1).withAssociatedComponent (this), nullptr);
    };
    for (auto* b : { &autoB, &manualB, &refB, &muteB, &reportB }) addAndMakeVisible (b);
    autoB.getProperties().set ("glow", true);

    auto setupSlider = [this] (juce::Slider& s, const juce::String& id, juce::Label& l, const juce::String& txt,
                               std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& att)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 74, 24);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxBackgroundColourId, tcol::panel2);
        l.setText (txt, juce::dontSendNotification); l.setColour (juce::Label::textColourId, tcol::muted); l.setFont (juce::FontOptions (13.0f));
        addAndMakeVisible (s); addAndMakeVisible (l);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, s);
    };
    setupSlider (tolSlider, pid::tol, tolLabel, u8 ("Tolerancia (± cents en verde)"), tolAtt);
    setupSlider (gateSlider, pid::gate, gateLabel, u8 ("Puerta de ruido (ignora lo más bajo)"), gateAtt);
    tolSlider.setTextValueSuffix (" cents"); gateSlider.setTextValueSuffix (" dB");
    namesBox.addItemList (proc.apvts.getParameter (pid::names)->getAllValueStrings(), 1);
    accBox.addItemList (proc.apvts.getParameter (pid::acc)->getAllValueStrings(), 1);
    namesAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, pid::names, namesBox);
    accAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, pid::acc, accBox);
    namesLabel.setText ("Nombres de nota", juce::dontSendNotification);
    accLabel.setText ("Alteraciones", juce::dontSendNotification);
    for (auto* l : { &namesLabel, &accLabel }) { l->setColour (juce::Label::textColourId, tcol::muted); l->setFont (juce::FontOptions (13.0f)); addAndMakeVisible (l); }
    addAndMakeVisible (namesBox); addAndMakeVisible (accBox);

    muteInfo.setText (u8 ("Corta el sonido de la pista mientras afinas. El tono de referencia suena igualmente."), juce::dontSendNotification);
    helpInfo.setText (u8 ("Pon el plugin al principio de la cadena, con el instrumento conectado a la pista. Toca las cuerdas una a una."), juce::dontSendNotification);
    for (auto* l : { &muteInfo, &helpInfo }) { l->setColour (juce::Label::textColourId, tcol::muted); l->setFont (juce::FontOptions (12.0f)); l->setJustificationType (juce::Justification::topLeft); addAndMakeVisible (l); }
    footer.setText (u8 ("SampledHead Tuner · VST v") + TunerProcessor::kVersion + u8 (" · Sampled Head"), juce::dontSendNotification);
    footer.setColour (juce::Label::textColourId, tcol::muted); footer.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (footer);

    setSize (840, 640);
    timerCallback();
    startTimerHz (30);
}

TunerEditor::~TunerEditor() { setLookAndFeel (nullptr); }

void TunerEditor::setParam (const juce::String& id, float v)
{
    if (auto* r = proc.apvts.getParameter (id)) { r->beginChangeGesture(); r->setValueNotifyingHost (r->convertTo0to1 (v)); r->endChangeGesture(); }
}

void TunerEditor::rebuildTunings()
{
    const int inst = juce::roundToInt (par (pid::instrument));
    tuningBox.clear (juce::dontSendNotification);
    if (inst == Chromatic) { tuningBox.addItem (u8 ("Cromático: cualquier nota"), 999); tuningBox.setSelectedId (999, juce::dontSendNotification); tuningBox.setEnabled (false); return; }
    tuningBox.setEnabled (true);
    for (int i : tuningsFor (inst))
        tuningBox.addItem (u8 (kTunings[i].name) + "   " + juce::String (kTunings[i].notes), i + 1);
    const int tu = resolveTuning (inst, juce::roundToInt (par (pid::tuning)));
    tuningBox.setSelectedId (tu + 1, juce::dontSendNotification);
    if (tu != juce::roundToInt (par (pid::tuning))) setParam (pid::tuning, (float) tu);   // al cambiar de instrumento
}

void TunerEditor::refreshStrings()
{
    stringButtons.clear();
    const int inst = juce::roundToInt (par (pid::instrument));
    const int tu = resolveTuning (inst, juce::roundToInt (par (pid::tuning)));
    if (tu >= 0)
    {
        const auto& T = kTunings[tu];
        const int capo = juce::roundToInt (par (pid::capo));
        const int names = juce::roundToInt (par (pid::names)), acc = juce::roundToInt (par (pid::acc));
        const bool flat = tuningPrefersFlat (tu);
        for (int i = 0; i < T.numStrings; ++i)
        {
            const int m = T.midi[(size_t) i] + capo;
            auto* b = stringButtons.add (new juce::TextButton (noteName (m, names, acc, flat) + juce::String (octaveOf (m))));
            b->setTooltip ("Cuerda " + juce::String (T.numStrings - i));
            b->onClick = [this, i] { setParam (pid::autoString, 0); setParam (pid::string, (float) i); };
            addAndMakeVisible (b);
        }
    }
    resized();
}

void TunerEditor::timerCallback()
{
    const int inst = juce::roundToInt (par (pid::instrument)), tun = juce::roundToInt (par (pid::tuning)), capo = juce::roundToInt (par (pid::capo));
    const int names = juce::roundToInt (par (pid::names)), acc = juce::roundToInt (par (pid::acc));
    if (inst != lastInst) { lastInst = inst; rebuildTunings(); lastTuning = -2; }
    if (tun != lastTuning || capo != lastCapo || names != lastNames || acc != lastAcc)
    { lastTuning = tun; lastCapo = capo; lastNames = names; lastAcc = acc; refreshStrings(); }

    for (int i = 0; i < instButtons.size(); ++i) instButtons[i]->setToggleState (i == inst, juce::dontSendNotification);
    const bool isAuto = par (pid::autoString) > 0.5f;
    autoB.setToggleState (isAuto, juce::dontSendNotification); manualB.setToggleState (! isAuto, juce::dontSendNotification);
    refB.setToggleState (par (pid::refTone) > 0.5f, juce::dontSendNotification);
    muteB.setToggleState (par (pid::mute) > 0.5f, juce::dontSendNotification);
    capoLabel.setText ("Capo " + juce::String (capo), juce::dontSendNotification);
    const float a4v = par (pid::a4);
    a4Label.setText ("La " + juce::String (a4v, std::abs (a4v - std::round (a4v)) < 0.01f ? 0 : 1) + " Hz", juce::dontSendNotification);
    for (auto* b : { &capoMinus, &capoPlus, &autoB, &manualB }) b->setEnabled (inst != Chromatic);

    // lectura actual (se mantiene 1,5 s al desaparecer la señal)
    const auto r = proc.getReading();
    auto t = proc.currentTarget();
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (t.valid) { held = t; heldHz = r.hz; holdUntil = now + 1500.0; if (t.stringIndex >= 0) proc.lastString.store (t.stringIndex); }
    const bool showing = t.valid || now < holdUntil;
    const auto& T = t.valid ? t : held;
    const int tu = resolveTuning (inst, tun);
    juce::String note = "-", octv, sub;
    if (showing)
    {
        note = noteName (T.midi, names, acc, tuningPrefersFlat (tu));
        octv = juce::String (octaveOf (T.midi));
        if (T.stringIndex >= 0 && tu >= 0) sub = "Cuerda " + juce::String (kTunings[tu].numStrings - T.stringIndex);
    }
    meter.set (t.valid, T.cents, juce::roundToInt (par (pid::tol)), note, octv, sub, t.valid ? r.hz : heldHz, r.levelDb, par (pid::gate));
    meter.setAlpha (t.valid || ! showing ? 1.0f : 0.6f);

    const int sel = isAuto ? (showing ? T.stringIndex : -1) : juce::roundToInt (par (pid::string));
    for (int i = 0; i < stringButtons.size(); ++i) stringButtons[i]->setToggleState (i == sel, juce::dontSendNotification);
}

void TunerEditor::paint (juce::Graphics& g)
{
    g.fillAll (tcol::bg);
    // logotipo: barras de colores (onda) + nombre
    const juce::Colour bars[] = { tcol::teal, tcol::teal, tcol::purple, tcol::pink, tcol::purple };
    const float hs[] = { 10, 18, 24, 16, 8 };
    for (int i = 0; i < 5; ++i) { g.setColour (bars[i]); g.fillRoundedRectangle (18.0f + i * 6.0f, 28.0f - hs[i] * 0.5f, 3.5f, hs[i], 1.75f); }
    g.setColour (tcol::text); g.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    g.drawText ("SampledHead", 56, 14, 130, 28, juce::Justification::centredLeft);
    g.setColour (tcol::teal); g.drawText ("Tuner", 172, 14, 70, 28, juce::Justification::centredLeft);
    g.setColour (tcol::panel2); g.fillRoundedRectangle (236.0f, 18.0f, 42.0f, 20.0f, 6.0f);
    g.setColour (tcol::muted); g.setFont (juce::FontOptions (11.0f, juce::Font::bold)); g.drawText ("VST", 236, 18, 42, 20, juce::Justification::centred);
    g.setColour (tcol::line); g.drawLine (0, 54, (float) getWidth(), 54);
    // paneles
    g.setColour (tcol::panel); g.fillRoundedRectangle (16.0f, 66.0f, 540.0f, 116.0f, 14.0f);
    g.fillRoundedRectangle (572.0f, 66.0f, 252.0f, 524.0f, 14.0f);
    g.setColour (tcol::line); g.drawRoundedRectangle (16.5f, 66.5f, 539.0f, 115.0f, 14.0f, 1.0f); g.drawRoundedRectangle (572.5f, 66.5f, 251.0f, 523.0f, 14.0f, 1.0f);
    g.setColour (tcol::teal); g.fillRoundedRectangle (16.0f, 74.0f, 3.0f, 100.0f, 1.5f);
    g.setColour (tcol::text); g.setFont (juce::FontOptions (13.0f, juce::Font::bold)); g.drawText ("AJUSTES", 590, 78, 200, 20, juce::Justification::centredLeft);
}

void TunerEditor::resized()
{
    a4Minus.setBounds (getWidth() - 186, 14, 32, 28); a4Label.setBounds (getWidth() - 152, 14, 104, 28); a4Plus.setBounds (getWidth() - 48, 14, 32, 28);

    const int bw = (520 - 4 * 4) / NumInstruments;
    for (int i = 0; i < instButtons.size(); ++i) instButtons[i]->setBounds (26 + i * (bw + 4), 76, bw, 38);
    tuningBox.setBounds (26, 124, 380, 48);
    capoMinus.setBounds (414, 124, 34, 48); capoLabel.setBounds (448, 124, 64, 48); capoPlus.setBounds (512, 124, 34, 48);

    meter.setBounds (16, 192, 540, 298);
    const int n = juce::jmax (1, stringButtons.size());
    const int sw = (540 - (n - 1) * 6) / n;
    for (int i = 0; i < stringButtons.size(); ++i) stringButtons[i]->setBounds (16 + i * (sw + 6), 500, sw, 44);
    autoB.setBounds (16, 554, 150, 36); manualB.setBounds (170, 554, 150, 36); refB.setBounds (336, 554, 220, 36);

    int y = 108;
    tolLabel.setBounds (588, y, 220, 18); tolSlider.setBounds (584, y + 20, 228, 28); y += 64;
    gateLabel.setBounds (588, y, 220, 18); gateSlider.setBounds (584, y + 20, 228, 28); y += 64;
    namesLabel.setBounds (588, y, 220, 18); namesBox.setBounds (588, y + 22, 220, 34); y += 70;
    accLabel.setBounds (588, y, 220, 18); accBox.setBounds (588, y + 22, 220, 34); y += 74;
    muteB.setBounds (588, y, 220, 36); muteInfo.setBounds (588, y + 40, 220, 34); y += 84;
    helpInfo.setBounds (588, y, 220, 64);
    reportB.setBounds (588, 540, 220, 36);
    footer.setBounds (16, getHeight() - 36, 600, 24);
}
