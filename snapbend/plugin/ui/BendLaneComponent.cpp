#include "BendLaneComponent.h"
#include "SnapBendLookAndFeel.h"

using namespace snapbend;

namespace
{
    /** Left gutter where the semitone numbers live. */
    constexpr float labelGutter = 46.0f;
    constexpr float lanePadding = 10.0f;

    /** Bar-number strip along the top, matching the proportions Logic uses. */
    constexpr float rulerHeight = 22.0f;

    /** Semitones that get a brighter line, because they are the ones people
        actually bend to: octave, fifth, fourth, minor/major third, tone. */
    bool isLandmarkInterval (int semitone)
    {
        const int s = std::abs (semitone);
        return s == 0 || s == 2 || s == 3 || s == 4 || s == 5 || s == 7 || s == 12 || s == 24;
    }

    /** A deliberately sparse set of *labelled* intervals. Numbering every line
        turns the gutter into a wall of digits; these are far enough apart to
        read at a glance and are the ones you navigate by. */
    bool isLabelledInterval (int semitone)
    {
        const int s = std::abs (semitone);
        return s == 0 || s == 2 || s == 5 || s == 7 || s == 12
            || s == 19 || s == 24 || s == 36 || s == 48;
    }

    /** Octaves only — all that fits once the view is zoomed right out. */
    bool isOctave (int semitone)
    {
        return std::abs (semitone) % 12 == 0;
    }
}

BendLaneComponent::BendLaneComponent()
{
    setWantsKeyboardFocus (false);
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
}

BendLaneComponent::~BendLaneComponent() = default;

void BendLaneComponent::setCurve (const snapbend::BendCurve& newCurve)
{
    curve = newCurve;
    repaint();
}

void BendLaneComponent::setBendRange (double semitones)
{
    if (std::abs (bendRange - semitones) > 1.0e-9)
    {
        bendRange = semitones;
        repaint();
    }
}

void BendLaneComponent::setSnapStrength (double strength)
{
    snapStrength = juce::jlimit (0.0, 1.0, strength);
}

void BendLaneComponent::setShapeBias (double bias)
{
    if (std::abs (shapeBias - bias) > 1.0e-9)
    {
        shapeBias = bias;
        repaint();
    }
}

void BendLaneComponent::setPlayheadPosition (double beat, bool isPlaying)
{
    // This arrives on a timer, so it fires whether or not anything moved.
    // Repainting regardless meant the whole lane — grid, curve, every label —
    // was redrawn thirty times a second while sitting completely still, which
    // is what made dragging feel sticky.
    const bool positionMoved = std::abs (beat - playheadBeat) > 1.0e-9;
    const bool stateChanged  = isPlaying != transportRunning;

    if (! positionMoved && ! stateChanged)
        return;

    playheadBeat     = beat;
    transportRunning = isPlaying;

    const double span = getVisibleBeats();

    // Follow the playhead when it leaves the window, so a long bend does not
    // require chasing it by hand.
    if (isPlaying && (beat < viewStartBeat || beat > viewStartBeat + span))
        viewStartBeat = std::floor (beat / span) * span;

    repaint();
}

void BendLaneComponent::setZoom (double horizontal, double vertical)
{
    const double newHorizontal = juce::jlimit (0.0, 1.0, horizontal);
    const double newVertical   = juce::jlimit (0.0, 1.0, vertical);

    if (std::abs (newHorizontal - horizontalZoom) < 1.0e-9
        && std::abs (newVertical - verticalZoom) < 1.0e-9)
        return;

    // Zoom around the middle of what is currently on screen, so the thing you
    // were looking at stays roughly where it was instead of flying off.
    const double centreBeat = viewStartBeat + getVisibleBeats() * 0.5;

    horizontalZoom = newHorizontal;
    verticalZoom   = newVertical;

    viewStartBeat = juce::jmax (0.0, centreBeat - getVisibleBeats() * 0.5);

    repaint();
}

void BendLaneComponent::setCalibrating (bool shouldShow)
{
    if (calibrating != shouldShow)
    {
        calibrating = shouldShow;
        repaint();
    }
}

