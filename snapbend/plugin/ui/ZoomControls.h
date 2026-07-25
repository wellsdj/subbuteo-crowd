#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/** The pair of zoom sliders, laid out like the ones in Logic's own editors:
    a vertical-arrows icon for how many semitones are on screen, and a
    horizontal-arrows icon for how much of the timeline is.

    Both run 0 (fully zoomed out) to 1 (fully zoomed in), and both sit at 0.5
    by default, which is the view you get when you first open the plugin.
*/
class ZoomControls : public juce::Component
{
public:
    ZoomControls();

    void paint (juce::Graphics&) override;
    void resized() override;

    void setZoom (double horizontal, double vertical);

    double getHorizontalZoom() const { return horizontalSlider.getValue(); }
    double getVerticalZoom() const   { return verticalSlider.getValue(); }

    /** Fired when either slider moves. */
    std::function<void()> onZoomChanged;

private:
    juce::Slider verticalSlider;   ///< semitones on screen
    juce::Slider horizontalSlider; ///< beats on screen

    juce::Rectangle<int> verticalIconArea, horizontalIconArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZoomControls)
};
