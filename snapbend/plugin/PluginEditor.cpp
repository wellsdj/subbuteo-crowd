#include "PluginEditor.h"

using namespace snapbend;

// ---------------------------------------------------------------------------
// LabelledKnob
// ---------------------------------------------------------------------------

LabelledKnob::LabelledKnob (const juce::String& name,
                            std::function<juce::String (double)> formatter)
    : knobName (name), format (std::move (formatter))
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);

    slider.onValueChange = [this] { updateReadout(); };

    addAndMakeVisible (slider);
    updateReadout();
}

void LabelledKnob::updateReadout()
{
    const auto text = format != nullptr ? format (slider.getValue())
                                        : juce::String (slider.getValue(), 2);

    if (text != readout)
    {
        readout = text;
        repaint();
    }
}

void LabelledKnob::resized()
{
    auto area = getLocalBounds();

    area.removeFromTop (18);    // value readout
    area.removeFromBottom (20); // name

    slider.setBounds (area.reduced (2));
}

void LabelledKnob::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();

    g.setColour (colours::text);
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawText (readout, area.removeFromTop (18), juce::Justification::centred, false);

    g.setColour (colours::textDim);
    g.setFont (juce::Font (juce::FontOptions (10.5f)));

    // Letter-spaced small caps read as a hardware legend rather than a form
    // label.
    juce::String spaced;
    for (int i = 0; i < knobName.length(); ++i)
    {
        spaced += knobName[i];
        if (i < knobName.length() - 1)
            spaced += " ";
    }

    g.drawText (spaced, area.removeFromBottom (20), juce::Justification::centred, false);
}

// ---------------------------------------------------------------------------
// SnapBendEditor
// ---------------------------------------------------------------------------

SnapBendEditor::SnapBendEditor (SnapBendProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (bendLane);
    addAndMakeVisible (rangeKnob);
    addAndMakeVisible (fineKnob);
    addAndMakeVisible (curveKnob);
    addAndMakeVisible (snapKnob);
    addAndMakeVisible (mixKnob);
    addAndMakeVisible (autoRangeButton);
    addAndMakeVisible (clearButton);
    addAndMakeVisible (panicButton);
    addAndMakeVisible (zoomControls);

    addChildComponent (rangeWarning); // hidden until it is needed

    bendLane.setCurve (processor.getCurve());

    bendLane.onCurveChanged = [this] (const snapbend::BendCurve& newCurve)
    {
        processor.setCurve (newCurve);
        curveDirty = true;
        refreshRangeWarning();
    };

    rangeWarning.onRaiseRange = [this] (double newRange)
    {
        if (auto* parameter = processor.apvts.getParameter ("range"))
        {
            const auto normalised = parameter->convertTo0to1 (static_cast<float> (newRange));

            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (normalised);
            parameter->endChangeGesture();
        }

        rangeWarning.setVisible (false);
        dismissedAtRequirement = -1.0;
    };

    rangeWarning.onDismiss = [this]
    {
        // Remember what we were warning about, so it stays quiet until the
        // curve is pushed even further.
        dismissedAtRequirement = processor.getCurve()
                                     .getRangeRequirement (0.0).maxAbsSemitones;
        rangeWarning.setVisible (false);
    };

    clearButton.onClick = [this] { bendLane.clearCurve(); };
    clearButton.setTooltip ("Remove every point and start the curve again.");

    panicButton.onClick = [this] { processor.requestSafetyReset(); };
    panicButton.setTooltip ("Send the synth back to normal pitch, if a bend is ever left hanging.");

    zoomControls.setZoom (processor.getHorizontalZoom(), processor.getVerticalZoom());

    zoomControls.onZoomChanged = [this]
    {
        const double horizontal = zoomControls.getHorizontalZoom();
        const double vertical   = zoomControls.getVerticalZoom();

        bendLane.setZoom (horizontal, vertical);
        processor.setZoom (horizontal, vertical);
    };

    bendLane.setZoom (processor.getHorizontalZoom(), processor.getVerticalZoom());

    autoRangeButton.setTooltip (
        "Sends the bend range to the synth over MIDI (RPN, on CC 6 and CC 38).\n\n"
        "Leave this off unless you know the synth handles RPN: those CCs will "
        "otherwise be applied to whatever they are mapped to, which can change "
        "the patch. With it off, set the bend range in the synth by hand.");

    fineKnob.getSlider().setTooltip (
        "Corrects a synth whose real bend range does not quite match its "
        "setting. Bend all the way down, then turn this until the note is in "
        "tune — measured in cents at full bend.");

    rangeAttachment = std::make_unique<SliderAttachment> (processor.apvts, "range", rangeKnob.getSlider());
    fineAttachment  = std::make_unique<SliderAttachment> (processor.apvts, "fine",  fineKnob.getSlider());
    curveAttachment = std::make_unique<SliderAttachment> (processor.apvts, "curve", curveKnob.getSlider());
    snapAttachment  = std::make_unique<SliderAttachment> (processor.apvts, "snap",  snapKnob.getSlider());
    mixAttachment   = std::make_unique<SliderAttachment> (processor.apvts, "mix",   mixKnob.getSlider());

    autoRangeAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "sendRPN", autoRangeButton);

    setResizable (true, true);
    setResizeLimits (620, 440, 1600, 1100);
    setSize (780, 560);

    startTimerHz (30);
}

