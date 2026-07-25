#include "BendCurve.h"

#include "BendMath.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace snapbend
{

void BendCurve::sortNodes()
{
    std::stable_sort (nodes.begin(), nodes.end(),
                      [] (const BendNode& a, const BendNode& b) { return a.beat < b.beat; });
}

size_t BendCurve::addNode (BendNode node)
{
    node.shape = std::clamp (node.shape, -1.0, 1.0);
    nodes.push_back (node);
    sortNodes();

    const auto it = std::find_if (nodes.begin(), nodes.end(),
                                  [&] (const BendNode& n)
                                  {
                                      return n.beat == node.beat
                                          && n.semitones == node.semitones;
                                  });

    return static_cast<size_t> (std::distance (nodes.begin(), it));
}

void BendCurve::removeNode (size_t index)
{
    if (index < nodes.size())
        nodes.erase (nodes.begin() + static_cast<long> (index));
}

void BendCurve::clear()
{
    nodes.clear();
}

size_t BendCurve::moveNode (size_t index, double newBeat, double newSemitones)
{
    if (index >= nodes.size())
        return index;

    // Keep hold of the shape, since re-sorting may move this node elsewhere.
    BendNode moved = nodes[index];
    moved.beat      = newBeat;
    moved.semitones = newSemitones;

    nodes.erase (nodes.begin() + static_cast<long> (index));
    return addNode (moved);
}

void BendCurve::setNodeShape (size_t index, double shape)
{
    if (index < nodes.size())
        nodes[index].shape = std::clamp (shape, -1.0, 1.0);
}

double BendCurve::applyShape (double t, double shape) noexcept
{
    t = std::clamp (t, 0.0, 1.0);

    const double s = std::clamp (shape, -1.0, 1.0);

    // Below this the exponential form loses precision and a straight line is
    // indistinguishable anyway.
    if (std::abs (s) < 1.0e-4)
        return t;

    // Exponential warp. Monotonic for every value of a, and passes through
    // (0,0) and (1,1) exactly, so segment endpoints stay put.
    const double a = s * 5.0;

    return (1.0 - std::exp (-a * t)) / (1.0 - std::exp (-a));
}

double BendCurve::valueAtBeat (double beat, double shapeBias) const noexcept
{
    if (nodes.empty())
        return 0.0;

    if (beat <= nodes.front().beat)
        return nodes.front().semitones;

    if (beat >= nodes.back().beat)
        return nodes.back().semitones;

    // Find the segment containing this beat.
    for (size_t i = 0; i + 1 < nodes.size(); ++i)
    {
        const BendNode& a = nodes[i];
        const BendNode& b = nodes[i + 1];

        if (beat >= a.beat && beat <= b.beat)
        {
            const double span = b.beat - a.beat;

            // Two nodes stacked on the same beat: a deliberate instant jump.
            if (span <= 0.0)
                return b.semitones;

            const double shape = std::clamp (a.shape + shapeBias, -1.0, 1.0);
            const double t     = applyShape ((beat - a.beat) / span, shape);
            return a.semitones + (b.semitones - a.semitones) * t;
        }
    }

    return nodes.back().semitones;
}

RangeRequirement BendCurve::getRangeRequirement (double configuredRange) const noexcept
{
    RangeRequirement result;

    for (const auto& n : nodes)
        result.maxAbsSemitones = std::max (result.maxAbsSemitones, std::abs (n.semitones));

    // Shaped interpolation is monotonic between nodes, so the extremes can only
    // ever sit on a node — no need to sample the segments.
    result.exceedsRange = snapbend::exceedsRange (result.maxAbsSemitones, configuredRange);
    result.suggestedRange = std::ceil (result.maxAbsSemitones);

    return result;
}

double BendCurve::snapToSemitone (double semitones) noexcept
{
    return std::round (semitones);
}

double BendCurve::snapToSemitone (double semitones, double strength) noexcept
{
    const double s = std::clamp (strength, 0.0, 1.0);
    return semitones + (std::round (semitones) - semitones) * s;
}

double BendCurve::snapToGrid (double beat, double gridInQuarterNotes) noexcept
{
    if (gridInQuarterNotes <= 0.0)
        return beat;

    return std::round (beat / gridInQuarterNotes) * gridInQuarterNotes;
}

std::string BendCurve::toString() const
{
    std::ostringstream out;
    out.precision (10);

    for (size_t i = 0; i < nodes.size(); ++i)
    {
        if (i > 0)
            out << ';';

        out << nodes[i].beat << ',' << nodes[i].semitones << ',' << nodes[i].shape;
    }

    return out.str();
}

std::optional<BendCurve> BendCurve::fromString (const std::string& text)
{
    BendCurve curve;

    if (text.empty())
        return curve;

    std::istringstream in (text);
    std::string        chunk;

    while (std::getline (in, chunk, ';'))
    {
        if (chunk.empty())
            continue;

        std::istringstream nodeIn (chunk);
        std::string        beatText, semiText, shapeText;

        if (! std::getline (nodeIn, beatText,  ',')) return std::nullopt;
        if (! std::getline (nodeIn, semiText,  ',')) return std::nullopt;
        if (! std::getline (nodeIn, shapeText, ',')) return std::nullopt;

        try
        {
            curve.nodes.push_back ({ std::stod (beatText),
                                     std::stod (semiText),
                                     std::clamp (std::stod (shapeText), -1.0, 1.0) });
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    curve.sortNodes();
    return curve;
}

} // namespace snapbend