void BendLaneComponent::clearCurve()
{
    if (curve.isEmpty())
        return;

    curve.clear();
    hoverNode    = -1;
    hoverSegment = -1;

    notifyCurveChanged();
    repaint();
}

double BendLaneComponent::getVisibleBeats() const
{
    // Two beats when fully in, 128 when fully out, with the default 0.5 landing
    // on exactly 16 — four bars, which is the view the plugin opens with.
    return 2.0 * std::pow (64.0, 1.0 - horizontalZoom);
}

// ---------------------------------------------------------------------------
// Coordinates
// ---------------------------------------------------------------------------

juce::Rectangle<float> BendLaneComponent::getLaneBounds() const
{
    return getLocalBounds().toFloat()
             .withTrimmedLeft (labelGutter)
             .withTrimmedTop (rulerHeight)
             .reduced (0.0f, lanePadding);
}

juce::Rectangle<float> BendLaneComponent::getRulerBounds() const
{
    return getLocalBounds().toFloat()
             .withTrimmedLeft (labelGutter)
             .withHeight (rulerHeight);
}

int BendLaneComponent::getBarNumber (double beat) const
{
    // Logic counts bars from 1, and a project starts at bar 1 rather than 0.
    return static_cast<int> (std::floor (beat / quartersPerBar)) + 1;
}

void BendLaneComponent::setTimeSignature (int numerator, int denominator)
{
    if (numerator <= 0 || denominator <= 0)
        return;

    // A bar of 6/8 is three quarter notes, not six — the playhead is reported
    // in quarter notes regardless of what the bar is made of.
    const double newQuartersPerBar = numerator * 4.0 / denominator;

    if (std::abs (newQuartersPerBar - quartersPerBar) > 1.0e-9)
    {
        quartersPerBar = newQuartersPerBar;
        repaint();
    }
}

double BendLaneComponent::getDisplayRange() const
{
    // A little past the allowed range, so there is always visible territory
    // beyond what the synth can reach — that shaded band is what teaches the
    // limit. Only a little, though: shading half the grid makes the whole lane
    // look dirty and buries the curve.
    //
    // The zoom slider then scales that: a third of it when fully in, three
    // times when fully out, which at the widest reaches the ±48 semitone
    // ceiling that MIDI itself imposes.
    const double zoomFactor = std::pow (3.0, 1.0 - 2.0 * verticalZoom);

    return juce::jlimit (2.0, 48.0, bendRange * 1.35 * zoomFactor);
}

float BendLaneComponent::beatToX (double beat) const
{
    const auto lane = getLaneBounds();
    const double proportion = (beat - viewStartBeat) / getVisibleBeats();

    return lane.getX() + static_cast<float> (proportion) * lane.getWidth();
}

double BendLaneComponent::xToBeat (float x) const
{
    const auto lane = getLaneBounds();

    if (lane.getWidth() <= 0.0f)
        return viewStartBeat;

    return viewStartBeat + ((x - lane.getX()) / lane.getWidth()) * getVisibleBeats();
}

float BendLaneComponent::semitoneToY (double semitones) const
{
    const auto lane = getLaneBounds();
    const double proportion = (semitones / getDisplayRange() + 1.0) * 0.5; // 0 at bottom, 1 at top

    return lane.getBottom() - static_cast<float> (proportion) * lane.getHeight();
}

double BendLaneComponent::yToSemitone (float y) const
{
    const auto lane = getLaneBounds();

    if (lane.getHeight() <= 0.0f)
        return 0.0;

    const double proportion = (lane.getBottom() - y) / lane.getHeight();
    return (proportion * 2.0 - 1.0) * getDisplayRange();
}

// ---------------------------------------------------------------------------
// Snapping
// ---------------------------------------------------------------------------

double BendLaneComponent::applySnapToSemitone (double raw, bool snapDisabled) const
{
    if (snapDisabled)
        return raw;

    return snapbend::BendCurve::snapToSemitone (raw, snapStrength);
}

