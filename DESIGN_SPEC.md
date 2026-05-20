# DESIGN SPEC — Grano

The visual identity, design tokens, component specifications, and motion language of the Grano plugin UI.

This is the source of truth for the JUCE `LookAndFeel` implementation in `Source/UI/LookAndFeel/GranoLAF.{h,cpp}`. Every visual decision in code must reference a token defined here.

---

## Identity

**Concept**: Lab Equipment Brutalist.

The plugin should feel like a piece of laboratory measurement equipment — rigorous, technical, function-first — but warmed by careful color choice and energized by live visualizations. Not synthwave. Not industrial. Not Ableton-minimalist. The reference frame is closer to a Tektronix oscilloscope in a contemporary research lab.

Principles:

1. **Semantic color, not decorative.** Every color carries meaning. No accent for accent's sake.
2. **Typography is structure.** Two typefaces, two roles, no exceptions.
3. **Geometry over skeumorphism.** Knobs and sliders are clearly virtual; no fake plastic or metal.
4. **Live visualization earns its space.** The waveform with grain particles is the centerpiece — the rest of the UI exists to control it.
5. **Restraint.** No gradients, no dramatic shadows, no neon. Halos are sutil.

---

## Window dimensions

- **Default**: 1000 × 620 px.
- **Min**: 800 × 500 px.
- **Max**: 2000 × 1240 px.
- **Aspect ratio**: not locked; layout is fluid.

Spacing baseline is 8 px. All paddings, gaps, and component offsets are multiples of 4 or 8 px.

---

## Color tokens

All colors below are defined as `juce::Colour::fromString` constants in `Source/UI/LookAndFeel/Colors.h`.

### Surfaces

| Token | Hex | Use |
|-------|-----|-----|
| `surface.base` | `#0A0B0D` | Outermost background. The "carbon" tone. |
| `surface.mid` | `#14161A` | Elevated panels (section panels). |
| `surface.high` | `#1F2329` | High-elevation surfaces (popovers, focused panels). |
| `surface.shaft` | `#050608` | The waveform shaft (slightly deeper than base). |

A subtle radial vignette darkens the corners of `surface.base` by ~6%. Implemented via a one-time gradient paint in the editor's `paint()` background. This is the **only gradient** allowed in the plugin.

### Text

| Token | Hex | Use |
|-------|-----|-----|
| `text.primary` | `#E8E6E1` | Section headers, primary labels, prominent values. |
| `text.secondary` | `#8B8985` | Sub-labels, value descriptors. |
| `text.tertiary` | `#4A4946` | Inactive labels, axis markers, fine-print info. |

### Accents (semantic)

| Token | Hex | Use |
|-------|-----|-----|
| `accent.vital` | `#7CF5C4` | Active values, master output, playhead, "OK" indicators, modulation destinations confirmed. Mint green. |
| `accent.grain` | `#C77CFF` | Grain particles on waveform, active-grain count display. Violet. |
| `accent.modulation` | `#FF6B9D` | LFO waveforms, mod matrix routing visualization. Magenta. |
| `accent.motion` | `#FFB347` | MOTION module visual tag and active state. Amber. |
| `accent.clipping` | `#FF5252` | Output clipping warning. Only this. |

The combination `vital + grain + modulation` is the plugin's signature triad. It should appear on every screenshot in some configuration.

### Strokes and dividers

| Token | Hex | Use |
|-------|-----|-----|
| `stroke.subtle` | `#1F2329` | Internal dividers within sections. |
| `stroke.default` | `#2A2E36` | Panel borders, control outlines (inactive). |
| `stroke.strong` | `#3A3F48` | Control outlines (hover, focus). |

All strokes are 1 px. The only exception is the focused control's halo (see Halos below).

### Halos (subtle glow on active controls)

Implemented in JUCE via `Graphics::drawDropShadow` with the accent color, low alpha, small radius.

| Token | Color | Radius | Alpha |
|-------|-------|--------|-------|
| `halo.modulating` | `accent.modulation` | 6 px | 0.35 |
| `halo.active` | `accent.vital` | 4 px | 0.25 |
| `halo.warning` | `accent.clipping` | 8 px | 0.50 |

