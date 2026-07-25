// SnapBend — turns a curve plus a transport position into MIDI pitch bend.
//
// This class is where the well-known pitch bend bugs get killed:
//
//   * the synth is told its bend range up front (RPN 0,0), so "snap to a
//     semitone" is true rather than aspirational;
//   * bend is returned to centre on stop, so nothing is left detuned;
//   * values are emitted on a fine grid with sample offsets, so slides are
//     smooth rather than stepped once per audio block;
//   * everything is derived from the host's musical position, so an offline
//     bounce produces exactly the same stream as live playback.

#pragma once

#include "BendCurve.h"
#include "BendMath.h"

namespace snapbend
{

struct EmitterSettings
{
    /** Semitone range we ask the synth for, and assume when converting. */
    double bendRangeSemitones = 12.0;

    /** The Mix knob: scales the whole curve. 0 = dry/no bend, 1 = as drawn. */
    double depth = 1.0;

    /** The Curve knob: leans every segment towards ease-in or ease-out. */
    double shapeBias = 0.0;

    /** How often a bend value may be emitted. 1 ms is far finer than any ear
        can resolve and keeps the MIDI stream sane. */
    double updateIntervalMs = 1.0;

    /** MIDI channel to send on, 1-16. */
    int channel = 1;

    /** Whether to transmit the RPN range handshake. Off for synths that
        mis-handle RPN and are configured by hand instead. */
    bool sendRangeRPN = true;
};

struct TransportInfo
{
    bool   isPlaying   = false;
    double ppqPosition = 0.0;  ///< musical position at the start of the block, in quarter notes
    double bpm         = 120.0;
};

class BendEmitter
{
public:
    void prepare (double sampleRateToUse);

    /** Forgets all cached state. The next playing block re-sends the range
        handshake and re-transmits the current bend value. */
    void reset();

    void setSettings (const EmitterSettings& newSettings);
    const EmitterSettings& getSettings() const noexcept { return settings; }

    /** Produces the MIDI for one audio block, in ascending sample order. */
    std::vector<RawMidiEvent> processBlock (const BendCurve&     curve,
                                            const TransportInfo& transport,
                                            int                  numSamples);

    /** Emits a centred bend regardless of state — for bypass, panic, and the
        editor being closed mid-bend. */
    std::vector<RawMidiEvent> makeSafetyReset();

private:
    EmitterSettings settings;
    double          sampleRate    = 44100.0;
    int             lastSentBend  = -1;    ///< -1 means "nothing sent yet"
    bool            wasPlaying    = false;
    bool            rangeSent     = false;
};

} // namespace snapbend
