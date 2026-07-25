#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "ui/BendLaneComponent.h"
#include "ui/RangeWarningPanel.h"
#include "ui/SnapBendLookAndFeel.h"

/** A rotary control with its name underneath and its value above.

    Kept as one component so the layout code can space them evenly and they can
    never drift out of alignment with each other.
*/
class LabelledKnob : public juce::Component
{
public:
    LabelledKnob (const juce::String& name, std::function<juce::String (double)> formatter);

    void resized() override;
    void paint (juce::Graphics&) override;

    juce::Slider& getSlider() noexcept { return slider; }

    /** Refreshes the value readout. */
    void updateReadout();

private:
    juce::Slider slider;
    juce::String knobName;
    juce::String readout;

    std::function<juce::String (double)> format;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LabelledKnob)
};

class SnapBendEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit SnapBendEditor (SnapBendProcessor&);
    ~SnapBendEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    /** Decides whether the "your synth can't reach this" prompt should be on
        screen, and puts the right numbers in it. */
    void refreshRangeWarning();

    SnapBendProcessor& processor;

    SnapBendLookAndFeel lookAndFeel;

    BendLaneComponent bendLane;
    RangeWarningPanel rangeWarning;

    LabelledKnob rangeKnob { "RANGE", [] (double v) { return juce::String (juce::roundToInt (v)) + " st"; } };
    LabelledKnob curveKnob { "CURVE", [] (double v)
                             {
                                 if (std::abs (v) < 0.02) return juce::String ("Linear");
                                 return juce::String (v > 0 ? "Early " : "Late ")
                                      + juce::String (juce::roundToInt (std::abs (v) * 100.0)) + "%";
                             } };
    LabelledKnob snapKnob  { "SNAP",  [] (double v)
                             {
                                 if (v >= 0.999) return juce::String ("Semitones");
                                 if (v <= 0.001) return juce::String ("Free");
                                 return juce::String (juce::roundToInt (v * 100.0)) + "%";
                             } };
    LabelledKnob mixKnob   { "MIX",   [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; } };

    juce::ToggleButton autoRangeButton { "Tell the synth its bend range automatically" };
    juce::TextButton   panicButton     { "Reset bend" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> rangeAttachment, curveAttachment,
                                      snapAttachment, mixAttachment;
    std::unique_ptr<ButtonAttachment> autoRangeAttachment;

    /** The furthest excursion we last warned about, so the prompt does not pop
        back up every time the mouse twitches after being dismissed. */
    double dismissedAtRequirement = -1.0;

    /** What the prompt currently says, so the timer can skip rebuilding it. */
    double lastWarnedRequirement = -1.0;
    double lastWarnedRange       = -1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnapBendEditor)
};
