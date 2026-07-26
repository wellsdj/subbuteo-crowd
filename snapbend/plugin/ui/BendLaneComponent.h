#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../core/BendCurve.h"

/** The bend lane: a semitone grid you draw pitch bend curves onto.

    Horizontal lines are semitones, and points snap to them, so a bend lands on
    a real interval instead of somewhere approximately musical. The area beyond
    the synth's allowed range is shaded, so it is obvious *before* you hear it
    that a bend is out of reach.
*/
class BendLaneComponent : public juce::Component
{
public:
    BendLaneComponent();
    ~BendLaneComponent() override;

    void setCurve (const snapbend::BendCurve& newCurve);
    const snapbend::BendCurve& getCurve() const noexcept { return curve; }

    /** The synth's allowed range, in semitones — drives the shaded zone. */
    void setBendRange (double semitones);

    /** 1 = hard snap to whole semitones, 0 = free. */
    void setSnapStrength (double strength);

    /** Global lean applied by the CURVE knob, so the drawn line matches what
        will actually be played. */
    void setShapeBias (double bias);

    void setPlayheadPosition (double beat, bool isPlaying);

    /** The host's time signature, so the bar ruler counts bars the way Logic
        does rather than assuming 4/4. */
    void setTimeSignature (int numerator, int denominator);

    /** Zoom, 0 = fully out, 1 = fully in, 0.5 = the default view.
        Horizontal sets how much of the timeline is on screen, vertical how
        many semitones. */
    void setZoom (double horizontal, double vertical);

    /** Removes every point. */
    void clearCurve();

    /** Called whenever the user edits the curve. */
    std::function<void (const snapbend::BendCurve&)> onCurveChanged;

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    // ---- coordinate mapping --------------------------------------------

    juce::Rectangle<float> getLaneBounds() const;

    /** The bar-number strip along the top, as in Logic's own editors. */
    juce::Rectangle<float> getRulerBounds() const;

    float  beatToX (double beat) const;
    double xToBeat (float x) const;
    float  semitoneToY (double semitones) const;
    double yToSemitone (float y) const;

    /** How many semitones are visible above and below the centre line. Always
        leaves headroom past the allowed range, so the shaded out-of-reach zone
        is visible and reachable rather than hidden off screen. */
    double getDisplayRange() const;

    // ---- hit testing ----------------------------------------------------

    int findNodeAt (juce::Point<float> position) const;
    int findSegmentAt (juce::Point<float> position) const;

    // ---- drawing --------------------------------------------------------

    void drawSemitoneGrid (juce::Graphics&, juce::Rectangle<float> lane) const;
    void drawBeatGrid (juce::Graphics&, juce::Rectangle<float> lane) const;
    void drawOutOfRangeZones (juce::Graphics&, juce::Rectangle<float> lane) const;
    void drawCurve (juce::Graphics&, juce::Rectangle<float> lane) const;
    void drawNodes (juce::Graphics&) const;
    void drawPlayhead (juce::Graphics&, juce::Rectangle<float> lane) const;
    void drawRuler (juce::Graphics&, juce::Rectangle<float> ruler) const;
    void drawEmptyHint (juce::Graphics&, juce::Rectangle<float> lane) const;

    /** Bar number at a musical position, counting from 1 as Logic does. */
    int getBarNumber (double beat) const;

    double applySnapToSemitone (double raw, bool snapDisabled) const;
    double applySnapToBeat (double raw, bool snapDisabled) const;

    void notifyCurveChanged();

    // ---- state ----------------------------------------------------------

    snapbend::BendCurve curve;

    double bendRange     = 12.0;
    double snapStrength  = 1.0;
    double shapeBias     = 0.0;
    double playheadBeat  = 0.0;
    bool   transportRunning = false;

    double viewStartBeat = 0.0;
    double beatGrid      = 0.25;   ///< sixteenth notes

    /** Quarter notes per bar. 4 in 4/4, 3 in 6/8, and so on — the host tells
        us, we do not assume. */
    double quartersPerBar = 4.0;

    /** Zoom, 0..1. Both default to the middle, which gives four bars across
        and a little past the bend range vertically. */
    double horizontalZoom = 0.5;
    double verticalZoom   = 0.5;

    /** How much of the timeline is on screen, derived from horizontalZoom. */
    double getVisibleBeats() const;

    /** Chooses a beat interval whose gridlines will not end up on top of each
        other at the current zoom. */
    double getBeatGridStep (juce::Rectangle<float> lane) const;

    enum class DragMode { none, node, shape };

    DragMode dragMode      = DragMode::none;
    int      dragIndex     = -1;
    int      hoverNode     = -1;
    int      hoverSegment  = -1;
    double   dragStartShape = 0.0;
    float    dragStartY     = 0.0f;

    static constexpr float nodeRadius = 5.0f;

    /** Generous on purpose: a near-miss used to drop a new point on top of the
        one you were aiming at. */
    static constexpr float nodeHitRadius = 14.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BendLaneComponent)
};
