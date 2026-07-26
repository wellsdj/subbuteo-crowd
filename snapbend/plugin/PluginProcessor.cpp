#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ids
{
    const juce::String range = "range";
    const juce::String curve = "curve";
    const juce::String snap  = "snap";
    const juce::String mix   = "mix";
}

juce::AudioProcessorValueTreeState::ParameterLayout SnapBendProcessor::createParameterLayout()
{
    using namespace juce;

    AudioProcessorValueTreeState::ParameterLayout layout;

    // This must match the synth's own bend range setting. Fractional values are
    // allowed because a synth's true range is not always the round number it
    // displays, and Calibrate is how you find the real one.
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::range, 1 }, "Range",
        NormalisableRange<float> (1.0f, 48.0f, 0.01f), 12.0f,
        AudioParameterFloatAttributes().withLabel ("semitones")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::curve, 1 }, "Curve",
        NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::snap, 1 }, "Snap",
        NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ids::mix, 1 }, "Mix",
        NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));

    return layout;
}

SnapBendProcessor::SnapBendProcessor()
    : AudioProcessor (BusesProperties()), // a MIDI effect has no audio buses
      apvts (*this, nullptr, "SnapBend", createParameterLayout())
{
}

void SnapBendProcessor::prepareToPlay (double sampleRate, int)
{
    emitter.prepare (sampleRate);
}

void SnapBendProcessor::releaseResources()
{
    calibrating.store (false);
    wasCalibrating = false;

    // Leaving the instrument bent when the plugin goes away is the single most
    // annoying bug in this whole category, so make sure state is forgotten.
    emitter.reset();
}

snapbend::BendCurve SnapBendProcessor::getCurve() const
{
    const juce::ScopedLock lock (curveLock);
    return curve;
}

void SnapBendProcessor::setCurve (const snapbend::BendCurve& newCurve)
{
    const juce::ScopedLock lock (curveLock);
    curve = newCurve;
}

void SnapBendProcessor::setZoom (double horizontal, double vertical) noexcept
{
    horizontalZoom.store (juce::jlimit (0.0, 1.0, horizontal));
    verticalZoom.store (juce::jlimit (0.0, 1.0, vertical));
}

void SnapBendProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer&        midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // A MIDI effect carries no audio, but hosts may still hand us a buffer.
    buffer.clear();

    const int numSamples = buffer.getNumSamples();

    // ---- gather the transport ------------------------------------------
    snapbend::TransportInfo transport;

    if (auto* hostPlayHead = getPlayHead())
    {
        if (const auto position = hostPlayHead->getPosition())
        {
            transport.isPlaying = position->getIsPlaying();

            if (const auto ppq = position->getPpqPosition())
                transport.ppqPosition = *ppq;

            if (const auto bpm = position->getBpm())
                transport.bpm = *bpm;

            if (const auto timeSig = position->getTimeSignature())
            {
                timeSigNumerator.store (timeSig->numerator);
                timeSigDenominator.store (timeSig->denominator);
            }
        }
    }

    currentBeat.store (transport.ppqPosition);
    transportRunning.store (transport.isPlaying);

    const auto localCurve = getCurve();

    // With nothing drawn, or the mix all the way down, there is no bend to
    // apply — and in that state the plugin must be acoustically invisible. It
    // sends nothing, and just as importantly it stops interfering with the
    // pitch bend already in the track or coming from your keyboard.
    const double mix    = apvts.getRawParameterValue (ids::mix)->load();
    const bool   active = ! localCurve.isEmpty() && mix > 0.0;

    // ---- push the current knob positions into the emitter ---------------
    snapbend::EmitterSettings settings;
    settings.enabled            = active;
    settings.bendRangeSemitones = apvts.getRawParameterValue (ids::range)->load();
    settings.shapeBias          = apvts.getRawParameterValue (ids::curve)->load();
    settings.depth              = mix;
    settings.channel            = 1;
    emitter.setSettings (settings);

    // ---- rebuild the MIDI stream ---------------------------------------
    juce::MidiBuffer output;

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        // While a bend is being applied we own the pitch wheel on this channel:
        // letting the region's own bend through as well means two sources
        // fighting, which reads as jitter. While idle we touch nothing.
        if (active && message.isPitchWheel())
            continue;

        output.addEvent (message, metadata.samplePosition);
    }

    // ---- calibration takes over the channel while it runs ----------------
    const bool nowCalibrating = calibrating.load();

    std::vector<snapbend::RawMidiEvent> events;

    if (nowCalibrating)
    {
        events = emitter.processCalibrationBlock (numSamples);
        wasCalibrating = true;
    }
    else if (wasCalibrating)
    {
        // Silence the tone before handing the channel back, so calibration can
        // never leave a note hanging.
        events = emitter.stopCalibration();
        wasCalibrating = false;
    }
    else
    {
        events = safetyResetPending.exchange (false)
                   ? emitter.makeSafetyReset()
                   : emitter.processBlock (localCurve, transport, numSamples);
    }

    // The one place CC 6 / CC 38 ever leave this plugin, and only because a
    // button was pressed.
    if (rangeAnnouncementPending.exchange (false))
    {
        const auto rpn = emitter.makeRangeAnnouncement();
        events.insert (events.begin(), rpn.begin(), rpn.end());
    }

    for (const auto& event : events)
        output.addEvent (juce::MidiMessage (event.status, event.data1, event.data2),
                         juce::jlimit (0, juce::jmax (0, numSamples - 1), event.sampleOffset));

    midiMessages.swapWith (output);
}

juce::AudioProcessorEditor* SnapBendProcessor::createEditor()
{
    return new SnapBendEditor (*this);
}

void SnapBendProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // The curve rides along inside the parameter tree, so it is saved with the
    // Logic project and restored with it.
    state.setProperty ("curve", juce::String (getCurve().toString()), nullptr);
    state.setProperty ("zoomH", horizontalZoom.load(), nullptr);
    state.setProperty ("zoomV", verticalZoom.load(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SnapBendProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);

        if (! state.isValid() || state.getType() != apvts.state.getType())
            return;

        apvts.replaceState (state);

        const auto text = state.getProperty ("curve").toString().toStdString();

        if (const auto restored = snapbend::BendCurve::fromString (text))
            setCurve (*restored);

        if (state.hasProperty ("zoomH") && state.hasProperty ("zoomV"))
            setZoom (state.getProperty ("zoomH"), state.getProperty ("zoomV"));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SnapBendProcessor();
}
