// SnapBend — low level pitch bend maths.
//
// Deliberately free of JUCE (and of any audio framework) so the parts that
// historically go wrong — bend range, 14-bit conversion, RPN handshakes —
// can be unit tested on any machine without a DAW.

#pragma once

#include <cstdint>
#include <vector>

namespace snapbend
{

/** A raw MIDI event with a sample offset into the current audio block. */
struct RawMidiEvent
{
    int     sampleOffset = 0;
    uint8_t status       = 0; // includes channel in the low nibble
    uint8_t data1        = 0;
    uint8_t data2        = 0;

    bool operator== (const RawMidiEvent& o) const
    {
        return sampleOffset == o.sampleOffset && status == o.status
            && data1 == o.data1 && data2 == o.data2;
    }
};

/** 14-bit pitch bend, centred. 0 = fully down, 8192 = centre, 16383 = fully up. */
inline constexpr int pitchBendCentre = 8192;
inline constexpr int pitchBendMax    = 16383;

/** Semitone range a synth can be asked to cover. ±48 is the MPE member-channel
    maximum; going beyond it is meaningless on every synth we can talk to. */
inline constexpr double minBendRangeSemitones = 1.0;
inline constexpr double maxBendRangeSemitones = 48.0;

/** Converts a signed semitone offset to a 14-bit bend value, given the range the
    receiving synth is configured for.

    Note the asymmetry: upward has 8191 steps of headroom and downward has 8192.
    Getting this backwards is a classic source of "my +12 is a hair flat".
*/
int semitonesToPitchBend (double semitones, double rangeSemitones) noexcept;

/** Inverse of the above — mostly here so tests can round-trip. */
double pitchBendToSemitones (int bendValue, double rangeSemitones) noexcept;

/** True when the requested bend cannot be reached with this range, i.e. the
    conversion above had to clamp. This is what drives the "go and raise the
    range in your synth" prompt in the UI. */
bool exceedsRange (double semitones, double rangeSemitones) noexcept;

/** Builds the RPN 0,0 (Pitch Bend Sensitivity) handshake that tells the
    downstream synth how far it is allowed to bend.

    Without this, virtually every synth sits at its ±2 semitone default and
    every "snap to a semitone" promise silently becomes a lie. We always close
    the sequence with RPN Null (127/127) so a later stray Data Entry message
    cannot accidentally re-write the parameter we just set.

    Fractional ranges are supported: the whole part goes in Data Entry MSB as
    semitones, the remainder in the LSB as cents.
*/
std::vector<RawMidiEvent> buildPitchBendRangeRPN (double rangeSemitones,
                                                  int     channel,
                                                  int     sampleOffset = 0);

/** A pitch bend message at dead centre — used on stop, bypass and panic so we
    never leave the instrument detuned after the transport stops. */
RawMidiEvent makeCentredBend (int channel, int sampleOffset = 0) noexcept;

/** A pitch bend message for an arbitrary 14-bit value. */
RawMidiEvent makeBend (int bendValue, int channel, int sampleOffset = 0) noexcept;

} // namespace snapbend
