// Pruebas offline de SampledHead Tuner (plugin): precisión en las 52 afinaciones, cromático, capo, La, modos,
// silencio, tono de referencia, puerta, estado, rendimiento, informe y captura del editor.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <iostream>

using namespace shtuner;
static constexpr double SR = 48000.0;
static int failures = 0;
#define CHECK(c, msg) do { if (!(c)) { std::cout << "  FALLO: " << msg << "\n"; ++failures; } } while (0)

static void setP (TunerProcessor& p, const juce::String& id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); }

// Procesa una señal (mono → estéreo) en bloques de 256; devuelve la salida (canal 0)
static std::vector<float> run (TunerProcessor& p, const std::vector<float>& x)
{
    juce::AudioBuffer<float> b (2, 256); juce::MidiBuffer m; std::vector<float> out;
    for (size_t pos = 0; pos < x.size(); pos += 256)
    {
        const int n = (int) std::min<size_t> (256, x.size() - pos);
        b.setSize (2, n, false, false, true);
        for (int i = 0; i < n; ++i) { b.setSample (0, i, x[pos + (size_t) i]); b.setSample (1, i, x[pos + (size_t) i]); }
        p.processBlock (b, m);
        for (int i = 0; i < n; ++i) out.push_back (b.getSample (0, i));
    }
    return out;
}
static std::vector<float> sine (double hz, double sec, float amp = 0.3f)
{
    std::vector<float> v ((size_t) (sec * SR));
    for (size_t i = 0; i < v.size(); ++i) v[i] = amp * (float) std::sin (juce::MathConstants<double>::twoPi * hz * (double) i / SR);
    return v;
}
static std::vector<float> pluck (double hz, double sec)
{
    const int N = (int) (sec * SR), P = juce::jmax (2, (int) std::round (SR / hz));
    std::vector<float> out ((size_t) N), buf ((size_t) P); uint32_t s = 777;
    for (auto& v : buf) { s = s * 1103515245u + 12345u; v = ((float) s / 4294967296.0f * 2 - 1) * 0.5f; }
    size_t k = 0;
    for (int i = 0; i < N; ++i) { const float a = buf[k], b2 = buf[(k + 1) % (size_t) P]; buf[k] = 0.997f * 0.5f * (a + b2); out[(size_t) i] = a; k = (k + 1) % (size_t) P; }
    return out;
}
static double freqByZeroCross (const std::vector<float>& v, size_t from)
{
    int zc = 0; size_t first = 0, last = 0;
    for (size_t i = from + 1; i < v.size(); ++i) if (v[i - 1] <= 0 && v[i] > 0) { if (! zc) first = i; last = i; ++zc; }
    return zc > 1 ? (zc - 1) * SR / (double) (last - first) : 0;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    std::cout << "== 52 afinaciones, cada cuerda (seno +7 cents, modo automático) ==\n";
    int ok = 0, total = 0; double worst = 0;
    for (int tu = 0; tu < kNumTunings; ++tu)
    {
        const auto& T = kTunings[tu];
        for (int s = 0; s < T.numStrings; ++s)
        {
            TunerProcessor p; p.prepareToPlay (SR, 256);
            setP (p, pid::instrument, (float) T.instrument); setP (p, pid::tuning, (float) tu);
            const double f = midiToHz (T.midi[(size_t) s] + 0.07, 440.0);
            run (p, sine (f, 0.5));
            auto t = p.currentTarget(); ++total;
            const double err = std::abs (t.cents - 7.0);
            const bool good = t.valid && t.midi == T.midi[(size_t) s] && err < 1.5;
            // cuerdas al unísono/octava en la misma afinación: basta con que la nota sea la correcta
            if (good) ++ok; else std::cout << "  " << juce::String (juce::CharPointer_UTF8 (T.name)) << " cuerda " << s << ": midi " << t.midi << " cents " << t.cents << "\n";
            worst = std::max (worst, t.valid ? err : 99.0);
        }
    }
    std::cout << "  " << ok << "/" << total << " correctas · error máximo " << worst << " cents\n";
    CHECK (ok == total, "afinaciones");

    std::cout << "== Guitarra estándar con cuerda pulsada (Karplus-Strong, afinada) ==\n";
    for (int s = 0; s < 6; ++s)
    {
        TunerProcessor p; p.prepareToPlay (SR, 256);
        const double f0 = midiToHz (kTunings[0].midi[(size_t) s], 440.0);
        run (p, pluck (f0, 0.6));
        // esta cuerda simulada suena a SR/(P-0,5): calculamos lo que de verdad suena
        const int P = juce::jmax (2, (int) std::round (SR / f0));
        const double expected = 1200.0 * std::log2 ((SR / (P - 0.5)) / f0);
        auto t = p.currentTarget();
        std::cout << "  cuerda " << (6 - s) << ": mide " << t.cents << " cents, suena a " << expected << " cents\n";
        CHECK (t.valid && t.stringIndex == s && std::abs (t.cents - expected) < 1.5, "pulsada");
    }

    std::cout << "== Cromático C1..C7 ==\n";
    { int good = 0;
      for (int m = 24; m <= 108; m += 6)
      { TunerProcessor p; p.prepareToPlay (SR, 256); setP (p, pid::instrument, Chromatic); run (p, sine (midiToHz (m - 0.12, 440), 0.5));
        auto t = p.currentTarget(); if (t.valid && t.midi == m && std::abs (t.cents + 12) < 1.5) ++good; else std::cout << "  midi " << m << " -> " << t.midi << " " << t.cents << "\n"; }
      std::cout << "  " << good << "/15 (C1..C8)\n"; CHECK (good == 15, "cromático"); }

    std::cout << "== La 432, capo, manual ==\n";
    { TunerProcessor p; p.prepareToPlay (SR, 256); setP (p, pid::instrument, Chromatic); setP (p, pid::a4, 432); run (p, sine (432, 0.5));
      auto t = p.currentTarget(); std::cout << "  La 432: midi " << t.midi << " cents " << t.cents << "\n"; CHECK (t.midi == 69 && std::abs (t.cents) < 1, "a4"); }
    { TunerProcessor p; p.prepareToPlay (SR, 256); setP (p, pid::capo, 2); run (p, sine (midiToHz (42, 440), 0.5));
      auto t = p.currentTarget(); std::cout << "  capo 2, F#2: cuerda " << t.stringIndex << " cents " << t.cents << "\n"; CHECK (t.stringIndex == 0 && std::abs (t.cents) < 1, "capo"); }
    { TunerProcessor p; p.prepareToPlay (SR, 256); setP (p, pid::autoString, 0); setP (p, pid::string, 2); run (p, sine (midiToHz (40, 440), 0.5));
      auto t = p.currentTarget(); std::cout << "  manual cuerda D3 tocando E2: " << t.cents << " cents\n"; CHECK (t.stringIndex == 2 && std::abs (t.cents + 1000) < 2, "manual"); }

    std::cout << "== Salida: silencio, tono de referencia y puerta ==\n";
    { TunerProcessor p; p.prepareToPlay (SR, 256); setP (p, pid::mute, 1); auto o = run (p, sine (110, 0.5));
      float pk = 0; for (size_t i = 4800; i < o.size(); ++i) pk = std::max (pk, std::abs (o[i])); std::cout << "  silenciado: pico " << pk << "\n"; CHECK (pk < 1e-4f, "mute"); }
    { TunerProcessor p; p.prepareToPlay (SR, 256); setP (p, pid::autoString, 0); setP (p, pid::string, 0); setP (p, pid::mute, 1); setP (p, pid::refTone, 1);
      auto o = run (p, std::vector<float> ((size_t) SR, 0.0f)); const double f = freqByZeroCross (o, 9600);
      std::cout << "  tono de referencia cuerda 6: " << f << " Hz (esperado 82.41)\n"; CHECK (std::abs (f - 82.41) < 0.3, "ref"); }
    { TunerProcessor p; p.prepareToPlay (SR, 256); run (p, sine (110, 0.5, juce::Decibels::decibelsToGain (-70.0f)));
      auto t = p.currentTarget(); std::cout << "  -70 dBFS con puerta -52: " << (t.valid ? "detecta (mal)" : "ignorado") << "\n"; CHECK (! t.valid, "gate");
      setP (p, pid::gate, -80); run (p, sine (110, 0.5, juce::Decibels::decibelsToGain (-70.0f))); CHECK (p.currentTarget().valid, "gate -80"); }
    { TunerProcessor p; p.prepareToPlay (SR, 256); auto in = sine (110, 0.5); auto o = run (p, in);
      double d = 0; for (size_t i = 0; i < o.size(); ++i) d = std::max (d, (double) std::abs (o[i] - in[i])); std::cout << "  paso directo: diferencia máx " << d << "\n"; CHECK (d < 1e-6, "passthrough"); }

    std::cout << "== Estado, rendimiento, informe ==\n";
    { TunerProcessor a; setP (a, pid::instrument, 1); setP (a, pid::tuning, 33); setP (a, pid::a4, 442); setP (a, pid::capo, 3);
      juce::MemoryBlock mb; a.getStateInformation (mb); TunerProcessor b; b.setStateInformation (mb.getData(), (int) mb.getSize());
      CHECK (b.p (pid::instrument) == 1 && b.p (pid::tuning) == 33 && b.p (pid::a4) == 442 && b.p (pid::capo) == 3, "estado"); std::cout << "  estado restaurado OK\n"; }
    { TunerProcessor p; p.prepareToPlay (SR, 256); auto x = pluck (82.41, 10.0);
      const auto t0 = juce::Time::getMillisecondCounterHiRes(); run (p, x); const double ms = juce::Time::getMillisecondCounterHiRes() - t0;
      std::cout << "  10 s de audio en " << ms << " ms (" << 10000.0 / ms << "x tiempo real)\n"; CHECK (ms < 2000, "rendimiento"); }

    {   TunerProcessor p; p.prepareToPlay (SR, 256); setP (p, pid::tuning, 0);
        run (p, sine (midiToHz (45.04, 440), 0.4));     // A2 +4 cents
        auto f = p.saveReport ("prueba"); std::cout << "  informe: " << f.getFileName() << " (" << f.getSize() << " bytes)\n"; CHECK (f.existsAsFile() && f.getSize() > 500, "informe");
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        auto cont = sine (midiToHz (45.04, 440), 0.6);    // señal continua troceada en bloques de 20 ms
        for (int i = 0; i < 30; ++i) { run (p, std::vector<float> (cont.begin() + i * 960, cont.begin() + (i + 1) * 960)); juce::MessageManager::getInstance()->runDispatchLoopUntil (35); }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
        juce::File out ("/tmp/tuner_editor.png"); out.deleteFile(); juce::FileOutputStream fos (out); juce::PNGImageFormat png; png.writeImageToStream (img, fos);
        std::cout << "  captura " << img.getWidth() << "x" << img.getHeight() << "\n";
    }
    // Carga del .vst3 compilado como lo haría un DAW (si se indica la ruta)
    if (auto* path = std::getenv ("TUNER_VST3"))
    {
        std::cout << "== Plugin VST3 cargado como en un DAW ==\n";
        juce::VST3PluginFormat fmt; juce::OwnedArray<juce::PluginDescription> found;
        fmt.findAllTypesForFile (found, path);
        CHECK (found.size() == 1, "escaneo VST3");
        if (found.size() == 1)
        {
            std::cout << "  " << found[0]->name << " · " << found[0]->manufacturerName << " · " << found[0]->category << " · v" << found[0]->version << "\n";
            juce::String err;
            auto inst = fmt.createInstanceFromDescription (*found[0], SR, 256, err);
            CHECK (inst != nullptr, "instancia VST3: " + err.toStdString());
            if (inst)
            {
                inst->enableAllBuses(); inst->prepareToPlay (SR, 256);
                juce::AudioBuffer<float> b (2, 256); juce::MidiBuffer mb; auto x = sine (110.0, 1.0); float pk = 0;
                for (size_t pos = 0; pos + 256 <= x.size(); pos += 256)
                { for (int i = 0; i < 256; ++i) { b.setSample (0, i, x[pos + (size_t) i]); b.setSample (1, i, x[pos + (size_t) i]); }
                  inst->processBlock (b, mb); pk = std::max (pk, b.getMagnitude (0, 256)); }
                std::cout << "  " << inst->getParameters().size() << " parámetros · audio procesado, pico de salida " << pk << "\n";
                CHECK (std::abs (pk - 0.3f) < 0.01f, "paso de audio VST3");
                std::unique_ptr<juce::AudioProcessorEditor> ed (inst->createEditorIfNeeded());
                CHECK (ed != nullptr, "editor VST3"); if (ed) std::cout << "  editor " << ed->getWidth() << "x" << ed->getHeight() << "\n";
                ed.reset(); inst->releaseResources();
            }
        }
    }
    std::cout << (failures ? "FALLOS: " + std::to_string (failures) : std::string ("TODO OK")) << "\n";
    return failures ? 1 : 0;
}
