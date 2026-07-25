// Tests for the SnapBend core.
//
// Each of these corresponds to a real-world failure mode of hand-rolled pitch
// bend: wrong range, stepped slides, a synth left detuned after stop, or a
// bounce that does not match what you heard.

#include "TestFramework.h"

#include "../core/BendCurve.h"
#include "../core/BendEmitter.h"
#include "../core/BendMath.h"

#include <algorithm>

using namespace snapbend;

// =============================================================================
// Bend maths
// =============================================================================

TEST (centre_is_exactly_8192)
{
    CHECK_EQ (semitonesToPitchBend (0.0, 12.0), pitchBendCentre);
}

TEST (full_up_and_down_hit_the_rails)
{
    CHECK_EQ (semitonesToPitchBend (12.0, 12.0), pitchBendMax);
    CHECK_EQ (semitonesToPitchBend (-12.0, 12.0), 0);
}

TEST (a_semitone_is_the_same_size_wherever_it_lands)
{
    // With a range of 12, each semitone is 1/12 of the available travel. If this
    // drifts, chords bent by the same amount will not stay in tune with
    // themselves.
    for (int semitone = 1; semitone <= 12; ++semitone)
    {
        const double asSemitones = pitchBendToSemitones (
            semitonesToPitchBend (static_cast<double> (semitone), 12.0), 12.0);

        CHECK_NEAR (asSemitones, static_cast<double> (semitone), 0.001);
    }
}

TEST (round_trip_survives_awkward_ranges)
{
    for (double range : { 1.0, 2.0, 7.0, 12.0, 24.0, 48.0 })
        for (double semis : { -1.0, -0.5, 0.0, 0.5, 1.0 })
            CHECK_NEAR (pitchBendToSemitones (semitonesToPitchBend (semis * range, range), range),
                        semis * range, 0.01);
}

TEST (bend_value_is_split_into_seven_bit_halves)
{
    // A 14-bit bend is two 7-bit bytes, LSB first. Sending the MSB first, or
    // letting a byte exceed 127, is the classic "my bend jumps around" bug.
    const auto ev = makeBend (pitchBendMax, 1);

    CHECK_EQ (ev.status, 0xE0);
    CHECK_EQ (ev.data1, 127);
    CHECK_EQ (ev.data2, 127);

    const auto centre = makeCentredBend (1);
    CHECK_EQ (centre.data1, 0);
    CHECK_EQ (centre.data2, 64); // 8192 >> 7
}

TEST (out_of_range_values_clamp_rather_than_wrap)
{
    // Wrapping here would send a huge downward bend when the user asked for a
    // large upward one — an alarming and very audible bug.
    CHECK_EQ (semitonesToPitchBend (99.0, 12.0), pitchBendMax);
    CHECK_EQ (semitonesToPitchBend (-99.0, 12.0), 0);
}

TEST (channel_is_encoded_in_the_status_byte)
{
    CHECK_EQ (makeBend (pitchBendCentre, 1).status, 0xE0);
    CHECK_EQ (makeBend (pitchBendCentre, 16).status, 0xEF);
}

// =============================================================================
// The RPN handshake — the reason "snap to semitones" can be trusted
// =============================================================================

TEST (rpn_asks_for_the_right_range_and_closes_itself)
{
    const auto rpn = buildPitchBendRangeRPN (12.0, 1);

    CHECK_EQ (static_cast<long long> (rpn.size()), 6);

    CHECK_EQ (rpn[0].data1, 101); CHECK_EQ (rpn[0].data2, 0);   // RPN MSB
    CHECK_EQ (rpn[1].data1, 100); CHECK_EQ (rpn[1].data2, 0);   // RPN LSB -> bend sensitivity
    CHECK_EQ (rpn[2].data1, 6);   CHECK_EQ (rpn[2].data2, 12);  // 12 semitones
    CHECK_EQ (rpn[3].data1, 38);  CHECK_EQ (rpn[3].data2, 0);   // 0 cents

    // RPN Null. Without this, a later Data Entry message from anywhere else in
    // the chain silently re-writes the bend range we just set.
    CHECK_EQ (rpn[4].data1, 101); CHECK_EQ (rpn[4].data2, 127);
    CHECK_EQ (rpn[5].data1, 100); CHECK_EQ (rpn[5].data2, 127);
}

