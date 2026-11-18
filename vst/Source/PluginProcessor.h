#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "TunerLogic.h"

namespace pid
{
    static const juce::String instrument { "instrument" }, tuning { "tuning" }, capo { "capo" }, a4 { "a4" },
        autoString { "autoString" }, string { "string" }, tol { "tol" }, gate { "gate" }, names { "names" },
        acc { "acc" }, mute { "mute" }, refTone { "refTone" };
}

class TunerProcessor : public juce::AudioProcessor
{
public:
    TunerProcessor();
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "SampledHead Tuner"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float p (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }

    // Lectura desde la interfaz (hilo de mensajes)
    struct Reading { float hz = 0, clarity = 0, levelDb = -120; juce::uint32 frame = 0; };
    Reading getReading() const { return { hz.load(), clarity.load(), levelDb.load(), frames.load() }; }
    shtuner::Target currentTarget() const;   // objetivo con los parámetros actuales y la última lectura
    double getSampleRateSafe() const { return sr; }

    // Informe (.txt) para soporte
    juce::File saveReport (const juce::String& where);
    void log (const juce::String& msg);

    static constexpr const char* kVersion = "1.1.0";

private:
    shtuner::PitchTracker tracker;
    double sr = 44100.0;
    int blockSize = 512;
    std::array<float, 5> hist {};
    int histN = 0;
    float smoothLog = 0.0f;
    std::atomic<float> hz { 0.0f }, clarity { 0.0f }, levelDb { -120.0f };
    std::atomic<juce::uint32> frames { 0 };
    std::atomic<float> refHz { 0.0f };       // frecuencia del tono de referencia (la fija la interfaz)
    double refPhase = 0.0;
    juce::SmoothedValue<float> refGain, passGain;

    juce::CriticalSection logLock;
    juce::StringArray logLines;

public:
    std::atomic<int> lastString { 0 };   // última cuerda detectada en modo automático
    float getReferenceHz() const { return refHz.load(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TunerProcessor)
};
