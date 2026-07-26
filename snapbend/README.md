# SnapBend

A pitch bend plugin for Logic Pro where the bends land on actual semitones.

You draw a line on a grid. The grid's horizontal lines are semitones, and the
points you drop snap onto them — so "up a fifth" is genuinely up a fifth, not
somewhere in the region of a fifth. Drag the line between two points to curve
the slide, the same gesture as Logic's own automation.

![The bend lane](docs/screenshot-curve.png)

When a bend goes past what the synth has been allowed, it says so:

![The range prompt](docs/screenshot-range-prompt.png)

## What it is, technically

An **Audio Unit MIDI FX plugin**. It goes in the *MIDI FX* slot at the top of a
software instrument channel strip — above the synth, not in the audio chain —
and rewrites the MIDI on its way past, adding pitch bend that follows your
curve.

You still write your notes in Logic's own piano roll. SnapBend does not replace
it and cannot: Logic exposes no way for a plugin to touch its editor.

## Why bends usually go wrong, and what this does about it

Nearly every synth ignores large bends unless you ask permission first. Out of
the box the MIDI standard allows **±2 semitones**, so a drawn 7-semitone bend
comes out around 1.2 and the plugin looks broken.

SnapBend can transmit an **RPN 0,0 (Pitch Bend Sensitivity)** message to set the
synth's range for you, but **this is off by default and usually should stay
off**. The handshake is carried on **CC 6** and **CC 38**, and a synth that does
not implement RPN does not ignore those messages — it applies them to whatever
they happen to be mapped to on that patch. The result is a filter, envelope or
macro moving the instant the plugin loads, which sounds like the plugin has
ruined your patch. It has.

So the reliable route is to **set the bend range in the synth by hand** and put
the same number in the RANGE knob. Turn the handshake on only for synths you
know handle RPN properly.

If you drag a bend past what the synth has been allowed, a prompt appears
telling you — in plain words — to go and raise the range in the synth itself.

### It does nothing at all when it is doing nothing

With no points drawn, or MIX at zero, SnapBend passes MIDI through completely
untouched: no pitch bend, no handshake, not one byte added or removed. The
instrument sounds exactly as it does with the plugin taken off the strip.

This matters beyond tidiness — it previously swallowed pitch bend coming from
the region or your keyboard even while idle.

### If the semitones are slightly out

Bend all the way down, hold it, and compare against the note a semitone-count
lower on the keyboard. If it is a little sharp or flat, the synth's real bend
range is not quite what it claims.

Turn **FINE** until it is in tune. The trim is applied as a scale rather than a
fixed offset, because a range mismatch is worst at the extremes and vanishes at
the centre — so nulling it at full bend nulls it at every depth.

The other classic failures are handled too:

| Failure | What SnapBend does |
| --- | --- |
| Synth left detuned after pressing stop | Returns bend to centre on stop, bypass and panic |
| Stepped, zippery slides | 14-bit bend on a 1 ms grid with sample-accurate offsets |
| Bounce sounds different to playback | Everything is derived from the host's musical position, never wall-clock |
| Bend range silently re-written by something else | The RPN sequence is closed with RPN Null |
| Region's own bend fighting the plugin's | Incoming pitch bend is filtered out; SnapBend owns the channel |

## The controls

| Knob | What it does |
| --- | --- |
| **RANGE** | How far the synth is allowed to bend, 1–48 semitones. Set this to match what the synth is set to. |
| **FINE** | Corrects a synth whose real bend range does not quite match its setting. Measured in cents at full bend. |
| **CURVE** | Leans every slide towards moving early or late, without editing any points. |
| **SNAP** | Fully clockwise locks to whole semitones. Anticlockwise loosens it, for bends that deliberately sit just under the note. |
| **MIX** | Scales the whole effect, 0% to 100%. |

**Send bend range to synth** — **off by default, and it should usually stay
off.** See below.

**Clear points** — throws away the whole curve and starts again.

**Reset pitch** — sends the synth back to normal pitch, if a bend is ever left
hanging.