TEST (rpn_handles_fractional_ranges_as_cents)
{
    const auto rpn = buildPitchBendRangeRPN (12.5, 1);

    CHECK_EQ (rpn[2].data2, 12); // semitones
    CHECK_EQ (rpn[3].data2, 50); // cents
}

TEST (rpn_never_emits_a_byte_above_127)
{
    for (double range : { 0.0, 1.0, 12.0, 48.0, 999.0 })
        for (const auto& ev : buildPitchBendRangeRPN (range, 1))
        {
            CHECK (ev.data1 <= 127);
            CHECK (ev.data2 <= 127);
        }
}

// =============================================================================
// The curve
// =============================================================================

TEST (empty_curve_is_silent_not_random)
{
    BendCurve curve;
    CHECK_NEAR (curve.valueAtBeat (0.0), 0.0, 1e-9);
    CHECK_NEAR (curve.valueAtBeat (1234.5), 0.0, 1e-9);
}

TEST (curve_holds_its_value_outside_the_drawn_range)
{
    BendCurve curve;
    curve.addNode ({ 1.0, 0.0, 0.0 });
    curve.addNode ({ 2.0, 7.0, 0.0 });

    CHECK_NEAR (curve.valueAtBeat (0.0), 0.0, 1e-9);  // before the first node
    CHECK_NEAR (curve.valueAtBeat (99.0), 7.0, 1e-9); // after the last
}

TEST (linear_segment_interpolates_evenly)
{
    BendCurve curve;
    curve.addNode ({ 0.0, 0.0, 0.0 });
    curve.addNode ({ 4.0, 12.0, 0.0 });

    CHECK_NEAR (curve.valueAtBeat (0.0), 0.0, 1e-9);
    CHECK_NEAR (curve.valueAtBeat (1.0), 3.0, 1e-9);
    CHECK_NEAR (curve.valueAtBeat (2.0), 6.0, 1e-9);
    CHECK_NEAR (curve.valueAtBeat (4.0), 12.0, 1e-9);
}

TEST (segment_shape_bends_the_middle_but_pins_the_ends)
{
    // Dragging the middle of a segment must never move where the bend starts or
    // where it arrives — only how it gets there.
    for (double shape : { -1.0, -0.5, 0.0, 0.5, 1.0 })
    {
        BendCurve curve;
        curve.addNode ({ 0.0, 0.0, shape });
        curve.addNode ({ 4.0, 12.0, 0.0 });

        CHECK_NEAR (curve.valueAtBeat (0.0), 0.0, 1e-9);
        CHECK_NEAR (curve.valueAtBeat (4.0), 12.0, 1e-9);
    }

    BendCurve easeOut;
    easeOut.addNode ({ 0.0, 0.0, 1.0 });
    easeOut.addNode ({ 4.0, 12.0, 0.0 });

    // Positive shape moves early, so the halfway point is already past halfway.
    CHECK (easeOut.valueAtBeat (2.0) > 6.0);

    BendCurve easeIn;
    easeIn.addNode ({ 0.0, 0.0, -1.0 });
    easeIn.addNode ({ 4.0, 12.0, 0.0 });

    CHECK (easeIn.valueAtBeat (2.0) < 6.0);
}

TEST (shaped_curves_never_travel_backwards)
{
    // A non-monotonic shape would make a rising bend audibly dip on its way up.
    for (double shape : { -1.0, -0.7, -0.3, 0.0, 0.3, 0.7, 1.0 })
    {
        BendCurve curve;
        curve.addNode ({ 0.0, 0.0, shape });
        curve.addNode ({ 1.0, 12.0, 0.0 });

        double previous = -1.0;

        for (int i = 0; i <= 500; ++i)
        {
            const double value = curve.valueAtBeat (i / 500.0);
            CHECK (value >= previous - 1e-9);
            previous = value;
        }
    }
}