SnapBendEditor::~SnapBendEditor()
{
    setLookAndFeel (nullptr);
}

void SnapBendEditor::timerCallback()
{
    bendLane.setPlayheadPosition (processor.getCurrentBeat(), processor.isPlaying());
    bendLane.setTimeSignature (processor.getTimeSignatureNumerator(),
                               processor.getTimeSignatureDenominator());

    const double range = processor.apvts.getRawParameterValue ("range")->load();
    const double snap  = processor.apvts.getRawParameterValue ("snap")->load();
    const double curve = processor.apvts.getRawParameterValue ("curve")->load();

    bendLane.setBendRange (range);
    bendLane.setSnapStrength (snap);
    bendLane.setShapeBias (curve);

    rangeKnob.updateReadout();
    fineKnob.updateReadout();
    curveKnob.updateReadout();
    snapKnob.updateReadout();
    mixKnob.updateReadout();

    refreshRangeWarning();
}

void SnapBendEditor::refreshRangeWarning()
{
    const double range = processor.apvts.getRawParameterValue ("range")->load();

    if (curveDirty || std::abs (range - lastCheckedRange) > 1.0e-9)
    {
        cachedRequirement = processor.getCurve().getRangeRequirement (range);
        lastCheckedRange  = range;
        curveDirty        = false;
    }

    const auto& requirement = cachedRequirement;

    if (! requirement.exceedsRange)
    {
        rangeWarning.setVisible (false);
        dismissedAtRequirement = -1.0;
        return;
    }

    // Already told them about a bend at least this big and they closed it.
    if (dismissedAtRequirement >= 0.0
        && requirement.maxAbsSemitones <= dismissedAtRequirement + 1.0e-6)
        return;

    // Only rebuild the message when something it says has actually changed —
    // this runs on a timer, and repainting it thirty times a second would make
    // the text shimmer.
    if (rangeWarning.isVisible()
        && std::abs (requirement.maxAbsSemitones - lastWarnedRequirement) < 1.0e-6
        && std::abs (range - lastWarnedRange) < 1.0e-6)
        return;

    lastWarnedRequirement = requirement.maxAbsSemitones;
    lastWarnedRange       = range;

    rangeWarning.setRequirement (requirement.maxAbsSemitones, range);
    rangeWarning.toFront (false);
}

void SnapBendEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    auto header = getLocalBounds().removeFromTop (54).reduced (20, 0);

    g.setColour (colours::text);
    g.setFont (juce::Font (juce::FontOptions (19.0f, juce::Font::bold)));
    g.drawText ("SNAPBEND", header.removeFromLeft (140),
                juce::Justification::centredLeft, false);

    g.setColour (colours::textDim);
    g.setFont (juce::Font (juce::FontOptions (11.5f)));
    g.drawText ("pitch bend that lands on the note",
                header.removeFromLeft (240), juce::Justification::centredLeft, false);

    // A hairline under the header, to separate it without drawing a heavy box.
    g.setColour (colours::gridLine);
    g.fillRect (20, 53, getWidth() - 40, 1);
}

void SnapBendEditor::resized()
{
    auto area = getLocalBounds();

    // ---- header ---------------------------------------------------------
    auto header = area.removeFromTop (54).reduced (20, 12);

    panicButton.setBounds (header.removeFromRight (104));
    header.removeFromRight (10);
    clearButton.setBounds (header.removeFromRight (104));

    // ---- knob row -------------------------------------------------------
    //
    // Generous, even spacing: the row is divided into equal columns and each
    // knob is centred in its own, so nothing ever bunches up at one end.
    auto knobRow = area.removeFromBottom (118).reduced (20, 12);

    LabelledKnob* knobs[] { &rangeKnob, &fineKnob, &curveKnob, &snapKnob, &mixKnob };
    constexpr int numKnobs = 5;

    const int columnWidth = knobRow.getWidth() / numKnobs;
    const int knobWidth   = juce::jmin (110, columnWidth - 24);

    for (int i = 0; i < numKnobs; ++i)
    {
        const auto column = knobRow.withWidth (columnWidth)
                                   .withX (knobRow.getX() + i * columnWidth);

        knobs[i]->setBounds (column.withSizeKeepingCentre (knobWidth, column.getHeight()));
    }

    // ---- the strip beneath the lane -------------------------------------
    //
    // The synth-range toggle on the left, zoom on the right. Keeping both out
    // of the header stops the top of the window turning into a toolbar.
    auto strip = area.removeFromBottom (30).reduced (20, 4);

    zoomControls.setBounds (strip.removeFromRight (240));
    strip.removeFromRight (20);
    autoRangeButton.setBounds (strip);

    // ---- the lane -------------------------------------------------------
    bendLane.setBounds (area.reduced (20, 10).withTrimmedBottom (2));

    // ---- the warning ----------------------------------------------------
    const int warningWidth  = juce::jmin (460, bendLane.getWidth() - 40);
    const int warningHeight = 190;

    rangeWarning.setBounds (juce::Rectangle<int> (warningWidth, warningHeight)
                                .withCentre (bendLane.getBounds().getCentre()));
}
