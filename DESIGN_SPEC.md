# DESIGN SPEC — Grano

The visual system of the Grano editor as shipped in 08-09. Source of truth for
`Source/UI/`. Tokens live in `Source/UI/LookAndFeel/GranoTheme.h`; no other
file may construct a colour from a literal. The approved interactive mock is
`design/mock/grano-ui.html`.

---

## Identity

**Concept: lighting console.** Grano is a desk you perform on. Every parameter
is a channel with a scribble strip (amber name and value), channels are grouped
in banks labelled with console tape, and the command line spells out whatever
you touch.

Principles (from `PRODUCT.md`):

1. **State is never hidden.** On/off, loaded slots, modulation and the live
   snapshot read at a glance, never by colour alone (bypass also strikes the
   tape label; modulation adds a marker and a range bracket).
2. **Performable first.** One screen, no tabs; big targets; type floor 13 pt.
3. **The sound leads.** The monitor shows engine telemetry: live grains, the
   loop-swept read head, jitter span and grain-size shadow.
4. **Not a generic plugin.** Flat caps and encoders, matte desk, no glow, no
   fake chrome.

---

## Canvas

| | |
|---|---|
| Base size | 1200 × 780 |
| Scale | uniform `AffineTransform`, 0.75× – 1.6× (`Theme::kMinScale/kMaxScale`) |
| Aspect | fixed (editor constrainer) |
| Layout | absolute bounds in base px (`ConsoleView::resized`, bank constructors) |

| Region | Bounds (x, y, w, h) |
|---|---|
| Wordmark | 18, 12 |
| Command line | 160, 8, 428, 56 |
| Snapshot keys A–D | 604, 8 (keys 50 × 38, pitch 58) |
| Loop key | 844, 8, 60 × 38 |
| Master (readout, clip LED, L/R meter, fader) | 918, 8, 266, 62 |
| Monitor | 16, 72, 1168, 118 |
| Slot strips 1–4 | 16 + 296·i, 198, 288 (last 280), 36 |
| GRAIN / ENGINE / ENVELOPE | y 244, h 200: x 16 w 524 · x 548 w 404 · x 960 w 224 |
| MOTION / COLOR / PATTERN | y 452, h 150: x 16 w 412 · x 436 w 258 · x 702 w 482 |
| LFO 1 / LFO 2 / MOD MATRIX | y 610, h 140: x 16 w 300 · x 324 w 300 · x 632 w 552 |
| Status strip | 18, 756, 1164, 16 |

---

## Colour tokens (`Theme::C`)

| Token | Hex | Use |
|---|---|---|
| desk | #17181B | editor background |
| plate | #1D1F23 | bank plates, slot strips |
| well | #0B0C0D | recessed wells: fader tracks, monitor, previews |
| line / line2 | #2C2F35 / #3A3E46 | seams / touched or hover seams |
| silk / silkDim | #CFCBC1 / #8C887F | silkscreen text / secondary text |
| lcdBg | #0E0D09 | scribble-strip glass |
| lcd | #FFB547 | values |
| lcdName | #E9E2D0 | strip names |
| lcdDim | #A38D61 | unlit strip text (previous/default, empty slot) |
| tape / tapeInk | #ECE7DA / #18181A | console tape, lit-key ink |
| cap / capHi | #5B5F67 / #7A7F88 | fader caps / hover |
| key / keyHover / edge | #2A2D33 / #2D3036 / #0A0A0B | unlit keys, outlines |
| knob | #33373E | encoder body |
| live | #FFFFFF | live snapshot, playhead, cap index line |
| clip | #FF4438 | clipping only (LED, meter top zone, load failure) |
| meterLo / meterMid | #8FD08A / #E8C55A | meter zones (< 70 %, 70–88 %) |
| grainLo → grainHi | #FFCD6E → #FF963C | grain heads, by envelope amplitude |

Module edge colours (tape edge, fills, lit keys):

| GRAIN | ENGINE / ENVELOPE | MOTION | COLOR | PATTERN | LFO / MATRIX (modulation) |
|---|---|---|---|---|---|
| #F5D547 | #9DB0C4 | #FF8A4C | #EF6AA8 | #55CC92 | #4FB0F7 |

Modulation blue (`mMod`) is reserved for modulation: LFO banks, matrix and the
mod marks on target controls.

---

## Type (`Theme::font`)

Embedded static cuts (OFL, licences in `Resources/Fonts/`), loaded from
BinaryData:

| Face | Cut | Use |
|---|---|---|
| `silk` | Archivo wdth 78 wght 600 | labels, strip names, silkscreen (uppercase, tracked) |
| `silkBold` | Archivo wdth 76 wght 700 | tape, keys, command-line name |
| `text` | Archivo wdth 90 wght 500 | hints, tooltips, info copy |
| `wordmark` | Archivo wdth 125 wght 800 | GRANO, snapshot letters |
| `lcd` | Martian Mono wdth 75 wght 500 | values, filenames, status numbers |