TEST (nodes_stay_ordered_even_when_dragged_past_each_other)
{
    BendCurve curve;
    curve.addNode ({ 0.0, 0.0, 0.0 });
    const size_t middle = curve.addNode ({ 1.0, 5.0, 0.0 });
    curve.addNode ({ 2.0, 10.0, 0.0 });

    // Drag the middle node beyond the last one.
    curve.moveNode (middle, 3.0, 5.0);

    const auto& nodes = curve.getNodes();
    CHECK_EQ (static_cast<long long> (nodes.size()), 3);

    for (size_t i = 0; i + 1 < nodes.size(); ++i)
        CHECK (nodes[i].beat <= nodes[i + 1].beat);
}

TEST (stacked_nodes_produce_an_instant_jump_not_a_divide_by_zero)
{
    BendCurve curve;
    curve.addNode ({ 1.0, 0.0, 0.0 });
    curve.addNode ({ 1.0, 12.0, 0.0 });

    const double value = curve.valueAtBeat (1.0);
    CHECK (std::isfinite (value));
}

TEST (semitone_snapping_lands_on_whole_numbers)
{
    CHECK_NEAR (BendCurve::snapToSemitone (6.8), 7.0, 1e-9);
    CHECK_NEAR (BendCurve::snapToSemitone (7.2), 7.0, 1e-9);
    CHECK_NEAR (BendCurve::snapToSemitone (-2.4), -2.0, 1e-9);
}

TEST (partial_snapping_pulls_towards_the_semitone_without_committing)
{
    // The SNAP knob. Fully clockwise must be indistinguishable from hard
    // snapping, fully anticlockwise must leave the value completely alone.
    CHECK_NEAR (BendCurve::snapToSemitone (6.8, 1.0), 7.0, 1e-9);
    CHECK_NEAR (BendCurve::snapToSemitone (6.8, 0.0), 6.8, 1e-9);
    CHECK_NEAR (BendCurve::snapToSemitone (6.8, 0.5), 6.9, 1e-9);

    // Never overshoots past the semitone it is heading for.
    for (double strength = 0.0; strength <= 1.0; strength += 0.05)
    {
        const double snapped = BendCurve::snapToSemitone (6.8, strength);
        CHECK (snapped >= 6.8 - 1e-9);
        CHECK (snapped <= 7.0 + 1e-9);
    }
}

TEST (curve_knob_leans_every_segment_without_moving_the_nodes)
{
    BendCurve curve;
    curve.addNode ({ 0.0, 0.0, 0.0 });
    curve.addNode ({ 4.0, 12.0, 0.0 });

    // Endpoints must be untouched whatever the knob is set to.
    for (double bias : { -1.0, -0.5, 0.0, 0.5, 1.0 })
    {
        CHECK_NEAR (curve.valueAtBeat (0.0, bias), 0.0, 1e-9);
        CHECK_NEAR (curve.valueAtBeat (4.0, bias), 12.0, 1e-9);
    }

    CHECK (curve.valueAtBeat (2.0, 1.0) > curve.valueAtBeat (2.0, 0.0));
    CHECK (curve.valueAtBeat (2.0, -1.0) < curve.valueAtBeat (2.0, 0.0));
}

TEST (curve_knob_cannot_push_a_segment_past_a_legal_shape)
{
    // A node already fully eased plus a fully cranked knob must still behave —
    // clamping, not running off into an unstable exponential.
    BendCurve curve;
    curve.addNode ({ 0.0, 0.0, 1.0 });
    curve.addNode ({ 4.0, 12.0, 0.0 });

    for (int i = 0; i <= 100; ++i)
    {
        const double value = curve.valueAtBeat (i * 0.04, 1.0);
        CHECK (std::isfinite (value));
        CHECK (value >= -1e-9);
        CHECK (value <= 12.0 + 1e-9);
    }
}

TEST (grid_snapping_respects_the_musical_grid)
{
    CHECK_NEAR (BendCurve::snapToGrid (1.26, 0.25), 1.25, 1e-9);
    CHECK_NEAR (BendCurve::snapToGrid (1.26, 1.0), 1.0, 1e-9);
    CHECK_NEAR (BendCurve::snapToGrid (1.26, 0.0), 1.26, 1e-9); // grid off
}