double BendLaneComponent::applySnapToBeat (double raw, bool snapDisabled) const
{
    if (snapDisabled)
        return raw;

    return snapbend::BendCurve::snapToGrid (raw, beatGrid);
}

// ---------------------------------------------------------------------------
// Hit testing
// ---------------------------------------------------------------------------

int BendLaneComponent::findNodeAt (juce::Point<float> position) const
{
    const auto& nodes = curve.getNodes();

    for (int i = static_cast<int> (nodes.size()) - 1; i >= 0; --i)
    {
        const juce::Point<float> nodePoint { beatToX (nodes[static_cast<size_t> (i)].beat),
                                             semitoneToY (nodes[static_cast<size_t> (i)].semitones) };

        if (nodePoint.getDistanceFrom (position) <= nodeHitRadius)
            return i;
    }

    return -1;
}

int BendLaneComponent::findSegmentAt (juce::Point<float> position) const
{
    const auto& nodes = curve.getNodes();

    if (nodes.size() < 2)
        return -1;

    const double beat = xToBeat (position.x);

    for (size_t i = 0; i + 1 < nodes.size(); ++i)
    {
        if (beat >= nodes[i].beat && beat <= nodes[i + 1].beat)
        {
            const float curveY = semitoneToY (curve.valueAtBeat (beat, shapeBias));

            // Only counts as grabbing the line if the pointer is near it.
            if (std::abs (curveY - position.y) <= 8.0f)
                return static_cast<int> (i);

            return -1;
        }
    }

    return -1;
}

// ---------------------------------------------------------------------------
// Mouse
// ---------------------------------------------------------------------------

void BendLaneComponent::mouseDown (const juce::MouseEvent& event)
{
    const auto position = event.position;

    // The ruler is for reading, not editing. Clicking it used to drop a point
    // at the very top of the range.
    if (getRulerBounds().contains (position))
        return;

    // Holding alt escapes the grid entirely, for bends that deliberately sit
    // just under the note.
    const bool snapDisabled = event.mods.isAltDown();

    const int node = findNodeAt (position);

    // Right-click (or ctrl-click) removes a point. Double-clicking works too,
    // but it is fiddly on a trackpad and easy to miss — and a miss used to add
    // a point instead of removing one, which is the opposite of what you asked
    // for. Right-click never adds anything.
    if (event.mods.isPopupMenu())
    {
        if (node >= 0)
        {
            curve.removeNode (static_cast<size_t> (node));
            hoverNode = -1;

            notifyCurveChanged();
            repaint();
        }

        return;
    }

    if (node >= 0)
    {
        dragMode  = DragMode::node;
        dragIndex = node;
        return;
    }

    // Holding command turns a drag on the line into a curve adjustment, the way
    // dragging the middle of a Logic automation ramp does. Without the
    // modifier, clicking the line adds a point — which is what you nearly
    // always want, and what used to be impossible because the line swallowed
    // the click and started shaping instead.
    if (event.mods.isCommandDown())
    {
        const int segment = findSegmentAt (position);

        if (segment >= 0)
        {
            dragMode       = DragMode::shape;
            dragIndex      = segment;
            dragStartShape = curve.getNodes()[static_cast<size_t> (segment)].shape;
            dragStartY     = position.y;
            setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
            return;
        }
    }

    // Anywhere else — including directly on the line — drops a new point and
    // starts dragging it immediately. It lands on the nearest semitone, so a
    // click that was never going to be pixel-accurate still gives an exact
    // interval you can then nudge.
    // Beats are clamped at zero — there is no music before the start of the
    // project, and a point dragged off the left edge used to vanish there.
    const double beat      = juce::jmax (0.0, applySnapToBeat (xToBeat (position.x), snapDisabled));
    const double semitones = applySnapToSemitone (yToSemitone (position.y), snapDisabled);

    dragIndex = static_cast<int> (curve.addNode ({ beat, semitones, 0.0 }));
    dragMode  = DragMode::node;

    notifyCurveChanged();
    repaint();
}

void BendLaneComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (dragIndex < 0)
        return;

    const bool snapDisabled = event.mods.isAltDown();

    if (dragMode == DragMode::node)
    {
        const double beat      = juce::jmax (0.0, applySnapToBeat (xToBeat (event.position.x),
                                                                   snapDisabled));
        const double semitones = juce::jlimit (-getDisplayRange(), getDisplayRange(),
                                               applySnapToSemitone (yToSemitone (event.position.y),
                                                                    snapDisabled));

        dragIndex = static_cast<int> (curve.moveNode (static_cast<size_t> (dragIndex),
                                                      beat, semitones));

        notifyCurveChanged();
        repaint();
    }
    else if (dragMode == DragMode::shape)
    {
        const auto& nodes = curve.getNodes();

        if (static_cast<size_t> (dragIndex) + 1 >= nodes.size())
            return;

        // Dragging up always bulges the line upwards, whether the bend is going
        // up or down — which means the sign flips for a falling segment.
        const double direction = nodes[static_cast<size_t> (dragIndex) + 1].semitones
                                     >= nodes[static_cast<size_t> (dragIndex)].semitones
                               ? 1.0 : -1.0;

        const double delta = (dragStartY - event.position.y) * 0.012 * direction;

        curve.setNodeShape (static_cast<size_t> (dragIndex), dragStartShape + delta);

        notifyCurveChanged();
        repaint();
    }
}

void BendLaneComponent::mouseUp (const juce::MouseEvent&)
{
    dragMode  = DragMode::none;
    dragIndex = -1;
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
}

void BendLaneComponent::mouseMove (const juce::MouseEvent& event)
{
    const int node    = findNodeAt (event.position);
    const int segment = (node >= 0 || ! event.mods.isCommandDown())
                          ? -1 : findSegmentAt (event.position);

    if (node != hoverNode || segment != hoverSegment)
    {
        hoverNode    = node;
        hoverSegment = segment;

        setMouseCursor (node >= 0    ? juce::MouseCursor::DraggingHandCursor
                      : segment >= 0 ? juce::MouseCursor::UpDownResizeCursor
                                     : juce::MouseCursor::CrosshairCursor);
        repaint();
    }
}

void BendLaneComponent::mouseDoubleClick (const juce::MouseEvent& event)
{
    const int node = findNodeAt (event.position);

    if (node >= 0)
    {
        curve.removeNode (static_cast<size_t> (node));
        hoverNode = -1;

        notifyCurveChanged();
        repaint();
    }
}

void BendLaneComponent::mouseWheelMove (const juce::MouseEvent&,
                                        const juce::MouseWheelDetails& wheel)
{
    // Scroll along the timeline. Trackpads report horizontal movement directly;
    // a plain wheel only has the vertical axis, so use whichever moved.
    const float amount = std::abs (wheel.deltaX) > std::abs (wheel.deltaY) ? wheel.deltaX
                                                                          : wheel.deltaY;

    if (std::abs (amount) < 1.0e-6f)
        return;

    viewStartBeat = juce::jmax (0.0, viewStartBeat - amount * getVisibleBeats() * 0.5);
    repaint();
}

void BendLaneComponent::mouseExit (const juce::MouseEvent&)
{
    if (hoverNode >= 0 || hoverSegment >= 0)
    {
        hoverNode    = -1;
        hoverSegment = -1;
        repaint();
    }
}

void BendLaneComponent::notifyCurveChanged()
{
    if (onCurveChanged != nullptr)
        onCurveChanged (curve);
}

void BendLaneComponent::resized()
{
    repaint();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void BendLaneComponent::paint (juce::Graphics& g)
{
    const auto lane = getLaneBounds();

    g.setColour (colours::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);

    drawOutOfRangeZones (g, lane);
    drawBeatGrid (g, lane);
    drawSemitoneGrid (g, lane);

    if (curve.isEmpty())
        drawEmptyHint (g, lane);
    else
        drawCurve (g, lane);

    drawNodes (g);
    drawPlayhead (g, lane);
    drawRuler (g, getRulerBounds());

    if (calibrating)
    {
        // Dim the grid: while the tone is running the lane is not what you
        // should be looking at.
        g.setColour (colours::background.withAlpha (0.82f));
        g.fillRect (lane);

        auto text = lane.withSizeKeepingCentre (lane.getWidth() - 60.0f, 70.0f);

        g.setColour (colours::accent);
        g.setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)));
        g.drawText ("Calibrating", text.removeFromTop (22.0f),
                    juce::Justification::centred, false);

        g.setColour (colours::text);
        g.setFont (juce::Font (juce::FontOptions (12.5f)));
        g.drawFittedText ("Two notes are playing in turn. They should be the same pitch.\n"
                          "If you hear the pitch jump between them, turn FINE until it stops.",
                          text.toNearestInt(), juce::Justification::centredTop, 3);
    }
}

