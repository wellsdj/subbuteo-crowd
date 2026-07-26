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
    /** When false the emitter is completely silent: no bend, no range
        handshake, nothing. A plugin that is not being used must not alter a
        single byte of the MIDI passing through it. */
    bool enabled = true;

    /** Semitone range we ask the synth for, and assume when converting. */
    double bendRangeSemitones = 12.0;

    /** Trim, in cents at full bend, for synths whose real bend range does not
        quite match what they claim.

        A range mismatch produces an error proportional to how far you have
        bent — nothing at rest, worst at the extremes — so this is applied as a
        scale rather than a fixed offset. That way nulling it at full bend
        nulls it everywhere. */
    double fineTuneCents = 0.0;

    /** The Mix knob: scales the whole curve. 0 = dry/no bend, 1 = as drawn. */
    double depth = 1.0;

    /** The Curve knob: leans every segment towards ease-in or ease-out. */
    double shapeBias = 0.0;

    /** How often a bend value may be emitted. 1 ms is far finer than any ear
        can resolve and keeps the MIDI stream sane. */
    double updateIntervalMs = 1.0;

    /** MIDI channel to send on, 1-16. */
    int channel = 1;

    /** Whether to transmit the RPN range handshake.

        Off by default, and deliberately so. The handshake is carried on CC 6
        and CC 38, and a synth that does not implement RPN will happily apply
        those to whatever they are mapped to instead — changing the patch's
        filter, envelope or macros the moment the plugin loads. Opt in only
        when the synth is known to handle RPN. */
    bool sendRangeRPN = false;
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

    /** The range actually used when encoding, i.e. RANGE corrected by FINE. */
    double getEffectiveRange() const noexcept;

    // ---- calibration ----------------------------------------------------
    //
    // Tuning a bend by ear against nothing is hopeless — a few cents out is
    // inaudible on its own but obvious the moment you have something to
    // compare against. So this alternates two notes that must be identical if
    // the range is right: a reference note played directly, and the same pitch
    // reached by bending. Any mismatch is heard as a jump between them, which
    // the ear resolves far finer than absolute pitch.

    /** One second of reference, one second of bend, repeating. */
    std::vector<RawMidiEvent> processCalibrationBlock (int numSamples);

    /** Silences the calibration note and re-centres. Safe to call at any time. */
    std::vector<RawMidiEvent> stopCalibration();

    /** The note the calibration tone is currently sounding, or -1. */
    int getCalibrationNote() const noexcept { return calibrationNote; }

private:
    EmitterSettings settings;
    double          sampleRate    = 44100.0;
    int             lastSentBend  = -1;    ///< -1 means "nothing sent yet"
    bool            wasPlaying    = false;
    bool            rangeSent     = false;
    bool            wasEnabled    = false; ///< so switching off can un-bend the synth exactly once

    long long       calibrationCounter = 0;
    int             calibrationNote    = -1;  ///< -1 when nothing is sounding
};

} // namespace snapbend
