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

Nearly every synth ships with a bend range of **±2 semitones**. Draw a 12
semitone bend against that and you hear about 2 — six times too small — and
the plugin looks broken.

SnapBend cannot know what your synth's range is, and it cannot work around it.
Pitch bend does not carry a pitch — it carries *a fraction of a range* — so
"bend to the top" means +2 semitones on a synth set to 2 and +12 on a synth set
to 12. If **RANGE** does not match the synth, every bend is wrong by the same
multiple, and no amount of trimming will fix it.

So there is exactly one rule: **RANGE must equal the synth's own bend range.**
Get that right and every semitone on the grid is exact.

### Finding it: press Calibrate

Rather than guessing, press **Calibrate**. It plays two notes over and over: a
reference note, and the same pitch reached by bending. Turn **RANGE** until they
stop jumping — at that point RANGE reads your synth's real range.

Matching two pitches against each other is far more precise than judging one in
isolation, so this resolves down to a couple of cents. RANGE accepts fractional
values for exactly that reason: a synth's true range is not always the round
number on its display.

If the number you land on is smaller than you want, raise the bend range **in
the synth** and calibrate again.

### Where the setting lives

| Synth | Where |
| --- | --- |
| **Serum / Serum 2** | Global tab → Pitch Bend Range. Defaults to **2**. |
| **Logic's own instruments** (ES2, Retro Synth, Sculpture…) | Bend range in the synth's own settings, usually 2 |
| **Most others** | Look for "Bend Range", "Pitch Bend Up/Down", or "Pitch Bend Sensitivity" |

Nearly everything ships at **±2 semitones**. That is the single most likely
reason a bend sounds far too small.

### Set synth range

**Set synth range** asks the synth to change its range to RANGE over MIDI
(RPN 0,0), once, when you press it.

It is a button rather than something automatic on purpose. The message rides on
**CC 6** and **CC 38**, and a synth that does not implement RPN does not ignore
them — it applies them to whatever those CCs happen to be mapped to, moving a
filter or an envelope and appearing to wreck the patch. As a deliberate press,
cause and effect are at least obvious. If it changes the sound, undo it in the
synth and set the range there by hand instead.

### It does nothing at all when it is doing nothing

With no points drawn, or MIX at zero, SnapBend passes MIDI through completely
untouched: no pitch bend, no handshake, not one byte added or removed. The
instrument sounds exactly as it does with the plugin taken off the strip.

This matters beyond tidiness — it previously swallowed pitch bend coming from
the region or your keyboard even while idle.

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
| **RANGE** | **The bend range your synth is set to.** The one setting everything depends on — see below. |
| **CURVE** | Leans every slide towards moving early or late, without editing any points. |
| **SNAP** | Fully clockwise locks to whole semitones. Anticlockwise loosens it, for bends that deliberately sit just under the note. |
| **MIX** | Scales the whole effect, 0% to 100%. |

**Calibrate** — finds the bend range your synth is really set to. See below.

**Set synth range** — asks the synth to change its range to RANGE, once, now.
Many synths ignore it. See below.

**Clear points** — throws away the whole curve and starts again.

**Reset pitch** — sends the synth back to normal pitch, if a bend is ever left
hanging.

### Editing

- **Click anywhere — including directly on the line — to add a point.** It
  lands on the nearest semitone, so a click that was never going to be
  pixel-accurate still gives an exact interval, which you can then drag.
- **Drag** a point to move it (snapping to semitones and to the beat grid)
- **Right-click** a point to remove it (double-click works too)
- **⌘-drag the line** between two points to curve the slide
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
