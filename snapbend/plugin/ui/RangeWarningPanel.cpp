#include "RangeWarningPanel.h"
#include "SnapBendLookAndFeel.h"

using namespace snapbend;

RangeWarningPanel::RangeWarningPanel()
{
    addAndMakeVisible (raiseButton);
    addAndMakeVisible (dismissButton);

    raiseButton.setColour (juce::TextButton::buttonColourId, colours::warning);
    raiseButton.setColour (juce::TextButton::textColourOffId, colours::background);

    raiseButton.onClick = [this]
    {
        if (onRaiseRange != nullptr)
            onRaiseRange (needed);
    };

    dismissButton.onClick = [this]
    {
        if (onDismiss != nullptr)
            onDismiss();
    };

    setInterceptsMouseClicks (true, true);
}

RangeWarningPanel::~RangeWarningPanel() = default;

void RangeWarningPanel::setRequirement (double neededSemitones, double range)
{
    needed       = std::ceil (neededSemitones);
    currentRange = range;

    const auto neededText = juce::String (static_cast<int> (needed));
    const auto rangeText  = juce::String (static_cast<int> (range));

    headline = "Your synth can't reach this bend yet";

    // Deliberately written the way you would say it out loud, not the way a
    // MIDI spec would. Someone hitting this is confused, not curious.
    body = "This curve goes " + neededText + " semitones from the note, but the "
           "synth is only allowed " + rangeText + ".\n\n"
           "SnapBend has already asked it for " + neededText + " automatically. "
           "Plenty of synths ignore that request — so if the bend still sounds "
           "too small, open your synthesiser and find its pitch bend range "
           "(it may be called Bend Range, Pitch Bend Up / Down, or Pitch Bend "
           "Sensitivity) and set it to " + neededText + " semitones or more.";

    raiseButton.setButtonText ("Set range to " + neededText);

    setVisible (true);
    resized();
    repaint();
}

void RangeWarningPanel::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);

    // Shadow, so it reads as floating above the grid rather than drawn on it.
    juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 18, { 0, 4 })
        .drawForRectangle (g, bounds.toNearestInt());

    g.setColour (colours::panelRaised);
    g.fillRoundedRectangle (bounds, 8.0f);

    g.setColour (colours::warning.withAlpha (0.65f));
    g.drawRoundedRectangle (bounds, 8.0f, 1.5f);

    auto content = bounds.reduced (20.0f, 16.0f);

    // A warning mark, tucked left so the text has a clean edge.
    const auto iconArea = content.removeFromLeft (30.0f).withHeight (24.0f);
    g.setColour (colours::warning);
    g.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    g.drawText ("!", iconArea, juce::Justification::centredLeft, false);

    content.removeFromLeft (6.0f);

    auto textArea = content.withTrimmedBottom (44.0f);

    g.setColour (colours::text);
    g.setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)));
    g.drawText (headline, textArea.removeFromTop (22.0f), juce::Justification::topLeft, true);

    textArea.removeFromTop (6.0f);

    g.setColour (colours::textDim);
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawFittedText (body, textArea.toNearestInt(), juce::Justification::topLeft, 8);
}

void RangeWarningPanel::resized()
{
    auto row = getLocalBounds().reduced (20, 16).removeFromBottom (30);

    // Buttons sit at the bottom right with real space around them — nothing in
    // this plugin is allowed to feel cramped.
    dismissButton.setBounds (row.removeFromRight (92).reduced (2));
    row.removeFromRight (10);
    raiseButton.setBounds (row.removeFromRight (132).reduced (2));
}
