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

    // Always evaluate the final sample of the block as well as the grid points,
    // so a bend that lands exactly on a block boundary is not left a step short.
    // A synth whose real bend range differs from the one it reports is wrong by
    // an amount proportional to the bend — inaudible near the centre, worst at
    // the extremes. So the trim scales the curve rather than offsetting it, and
    // nulling it at full bend nulls it at every depth.
    const double fineScale = 1.0 + settings.fineTuneCents
                                     / (100.0 * settings.bendRangeSemitones);

    for (double pos = 0.0; pos < static_cast<double> (numSamples); pos += intervalSamples)
    {
        const int offset = static_cast<int> (pos);

        const double beat  = transport.ppqPosition + (pos / sampleRate) * quartersPerSec;
        const double value = curve.valueAtBeat (beat, settings.shapeBias)
                                 * settings.depth * fineScale;
        const int    bend  = semitonesToPitchBend (value, settings.bendRangeSemitones);

        if (bend != lastSentBend)
        {
            events.push_back (makeBend (bend, settings.channel, offset));
            lastSentBend = bend;
        }
    }

    return events;
}

} // namespace snapbend
