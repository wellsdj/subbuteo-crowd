// SnapBend — turns a curve plus a transport position into MIDI pitch bend.
//
// This class is where the well-known pitch bend bugs get killed:
//
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

    /** The bend range the synth is set to, in semitones.

        This is the single number the whole plugin depends on, and it is not
        negotiable: pitch bend carries a fraction of a range, not a pitch, so
        if this does not match the synth then nothing else can make the
        semitones land. Fractional values are allowed, because a synth's real
        range is not always the round number on its own display. */
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

    /** Forgets all cached state, so the current bend value is re-transmitted. */
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

    /** The range used when encoding, clamped to what MIDI can express. */
    double getEffectiveRange() const noexcept;

    /** The RPN 0,0 handshake telling the synth what range to use.

        Deliberately not sent automatically. It rides on CC 6 and CC 38, and a
        synth that does not implement RPN applies those to whatever they happen
        to be mapped to — so firing it off on load can quietly wreck a patch.
        As a button the user presses, the cause and effect are obvious. */
    std::vector<RawMidiEvent> makeRangeAnnouncement();

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
    bool            wasEnabled    = false; ///< so switching off can un-bend the synth exactly once

    long long       calibrationCounter = 0;
    int             calibrationNote    = -1;  ///< -1 when nothing is sounding
};

} // namespace snapbend