### Editing

- **Click** empty space to add a point
- **Drag** a point to move it (snapping to semitones and to the beat grid)
- **Drag the line** between two points to curve the slide
- **Right-click** a point to remove it (double-click works too)
- **Hold Alt** while dragging to ignore the grid entirely

### The ruler and playhead

A bar ruler runs along the top, numbered from bar 1 like Logic's own editors,
and following the host's time signature rather than assuming 4/4.

The playhead is drawn as a marker in the ruler and a line down the lane. It is
visible **whether or not the transport is running** — when stopped it shows
where playback will start from, which is how you line a bend up against a note
before pressing play. While playing, a dot rides the curve so you can watch the
bend happen.

The ruler is read-only; clicking it does not add points.

### Zoom

The two sliders at the bottom right work like the zoom controls in Logic's own
editors — vertical arrows for how many semitones are on screen, horizontal for
how much of the timeline.

Fully out is ±48 semitones and 128 beats; fully in is a couple of semitones and
two beats. **Double-click either slider** to return it to the default view.
Scrolling over the lane moves along the timeline, and the zoom setting is saved
with the project.

±48 really is as far out as it goes — that is the widest bend MIDI itself can
express.

## Building it

You need a Mac with **Xcode** installed (it is free from the App Store). Audio
Units only exist on macOS, so this is the one step that cannot happen anywhere
else.

```bash
cd snapbend
cmake -B build -G Xcode -DSNAPBEND_BUILD_PLUGIN=ON
cmake --build build --config Release
```

The first run downloads JUCE, which takes a few minutes. When it finishes, the
plugin has already been copied to
`~/Library/Audio/Plug-Ins/Components/SnapBend.component`.

### Then, in Logic

1. Restart Logic if it was open — it scans for plugins at launch.
2. Make a **software instrument** track and load any synth.
3. At the very top of the channel strip, click the **MIDI FX** slot.
4. Choose **SnapBend** (under the SnapBend manufacturer heading).

If it does not appear, Logic has failed it in validation. Run
`auval -v aumi Snb1 Snpb` in Terminal to see why. You may also need to open
**Logic Pro → Settings → Plug-in Manager** and reset the scan.

### Not wanting to touch Xcode at all

Push to GitHub and the included workflow (`.github/workflows/snapbend.yml`)
builds the Audio Unit on a free macOS runner and attaches
`SnapBend.component` as a downloadable artifact. Unzip it into
`~/Library/Audio/Plug-Ins/Components/` and restart Logic.

Because it is not signed by an Apple Developer account, macOS may block it the
first time. Right-click the `.component` in Finder and choose **Open**, or
allow it under **System Settings → Privacy & Security**.

## Running the tests

The bend engine is plain C++ with no dependencies, so its tests run anywhere —
no Mac, no DAW, no JUCE:

```bash
cd snapbend
cmake -B build -DSNAPBEND_BUILD_PLUGIN=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

Every test corresponds to a real way pitch bend goes wrong: wrong range, stepped
slides, a synth left detuned, a bounce that does not match playback.

## Layout

```
snapbend/
  core/          the engine — no JUCE, no dependencies, fully testable
    BendCurve    the curve model, snapping and shaped interpolation
    BendMath     14-bit conversion and the RPN range handshake
    BendEmitter  turns curve + transport position into MIDI
  plugin/        the JUCE Audio Unit
    ui/          the bend lane, the range prompt, the knobs
  tests/         core tests
```

## What is not here yet

- **MPE mode** — per-note bends, so one note in a chord can move on its own.
  Normal MIDI bend is channel-wide, so today a chord bends as a block.
- **An audio version** — pitch shifting the synth's output rather than bending
  its MIDI, for vocals and samples. The curve engine is already shared-ready.
- **Presets** for common bend shapes.

## Cost

Nothing. JUCE's free tier covers this, Xcode is free, and GitHub's macOS runners
are free on public repositories. An Apple Developer account ($99/yr) is only
needed to distribute a signed build to other people — not to use it yourself.
