#include "BendMath.h"

#include <algorithm>
#include <cmath>

namespace snapbend
{

static int clampBend (int v) noexcept
{
    return std::clamp (v, 0, pitchBendMax);
}

static uint8_t clampChannel (int channel) noexcept
{
    return static_cast<uint8_t> (std::clamp (channel, 1, 16) - 1);
}

int semitonesToPitchBend (double semitones, double rangeSemitones) noexcept
{
    if (rangeSemitones <= 0.0)
        return pitchBendCentre;

    const double normalised = semitones / rangeSemitones;

    // Upward has one fewer step than downward, because centre sits at 8192.
    const double steps = normalised >= 0.0 ? normalised * (pitchBendMax - pitchBendCentre)
                                           : normalised * pitchBendCentre;

    return clampBend (pitchBendCentre + static_cast<int> (std::lround (steps)));
}

double pitchBendToSemitones (int bendValue, double rangeSemitones) noexcept
{
    const int offset = clampBend (bendValue) - pitchBendCentre;

    const double normalised = offset >= 0
        ? static_cast<double> (offset) / (pitchBendMax - pitchBendCentre)
        : static_cast<double> (offset) / pitchBendCentre;

    return normalised * rangeSemitones;
}

bool exceedsRange (double semitones, double rangeSemitones) noexcept
{
    // A hair of tolerance so that drawing exactly on the range line — which is
    // reachable — does not trigger the warning.
    constexpr double tolerance = 1.0e-6;
    return std::abs (semitones) > rangeSemitones + tolerance;
}

std::vector<RawMidiEvent> buildPitchBendRangeRPN (double rangeSemitones,
                                                  int     channel,
                                                  int     sampleOffset)
{
    const double clamped = std::clamp (rangeSemitones,
                                       minBendRangeSemitones,
                                       maxBendRangeSemitones);

    const int semitonePart = static_cast<int> (std::floor (clamped));
    const int centsPart    = static_cast<int> (std::lround ((clamped - semitonePart) * 100.0));

    const uint8_t ch     = clampChannel (channel);
    const uint8_t status = static_cast<uint8_t> (0xB0 | ch); // Control Change

    return {
        { sampleOffset, status, 101, 0 },                                  // RPN MSB = 0
        { sampleOffset, status, 100, 0 },                                  // RPN LSB = 0  -> Pitch Bend Sensitivity
        { sampleOffset, status,   6, static_cast<uint8_t> (semitonePart) },// Data Entry MSB = semitones
        { sampleOffset, status,  38, static_cast<uint8_t> (centsPart) },   // Data Entry LSB = cents
        { sampleOffset, status, 101, 127 },                                // RPN Null — close the parameter
        { sampleOffset, status, 100, 127 },
    };
}

RawMidiEvent makeBend (int bendValue, int channel, int sampleOffset) noexcept
{
    const int v = clampBend (bendValue);

    return { sampleOffset,
             static_cast<uint8_t> (0xE0 | clampChannel (channel)),
             static_cast<uint8_t> (v & 0x7F),          // LSB
             static_cast<uint8_t> ((v >> 7) & 0x7F) }; // MSB
}

RawMidiEvent makeCentredBend (int channel, int sampleOffset) noexcept
{
    return makeBend (pitchBendCentre, channel, sampleOffset);
}

RawMidiEvent makeNoteOn (int note, int velocity, int channel, int sampleOffset) noexcept
{
    return { sampleOffset,
             static_cast<uint8_t> (0x90 | clampChannel (channel)),
             static_cast<uint8_t> (std::clamp (note, 0, 127)),
             static_cast<uint8_t> (std::clamp (velocity, 1, 127)) };
}

RawMidiEvent makeNoteOff (int note, int channel, int sampleOffset) noexcept
{
    return { sampleOffset,
             static_cast<uint8_t> (0x80 | clampChannel (channel)),
             static_cast<uint8_t> (std::clamp (note, 0, 127)),
             0 };
}

} // namespace snapbend