void BendLaneComponent::drawOutOfRangeZones (juce::Graphics& g, juce::Rectangle<float> lane) const
{
    const float topLimit    = semitoneToY (bendRange);
    const float bottomLimit = semitoneToY (-bendRange);

    // Everything above and below these lines is out of the synth's reach. It is
    // shaded rather than blocked, because drawing there is how you discover you
    // need a wider range — and that is when the prompt appears.
    g.setColour (colours::outOfRange);
    g.fillRect (lane.withBottom (topLimit));
    g.fillRect (lane.withTop (bottomLimit));

    g.setColour (colours::warning.withAlpha (0.35f));

    const float dashes[] = { 4.0f, 4.0f };

    g.drawDashedLine ({ lane.getX(), topLimit, lane.getRight(), topLimit }, dashes, 2, 1.0f);
    g.drawDashedLine ({ lane.getX(), bottomLimit, lane.getRight(), bottomLimit }, dashes, 2, 1.0f);
}

void BendLaneComponent::drawSemitoneGrid (juce::Graphics& g, juce::Rectangle<float> lane) const
{
    const int limit = static_cast<int> (std::floor (getDisplayRange()));

    // Line and label density both have to follow the zoom. Zoomed in, every
    // semitone can be drawn and numbered; zoomed right out, 97 lines across a
    // few hundred pixels would be a grey hatch rather than a grid.
    const float pixelsPerSemitone = lane.getHeight() / static_cast<float> (2 * limit + 1);

    const bool drawEveryLine = pixelsPerSemitone >= 5.0f;

    const auto shouldLabel = [&] (int semitone)
    {
        if (pixelsPerSemitone >= 16.0f) return true;
        if (pixelsPerSemitone >= 7.0f)  return isLabelledInterval (semitone);
        return isOctave (semitone);
    };

    for (int semitone = -limit; semitone <= limit; ++semitone)
    {
        const float y = semitoneToY (semitone);

        if (y < lane.getY() - 1.0f || y > lane.getBottom() + 1.0f)
            continue;

        const bool landmark = isLandmarkInterval (semitone);
        const bool centre   = semitone == 0;

        if (drawEveryLine || isOctave (semitone))
        {
            g.setColour (centre   ? colours::gridLineBold.brighter (0.25f)
                       : landmark ? colours::gridLineBold
                                  : colours::gridLine);

            g.fillRect (lane.getX(), y, lane.getWidth(), centre ? 1.4f : 1.0f);
        }

        if (shouldLabel (semitone))
        {
            g.setColour (centre ? colours::text : colours::textDim);
            g.setFont (juce::Font (juce::FontOptions (centre ? 11.5f : 10.5f,
                                                      centre ? juce::Font::bold : juce::Font::plain)));

            const auto label = semitone > 0 ? "+" + juce::String (semitone)
                                            : juce::String (semitone);

            g.drawText (label,
                        juce::Rectangle<float> (0.0f, y - 8.0f, labelGutter - 8.0f, 16.0f),
                        juce::Justification::centredRight, false);
        }
    }
}

double BendLaneComponent::getBeatGridStep (juce::Rectangle<float> lane) const
{
    const double pixelsPerBeat = lane.getWidth() / getVisibleBeats();

    // Zoomed right out, a line per beat would be a solid wall; zoomed right in,
    // sixteenths are useful. Pick the finest interval still worth drawing.
    for (double step : { 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0 })
        if (pixelsPerBeat * step >= 14.0)
            return step;

    return 64.0;
}