TEST (curve_survives_a_save_and_reload)
{
    BendCurve original;
    original.addNode ({ 0.0, 0.0, 0.5 });
    original.addNode ({ 2.5, -7.0, -0.25 });
    original.addNode ({ 4.0, 12.0, 0.0 });

    const auto restored = BendCurve::fromString (original.toString());
    CHECK (restored.has_value());

    if (restored)
    {
        CHECK_EQ (static_cast<long long> (restored->getNumNodes()), 3);

        for (size_t i = 0; i < original.getNumNodes(); ++i)
        {
            CHECK_NEAR (restored->getNodes()[i].beat,      original.getNodes()[i].beat, 1e-6);
            CHECK_NEAR (restored->getNodes()[i].semitones, original.getNodes()[i].semitones, 1e-6);
            CHECK_NEAR (restored->getNodes()[i].shape,     original.getNodes()[i].shape, 1e-6);
        }
    }
}

TEST (corrupt_saved_state_is_rejected_rather_than_crashing)
{
    CHECK (! BendCurve::fromString ("this is not a curve").has_value());
    CHECK (BendCurve::fromString ("").has_value()); // empty is a valid empty curve
}

// =============================================================================
// Range detection — what raises the "go and raise it in your synth" prompt
// =============================================================================

TEST (curve_within_range_raises_no_warning)
{
    BendCurve curve;
    curve.addNode ({ 0.0, 0.0, 0.0 });
    curve.addNode ({ 1.0, 7.0, 0.0 });

    const auto requirement = curve.getRangeRequirement (12.0);

    CHECK (! requirement.exceedsRange);
    CHECK_NEAR (requirement.maxAbsSemitones, 7.0, 1e-9);
}

TEST (curve_beyond_range_asks_for_a_bigger_one)
{
    BendCurve curve;
    curve.addNode ({ 0.0, 0.0, 0.0 });
    curve.addNode ({ 1.0, 14.0, 0.0 });

    const auto requirement = curve.getRangeRequirement (12.0);

    CHECK (requirement.exceedsRange);
    CHECK_NEAR (requirement.suggestedRange, 14.0, 1e-9);
}

TEST (downward_bends_count_too)
{
    BendCurve curve;
    curve.addNode ({ 0.0, -19.0, 0.0 });

    const auto requirement = curve.getRangeRequirement (12.0);

    CHECK (requirement.exceedsRange);
    CHECK_NEAR (requirement.suggestedRange, 19.0, 1e-9);
}

TEST (drawing_exactly_on_the_limit_is_allowed)
{
    BendCurve curve;
    curve.addNode ({ 0.0, 12.0, 0.0 });

    CHECK (! curve.getRangeRequirement (12.0).exceedsRange);
}

// =============================================================================
// The emitter
// =============================================================================

static BendCurve makeRisingCurve()
{
    BendCurve curve;
    curve.addNode ({ 0.0, 0.0, 0.0 });
    curve.addNode ({ 4.0, 12.0, 0.0 });
    return curve;
}

static bool containsPitchBend (const std::vector<RawMidiEvent>& events)
{
    return std::any_of (events.begin(), events.end(),
                        [] (const RawMidiEvent& e) { return (e.status & 0xF0) == 0xE0; });
}

static bool containsCentredBend (const std::vector<RawMidiEvent>& events)
{
    return std::any_of (events.begin(), events.end(),
                        [] (const RawMidiEvent& e)
                        {
                            return (e.status & 0xF0) == 0xE0 && e.data1 == 0 && e.data2 == 64;
                        });
}

static int countRPNSequences (const std::vector<RawMidiEvent>& events)
{
    return static_cast<int> (std::count_if (events.begin(), events.end(),
                                            [] (const RawMidiEvent& e)
                                            {
                                                return (e.status & 0xF0) == 0xB0
                                                    && e.data1 == 100 && e.data2 == 0;
                                            }));
}

