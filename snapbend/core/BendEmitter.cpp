#include "BendEmitter.h"

#include <algorithm>
#include <cmath>

namespace snapbend
{

void BendEmitter::prepare (double sampleRateToUse)
{
    sampleRate = sampleRateToUse > 0.0 ? sampleRateToUse : 44100.0;
    reset();
}

void BendEmitter::reset()
{
    lastSentBend = -1;
    wasPlaying   = false;
    rangeSent    = false;
    wasEnabled   = false;
}

void BendEmitter::setSettings (const EmitterSettings& newSettings)
{
    const bool rangeChanged = std::abs (newSettings.bendRangeSemitones
                                        - settings.bendRangeSemitones) > 1.0e-9;

    settings = newSettings;
    settings.bendRangeSemitones = std::clamp (settings.bendRangeSemitones,
                                              minBendRangeSemitones,
                                              maxBendRangeSemitones);
    settings.depth            = std::clamp (settings.depth, 0.0, 1.0);
    settings.shapeBias        = std::clamp (settings.shapeBias, -1.0, 1.0);
    settings.fineTuneCents    = std::clamp (settings.fineTuneCents, -100.0, 100.0);
    settings.channel          = std::clamp (settings.channel, 1, 16);
    settings.updateIntervalMs = std::max (0.1, settings.updateIntervalMs);

    if (rangeChanged)
    {
        // The semitone-to-14-bit mapping just changed underneath us, so the
        // cached value is meaningless and the synth needs telling again.
        rangeSent    = false;
        lastSentBend = -1;
    }
}

std::vector<RawMidiEvent> BendEmitter::stopCalibration()
{
    std::vector<RawMidiEvent> events;

    if (calibrationNote >= 0)
    {
        events.push_back (makeNoteOff (calibrationNote, settings.channel, 0));
        calibrationNote = -1;
    }

    events.push_back (makeCentredBend (settings.channel, 0));

    calibrationCounter = 0;
    lastSentBend       = pitchBendCentre;

    return events;
}

std::vector<RawMidiEvent> BendEmitter::processCalibrationBlock (int numSamples)
{
    std::vector<RawMidiEvent> events;

    if (numSamples <= 0)
        return events;

    const int phaseLength = std::max (1, static_cast<int> (sampleRate)); // one second

    // The reference note is a whole number of semitones above the root, so it
    // is a pitch the synth can produce with no bend at all — the thing we are
    // measuring against.
    const int rootNote      = 60;
    const int semitonesUp   = std::max (1, static_cast<int> (std::lround (settings.bendRangeSemitones)));
    const int referenceNote = std::min (127, rootNote + semitonesUp);

    int remaining = numSamples;
    int offset    = 0;

    while (remaining > 0)
    {
        const int positionInPhase = static_cast<int> (calibrationCounter % phaseLength);

        if (positionInPhase == 0)
        {
            if (calibrationNote >= 0)
                events.push_back (makeNoteOff (calibrationNote, settings.channel, offset));

            const bool bentPhase = ((calibrationCounter / phaseLength) % 2) != 0;

            if (bentPhase)
            {
                // The root note, bent up by the full range. If the range is
                // right this is exactly the reference pitch.
                events.push_back (makeBend (semitonesToPitchBend (settings.bendRangeSemitones,
                                                                  getEffectiveRange()),
                                            settings.channel, offset));
                calibrationNote = rootNote;
            }
            else
            {
                events.push_back (makeCentredBend (settings.channel, offset));
                calibrationNote = referenceNote;
            }

            events.push_back (makeNoteOn (calibrationNote, 100, settings.channel, offset));
        }

        const int chunk = std::min (remaining, phaseLength - positionInPhase);

        calibrationCounter += chunk;
        offset             += chunk;
        remaining          -= chunk;
    }

    // Calibration owns the bend while it runs, so normal playback has to
    // re-assert everything when it takes over again.
    lastSentBend = -1;
    rangeSent    = false;

    return events;
}

double BendEmitter::getEffectiveRange() const noexcept
{
    // The trim belongs on the range we *encode against*, not on the value being
    // encoded.
    //
    // Scaling the value instead looks equivalent — and is, in the middle — but
    // it breaks at exactly the point where the error is largest. Ask for the
    // full range plus a correction and the result falls outside the range, so
    // it clamps to the rail and the correction is thrown away. The trim then
    // does nothing at full bend, which is the one place anyone tests it.
    //
    // Adjusting the assumed range instead means FINE is simply "the synth's
    // real range is this many cents wider than it claims", the encoded value
    // stays comfortably inside the rails, and the correction holds everywhere.
    return std::clamp (settings.bendRangeSemitones + settings.fineTuneCents / 100.0,
                       minBendRangeSemitones,
                       maxBendRangeSemitones);
}

std::vector<RawMidiEvent> BendEmitter::makeSafetyReset()
{
    lastSentBend = pitchBendCentre;
    return { makeCentredBend (settings.channel, 0) };
}

std::vector<RawMidiEvent> BendEmitter::processBlock (const BendCurve&     curve,
                                                     const TransportInfo& transport,
                                                     int                  numSamples)
{
    std::vector<RawMidiEvent> events;

    if (numSamples <= 0)
        return events;

    // ---- switched off ------------------------------------------------------
    //
    // Nothing drawn, or the mix at zero. The plugin must then be completely
    // invisible: not one byte on the wire, so the instrument sounds exactly as
    // it does with the plugin removed. The only exception is the single
    // centring message needed to undo a bend we ourselves applied.
    if (! settings.enabled)
    {
        if (wasEnabled)
        {
            if (lastSentBend != pitchBendCentre)
                events.push_back (makeCentredBend (settings.channel, 0));

            lastSentBend = pitchBendCentre;
            wasEnabled   = false;
        }

        // Make sure switching back on re-announces everything from scratch.
        rangeSent  = false;
        wasPlaying = false;

        return events;
    }

    wasEnabled = true;

    // ---- transport stopped -------------------------------------------------
    if (! transport.isPlaying)
    {
        if (wasPlaying)
        {
            // The single most important line in this file. Without it the synth
            // stays bent after you hit stop and everything you play afterwards
            // is out of tune.
            events.push_back (makeCentredBend (settings.channel, 0));
            lastSentBend = pitchBendCentre;
            wasPlaying   = false;
        }

        return events;
    }

    // ---- playback just started --------------------------------------------
    if (! wasPlaying)
    {
        wasPlaying = true;

        // Re-assert on every start: the user may have reloaded the synth, or
        // another plugin may have moved the parameter since we last spoke.
        rangeSent = false;
    }

    if (settings.sendRangeRPN && ! rangeSent)
    {
        const auto rpn = buildPitchBendRangeRPN (settings.bendRangeSemitones,
                                                 settings.channel, 0);
        events.insert (events.end(), rpn.begin(), rpn.end());
        rangeSent = true;
    }

    // ---- the bend itself ---------------------------------------------------
    const double bpm            = transport.bpm > 0.0 ? transport.bpm : 120.0;
    const double quartersPerSec = bpm / 60.0;
    const double intervalSamples = std::max (1.0, settings.updateIntervalMs * 0.001 * sampleRate);

    const double effectiveRange = getEffectiveRange();

    for (double pos = 0.0; pos < static_cast<double> (numSamples); pos += intervalSamples)
    {
        const int offset = static_cast<int> (pos);

        const double beat  = transport.ppqPosition + (pos / sampleRate) * quartersPerSec;
        const double value = curve.valueAtBeat (beat, settings.shapeBias) * settings.depth;
        const int    bend  = semitonesToPitchBend (value, effectiveRange);

        if (bend != lastSentBend)
        {
            events.push_back (makeBend (bend, settings.channel, offset));
            lastSentBend = bend;
        }
    }

    return events;
}

} // namespace snapbend
