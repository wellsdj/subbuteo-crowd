#include "ZoomControls.h"
#include "SnapBendLookAndFeel.h"

using namespace snapbend;

namespace
{
    /** A double-headed arrow, drawn rather than typed, so it does not depend on
        the system font happening to carry the arrow glyphs. */
    juce::Path makeDoubleArrow (juce::Rectangle<float> area, bool vertical)
    {
        juce::Path path;

        const auto centre = area.getCentre();
        const float length = (vertical ? area.getHeight() : area.getWidth()) * 0.5f;
        const float head   = 3.2f;

        if (vertical)
        {
            path.startNewSubPath (centre.x, centre.y - length);
            path.lineTo (centre.x, centre.y + length);

            path.startNewSubPath (centre.x - head, centre.y - length + head);
            path.lineTo (centre.x, centre.y - length);
            path.lineTo (centre.x + head, centre.y - length + head);

            path.startNewSubPath (centre.x - head, centre.y + length - head);
            path.lineTo (centre.x, centre.y + length);
            path.lineTo (centre.x + head, centre.y + length - head);
        }
        else
        {
            path.startNewSubPath (centre.x - length, centre.y);
            path.lineTo (centre.x + length, centre.y);

            path.startNewSubPath (centre.x - length + head, centre.y - head);
            path.lineTo (centre.x - length, centre.y);
            path.lineTo (centre.x - length + head, centre.y + head);

            path.startNewSubPath (centre.x + length - head, centre.y - head);
            path.lineTo (centre.x + length, centre.y);
            path.lineTo (centre.x + length - head, centre.y + head);
        }

        return path;
    }
}

ZoomControls::ZoomControls()
{
    for (auto* slider : { &verticalSlider, &horizontalSlider })
    {
        slider->setSliderStyle (juce::Slider::LinearHorizontal);
        slider->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider->setRange (0.0, 1.0, 0.001);
        slider->setValue (0.5, juce::dontSendNotification);
        slider->setDoubleClickReturnValue (true, 0.5);

        slider->onValueChange = [this]
        {
            if (onZoomChanged != nullptr)
                onZoomChanged();
        };

        addAndMakeVisible (*slider);
    }

    verticalSlider.setTooltip ("How many semitones fit on screen. Double-click to reset.");
    horizontalSlider.setTooltip ("How much of the timeline fits on screen. Double-click to reset.");
}

void ZoomControls::setZoom (double horizontal, double vertical)
{
    horizontalSlider.setValue (horizontal, juce::dontSendNotification);
    verticalSlider.setValue (vertical, juce::dontSendNotification);
}

void ZoomControls::paint (juce::Graphics& g)
{
    g.setColour (colours::textDim);

    const juce::PathStrokeType stroke (1.3f, juce::PathStrokeType::curved,
                                       juce::PathStrokeType::rounded);

    g.strokePath (makeDoubleArrow (verticalIconArea.toFloat().reduced (2.0f, 1.0f), true), stroke);
    g.strokePath (makeDoubleArrow (horizontalIconArea.toFloat().reduced (1.0f, 2.0f), false), stroke);
}

void ZoomControls::resized()
{
    auto area = getLocalBounds();

    // Vertical zoom first, then horizontal — the same order and grouping Logic
    // uses, so the muscle memory carries over.
    const int iconWidth  = 14;
    const int sliderWidth = juce::jmax (40, (area.getWidth() - 2 * iconWidth - 18) / 2);

    verticalIconArea = area.removeFromLeft (iconWidth);
    verticalSlider.setBounds (area.removeFromLeft (sliderWidth).reduced (2, 0));

    area.removeFromLeft (18);

    horizontalIconArea = area.removeFromLeft (iconWidth);
    horizontalSlider.setBounds (area.removeFromLeft (sliderWidth).reduced (2, 0));
}