/** The handshake is opt-in, so the tests that exercise it have to ask. */
static EmitterSettings settingsWithRPN (double range = 12.0)
{
    EmitterSettings settings;
    settings.sendRangeRPN       = true;
    settings.bendRangeSemitones = range;
    return settings;
}

TEST (range_is_announced_when_playback_starts)
{
    BendEmitter emitter;
    emitter.prepare (48000.0);
    emitter.setSettings (settingsWithRPN());

    const auto events = emitter.processBlock (makeRisingCurve(), { true, 0.0, 120.0 }, 512);
    CHECK_EQ (countRPNSequences (events), 1);
}

TEST (range_is_not_re_announced_every_single_block)
{
    // Spamming RPN on every block floods the MIDI stream and upsets some synths.
    BendEmitter emitter;
    emitter.prepare (48000.0);
    emitter.setSettings (settingsWithRPN());

    emitter.processBlock (makeRisingCurve(), { true, 0.0, 120.0 }, 512);
    const auto second = emitter.processBlock (makeRisingCurve(), { true, 0.25, 120.0 }, 512);

    CHECK_EQ (countRPNSequences (second), 0);
}

TEST (range_is_re_announced_after_the_user_changes_it)
{
    BendEmitter emitter;
    emitter.prepare (48000.0);
    emitter.setSettings (settingsWithRPN());
    emitter.processBlock (makeRisingCurve(), { true, 0.0, 120.0 }, 512);

    emitter.setSettings (settingsWithRPN (24.0));

    const auto events = emitter.processBlock (makeRisingCurve(), { true, 0.25, 120.0 }, 512);
    CHECK_EQ (countRPNSequences (events), 1);
}

TEST (stopping_returns_the_synth_to_centre)
{
    // The bug that leaves your instrument permanently out of tune.
    BendEmitter emitter;
    emitter.prepare (48000.0);

    emitter.processBlock (makeRisingCurve(), { true, 2.0, 120.0 }, 512);

    const auto onStop = emitter.processBlock (makeRisingCurve(), { false, 2.1, 120.0 }, 512);
    CHECK (containsCentredBend (onStop));
}

TEST (a_stopped_transport_stays_quiet)
{
    BendEmitter emitter;
    emitter.prepare (48000.0);

    // Never played, so there is nothing to reset and nothing to send.
    const auto first = emitter.processBlock (makeRisingCurve(), { false, 0.0, 120.0 }, 512);
    CHECK (first.empty());

    emitter.processBlock (makeRisingCurve(), { true, 0.0, 120.0 }, 512);
    emitter.processBlock (makeRisingCurve(), { false, 0.5, 120.0 }, 512); // the reset block

    const auto afterReset = emitter.processBlock (makeRisingCurve(), { false, 0.5, 120.0 }, 512);
    CHECK (afterReset.empty());
}

TEST (identical_values_are_not_sent_twice)
{
    // A flat curve should produce one message, not one per millisecond.
    BendCurve flat;
    flat.addNode ({ 0.0, 5.0, 0.0 });

    BendEmitter emitter;
    emitter.prepare (48000.0);

    emitter.processBlock (flat, { true, 0.0, 120.0 }, 512);
    const auto second = emitter.processBlock (flat, { true, 0.25, 120.0 }, 512);

    CHECK (! containsPitchBend (second));
}

TEST (a_slide_is_emitted_in_many_small_steps)
{
    // If this collapses to one message per block you get audible stepping.
    BendEmitter emitter;
    emitter.prepare (48000.0);

    // One second of audio at 120bpm covers two beats, i.e. half of a 4-beat
    // rise to 12 semitones — around 6 semitones of travel.
    const auto events = emitter.processBlock (makeRisingCurve(), { true, 0.0, 120.0 }, 48000);

    int bendCount = 0;
    for (const auto& e : events)
        if ((e.status & 0xF0) == 0xE0)
            ++bendCount;

    // At a 1ms grid, ~1000 opportunities; dedupe trims it, but it must be many.
    CHECK (bendCount > 200);
}

