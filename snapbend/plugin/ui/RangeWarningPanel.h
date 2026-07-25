#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/** The prompt that appears when you drag a bend further than the synth has been
    told it may travel.

    Without it the failure is silent and baffling: you draw a 14 semitone bend,
    you hear about 12, and nothing anywhere tells you why. So the moment the
    curve leaves the reachable area, this slides in and says — in plain words —
    to go and allow more semitones in the synth.
*/
class RangeWarningPanel : public juce::Component
{
public:
    RangeWarningPanel();
    ~RangeWarningPanel() override;

    /** @param neededSemitones   furthest point the drawn curve reaches
        @param currentRange      what the synth is currently allowed */
    void setRequirement (double neededSemitones, double currentRange);

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Fired by the "set range" button, with the range to move to. */
    std::function<void (double)> onRaiseRange;

    /** Fired by the dismiss button. */
    std::function<void()> onDismiss;

private:
    juce::TextButton raiseButton   { "Raise range" };
    juce::TextButton dismissButton { "Dismiss" };

    double needed       = 0.0;
    double currentRange = 0.0;

    juce::String headline;
    juce::String body;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RangeWarningPanel)
};