Halos are deliberately sutil. They communicate state, not decoration.

---

## Typography

Two typefaces, embedded as `BinaryData` resources.

### Inter

- **Family**: Inter, designed by Rasmus Andersson.
- **License**: SIL Open Font License 1.1 (permits embedding and redistribution).
- **Source**: https://github.com/rsms/inter — use the Variable Font version, single file.
- **Weights used**: 400 (regular), 500 (medium).
- **Use**: All labels, section headers, button text, dropdown text. Anything that is human-readable language.

### JetBrains Mono

- **Family**: JetBrains Mono.
- **License**: SIL Open Font License 1.1.
- **Source**: https://www.jetbrains.com/lp/mono/ — Variable Font version.
- **Weights used**: 400 (regular), 500 (medium).
- **Use**: Numeric values, units, technical readouts, frequency/time displays.

### Type scale

| Role | Family | Size | Weight | Letter-spacing | Case |
|------|--------|------|--------|---|---|
| Display value (master, big readouts) | Inter | 22 px | 500 | 0 | as-is |
| Section header | Inter | 11 px | 500 | 0.16 em | UPPERCASE |
| Sub-label | Inter | 10 px | 500 | 0.12 em | UPPERCASE |
| Knob label | Inter | 9 px | 500 | 0.10 em | UPPERCASE |
| Body | Inter | 12 px | 400 | 0 | as-is |
| Numeric value (knob, slider) | JetBrains Mono | 11 px | 400 | 0 | as-is |
| Numeric value (large) | JetBrains Mono | 13 px | 500 | 0 | as-is |
| Logo | JetBrains Mono | 18 px | 500 | 0.32 em | UPPERCASE |

The 9 px minimum is reserved for knob labels in the densest layout area. Elsewhere, 11 px is the floor.

---

## Layout grid

12-column grid, 16 px gutter, 24 px outer padding on the editor.

```
┌──────────────────────────────────────────────────────────────────┐
│   HEADER (logo · snapshots · loop · master) — height 56 px       │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│   WAVEFORM DISPLAY — height ~ 33% of window (≥ 180 px)           │
│                                                                  │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│   CORE ENGINE — 4 knobs + 2 sliders (height ~ 140 px)            │
│                                                                  │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│   MODULES (MOTION + COLOR + PATTERN, 3 columns)                  │
│                                                                  │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│   MODULATION (LFO 1 + LFO 2 + MATRIX)                            │
│                                                                  │
├──────────────────────────────────────────────────────────────────┤
│   FOOTER (CPU · voices · grains · status) — height 28 px         │
└──────────────────────────────────────────────────────────────────┘
```

Layout implemented via `juce::FlexBox` per row. Modules and modulation panels reflow when the window narrows.

---

## Component specifications

### Knob

- **Diameter**: 64 px (default), scales to 72 px in compact layouts (none in F3, may apply in F5/F6).
- **Track**: 2 px outer ring, `stroke.default`.
- **Indicator**: 2 px wide, 18 px tall, centered, rotates from -135° (min) to +135° (max). Color: `accent.vital`.
- **Center value display**: JetBrains Mono 11 px, `text.primary`.
- **Label below**: Inter 9 px UPPERCASE letter-spaced, `text.secondary`.

**States**:
- Idle: indicator `accent.vital`, no halo.
- Hover: stroke becomes `stroke.strong`, indicator subtly brighter.
- Focused (keyboard or fine-edit mode): `halo.active` applied.
- Modulating: `halo.modulating` applied; indicator color shifts toward `accent.modulation` proportional to mod amount.

**Interaction**:
- Click + drag vertical: changes value. 1 px = 1% of range.
- Cmd/Ctrl + drag: fine mode, 1 px = 0.1% of range.
- Double click: reset to default.
- Shift + drag: snap to integer ticks where applicable.
- Right click: parameter popup (assign LFO, MIDI learn, reset, default value, copy, paste).

### Slider