TEST (events_are_ordered_and_inside_the_block)
{
    // Out-of-order or out-of-range sample offsets are rejected by some hosts and
    // silently mangled by others.
    BendEmitter emitter;
    emitter.prepare (48000.0);

    const int  numSamples = 1024;
    const auto events = emitter.processBlock (makeRisingCurve(), { true, 0.0, 120.0 }, numSamples);

    int previousOffset = -1;

    for (const auto& e : events)
    {
        CHECK (e.sampleOffset >= 0);
        CHECK (e.sampleOffset < numSamples);
        CHECK (e.sampleOffset >= previousOffset);
        previousOffset = e.sampleOffset;
    }
}

TEST (the_mix_knob_at_zero_produces_no_bend)
{
    BendEmitter emitter;
    emitter.prepare (48000.0);

    EmitterSettings settings;
    settings.depth = 0.0;
    emitter.setSettings (settings);

    emitter.processBlock (makeRisingCurve(), { true, 0.0, 120.0 }, 4096);
    const auto events = emitter.processBlock (makeRisingCurve(), { true, 1.0, 120.0 }, 4096);

    // Everything sits at centre, so after the first message there is nothing
    // more to say.
    CHECK (! containsPitchBend (events));
}

TEST (the_mix_knob_scales_the_bend)
{
    BendCurve curve;
    curve.addNode ({ 0.0, 12.0, 0.0 });

    BendEmitter full, half;
    full.prepare (48000.0);
    half.prepare (48000.0);

    EmitterSettings halfSettings;
    halfSettings.depth = 0.5;
    half.setSettings (halfSettings);

    const auto fullEvents = full.processBlock (curve, { true, 0.0, 120.0 }, 512);
    const auto halfEvents = half.processBlock (curve, { true, 0.0, 120.0 }, 512);

    const auto valueOf = [] (const std::vector<RawMidiEvent>& events)
    {
        for (const auto& e : events)
            if ((e.status & 0xF0) == 0xE0)
                return (static_cast<int> (e.data2) << 7) | static_cast<int> (e.data1);

        return -1;
    };

    CHECK_EQ (valueOf (fullEvents), pitchBendMax);
    CHECK_NEAR (pitchBendToSemitones (valueOf (halfEvents), 12.0), 6.0, 0.05);
}

TEST (playback_position_drives_the_bend_so_bounces_match)
{
    // Two emitters at the same musical position must produce the same value,
    // regardless of how many blocks it took to get there. This is what makes an
    // offline bounce sound identical to live playback.
    BendEmitter live, offline;
    live.prepare (48000.0);
    offline.prepare (48000.0);

    const auto curve = makeRisingCurve();

    // Live: many small blocks up to beat 2.
    for (int i = 0; i < 8; ++i)
        live.processBlock (curve, { true, i * 0.25, 120.0 }, 6000);

    // Offline: one big jump to the same place.
    offline.processBlock (curve, { true, 0.0, 120.0 }, 64);
    const auto offlineEvents = offline.processBlock (curve, { true, 2.0, 120.0 }, 64);
    const auto liveEvents    = live.processBlock (curve, { true, 2.0, 120.0 }, 64);

    const auto lastBendValue = [] (const std::vector<RawMidiEvent>& events, int fallback)
    {
        int value = fallback;

        for (const auto& e : events)
            if ((e.status & 0xF0) == 0xE0)
                value = (static_cast<int> (e.data2) << 7) | static_cast<int> (e.data1);

        return value;
    };

    const int offlineValue = lastBendValue (offlineEvents, -1);
    const int liveValue    = lastBendValue (liveEvents, offlineValue);

    // Beat 2 of a 4-beat rise to 12 semitones = 6 semitones.
    CHECK_NEAR (pitchBendToSemitones (offlineValue, 12.0), 6.0, 0.05);
    CHECK_EQ (liveValue, offlineValue);
}

