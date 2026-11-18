#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace shtuner;

juce::AudioProcessorValueTreeState::ParameterLayout TunerProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;
    auto P = [] (const juce::String& id) { return juce::ParameterID { id, 1 }; };
    v.push_back (std::make_unique<juce::AudioParameterChoice> (P (pid::instrument), "Instrumento",
        juce::StringArray { "Guitarra", "Bajo", "Ukelele", "Banjo", u8 ("Cromático") }, 0));
    v.push_back (std::make_unique<juce::AudioParameterInt> (P (pid::tuning), u8 ("Afinación"), 0, kNumTunings - 1, 0));
    v.push_back (std::make_unique<juce::AudioParameterInt> (P (pid::capo), "Capo", 0, 12, 0));
    v.push_back (std::make_unique<juce::AudioParameterFloat> (P (pid::a4), "La (A4)", juce::NormalisableRange<float> (415.0f, 466.0f, 0.5f), 440.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    v.push_back (std::make_unique<juce::AudioParameterBool> (P (pid::autoString), u8 ("Cuerda automática"), true));
    v.push_back (std::make_unique<juce::AudioParameterInt> (P (pid::string), "Cuerda manual", 0, 7, 0));
    v.push_back (std::make_unique<juce::AudioParameterInt> (P (pid::tol), "Tolerancia", 1, 10, 3,
        juce::AudioParameterIntAttributes().withLabel ("cents")));
    v.push_back (std::make_unique<juce::AudioParameterFloat> (P (pid::gate), "Puerta", juce::NormalisableRange<float> (-80.0f, -20.0f, 1.0f), -52.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));
    v.push_back (std::make_unique<juce::AudioParameterChoice> (P (pid::names), "Nombres", juce::StringArray { "C D E", "Do Re Mi" }, 0));
    v.push_back (std::make_unique<juce::AudioParameterChoice> (P (pid::acc), "Alteraciones", juce::StringArray { "Auto", u8 ("♯"), u8 ("♭") }, 0));
    v.push_back (std::make_unique<juce::AudioParameterBool> (P (pid::mute), "Silenciar salida", false));
    v.push_back (std::make_unique<juce::AudioParameterBool> (P (pid::refTone), "Tono de referencia", false));
    return { v.begin(), v.end() };
}

TunerProcessor::TunerProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "TUNER", createLayout())
{
    log ("Plugin creado (v" + juce::String (kVersion) + ")");
}

bool TunerProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    auto ok = [] (const juce::AudioChannelSet& s) { return s == juce::AudioChannelSet::mono() || s == juce::AudioChannelSet::stereo(); };
    return ok (in) && ok (out);
}

void TunerProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate; blockSize = samplesPerBlock;
    tracker.prepare (sr, 26.0f, 4200.0f);      // de La0 (bajo de 5 cuerdas en Drop A) a Do8 (piano)
    histN = 0; smoothLog = 0.0f;
    hz = 0.0f; clarity = 0.0f; levelDb = -120.0f;
    refPhase = 0.0;
    refGain.reset (sr, 0.03); refGain.setCurrentAndTargetValue (0.0f);
    passGain.reset (sr, 0.02); passGain.setCurrentAndTargetValue (p (pid::mute) > 0.5f ? 0.0f : 1.0f);
    log ("prepareToPlay: " + juce::String (sr) + " Hz, bloque " + juce::String (samplesPerBlock));
}

Target TunerProcessor::currentTarget() const
{
    return computeTarget (hz.load(), p (pid::a4), juce::roundToInt (p (pid::instrument)), juce::roundToInt (p (pid::tuning)),
                          juce::roundToInt (p (pid::capo)), p (pid::autoString) > 0.5f, juce::roundToInt (p (pid::string)));
}

void TunerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels(), outCh = getTotalNumOutputChannels();
    tracker.setGateDb (p (pid::gate));

    // 1) Análisis (mono)
    for (int i = 0; i < n; ++i)
    {
        float x = 0.0f;
        for (int c = 0; c < inCh; ++c) x += buffer.getSample (c, i);
        x /= (float) juce::jmax (1, inCh);
        if (tracker.push (x))
        {
            levelDb.store (tracker.getRmsDb());
            const float f = tracker.getPitch();
            if (f > 0.0f)
            {
                // mediana de 5 lecturas + suavizado (sin arrastrar en los cambios de nota)
                hist[(size_t) (histN % 5)] = std::log2 (f); ++histN;
                const int cnt = juce::jmin (histN, 5);
                std::array<float, 5> s = hist;
                for (int a = 1; a < cnt; ++a)       // ordenación por inserción de las cnt lecturas (cnt ≤ 5)
                    for (int b = a; b > 0 && s[(size_t) b - 1] > s[(size_t) b]; --b) std::swap (s[(size_t) b - 1], s[(size_t) b]);
                const float med = s[(size_t) ((cnt - 1) / 2)];
                if (cnt < 3 || std::abs (med - smoothLog) > 50.0f / 1200.0f) smoothLog = med;
                else smoothLog += (med - smoothLog) * 0.35f;
                hz.store (std::exp2 (smoothLog));
                clarity.store (tracker.getClarity());
            }
            else { histN = 0; hz.store (0.0f); clarity.store (0.0f); }
            frames.store (frames.load() + 1);
        }
    }

    // 2) Salida: señal original (o silencio) + tono de referencia
    passGain.setTargetValue (p (pid::mute) > 0.5f ? 0.0f : 1.0f);
    const bool refOn = p (pid::refTone) > 0.5f;
    float refF = 0.0f;
    if (refOn)
    {
        const int inst = juce::roundToInt (p (pid::instrument));
        const int tu = resolveTuning (inst, juce::roundToInt (p (pid::tuning)));
        const double a4 = p (pid::a4);
        auto t = currentTarget();
        if (t.valid) { refF = (float) t.targetHz; if (t.stringIndex >= 0) lastString.store (t.stringIndex); }
        else if (tu >= 0)
        {
            // sin señal: la última cuerda detectada (auto) o la elegida (manual)
            const int want = p (pid::autoString) > 0.5f ? lastString.load() : juce::roundToInt (p (pid::string));
            const int s = juce::jlimit (0, kTunings[tu].numStrings - 1, want);
            refF = (float) midiToHz (kTunings[tu].midi[(size_t) s] + juce::roundToInt (p (pid::capo)), a4);
        }
        else refF = (float) a4;
        refHz.store (refF);
    }
    refGain.setTargetValue (refOn ? juce::Decibels::decibelsToGain (-18.0f) : 0.0f);
    const double inc = refF / sr;

    for (int i = 0; i < n; ++i)
    {
        const float g = passGain.getNextValue();
        const float rg = refGain.getNextValue();
        float tone = 0.0f;
        if (rg > 1e-5f) { tone = rg * (float) std::sin (juce::MathConstants<double>::twoPi * refPhase); refPhase += inc; if (refPhase >= 1.0) refPhase -= 1.0; }
        for (int c = 0; c < outCh; ++c)
        {
            const float in = c < inCh ? buffer.getSample (c, i) : (inCh > 0 ? buffer.getSample (0, i) : 0.0f);
            buffer.setSample (c, i, in * g + tone);
        }
    }
}

void TunerProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}
void TunerProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) { apvts.replaceState (juce::ValueTree::fromXml (*xml)); log ("Estado restaurado desde el proyecto"); }
}

void TunerProcessor::log (const juce::String& msg)
{
    const juce::ScopedLock sl (logLock);
    logLines.add (juce::Time::getCurrentTime().formatted ("%H:%M:%S") + "  " + msg);
    while (logLines.size() > 300) logLines.remove (0);
}

juce::File TunerProcessor::saveReport (const juce::String& where)
{
    juce::StringArray L;
    const auto r = getReading();
    const auto t = currentTarget();
    juce::PluginHostType host;
    L.add (u8 ("INFORME · SampledHead Tuner (plugin)"));
    L.add (u8 ("Versión: v") + juce::String (kVersion));
    L.add ("Fecha: " + juce::Time::getCurrentTime().toString (true, true, true, true));
    L.add (u8 ("Dónde: ") + where);
    L.add ("");
    L.add ("Formato: " + juce::String (juce::AudioProcessor::getWrapperTypeDescription (wrapperType)) + u8 (" · Host: ") + host.getHostDescription());
    L.add ("Sistema: " + juce::SystemStats::getOperatingSystemName() + (juce::SystemStats::isOperatingSystem64Bit() ? " (64 bits)" : ""));
    L.add ("CPU: " + juce::SystemStats::getCpuModel() + u8 (" · Núcleos: ") + juce::String (juce::SystemStats::getNumCpus()));
    L.add ("RAM: " + juce::String (juce::SystemStats::getMemorySizeInMegabytes()) + " MB");
    L.add ("Audio: " + juce::String (sr) + u8 (" Hz · bloque ") + juce::String (blockSize) + u8 (" · entradas ") + juce::String (getTotalNumInputChannels())
           + u8 (" · salidas ") + juce::String (getTotalNumOutputChannels()));
    L.add (u8 ("Última lectura: ") + juce::String (r.hz, 2) + u8 (" Hz · claridad ") + juce::String (r.clarity, 2) + u8 (" · nivel ") + juce::String (r.levelDb, 1)
           + u8 (" dBFS · análisis nº ") + juce::String (r.frame));
    const int tuIdx = resolveTuning (juce::roundToInt (p (pid::instrument)), juce::roundToInt (p (pid::tuning)));
    const juce::String cuerda = (t.stringIndex >= 0 && tuIdx >= 0) ? u8 (" · cuerda ") + juce::String (kTunings[tuIdx].numStrings - t.stringIndex) : juce::String();
    L.add ("Objetivo: " + (t.valid ? "MIDI " + juce::String (t.midi) + cuerda + u8 (" · ") + juce::String (t.cents, 1) + " cents" : juce::String (u8 ("(sin señal)"))));
    L.add ("");
    L.add (u8 ("== Parámetros =="));
    if (auto xml = apvts.copyState().createXml()) L.add (xml->toString());
    L.add ("== Registro ==");
    { const juce::ScopedLock sl (logLock); L.addArray (logLines); }
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("SampledHead Tuner");
    dir.createDirectory();
    auto f = dir.getChildFile ("informe-tuner-v" + juce::String (kVersion) + "-" + juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S") + ".txt");
    f.replaceWithText (L.joinIntoString ("\n"));
    log ("Informe guardado: " + f.getFullPathName());
    return f;
}

juce::AudioProcessorEditor* TunerProcessor::createEditor() { return new TunerEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TunerProcessor(); }