- **Track**: 4 px tall, full available width, rounded ends (2 px radius), `stroke.default` background.
- **Fill**: from min to current value, `accent.vital`.
- **Thumb**: 12 × 12 px square with 2 px radius, `accent.vital`.
- **Value displayed**: above the slider, JetBrains Mono 11 px, right-aligned.
- **Label**: above the slider, Inter 10 px UPPERCASE, left-aligned.

**States** identical to Knob.

**Interaction**:
- Click + drag horizontal.
- Cmd/Ctrl + drag: fine.
- Double click: reset.

### Toggle switch

- **Track**: 32 × 16 px rounded (8 px radius), background `stroke.default` when off, `accent.vital` when on.
- **Thumb**: 12 × 12 px circle, `surface.mid`, slides 16 px on toggle.
- **Animation**: 120 ms ease-out.
- **Label**: positioned to the left, Inter 10 px UPPERCASE, `text.secondary`.

### Snapshot button

- **Size**: 22 × 22 px.
- **Background**: `surface.mid` when inactive, `surface.base` when active.
- **Border**: 1 px, `stroke.default` inactive, `accent.vital` active.
- **Text**: JetBrains Mono 11 px, `text.secondary` inactive, `accent.vital` active.
- **Layout**: four buttons side-by-side with 4 px gaps.

**Interaction**:
- Click: switch snapshot (instant or with 250 ms ramp depending on user setting).
- Shift + click: save current state into this slot, overwriting.
- Right click: snapshot menu (rename, clear, copy from another slot).

### Section panel