TEST (tempo_changes_are_followed)
{
    BendEmitter slow, fast;
    slow.prepare (48000.0);
    fast.prepare (48000.0);

    const auto curve = makeRisingCurve();

    // At double the tempo, one second of audio covers twice as much of the curve.
    const auto slowEvents = slow.processBlock (curve, { true, 0.0, 60.0 }, 48000);
    const auto fastEvents = fast.processBlock (curve, { true, 0.0, 240.0 }, 48000);

    const auto lastValue = [] (const std::vector<RawMidiEvent>& events)
    {
        int value = pitchBendCentre;

        for (const auto& e : events)
            if ((e.status & 0xF0) == 0xE0)
                value = (static_cast<int> (e.data2) << 7) | static_cast<int> (e.data1);

        return value;
    };

    CHECK (lastValue (fastEvents) > lastValue (slowEvents));
}

// =============================================================================
// Being completely invisible when not in use
//
// A plugin sitting in the chain doing nothing must not change the sound. The
// range handshake in particular rides on CC 6 and CC 38, which synths that do
// not implement RPN will apply to whatever those are mapped to — silently
// altering the patch.
// =============================================================================

TEST (a_disabled_emitter_sends_absolutely_nothing)
{
    BendEmitter emitter;
    emitter.prepare (48000.0);

    EmitterSettings settings;
    settings.enabled      = false;
    settings.sendRangeRPN = true; // even asked for, it must stay silent
    emitter.setSettings (settings);

    for (int i = 0; i < 8; ++i)
    {
        const auto events = emitter.processBlock (makeRisingCurve(),
                                                  { true, i * 0.5, 120.0 }, 4096);
        CHECK (events.empty());
    }
}

TEST (the_range_handshake_is_off_unless_asked_for)
{
    // The default must be silent, because the failure mode is a changed patch
    // rather than an error message.
    BendEmitter emitter;
    emitter.prepare (48000.0);

    CHECK (! emitter.getSettings().sendRangeRPN);

    const auto events = emitter.processBlock (makeRisingCurve(), { true, 0.0, 120.0 }, 512);
    CHECK_EQ (countRPNSequences (events), 0);
}

TEST (switching_off_mid_bend_un_bends_the_synth_exactly_once)
{
    BendEmitter emitter;
    emitter.prepare (48000.0);
    emitter.processBlock (makeRisingCurve(), { true, 2.0, 120.0 }, 512);

    EmitterSettings settings;
    settings.enabled = false;
    emitter.setSettings (settings);

    const auto onDisable = emitter.processBlock (makeRisingCurve(), { true, 2.1, 120.0 }, 512);
    CHECK (containsCentredBend (onDisable));

    // ...and then nothing further, ever.
    const auto afterwards = emitter.processBlock (makeRisingCurve(), { true, 2.2, 120.0 }, 512);
    CHECK (afterwards.empty());
}

TEST (an_emitter_that_never_ran_stays_silent_when_disabled)
{
    // Nothing was bent, so there is nothing to undo, so not one byte goes out.
    BendEmitter emitter;
    emitter.prepare (48000.0);

    EmitterSettings settings;
    settings.enabled = false;
    emitter.setSettings (settings);

    CHECK (emitter.processBlock (makeRisingCurve(), { true, 0.0, 120.0 }, 512).empty());
}

TEST (re_enabling_starts_cleanly)
{
    BendEmitter emitter;
    emitter.prepare (48000.0);

    EmitterSettings off;
    off.enabled = false;
    emitter.setSettings (off);
    emitter.processBlock (makeRisingCurve(), { true, 0.0, 120.0 }, 512);

    EmitterSettings on;
    on.enabled      = true;
    on.sendRangeRPN = true;
    emitter.setSettings (on);

    const auto events = emitter.processBlock (makeRisingCurve(), { true, 2.0, 120.0 }, 512);

    CHECK_EQ (countRPNSequences (events), 1);
    CHECK (containsPitchBend (events));
}

// =============================================================================
// Fine tuning
// =============================================================================

static double semitonesOfLastBend (const std::vector<RawMidiEvent>& events, double range)
{
    int value = pitchBendCentre;

    for (const auto& e : events)
        if ((e.status & 0xF0) == 0xE0)
            value = (static_cast<int> (e.data2) << 7) | static_cast<int> (e.data1);

    return pitchBendToSemitones (value, range);
}

