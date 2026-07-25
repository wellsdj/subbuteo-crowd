#include "SnapBendLookAndFeel.h"

using namespace snapbend;

SnapBendLookAndFeel::SnapBendLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, colours::background);
    setColour (juce::Label::textColourId,                 colours::text);
    setColour (juce::Slider::textBoxTextColourId,         colours::text);
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
    setColour (juce::TextButton::buttonColourId,          colours::panelRaised);
    setColour (juce::TextButton::textColourOffId,         colours::text);
    setColour (juce::TextButton::textColourOnId,          colours::background);
    setColour (juce::TooltipWindow::backgroundColourId,   colours::panelRaised);
    setColour (juce::TooltipWindow::textColourId,         colours::text);
}

void SnapBendLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float rotaryStartAngle,
                                            float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const float trackWidth = juce::jmax (2.5f, radius * 0.14f);
    const float arcRadius  = radius - trackWidth * 0.6f;

    // Background arc.
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);

    g.setColour (colours::gridLine);
    g.strokePath (track, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Filled arc. Bipolar controls fill outwards from the top rather than from
    // the far left, so "no effect" reads as centred.
    const bool  bipolar   = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float fillStart = bipolar ? rotaryStartAngle + 0.5f * (rotaryEndAngle - rotaryStartAngle)
                                    : rotaryStartAngle;

    if (std::abs (angle - fillStart) > 0.001f)
    {
        juce::Path fill;
        fill.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                            juce::jmin (fillStart, angle), juce::jmax (fillStart, angle), true);

        g.setColour (slider.isEnabled() ? colours::accent : colours::textDim);
        g.strokePath (fill, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // Knob body.
    const float bodyRadius = arcRadius - trackWidth * 1.1f;

    g.setColour (colours::panelRaised);
    g.fillEllipse (juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre));

    g.setColour (colours::gridLineBold);
    g.drawEllipse (juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre), 1.0f);

    // Pointer.
    juce::Path pointer;
    const float pointerLength = bodyRadius * 0.62f;
    const float pointerThickness = juce::jmax (2.0f, bodyRadius * 0.13f);

    pointer.addRoundedRectangle (-pointerThickness * 0.5f, -bodyRadius * 0.86f,
                                 pointerThickness, pointerLength, pointerThickness * 0.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre));

    g.setColour (slider.isEnabled() ? colours::text : colours::textDim);
    g.fillPath (pointer);
}

void SnapBendLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                const juce::Colour& backgroundColour,
                                                bool shouldDrawButtonAsHighlighted,
                                                bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const float corner = juce::jmin (6.0f, bounds.getHeight() * 0.3f);

    auto fill = backgroundColour;

    if (shouldDrawButtonAsDown)
        fill = fill.brighter (0.25f);
    else if (shouldDrawButtonAsHighlighted)
        fill = fill.brighter (0.12f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (colours::gridLineBold);
    g.drawRoundedRectangle (bounds, corner, 1.0f);
}

void SnapBendLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                            bool shouldDrawButtonAsHighlighted, bool)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const float switchWidth  = juce::jmin (34.0f, bounds.getWidth());
    const float switchHeight = juce::jmin (18.0f, bounds.getHeight());

    const auto track = juce::Rectangle<float> (switchWidth, switchHeight)
                           .withY (bounds.getCentreY() - switchHeight * 0.5f);

    const bool on = button.getToggleState();

    g.setColour (on ? colours::accent.withAlpha (0.85f) : colours::gridLine);
    g.fillRoundedRectangle (track, switchHeight * 0.5f);

    const float thumbSize = switchHeight - 4.0f;
    const float thumbX = on ? track.getRight() - thumbSize - 2.0f : track.getX() + 2.0f;

    g.setColour (on ? colours::background : colours::textDim);
    g.fillEllipse (thumbX, track.getY() + 2.0f, thumbSize, thumbSize);

    g.setColour (shouldDrawButtonAsHighlighted ? colours::text : colours::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (button.getButtonText(),
                bounds.withTrimmedLeft (switchWidth + 10.0f),
                juce::Justification::centredLeft, true);
}

juce::Font SnapBendLookAndFeel::getLabelFont (juce::Label& label)
{
    return juce::Font (juce::FontOptions (label.getFont().getHeight()));
}

juce::Font SnapBendLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return juce::Font (juce::FontOptions (juce::jmin (14.0f, buttonHeight * 0.55f)));
}