void BendLaneComponent::drawBeatGrid (juce::Graphics& g, juce::Rectangle<float> lane) const
{
    const double step      = getBeatGridStep (lane);
    const double firstBeat = std::floor (viewStartBeat / step) * step;
    const double lastBeat  = viewStartBeat + getVisibleBeats();

    for (double beat = firstBeat; beat <= lastBeat; beat += step)
    {
        const float x = beatToX (beat);

        if (x < lane.getX() - 1.0f || x > lane.getRight() + 1.0f)
            continue;

        const bool barLine = std::fmod (std::abs (beat), quartersPerBar) < 1.0e-6;

        g.setColour (barLine ? colours::gridLineBold : colours::gridLine);
        g.fillRect (x, lane.getY(), barLine ? 1.4f : 1.0f, lane.getHeight());
    }
}

void BendLaneComponent::drawCurve (juce::Graphics& g, juce::Rectangle<float> lane) const
{
    const auto& nodes = curve.getNodes();

    if (nodes.empty())
        return;

    juce::Path path;

    // Sampled per pixel, using exactly the same evaluation the audio thread
    // uses — so what is drawn is what is played, shape knob included.
    const int steps = juce::jmax (2, static_cast<int> (lane.getWidth()));

    for (int i = 0; i <= steps; ++i)
    {
        const float  x     = lane.getX() + (lane.getWidth() * i) / static_cast<float> (steps);
        const double beat  = xToBeat (x);
        const float  y     = semitoneToY (curve.valueAtBeat (beat, shapeBias));

        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }

    // A soft fill back to the centre line gives the lane some weight without
    // shouting.
    juce::Path fill = path;
    fill.lineTo (lane.getRight(), semitoneToY (0.0));
    fill.lineTo (lane.getX(), semitoneToY (0.0));
    fill.closeSubPath();

    g.setColour (colours::accentSoft);
    g.fillPath (fill);

    g.setColour (colours::accent);
    g.strokePath (path, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
}

void BendLaneComponent::drawNodes (juce::Graphics& g) const
{
    const auto& nodes = curve.getNodes();

    for (size_t i = 0; i < nodes.size(); ++i)
    {
        const juce::Point<float> centre { beatToX (nodes[i].beat), semitoneToY (nodes[i].semitones) };
        const bool hovered = static_cast<int> (i) == hoverNode;
        const bool beyondRange = std::abs (nodes[i].semitones) > bendRange + 1.0e-6;

        const float radius = hovered ? nodeRadius + 2.0f : nodeRadius;

        g.setColour (colours::background);
        g.fillEllipse (juce::Rectangle<float> (radius * 2.0f + 3.0f, radius * 2.0f + 3.0f)
                           .withCentre (centre));

        // A point the synth cannot reach is drawn in the warning colour, so the
        // problem is visible on the grid and not only in the prompt.
        g.setColour (beyondRange ? colours::warning : colours::accent);
        g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));

        if (hovered)
        {
            g.setColour (colours::text);
            g.setFont (juce::Font (juce::FontOptions (11.0f)));

            const double value = nodes[i].semitones;
            const bool   whole = std::abs (value - std::round (value)) < 0.005;

            const auto readout = juce::String (value >= 0.0 ? "+" : "-")
                               + juce::String (std::abs (value), whole ? 0 : 2)
                               + " st";

            g.drawText (readout,
                        juce::Rectangle<float> (centre.x - 40.0f, centre.y - 26.0f, 80.0f, 14.0f),
                        juce::Justification::centred, false);
        }
    }
}

void BendLaneComponent::drawPlayhead (juce::Graphics& g, juce::Rectangle<float> lane) const
{
    const float x = beatToX (playheadBeat);

    if (x < lane.getX() || x > lane.getRight())
        return;

    // Visible whether or not the transport is running — when stopped it marks
    // where playback will start from, which is how Logic behaves and is the
    // only way to line a bend up with a note before pressing play.
    g.setColour (colours::playhead.withAlpha (transportRunning ? 0.65f : 0.3f));
    g.fillRect (x, lane.getY(), 1.0f, lane.getHeight());

    // A dot riding the curve, so you can see the bend happening.
    if (! curve.isEmpty())
    {
        const float y = semitoneToY (curve.valueAtBeat (playheadBeat, shapeBias));

        g.setColour (colours::playhead.withAlpha (transportRunning ? 1.0f : 0.45f));
        g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ x, y }));
    }
}