Sizes are point heights at 1×: floor 13 (labels), values 14–15, command-line
value 42. Units render in `silk` 13 at 80 % alpha next to the number
(`Console::drawValue`). Labels that do not fit squeeze horizontally (min 70 %)
instead of truncating.

---

## Components (`Source/UI/Console.h`)

All controls bind to APVTS through `juce::ParameterAttachment` (gestures,
automation, host sync). Shared behaviour: drag (Ctrl/Cmd = fine ×0.1),
double-click = default, wheel = step, arrows = step when focused (Tab focus;
a clicked control draws no focus ring), accessible slider role with value text,
tooltip + status hint from `ParamFormat::hint`.

| Component | Look | Used for |
|---|---|---|
| **Fader** | scribble strip (44 px) over a well with ticks, groove, module-colour fill (from centre if bipolar) and a flat 38 × 22 cap with a white index line | GRAIN channels |
| **Encoder** | 34 px flat knob, white indicator over 270°, strip below | MOTION, COLOR, feedback, spectral, LFO rate/depth, transient sensitivity |
| **ValueStrip** | 28 px strip, name left, value right, 2 px module-colour bar | Euclid pulses/steps/rotation, probability, reverse, spray, LFO phase |
| **HFader** | horizontal well + 14 × 18 cap | master volume, slot weights |
| **BipolarBar** | well, centre mark, blue fill from centre, value centred | matrix amounts |
| **Key / ChoiceKeys** | square lit key; lit = module colour + tape ink | choices (sub-grain, timing, envelope, trigger, spectral mode, LFO wave, matrix source) |
| **ToggleKey** | key showing On/Off | module bypass, feedback, spectral, LFO sync, loop |
| **ChoiceSelect** | strip-styled ComboBox with amber chevron | sync division, quantize scale, matrix destination |
| **Bank** | plate + console tape (title, module edge, optional On key); body at 38 % alpha and tape title struck through when off | every module |

Matrix destination menu: reserved destinations (no effect in v1.0,
`ParamFormat::modDestActive`) are disabled items; a row routed to one dims
like an unset row.

Modulation marks: a target of an active matrix route gets a 3 px blue
underline and a square marker on its strip, plus a range bracket (faders) or
arc (encoders) sized by |amount| × LFO depth.

---

## Header

- **Command line**: touched control's full name; `← previous · default X` in
  `lcdDim`; current value 42 pt right-aligned in a 196 px column. Also reports
  snapshot store/recall/clear and slot load/clear/failure.
- **Snapshots A–D**: lit white = live (last stored/recalled), amber letter =
  stored, status word below (live / stored / empty). Click recalls (or stores
  into an empty slot), Ctrl/Cmd-click stores, right-click menu Store current /
  Recall / Clear.
- **Loop**: lit amber key with loop icon, "Loop on/off" below.
- **Master**: dB readout, clip LED (holds 1.5 s), segmented L/R peak meter
  (−60…+6 dB, 5 px segments), volume fader.

## Monitor

Waveform of the selected slot (first channel; the engine plays a mono
downmix). Read head = Position + loop-sweep offset when Loop is on, labelled
LOOP. Jitter span (tape 6 %) and grain-size shadow (amber 10 %). Live grains
from the engine snapshot: head radius 2–4.6 px and colour warmed by envelope
amplitude, with a short read trace. Filename, duration · rate · channels,
time axis and grain count. Click/drag sets Position as one gesture. Drag a
file over the editor: amber outline; drop loads into the selected slot.
Empty: "DROP A SAMPLE TO START". While the engine reports no grain source
(`GranularEngine::isNoSourceActive`), a **NO SOURCE** label and two tape keys
(Test tone / Silence, `noSourceMode`) replace the grain count at the bottom
centre.

## Slot strips

Number key (lit tape = shown in monitor) · filename strip (click to load;
middle-truncated, extension kept; "click to load" when empty; "load failed" in
clip red with the error as tooltip) · weight fader (dimmed when empty; a
loaded slot at weight 0 is raised to 50 %) · clear key.

## Pattern context

Trigger keys Free / Sync / Euclid / Audio. Div select only in Sync and Euclid;
Euclid shows the ring (engine `Pattern::buildEuclidean`) and Pulses / Steps /
Rotation; Audio shows Transient Sensitivity; Free and Sync show an info line.

## Status strip

`N grains · cpu X % · K of 4 slots loaded` (numbers amber) and the hover hint
of whatever is under the pointer (default: drag / store / reset tips).

---

## Performance

One 30 Hz editor timer: monitor + meter every tick; slots, modulation marks,
snapshots and status at 10 Hz, repainting only on change. Controls repaint on
parameter change only. Audio-thread telemetry is relaxed atomics only (grain
snapshot, loop sweep, output peaks, load measurer).
