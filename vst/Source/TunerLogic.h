// SampledHead Tuner — lógica de afinación (cuerda objetivo, cents y nombres de nota). Sin dependencias de audio.
#pragma once
#include <juce_core/juce_core.h>
#include "Tunings.h"
#include "TunerDSP.h"

namespace shtuner
{
enum Instrument { Guitar = 0, Bass, Ukulele, Banjo, Chromatic, NumInstruments };

inline juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }

struct Target
{
    bool valid = false;
    int midi = 0;          // nota objetivo (con capo)
    int stringIndex = -1;  // -1 en modo cromático
    double cents = 0.0;    // desviación respecto a la nota objetivo
    double targetHz = 0.0;
};

// Índices (en kTunings) de las afinaciones de un instrumento
inline juce::Array<int> tuningsFor (int instrument)
{
    juce::Array<int> r;
    for (int i = 0; i < kNumTunings; ++i) if (kTunings[i].instrument == instrument) r.add (i);
    return r;
}

// Afinación válida para el instrumento (si no coincide, la primera del instrumento)
inline int resolveTuning (int instrument, int tuningIndex)
{
    if (instrument == Chromatic) return -1;
    if (juce::isPositiveAndBelow (tuningIndex, kNumTunings) && kTunings[tuningIndex].instrument == instrument) return tuningIndex;
    auto l = tuningsFor (instrument);
    return l.isEmpty() ? -1 : l[0];
}

inline Target computeTarget (double hz, double a4, int instrument, int tuningIndex, int capo, bool autoString, int selString)
{
    Target t;
    if (! (hz > 0.0)) return t;
    const double m = hzToMidi (hz, a4);
    const int tu = resolveTuning (instrument, tuningIndex);
    if (tu < 0)
    {
        t.midi = (int) std::lround (m);
        t.stringIndex = -1;
    }
    else
    {
        const auto& T = kTunings[tu];
        int best = 0;
        if (autoString)
        {
            double bd = 1e9;
            for (int i = 0; i < T.numStrings; ++i)
            {
                const double d = std::abs (m - (T.midi[(size_t) i] + capo));
                if (d < bd) { bd = d; best = i; }
            }
        }
        else best = juce::jlimit (0, T.numStrings - 1, selString);
        t.stringIndex = best;
        t.midi = T.midi[(size_t) best] + capo;
    }
    t.cents = (m - t.midi) * 100.0;
    t.targetHz = midiToHz (t.midi, a4);
    t.valid = true;
    return t;
}

// Nombre de nota. names: 0 letras (C D E), 1 Do Re Mi. acc: 0 auto, 1 sostenidos, 2 bemoles. preferFlat: para "auto".
inline juce::String noteName (int midi, int names, int acc, bool preferFlat)
{
    static const char* letters[] = { "C", "C", "D", "D", "E", "F", "F", "G", "G", "A", "A", "B" };
    static const char* solfeo[]  = { "Do", "Do", "Re", "Re", "Mi", "Fa", "Fa", "Sol", "Sol", "La", "La", "Si" };
    static const bool black[]    = { false, true, false, true, false, false, true, false, true, false, true, false };
    const int pc = ((midi % 12) + 12) % 12;
    const bool flat = acc == 2 || (acc == 0 && preferFlat);
    const auto& base = names == 1 ? solfeo : letters;
    if (! black[pc]) return base[pc];
    // tecla negra: sostenido de la nota de abajo o bemol de la de arriba
    return flat ? juce::String (base[(pc + 1) % 12]) + u8 ("♭") : juce::String (base[(pc + 11) % 12]) + u8 ("♯");
}
inline int octaveOf (int midi) { return (int) std::floor (midi / 12.0) - 1; }

// ¿La afinación está escrita con bemoles? (para "auto")
inline bool tuningPrefersFlat (int tuningIndex)
{
    if (! juce::isPositiveAndBelow (tuningIndex, kNumTunings)) return false;
    const juce::String n (kTunings[tuningIndex].notes);
    for (auto tok : juce::StringArray::fromTokens (n, " ", ""))
        if (tok.length() >= 2 && tok[1] == 'b') return true;
    return false;
}
} // namespace shtuner
