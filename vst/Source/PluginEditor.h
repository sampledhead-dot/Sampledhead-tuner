#pragma once
#include "PluginProcessor.h"

namespace tcol
{
    const juce::Colour bg (0xff0b0f14), panel (0xff111821), panel2 (0xff17202b), line (0xff222c38),
                       text (0xffe6edf3), muted (0xff8b98a5), teal (0xff2dd4bf), tealD (0xff159c8c),
                       purple (0xff8b5cf6), pink (0xffec4899), warn (0xfff59e0b), bad (0xfff87171);
}

class TunerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TunerLookAndFeel();
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return juce::FontOptions (juce::jmin (15.0f, h * 0.48f), juce::Font::bold); }
};

// Medidor de aguja: -50 … +50 cents
class NeedleMeter : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void set (bool active, double cents, int tol, const juce::String& note, const juce::String& octave,
              const juce::String& sub, double hz, float levelDb, float gateDb);
private:
    bool active = false; double cents = 0, shown = 0; int tol = 3;
    juce::String note { "-" }, octave, sub; double hz = 0; float level = -120, gate = -52;
};

class TunerEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TunerEditor (TunerProcessor&);
    ~TunerEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setParam (const juce::String& id, float v);
    float par (const juce::String& id) const { return proc.p (id); }
    void rebuildTunings();
    void refreshStrings();

    TunerProcessor& proc;
    TunerLookAndFeel lnf;
    juce::OwnedArray<juce::TextButton> instButtons, stringButtons;
    juce::ComboBox tuningBox, namesBox, accBox;
    juce::TextButton capoMinus { "-" }, capoPlus { "+" }, a4Minus { "-" }, a4Plus { "+" };
    juce::TextButton autoB { "Auto" }, manualB { "Manual" }, refB, muteB, reportB;
    juce::Label capoLabel, a4Label, tolLabel, gateLabel, namesLabel, accLabel, footer, muteInfo, helpInfo;
    juce::Slider tolSlider, gateSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tolAtt, gateAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> namesAtt, accAtt;
    NeedleMeter meter;
    int lastInst = -1, lastTuning = -2, lastCapo = -1, lastNames = -1, lastAcc = -1;
    double holdUntil = 0; shtuner::Target held; double heldHz = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TunerEditor)
};