TEST (fine_tune_shifts_full_bend_by_the_stated_cents)
{
    BendCurve curve;
    curve.addNode ({ 0.0, -11.0, 0.0 }); // just inside the range, so nothing clamps

    BendEmitter emitter;
    emitter.prepare (48000.0);

    EmitterSettings settings;
    settings.bendRangeSemitones = 12.0;
    settings.fineTuneCents      = 0.0;
    emitter.setSettings (settings);

    const double plain = semitonesOfLastBend (
        emitter.processBlock (curve, { true, 0.0, 120.0 }, 512), 12.0);

    CHECK_NEAR (plain, -11.0, 0.01);

    // 50 cents of trim, measured at full bend, is 11/12 of that at 11 semitones.
    BendEmitter trimmed;
    trimmed.prepare (48000.0);

    settings.fineTuneCents = 50.0;
    trimmed.setSettings (settings);

    const double corrected = semitonesOfLastBend (
        trimmed.processBlock (curve, { true, 0.0, 120.0 }, 512), 12.0);

    CHECK_NEAR (corrected, -11.0 - (11.0 / 12.0) * 0.5, 0.01);
}

TEST (fine_tune_is_proportional_so_nulling_it_once_nulls_it_everywhere)
{
    // A range mismatch is a scale error, not an offset. If this were applied as
    // a fixed offset, correcting it at full bend would break the middle.
    BendEmitter emitter;
    emitter.prepare (48000.0);

    EmitterSettings settings;
    settings.bendRangeSemitones = 12.0;
    settings.fineTuneCents      = 100.0; // one semitone at full bend
    emitter.setSettings (settings);

    for (double drawn : { -12.0, -6.0, -3.0, 3.0, 6.0, 12.0 })
    {
        BendCurve curve;
        curve.addNode ({ 0.0, drawn * 0.9, 0.0 }); // scaled in to avoid clamping

        BendEmitter fresh;
        fresh.prepare (48000.0);
        fresh.setSettings (settings);

        const double expected = drawn * 0.9 * (1.0 + 1.0 / 12.0);
        const double actual   = semitonesOfLastBend (
            fresh.processBlock (curve, { true, 0.0, 120.0 }, 512), 12.0);

        CHECK_NEAR (actual, expected, 0.02);
    }
}

TEST (fine_tune_leaves_the_centre_alone)
{
    // Whatever the trim, an unbent note must stay exactly where it was.
    BendCurve curve;
    curve.addNode ({ 0.0, 0.0, 0.0 });

    for (double cents : { -100.0, -25.0, 0.0, 25.0, 100.0 })
    {
        BendEmitter emitter;
        emitter.prepare (48000.0);

        EmitterSettings settings;
        settings.fineTuneCents = cents;
        emitter.setSettings (settings);

        const auto events = emitter.processBlock (curve, { true, 0.0, 120.0 }, 512);
        CHECK_NEAR (semitonesOfLastBend (events, 12.0), 0.0, 0.001);
    }
}

TEST (safety_reset_always_centres)
{
    BendEmitter emitter;
    emitter.prepare (48000.0);
    emitter.processBlock (makeRisingCurve(), { true, 3.0, 120.0 }, 512);

    CHECK (containsCentredBend (emitter.makeSafetyReset()));
}

TEST (a_silly_sample_rate_does_not_break_anything)
{
    BendEmitter emitter;
    emitter.prepare (0.0); // some hosts do this before they are fully set up

    const auto events = emitter.processBlock (makeRisingCurve(), { true, 0.0, 0.0 }, 512);

    for (const auto& e : events)
    {
        CHECK (e.sampleOffset >= 0);
        CHECK (e.sampleOffset < 512);
    }
}

TEST (zero_length_blocks_are_ignored)
{
    BendEmitter emitter;
    emitter.prepare (48000.0);

    CHECK (emitter.processBlock (makeRisingCurve(), { true, 0.0, 120.0 }, 0).empty());
}

int main()
{
    std::cout << "SnapBend core tests\n\n";
    return testing::runAll();
}
