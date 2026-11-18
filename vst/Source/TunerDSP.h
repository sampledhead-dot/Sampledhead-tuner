// SampledHead Tuner — detector de tono (YIN causal con correlación por FFT).
// Mismo algoritmo que FuzzHead; aquí con rango amplio para todos los instrumentos.
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>
#include <algorithm>

namespace shtuner
{
class PitchTracker
{
public:
    void prepare (double sampleRate, float minF, float maxF)
    {
        sr = (float) sampleRate;
        struct { float minF, maxF; } r { minF, maxF };
        // un afinador no necesita 3 ms de reacción: un análisis cada ~10 ms ahorra CPU en el DAW
        hop = std::max (128, (int) std::pow (2.0, std::round (std::log2 (sr / 95.0))));
        W = 1; while ((float) W < sr / r.minF + 2.0f) W <<= 1;
        tauMin = std::max (2, (int) std::floor (sr / r.maxF));
        tauMax = std::min (W - 1, (int) std::ceil (sr / r.minF));
        const int n = 4 * W;
        int order = 0; while ((1 << order) < n) ++order;
        fft = std::make_unique<juce::dsp::FFT> (order);
        fa.assign ((size_t) (2 * n), 0.0f);
        fb.assign ((size_t) (2 * n), 0.0f);
        hist.assign ((size_t) (2 * W), 0.0f);
        P.assign ((size_t) (2 * W + 1), 0.0);
        cm.assign ((size_t) W, 1.0f);
        writePos = 0; counter = 0; pitch = 0.0f;
        gateLin = juce::Decibels::decibelsToGain (-52.0f);
    }

    // Devuelve true si se acaba de calcular un nuevo frame
    bool push (float x)
    {
        hist[(size_t) writePos] = x;
        writePos = (writePos + 1) % (2 * W);
        if (++counter < hop) return false;
        counter = 0;
        analyse();
        return true;
    }
    float getPitch() const { return pitch; }   // Hz, 0 = sin voz
    float getClarity() const { return clarity; }
    float getRmsDb() const { return rmsDb; }
    void setGateDb (float db) { gateLin = juce::Decibels::decibelsToGain (db); }

private:
    void analyse()
    {
        const int n = 4 * W;
        std::fill (fa.begin(), fa.end(), 0.0f);
        std::fill (fb.begin(), fb.end(), 0.0f);
        P[0] = 0.0;
        double eRecent = 0.0;
        for (int i = 0; i < 2 * W; ++i)
        {
            const float v = hist[(size_t) ((writePos + i) % (2 * W))]; // más antiguo -> más reciente
            fb[(size_t) i] = v;
            if (i < W) fa[(size_t) i] = v;
            P[(size_t) i + 1] = P[(size_t) i] + (double) v * v;
            if (i >= W) eRecent += (double) v * v;
        }
        const float rms = (float) std::sqrt (eRecent / W);
        rmsDb = juce::Decibels::gainToDecibels (rms, -120.0f);
        if (rms < gateLin) { pitch = 0.0f; clarity = 0.0f; return; }

        fft->performRealOnlyForwardTransform (fa.data());
        fft->performRealOnlyForwardTransform (fb.data());
        for (size_t k = 0; k < (size_t) n; ++k)   // conj(A) * B
        {
            const float ar = fa[2 * k], ai = fa[2 * k + 1];
            const float br = fb[2 * k], bi = fb[2 * k + 1];
            fb[2 * k]     = ar * br + ai * bi;
            fb[2 * k + 1] = ar * bi - ai * br;
        }
        fft->performRealOnlyInverseTransform (fb.data());

        const double e0 = P[(size_t) W];
        // r(0) debe valer e0: así obtenemos la escala de la FFT inversa sea cual sea el backend
        const double scale = std::abs (fb[0]) > 1e-20 ? e0 / (double) fb[0] : 1.0;
        double run = 0.0;
        cm[0] = 1.0f;
        for (int t = 1; t <= tauMax; ++t)
        {
            const double et = P[(size_t) (t + W)] - P[(size_t) t];
            const double d = std::max (0.0, e0 + et - 2.0 * fb[(size_t) t] * scale);
            run += d;
            cm[(size_t) t] = run > 0.0 ? (float) (d * t / run) : 1.0f;
        }
        int tau = -1;
        for (int t = tauMin; t <= tauMax; ++t)
        {
            if (cm[(size_t) t] < 0.15f)
            {
                while (t + 1 <= tauMax && cm[(size_t) t + 1] < cm[(size_t) t]) ++t;
                tau = t; break;
            }
        }
        if (tau < 0)
        {
            tau = tauMin;
            for (int t = tauMin; t <= tauMax; ++t) if (cm[(size_t) t] < cm[(size_t) tau]) tau = t;
        }
        if (cm[(size_t) tau] > 0.35f) { pitch = 0.0f; clarity = 0.0f; return; }
        float tf = (float) tau;
        if (tau > 1 && tau < tauMax)
        {
            const float s0 = cm[(size_t) tau - 1], s1 = cm[(size_t) tau], s2 = cm[(size_t) tau + 1];
            const float den = s0 - 2.0f * s1 + s2;
            if (std::abs (den) > 1e-12f) tf = (float) tau + 0.5f * (s0 - s2) / den;
        }
        // Notas agudas (periodo corto): medir sobre varios periodos y dividir, para no perder precisión
        if (tau < 100)
        {
            const int mult = (int) std::ceil (100.0 / tau);
            const int c = (int) std::lround (tf * mult);
            int best = -1;
            for (int t = c - 2; t <= c + 2; ++t)
                if (t > 1 && t < tauMax && (best < 0 || cm[(size_t) t] < cm[(size_t) best])) best = t;
            if (best > 1 && best < tauMax)
            {
                const float s0 = cm[(size_t) best - 1], s1 = cm[(size_t) best], s2 = cm[(size_t) best + 1];
                const float den = s0 - 2.0f * s1 + s2;
                const float tm = (float) best + (std::abs (den) > 1e-12f ? 0.5f * (s0 - s2) / den : 0.0f);
                if (std::abs (tm / mult - tf) < 0.5f) tf = tm / (float) mult;
            }
        }
        pitch = sr / tf;
        clarity = 1.0f - cm[(size_t) tau];
    }

    float sr = 44100.0f, gateLin = 0.0f, pitch = 0.0f, clarity = 0.0f, rmsDb = -120.0f;
    int hop = 128, W = 1024, tauMin = 2, tauMax = 600, writePos = 0, counter = 0;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> fa, fb, hist, cm;
    std::vector<double> P;
};


// Nota MIDI (con decimales) a partir de Hz y la referencia del La
inline double hzToMidi (double hz, double a4) { return 69.0 + 12.0 * std::log2 (hz / a4); }
inline double midiToHz (double m, double a4) { return a4 * std::pow (2.0, (m - 69.0) / 12.0); }
} // namespace shtuner
