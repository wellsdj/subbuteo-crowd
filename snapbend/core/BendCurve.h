// SnapBend — the bend curve itself.
//
// A curve is an ordered list of nodes in musical time (quarter notes from the
// start of the timeline). Between any two nodes the value is interpolated with
// a shape control, giving the "drag the middle of the segment" feel of Logic's
// own automation.
//
// Everything here is in semitones, not in 14-bit MIDI units. Conversion to MIDI
// happens once, at the very edge, in BendEmitter.

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace snapbend
{

struct BendNode
{
    /** Position on the timeline, in quarter notes. */
    double beat = 0.0;

    /** Target offset in semitones. Positive is up. */
    double semitones = 0.0;

    /** Shape of the segment that *follows* this node.
         0  = straight line
        > 0 = eases out (moves early, settles late) — the vocal/guitar feel
        < 0 = eases in  (hangs, then rushes)
        Range is clamped to [-1, 1]. */
    double shape = 0.0;
};

/** Result of asking the curve what it needs from the synth. */
struct RangeRequirement
{
    double maxAbsSemitones = 0.0;  ///< furthest excursion anywhere on the curve
    bool   exceedsRange    = false;///< true if that is beyond the configured range
    double suggestedRange  = 0.0;  ///< range that would accommodate it, rounded up
};

class BendCurve
{
public:
    BendCurve() = default;

    // ---- editing -------------------------------------------------------

    /** Adds a node, keeping the list ordered by beat. Returns its index. */
    size_t addNode (BendNode node);

    void removeNode (size_t index);
    void clear();

    /** Moves a node, re-sorting if it has crossed a neighbour. Returns the new
        index, which may differ from the old one. */
    size_t moveNode (size_t index, double newBeat, double newSemitones);

    void setNodeShape (size_t index, double shape);

    const std::vector<BendNode>& getNodes() const noexcept { return nodes; }
    size_t getNumNodes() const noexcept { return nodes.size(); }
    bool   isEmpty()    const noexcept { return nodes.empty(); }

    // ---- evaluation ----------------------------------------------------

    /** The semitone offset at a given point on the timeline.

        Before the first node and after the last, the curve holds that node's
        value rather than snapping to zero — so a bend you draw stays put until
        you tell it otherwise, instead of flicking back at the edges.

        @param shapeBias  added to every segment's own shape, so the CURVE knob
                          can lean the whole performance without editing nodes. */
    double valueAtBeat (double beat, double shapeBias = 0.0) const noexcept;

    /** Largest absolute excursion, and whether the given synth range covers it.
        This is what raises the "raise the range in your synth" prompt. */
    RangeRequirement getRangeRequirement (double configuredRange) const noexcept;

    // ---- snapping ------------------------------------------------------

    /** Rounds to the nearest whole semitone. This is the whole point of the
        plugin: you cannot land between two notes by accident. */
    static double snapToSemitone (double semitones) noexcept;

    /** Partial snapping, for the SNAP knob. 1 = hard snap to the semitone,
        0 = completely free, in between pulls towards the semitone without
        fully committing — useful for blues-style bends that sit just under
        the note. */
    static double snapToSemitone (double semitones, double strength) noexcept;

    /** Rounds a beat position to a musical grid (e.g. 0.25 = sixteenth notes).
        A grid of 0 or less leaves the position untouched. */
    static double snapToGrid (double beat, double gridInQuarterNotes) noexcept;

    /** Applies the segment shape to a linear 0..1 progress value.
        Monotonic for every shape, so a bend can never travel backwards. */
    static double applyShape (double t, double shape) noexcept;

    // ---- serialisation -------------------------------------------------

    /** Compact text form, so curves survive a Logic project save/reload and can
        be diffed in tests. */
    std::string toString() const;
    static std::optional<BendCurve> fromString (const std::string& text);

private:
    void sortNodes();

    std::vector<BendNode> nodes;
};

} // namespace snapbend