- **Padding**: 12 px.
- **Border**: 1 px `stroke.default`.
- **Border radius**: 4 px.
- **Background**: `surface.mid`.
- **Header**: section name UPPERCASE letter-spaced, plus optional tag dot (6 px circle in the module's accent color), plus optional bypass toggle on the right.

### Waveform display

- **Background**: `surface.shaft`.
- **Border**: 1 px `stroke.default`, radius 4 px.
- **Waveform**: rendered by `juce::AudioThumbnail`, color `text.tertiary` (`#4A4946`).
- **Playhead**: vertical line, 1.5 px wide, `accent.vital`, with a 3 px circle at the current position's vertical midpoint.
- **Grain particles**: 2-3 px translucent dots, `accent.grain`. Alpha equals current grain envelope amplitude. Y-coordinate distributed across the waveform's height with a small per-grain random offset for visual depth (deterministic from grain ID). Particles fade over 60 ms after the grain ends (motion trail).
- **Top-left label**: filename in Inter 10 px, `text.secondary`.
- **Top-right label**: duration · sample rate · channel count in JetBrains Mono 11 px, `text.secondary`.
- **Bottom-left label**: `0.0s` in JetBrains Mono 11 px, `text.tertiary`.
- **Bottom-center label**: `▪ N grains active` in JetBrains Mono 11 px, `accent.grain`.
- **Bottom-right label**: total duration in JetBrains Mono 11 px, `text.tertiary`.

**Interaction**:
- Click on waveform: sets `position` parameter to that point.
- Click + drag: scrub.
- Cmd/Ctrl + click + drag: place a marker.
- Drag file from OS: loads sample (drop target highlight in `accent.vital` at 0.15 alpha while dragging).
- Right click: menu (clear sample, load file dialog, save current state, etc.).

### LFO visualization

- **Container**: section panel with header "LFO 1" or "LFO 2" plus current rate (JetBrains Mono 11 px in `accent.modulation`).
- **Waveform**: SVG-like path rendered by JUCE `Path`, stroked in `accent.modulation`, 1.2 px width.
- **Below**: destination assignment (small dropdown) and modulation amount (small slider).

Both LFOs side by side in the modulation row.

### Module tag dot

A 6 px circle to the left of a module's section header, colored per the module's identity:

| Module | Color | Token |
|--------|-------|---|
| MOTION | Amber | `accent.motion` |
| COLOR | Violet | `accent.grain` |
| PATTERN | Mint | `accent.vital` |
| MODULATION | Magenta | `accent.modulation` |

---

## Header bar specification

```
LOGO              SNAPSHOTS    LOOP    MASTER VOLUME
[GRANO]           [A][B][C][D] [▮▯] ON  -6.0 dB
granular v0.1                                    ▼
                                            (slider here)
```

- **Logo**: "GRANO" in JetBrains Mono 18 px UPPERCASE letter-spaced 0.32 em.
- **Subtitle**: "granular · v<version>" in Inter 9 px UPPERCASE letter-spaced, `text.tertiary`.
- **Snapshots**: as specified above.
- **Loop**: toggle switch with "LOOP" label.
- **Master volume**: small vertical slider with numeric readout, JetBrains Mono 11 px in `accent.vital`. Range -∞ to +6 dB.

---

## Footer bar specification

A single row of telemetry, all in Inter 10 px or JetBrains Mono 11 px, left and right aligned:

```
cpu 8.2% · voices 12/64 · grains 42      midi learn · drag sample · ⌘+click for fine
```

- Labels in `text.tertiary`.
- Numeric values in their semantic accent (`accent.vital` for CPU and voices, `accent.grain` for grain count).
- Right side: contextual hints in `text.tertiary`. The hints rotate based on the most recently hovered control.

---

## Motion (animation)

Plugin UI animations follow strict rules:

| Element | Trigger | Duration | Easing |
|---------|---------|----------|--------|
| Toggle thumb | toggle click | 120 ms | ease-out |
| Snapshot switch | snapshot click | 250 ms (configurable) | ease-in-out |
| Halo on control focus | focus | 80 ms in / 200 ms out | ease-out |
| Particle fade | grain end | 60 ms | linear |
| Waveform repaint | playhead position change | 33 ms (30 fps cadence) | n/a (no easing) |
| Panel hover | mouse enter/leave | 100 ms | ease-out |

No animations exceed 300 ms. No spring physics, no bouncing.

---

## Iconography

Icons are rare in this UI. Where used, they follow the Tabler outline style (one-stroke, no fill). Embedded as SVG paths in `BinaryData::Icons`. Always at 16 × 16 px or 20 × 20 px. Color matches the surrounding text token unless semantic (e.g., red `accent.clipping` for warning).

Icons used in v1.0:

- `play` (player play)
- `pause`
- `loop` (refresh-style arrow loop)
- `drop` (upload arrow into tray)
- `info` (small i in circle)
- `warning` (small ! in triangle)

That's it.

---

## Accessibility notes

- Minimum text contrast: WCAG AA on all text against its background. The off-white `text.primary` on `surface.base` passes AAA. Sub-labels in `text.secondary` pass AA.
- Color is not the only carrier of information: modulation state has both a color shift and a halo; clipping has both a color shift and a numeric label.
- Keyboard navigation: tab traverses controls left-to-right, top-to-bottom. Space activates. Arrow keys nudge value (Shift for larger increments).
- Screen reader labels for every control via `setAccessibleName`.

---

## Reference frame

These are not direct copies, but the visual decisions are informed by what works in:

- Hardware: oscilloscopes (Tektronix, Rigol), modular synth front panels (Mutable Instruments), measurement equipment.
- Software: editorial UI work like Linear, Vercel, Stripe — for typography discipline; data viz libraries like Observable Plot — for restraint.

The plugin should look like it belongs in a production studio in 2030, not 2015.

---

## Asset checklist

F6 shipped without font embedding. Status as of v1.0.0-beta.1:

- [ ] Inter Variable Font (single file, ~ 200 KB) embedded via BinaryData — **deferred to v1.0.0 stable**
- [ ] JetBrains Mono Variable Font (~ 200 KB) embedded via BinaryData — **deferred to v1.0.0 stable**
- [ ] Tabler icon SVG subset (6 icons, ~ 4 KB) embedded — **deferred to v1.0.0 stable**
- [ ] Plugin window favicon for standalone (`.icns`, `.ico`, `.png`) — **post-v1.0**
- [ ] Splash screen art (loading state, ~ 800 ms max) — **post-v1.0**
- [ ] Cursor icons (default, fine-edit crosshair) — **post-v1.0**

All assets respect their licenses; SIL OFL fonts allow embedding without attribution display.

Until Inter and JetBrains Mono are embedded, `GranoLAF` falls back to `juce::Font::getDefaultSansSerifFontName()` for labels and `juce::Font::getDefaultMonospacedFontName()` for numeric values.