void BendLaneComponent::drawRuler (juce::Graphics& g, juce::Rectangle<float> ruler) const
{
    g.setColour (colours::panelRaised);
    g.fillRect (ruler);

    g.setColour (colours::gridLineBold);
    g.fillRect (ruler.getX(), ruler.getBottom() - 1.0f, ruler.getWidth(), 1.0f);

    const double firstBar = std::floor (viewStartBeat / quartersPerBar);
    const double lastBeat = viewStartBeat + getVisibleBeats();

    // Thin the numbering out rather than letting bar labels collide.
    const double pixelsPerBar = ruler.getWidth() / (getVisibleBeats() / quartersPerBar);

    // Squeezed to nothing by a resize, there is nothing worth drawing — and
    // without this guard the loop below would never terminate.
    if (pixelsPerBar <= 0.0)
        return;

    int barStep = 1;
    while (pixelsPerBar * barStep < 46.0 && barStep < 1024)
        barStep *= 2;

    for (double bar = firstBar; bar * quartersPerBar <= lastBeat; bar += 1.0)
    {
        const double beat = bar * quartersPerBar;
        const float  x    = beatToX (beat);

        if (x < ruler.getX() - 1.0f || x > ruler.getRight() + 1.0f)
            continue;

        const int barNumber = getBarNumber (beat + 1.0e-9);

        if ((barNumber - 1) % barStep != 0)
            continue;

        g.setColour (colours::gridLineBold);
        g.fillRect (x, ruler.getY() + 6.0f, 1.0f, ruler.getHeight() - 7.0f);

        g.setColour (colours::textDim);
        g.setFont (juce::Font (juce::FontOptions (10.5f)));
        g.drawText (juce::String (barNumber),
                    juce::Rectangle<float> (x + 8.0f, ruler.getY(), 40.0f, ruler.getHeight()),
                    juce::Justification::centredLeft, false);
    }

    // The playhead's head, sitting in the ruler exactly as it does in Logic.
    const float playheadX = beatToX (playheadBeat);

    if (playheadX >= ruler.getX() && playheadX <= ruler.getRight())
    {
        juce::Path head;
        head.addTriangle (playheadX - 5.0f, ruler.getY() + 3.0f,
                          playheadX + 5.0f, ruler.getY() + 3.0f,
                          playheadX,        ruler.getBottom() - 3.0f);

        g.setColour (colours::playhead.withAlpha (transportRunning ? 0.95f : 0.45f));
        g.fillPath (head);
    }
}

void BendLaneComponent::drawEmptyHint (juce::Graphics& g, juce::Rectangle<float> lane) const
{
    g.setColour (colours::textDim.withAlpha (0.75f));
    g.setFont (juce::Font (juce::FontOptions (13.0f)));

    g.drawText ("Click anywhere to add a bend point",
                lane.withHeight (24.0f).withY (lane.getCentreY() - 24.0f),
                juce::Justification::centred, false);

    g.setColour (colours::textDim.withAlpha (0.5f));
    g.setFont (juce::Font (juce::FontOptions (11.5f)));

    // CharPointer_UTF8 is required for anything above plain ASCII: JUCE's
    // String(const char*) decodes as Latin-1, which turns the separator dots
    // into "Â·".
    g.drawText (juce::CharPointer_UTF8 (
                    "Click the line to add a point  ·  drag a point to move it  "
                    "·  right-click to remove it  ·  \xe2\x8c\x98-drag the line to curve it  "
                    "·  hold Alt to ignore the grid"),
                lane.withHeight (20.0f).withY (lane.getCentreY() + 4.0f),
                juce::Justification::centred, false);
}
