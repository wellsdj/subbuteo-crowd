#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace snapbend::colours
{
    // A deliberately quiet palette. The curve is the only saturated thing on
    // screen, so the eye goes straight to it.
    const juce::Colour background   { 0xff14161a };
    const juce::Colour panel        { 0xff1b1f26 };
    const juce::Colour panelRaised  { 0xff232833 };
    const juce::Colour gridLine     { 0xff262b34 };
    const juce::Colour gridLineBold { 0xff333b47 };
    const juce::Colour text         { 0xffe6e9ef };
    const juce::Colour textDim      { 0xff7c8595 };
    const juce::Colour accent       { 0xff5ad1b4 };
    const juce::Colour accentSoft   { 0x335ad1b4 };
    const juce::Colour warning      { 0xffe8a33d };
    const juce::Colour outOfRange   { 0x22e8a33d };
    const juce::Colour playhead     { 0xffe6e9ef };
}

/** Knobs, buttons and text styled to look like a piece of hardware rather than
    a default JUCE dialog. */
class SnapBendLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SnapBendLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
};
