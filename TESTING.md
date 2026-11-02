# TESTING.md — Grano Manual Acceptance Test

**Version:** v1.3 (multi-platform, console editor, No Source)

| Field          | Windows                     | macOS                        |
|----------------|-----------------------------|------------------------------|
| Tester         |                             |                              |
| Date           |                             |                              |
| Build          | v1.0.0-beta.4               | v1.0.0-beta.4                |
| DAW / Host     |                             |                              |
| OS Version     |                             |                              |
| Plugin formats | VST3 in DAW, Standalone     | VST3, AU, Standalone         |

Mark each result `[x]` (pass) or `[F]` (fail). For any `[F]` add one line below: `> Failure: <short description>`. Skipped tests: `[S]` with a note.

---

## 1. First Launch

### 1.1 Plugin loads without error
**Setup:** Plugin installed. Host DAW open. No project loaded.  
**Steps:**
1. Open host DAW.
2. Create a new audio or instrument track.
3. Insert Grano as a VST3 (Windows/Linux) or AU (macOS) plugin.

**Expected result:** Plugin window opens with no error dialogs. The whole console is visible on one screen (header, monitor, 4 slots, 9 banks, status strip). No audio glitches or crashes.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 1.2 Default state on init
**Setup:** Plugin just loaded (step 1.1 complete).  
**Steps:**
1. Read every scribble strip (each control shows its value and unit).

**Expected result:**
- Grain Size ≈ 100ms, Density ≈ 10 g/s, Position = 0%, Position Jitter = 0%.
- Pitch Shift = 0st, Stereo Spread = 50%, Master Volume = 0dB, Loop = off.
- MOTION, COLOR, PATTERN off: banks dimmed, tape titles struck through, On keys read OFF.
- LFO 1 and LFO 2 Rate = 1Hz, Sine waveform, Depth = 50%.
- Modulation matrix: all 8 slots = None → None × 0%.
- Snapshots A/B/C/D read EMPTY.
- Sub-grain Depth = 0, Stochastic Distribution = Uniform.
- Feedback off, Spectral off.
- Monitor reads "DROP A SAMPLE TO START"; all 4 slot strips read "click to load".  

**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 1.3 Standalone application launches
**Setup:** Grano Standalone installed.  
**Steps:**
1. Launch the Grano Standalone executable directly (not from a DAW).
2. Select an audio output device when prompted.

**Expected result:** Application window opens. Audio device initializes without errors. UI is fully functional.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

## 2. Sample Loading

### 2.1 Load WAV via slot strip
**Setup:** Plugin open, no sample loaded.  
**Steps:**
1. Click slot 1's filename strip ("click to load").
2. Navigate to a WAV file in the file picker.
3. Click **Open** (or equivalent).

**Expected result:** Monitor renders the waveform with filename, duration · rate · channels. Slot 1 shows the filename. The command line reports the load. Read head at position 0.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 2.2 Load AIFF into another slot
**Setup:** Plugin open, no sample loaded.  
**Steps:**
1. Click slot 2's filename strip.
2. Select an AIFF file.
3. Confirm.

**Expected result:** Slot 2 shows the filename; its weight fader is raised to 50% if it was at 0. Click slot number **2**: the monitor shows the AIFF. Engine outputs audio when the host is playing.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 2.3 Load FLAC via drag-and-drop
**Setup:** Plugin open, no sample loaded. File manager open beside the DAW.  
**Steps:**
1. Drag a FLAC file from the file manager over the Grano window (amber outline appears) and drop it.

**Expected result:** FLAC loads into the slot shown in the monitor (lit slot number). Waveform renders. No crash or silent failure.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 2.4 Load OGG via drag-and-drop
**Setup:** Plugin open, no sample loaded.  
**Steps:**
1. Drag an OGG file onto the Grano window.

**Expected result:** OGG sample loads and waveform renders correctly.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 2.5 Load MP3 via drag-and-drop
**Setup:** Plugin open, no sample loaded.  
**Steps:**
1. Drag an MP3 file onto the Grano window.

**Expected result:** MP3 sample loads and waveform renders correctly.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 2.6 Invalid file rejected gracefully
**Setup:** Plugin open.  
**Steps:**
1. Drag a non-audio file (e.g., a `.txt` or `.png`) onto the Grano window, then load one via a slot strip.

**Expected result:** File is rejected. No crash. Previously loaded sample (if any) remains unchanged. Via the slot strip, the strip reads "load failed" in red (hover for the reason) and the command line reports the failure.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 2.7 Engine produces audio after sample load
**Setup:** WAV sample loaded (test 2.1). Host is playing or standalone is running.  
**Steps:**
1. Ensure host transport is running.
2. Watch the MASTER meter and listen through monitors.

**Expected result:** Audible granular output. Grains are heard at roughly the expected density.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

## 3. Core Engine Parameters

> All tests in this section: WAV sample loaded, all modules bypassed, Feedback off, Spectral off, host playing.

### 3.1 Grain Size — minimum (5ms)
**Setup:** Default state + sample loaded.  
**Steps:**
1. Turn Grain Size to its minimum (5ms).

**Expected result:** Output sounds grainy, dense, and short — almost pitched noise. Distinct from default.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 3.2 Grain Size — maximum (500ms)
**Setup:** Default state + sample loaded.  
**Steps:**
1. Turn Grain Size to its maximum (500ms).

**Expected result:** Output sounds smooth, slow-moving, with very long overlapping grains.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 3.3 Density — minimum (1 g/s)
**Setup:** Default state + sample loaded.  
**Steps:**
1. Set Density to 1 g/s.

**Expected result:** Clearly spaced, sparse grain triggers. Audible gaps or silence between grains.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 3.4 Density — maximum (100 g/s)
**Setup:** Default state + sample loaded.  
**Steps:**
1. Set Density to 100 g/s.

**Expected result:** Dense, washy output. Significantly louder/fuller than at 1 g/s.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 3.5 Position — sweep from 0 to 100%
**Setup:** Default state + sample loaded (use a sample with distinct content at start vs. end).  
**Steps:**
1. Slowly sweep Position from 0% to 100%.

**Expected result:** The timbral character of the output changes continuously as the read position moves through the sample. Start and end positions sound distinctly different.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 3.6 Position Jitter — 0% vs. 100%
**Setup:** Default state + sample loaded. Position = 50%.  
**Steps:**
1. Set Position Jitter = 0%. Listen for a few seconds.
2. Set Position Jitter = 100%. Listen.

**Expected result:** At 0%: stable, consistent grain starting point. At 100%: grains scatter across the full sample, producing a diffuse, variable texture.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 3.7 Pitch Shift — +24st (two octaves up)
**Setup:** Default state + sample loaded. Grain Size = 200ms.  
**Steps:**
1. Set Pitch Shift to +24st.

**Expected result:** Output pitch is clearly two octaves above the natural sample pitch.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 3.8 Pitch Shift — −24st (two octaves down)
**Setup:** Default state + sample loaded. Grain Size = 200ms.  
**Steps:**
1. Set Pitch Shift to −24st.

**Expected result:** Output pitch is clearly two octaves below the natural sample pitch.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 3.9 Pitch Shift — 0st (no pitch change)
**Setup:** Pitch shifted (test 3.7 or 3.8 complete).  
**Steps:**
1. Return Pitch Shift to 0st.

**Expected result:** Output pitch matches the original sample pitch (within granular approximation).  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 3.10 Stereo Spread — 0% (mono)
**Setup:** Default state + sample loaded.  
**Steps:**
1. Set Stereo Spread to 0%.
2. Pan-check on a stereo scope or by toggling L/R solo.

**Expected result:** Left and right channels are identical (mono). No stereo width.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 3.11 Stereo Spread — 100% (full width)
**Setup:** Default state + sample loaded.  
**Steps:**
1. Set Stereo Spread to 100%.
2. Check stereo scope.

**Expected result:** Wide stereo image. Left and right grains diverge noticeably.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 3.12 Master Volume — minimum (−60dB)
**Setup:** Default state + sample loaded.  
**Steps:**
1. Set Master Volume to −60dB.

**Expected result:** Output is near-silent (−60dB is effectively inaudible on most monitors).  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 3.13 Master Volume — +6dB
**Setup:** Default state + sample loaded.  
**Steps:**
1. Set Master Volume to +6dB.

**Expected result:** Output is louder than at 0dB. Ensure no unexpected clipping in the plugin's own output stage (may clip downstream).  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 3.14 Loop toggle — on
**Setup:** Short sample loaded (< 2s). Position = 0%.  
**Steps:**
1. Enable Loop.
2. Let the engine run for 10 seconds.

**Expected result:** Granular output continues indefinitely. No silence or dropout when the playhead would otherwise reach the end of the sample.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 3.15 Loop toggle — off
**Setup:** Same short sample. Loop was on (test 3.14).  
**Steps:**
1. Disable Loop.
2. Advance Position to 100%.

**Expected result:** Grains at or near the end of the sample may become sparse or silent if the position playhead passes the end. Output does not loop back automatically.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

## 4. MOTION Module

> All tests: sample loaded, host playing, COLOR/PATTERN bypassed.

### 4.1 MOTION bypass toggle
**Setup:** Default state + sample loaded. MOTION module bypassed.  
**Steps:**
1. Click the **On** key on the MOTION tape (bank undims).
2. Set Wow Depth = 50%, Wow Rate = 0.5Hz.
3. Listen.
4. Click the On key again (MOTION off; bank dims, title struck through).

**Expected result:** With MOTION on: slow pitch wavering (wow) audible. With MOTION off (bypass): wow disappears. Clean before/after comparison.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 4.2 Wow Depth — sweep
**Setup:** MOTION enabled. Wow Rate = 0.5Hz.  
**Steps:**
1. Sweep Wow Depth from 0 to max.

**Expected result:** Pitch modulation depth increases continuously. At max, a pronounced slow pitch wobble is clearly audible.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 4.3 Wow Rate — sweep
**Setup:** MOTION enabled. Wow Depth = 50%.  
**Steps:**
1. Sweep Wow Rate from minimum to maximum.

**Expected result:** The speed of the pitch wobble increases from very slow (tape wow) to faster oscillation.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 4.4 Flutter Depth — sweep
**Setup:** MOTION enabled. Flutter Rate = 10Hz.  
**Steps:**
1. Sweep Flutter Depth from 0 to max.

**Expected result:** A faster, shallower pitch variation (flutter) increases in prominence. Distinctly higher frequency than wow.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 4.5 Flutter Rate — sweep
**Setup:** MOTION enabled. Flutter Depth = 50%.  
**Steps:**
1. Sweep Flutter Rate from min to max.

**Expected result:** The flutter frequency changes from slow to fast.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 4.6 Drift Amount
**Setup:** MOTION enabled.  
**Steps:**
1. Set Drift Amount to maximum.
2. Listen for 10–15 seconds.

**Expected result:** Slow, random-walk pitch drift accumulates over time. Not periodic like wow. Output wanders in pitch unpredictably.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 4.7 Crackle Level
**Setup:** MOTION enabled.  
**Steps:**
1. Set Crackle Level to maximum.

**Expected result:** Audible crackle artifacts added to the output — irregular, impulsive noise like a vinyl record.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 4.8 Crackle Color
**Setup:** MOTION enabled. Crackle Level = 75%.  
**Steps:**
1. Sweep Crackle Color from minimum to maximum.

**Expected result:** The tonal quality of the crackle changes — from darker/duller to brighter/harsher (or vice versa depending on implementation direction).  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

## 5. COLOR Module

> All tests: sample loaded, host playing, MOTION/PATTERN bypassed.

### 5.1 COLOR bypass toggle
**Setup:** COLOR bypassed. Saturate = 100%.  
**Steps:**
1. Enable COLOR (bypass off). Listen.
2. Re-enable bypass. Listen.

**Expected result:** With COLOR on: audible saturation/distortion. With bypass on: clean granular output.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 5.2 Saturate — sweep
**Setup:** COLOR enabled.  
**Steps:**
1. Sweep Saturate from 0 to 100%.

**Expected result:** Increasing harmonic saturation/warmth. At high values, audible harmonic distortion.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 5.3 Decimate — sweep
**Setup:** COLOR enabled. Saturate = 0.  
**Steps:**
1. Sweep Decimate from 0 to 100%.

**Expected result:** Bit-crushing / sample-rate reduction effect increases. At high values, clearly lo-fi / aliased character.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 5.4 Tilt EQ — negative (dark)
**Setup:** COLOR enabled.  
**Steps:**
1. Set Tilt to −1.

**Expected result:** High frequencies roll off, low frequencies boosted. Output sounds darker/bassier.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 5.5 Tilt EQ — positive (bright)
**Setup:** COLOR enabled.  
**Steps:**
1. Set Tilt to +1.

**Expected result:** High frequencies boosted, lows rolled off. Output sounds brighter/thinner.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 5.6 Tilt EQ — zero (flat)
**Setup:** COLOR enabled. Tilt was at an extreme.  
**Steps:**
1. Set Tilt to 0.

**Expected result:** Output returns to spectrally unaltered. No tonal coloring from Tilt alone.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 5.7 Verb Mix — sweep
**Setup:** COLOR enabled.  
**Steps:**
1. Sweep Verb Mix from 0 to 100%.

**Expected result:** Reverb tail builds in. At 100%, a clearly reverberant, decaying tail follows each grain cluster. At 0%, no reverb.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

## 6. PATTERN Module

> All tests: sample loaded, host playing, MOTION/COLOR bypassed.

### 6.1 PATTERN bypass toggle
**Setup:** PATTERN bypassed. Trigger Mode = Euclidean (32 steps, 8 pulses).  
**Steps:**
1. Enable PATTERN. Listen.
2. Bypass PATTERN. Listen.

**Expected result:** With PATTERN on: rhythmic, structured grain trigger pattern. With bypass: free (uniform) grain triggering.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 6.2 Trigger Mode — Free
**Setup:** PATTERN enabled.  
**Steps:**
1. Set Trigger Mode to **Free**.

**Expected result:** Grains trigger continuously at the rate set by Density. Not sync-locked, not rhythmic.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 6.3 Trigger Mode — Sync
**Setup:** PATTERN enabled. Host transport running with a clear tempo.  
**Steps:**
1. Set Trigger Mode to **Sync**.
2. Adjust Sync Division (e.g., 1/8).

**Expected result:** Grain triggers lock to the host BPM at the chosen subdivision. Rhythmically predictable when monitored against a click.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 6.4 Sync Division — change value
**Setup:** PATTERN enabled, Trigger Mode = Sync, host at 120BPM.  
**Steps:**
1. Change Sync Division from 1/4 to 1/16.

**Expected result:** Grain trigger rate increases 4×. At 1/16, triggers occur four times as frequently as at 1/4.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 6.5 Trigger Mode — Euclidean
**Setup:** PATTERN enabled.  
**Steps:**
1. Set Trigger Mode to **Euclidean**.
2. Set Eucl Steps = 16, Eucl Pulses = 4, Eucl Rotation = 0.

**Expected result:** 4 evenly spaced grain triggers per 16-step cycle. Rhythmic and repetitive.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 6.6 Euclidean — Rotation offset
**Setup:** PATTERN enabled, Trigger Mode = Euclidean, Steps = 16, Pulses = 4, Rotation = 0.  
**Steps:**
1. Increment Eucl Rotation from 0 to 4.

**Expected result:** The phase of the euclidean pattern shifts. Triggers shift in time relative to the cycle start.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 6.7 Trigger Mode — Audio
**Setup:** PATTERN enabled. Route an audio signal with clear transients (e.g., drums) into the sidechain input if available, or use the plugin's own output.  
**Steps:**
1. Set Trigger Mode to **Audio**.
2. Play the transient-rich source.
3. Adjust Transient Sensitivity from low to high.

**Expected result:** Grain triggers fire in response to incoming transients. Higher Transient Sensitivity fires on subtler transients. At low sensitivity, only loud hits trigger.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 6.8 Probability — effect
**Setup:** PATTERN enabled, Trigger Mode = Free.  
**Steps:**
1. Set Probability = 100%. Count grain events.
2. Set Probability = 50%. Count grain events over same duration.

**Expected result:** At 50%, roughly half as many grain triggers fire compared to 100%. Output is sparser and intermittent.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 6.9 Reverse Prob — effect
**Setup:** PATTERN enabled. Sample with recognizable directionality loaded.  
**Steps:**
1. Set Reverse Prob = 0%. Listen.
2. Set Reverse Prob = 100%. Listen.

**Expected result:** At 0%: all grains play forward. At 100%: all grains play backward. Distinct timbral change.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 6.10 Quantize Scale — change
**Setup:** PATTERN enabled. Pitch Shift active (±5st sweep).  
**Steps:**
1. Set Quantize Scale to **Major**.
2. Sweep Pitch Shift across its range.

**Expected result:** Pitch Shift values snap to semitones of the major scale. Perceived pitch jumps in scale steps, not chromatically.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 6.11 Quantize Scale — all 11 scales enumerate
**Setup:** PATTERN enabled.  
**Steps:**
1. Click through all 11 Quantize Scale options and confirm each label is distinct and selectable.

**Expected result:** 11 distinct scale names appear (e.g., Major, Minor, Dorian, Pentatonic, etc.). No duplicate entries, no blank slots.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 6.12 Spray — sweep
**Setup:** PATTERN enabled, Trigger Mode = Free.  
**Steps:**
1. Set Spray = 0. Listen to grain trigger timing.
2. Set Spray = 100%. Listen.

**Expected result:** At 0%: triggers evenly spaced. At 100%: trigger timing is randomized/stochastic, producing an uneven, humanized rhythm.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

## 7. LFO

### 7.1 LFO 1 — Rate sweep (Hz mode)
**Setup:** LFO 1 assigned in modulation matrix: Source = LFO1, Dest = Grain Size, Amount = 50%. Sync off.  
**Steps:**
1. Sweep LFO 1 Rate from 0.01Hz to 8000Hz.

**Expected result:** At low rates (< 1Hz): slow, perceptible sweep of Grain Size. At audio rates (> 20Hz): the grain size modulation becomes a timbral/tonal effect rather than a slow sweep.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 7.2 LFO 1 — Waveform: Sine
**Setup:** LFO 1 modulating Pitch Shift ±5st, Rate = 2Hz.  
**Steps:**
1. Select Sine waveform on LFO 1.

**Expected result:** Smooth, continuous pitch modulation — slow vibrato with no sharp edges.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 7.3 LFO 1 — Waveform: Triangle
**Setup:** Same as 7.2.  
**Steps:**
1. Select Triangle waveform.

**Expected result:** Linear ramp up/down pitch — audibly different from sine (V-shaped rather than curved).  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 7.4 LFO 1 — Waveform: Saw
**Setup:** Same as 7.2.  
**Steps:**
1. Select Saw waveform.

**Expected result:** Pitch ramps in one direction then resets sharply. Asymmetric modulation clearly audible.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 7.5 LFO 1 — Waveform: Square
**Setup:** Same as 7.2.  
**Steps:**
1. Select Square waveform.

**Expected result:** Pitch alternates between two values with hard transitions — a pitch trill effect.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 7.6 LFO 1 — Waveform: Random
**Setup:** Same as 7.2.  
**Steps:**
1. Select Random waveform.

**Expected result:** Pitch modulation is continuously random and non-periodic. No repeating pattern audible.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 7.7 LFO 1 — Waveform: S&H (Sample & Hold)
**Setup:** Same as 7.2.  
**Steps:**
1. Select S&H waveform.

**Expected result:** Pitch snaps to a new random value at each cycle — staircase-style random steps, not continuous.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 7.8 LFO 1 — Waveform keys and preview
**Setup:** Same as 7.2.  
**Steps:**
1. Click each of the six waveform keys in turn.
2. Change Rate, Phase and Depth.

**Expected result:** Exactly six keys (Sine, Triangle, Saw, Square, Random, S&H; the Drawable choice is not offered in v1.0). The LFO preview redraws for each waveform, phase and depth. Hovering a key names the waveform.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 7.9 LFO 1 — Phase offset
**Setup:** LFO 1 and LFO 2 both assigned to Pitch Shift, same rate and amount, Sine waveform.  
**Steps:**
1. Set LFO 1 Phase = 0°, LFO 2 Phase = 180°.

**Expected result:** The two LFOs are in anti-phase. Their effects partially or fully cancel when their amounts are equal. With both modulating different L/R destinations or observed on a scope, the 180° offset is visible/audible.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 7.10 LFO 1 — Sync toggle
**Setup:** Host running at 120BPM. LFO 1 modulating Grain Size.  
**Steps:**
1. Enable LFO Sync on LFO 1.
2. Change host tempo from 120BPM to 90BPM.

**Expected result:** LFO rate tracks the host tempo change. The modulation period changes proportionally when tempo changes.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 7.11 LFO 2 — independent operation
**Setup:** LFO 1 modulating Grain Size (Sine, 0.5Hz). LFO 2 modulating Position (Saw, 2Hz).  
**Steps:**
1. Enable both in the matrix.
2. Listen and observe both modulations running simultaneously.

**Expected result:** Two independent modulations occur concurrently. Grain Size and Position both modulate at their respective rates without interfering with each other.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 7.12 LFO Depth — zero
**Setup:** LFO 1 modulating Pitch Shift, Rate = 1Hz, Depth = 50%.  
**Steps:**
1. Set LFO 1 Depth = 0.

**Expected result:** Pitch Shift modulation ceases. Output is static in pitch.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

## 8. Modulation Matrix

### 8.1 Single slot — LFO1 → Grain Size
**Setup:** All matrix slots cleared (Source = None).  
**Steps:**
1. Set slot 1: Source = LFO1, Dest = Grain Size, Amount = 100%.
2. LFO 1 Rate = 1Hz, Sine.

**Expected result:** Grain Size modulates at 1Hz. Audible cyclic texture change.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 8.2 Single slot — negative amount (invert)
**Setup:** Slot 1: Source = LFO1, Dest = Position, Amount = +50%.  
**Steps:**
1. Change Amount to −50%.

**Expected result:** The modulation direction inverts. When LFO is at its positive peak, Position moves in the opposite direction compared to +50%.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 8.3 All 8 slots active simultaneously
**Setup:** LFO 1 and LFO 2 active with different rates.  
**Steps:**
1. Fill all 8 matrix slots with valid Source/Dest/Amount combinations.
2. Ensure some slots use LFO1 and some use LFO2.
3. Let run for 10 seconds.

**Expected result:** All 8 modulations operate simultaneously without crash, silence, or excessive CPU spike. Output is complex but continuous.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 8.4 Verify all 29 destinations are selectable
**Setup:** Any single matrix slot.  
**Steps:**
1. Click the destination select of any MOD MATRIX row.
2. Count/verify all destination options appear.

**Expected result:** Exactly 29 destinations listed. No blank entries, no duplicates. All parameter names are legible.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 8.5 Clear a slot (Source = None)
**Setup:** Slot 1 active with LFO1 → Pitch Shift.  
**Steps:**
1. Click the lit **L1** key in row 1 so neither L1 nor L2 is lit (Source = None).

**Expected result:** Modulation stops immediately. Pitch Shift returns to its static value.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

## 9. Snapshots

### 9.1 Save to Snapshot A
**Setup:** Default state + sample loaded.  
**Steps:**
1. Set Grain Size = 50ms, Density = 80 g/s, Pitch Shift = +7st.
2. Click the empty Snapshot **A** key (or Ctrl/Cmd-click it, or right-click → **Store current**).

**Expected result:** A is lit white and reads LIVE. The command line reports the store. After storing B, A reads STORED (amber letter).  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 9.2 Recall Snapshot A
**Setup:** Snapshot A saved (test 9.1). Change parameters to something different.  
**Steps:**
1. Adjust Grain Size = 200ms, Density = 5 g/s, Pitch Shift = 0st.
2. Click Snapshot **A** to recall.

**Expected result:** All parameters snap back to the values saved in step 9.1: Grain Size = 50ms, Density = 80 g/s, Pitch Shift = +7st. A is lit white (LIVE).  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 9.3 Save and recall all four snapshots (A/B/C/D)
**Setup:** Default state.  
**Steps:**
1. Set a unique, easily recognizable parameter combination and save to **A**.
2. Set a different combination and save to **B**.
3. Set another and save to **C**.
4. Set another and save to **D**.
5. Recall each snapshot in order: D → B → A → C.

**Expected result:** Each recall loads the correct unique parameter set saved to that slot. No cross-contamination between slots.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 9.4 Snapshots persist across plugin close/reopen
**Setup:** Snapshot A saved with identifiable settings (test 9.1). Host project saved.  
**Steps:**
1. Save the DAW project.
2. Close and reopen the DAW (or close and reopen the plugin window).
3. Recall Snapshot A.

**Expected result:** Snapshot A retains the saved parameter values after reload.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

## 10. Sub-Grain

### 10.1 Sub-grain Depth 0 (off)
**Setup:** Default state + sample loaded. Density = 10 g/s.  
**Steps:**
1. Set Sub-grain Depth = 0.
2. Listen and note the texture.

**Expected result:** Standard granular output. No recursive grain spawning.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 10.2 Sub-grain Depth 1
**Setup:** Same as 10.1.  
**Steps:**
1. Set Sub-grain Depth = 1.

**Expected result:** Each primary grain spawns one child grain. Output becomes denser and more complex than at depth 0. Audible increase in grain density without changing Density knob.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 10.3 Sub-grain Depth 2
**Setup:** Same as 10.1.  
**Steps:**
1. Set Sub-grain Depth = 2.

**Expected result:** Two levels of recursive grain spawning. Further density increase over depth 1. Output is significantly more complex. CPU usage may rise noticeably.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

## 11. Stochastic Distribution

> All tests: sample loaded, host playing, Density = 10 g/s, all modules bypassed.

### 11.1 Uniform distribution
**Setup:** Default state.  
**Steps:**
1. Set Stochastic Distribution = **Uniform**.
2. Listen to inter-grain timing.

**Expected result:** Even, regular grain spacing. Metronomic feel.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 11.2 Gaussian distribution
**Steps:**
1. Set Stochastic Distribution = **Gaussian**.

**Expected result:** Grain timing clusters around the mean. Subtle timing variation — slightly uneven but centered.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 11.3 Poisson distribution
**Steps:**
1. Set Stochastic Distribution = **Poisson**.

**Expected result:** Grain timing has occasional bursts and gaps. Statistically random but with memoryless inter-arrival times.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 11.4 Exponential distribution
**Steps:**
1. Set Stochastic Distribution = **Exponential**.

**Expected result:** Timing skewed toward short inter-grain intervals with rare long gaps. Output sounds front-loaded.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 11.5 Pareto distribution
**Steps:**
1. Set Stochastic Distribution = **Pareto**.

**Expected result:** Timing has heavy tail — most grains arrive quickly, but occasional very long silences occur.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 11.6 1/f (Pink noise) distribution
**Steps:**
1. Set Stochastic Distribution = **1/f**.

**Expected result:** Grain timing exhibits self-similarity and long-range correlation. Feels more "organic" than uniform — neither fully random nor periodic.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 11.7 Distribution change causes no dropout
**Steps:**
1. Rapidly cycle through all 6 distributions while audio is running.

**Expected result:** No audio dropout, glitch, or crash when switching distributions in real time.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

## 12. Feedback Path

### 12.1 Feedback toggle — on
**Setup:** Default state + sample loaded. Grain Size = 150ms, Density = 10 g/s.  
**Steps:**
1. Enable Feedback.
2. Set Feedback Gain = 0.5.

**Expected result:** Audible feedback loop develops. Output becomes richer/thicker due to recycled signal feeding back into the granular engine.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 12.2 Feedback Gain sweep
**Setup:** Feedback enabled.  
**Steps:**
1. Slowly raise Feedback Gain from 0 to 0.95.

**Expected result:** Feedback intensity increases. At high gain (0.9–0.95), a buildup of recycled signal produces a dense, echoing texture. Must not self-oscillate into an uncontrolled blow-up.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 12.3 Feedback Gain at maximum (0.95) is stable
**Setup:** Feedback enabled, Feedback Gain = 0.95.  
**Steps:**
1. Run for 30 seconds without touching any controls.

**Expected result:** Output remains bounded — no exponential volume blowup. May be dense and loud, but stable.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 12.4 Feedback Damp sweep
**Setup:** Feedback enabled, Feedback Gain = 0.7.  
**Steps:**
1. Sweep Feedback Damp from 0 to 100%.

**Expected result:** At 0%: bright, full-spectrum feedback. At 100%: high-frequency content in the feedback loop is attenuated — warmer, darker feedback tail.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 12.5 Feedback toggle — off
**Setup:** Feedback enabled, audible feedback running.  
**Steps:**
1. Disable Feedback toggle.

**Expected result:** Feedback signal disappears. Output returns to clean dry granular output within a short decay.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

## 13. Spectral Processor

### 13.1 Spectral toggle — on (Freeze mode)
**Setup:** Default state + sample loaded. Position sweeping slowly.  
**Steps:**
1. Enable Spectral.
2. Set Mode = **Freeze**.

**Expected result:** Output freezes spectrally — a sustained, static pad-like tone derived from the spectral snapshot at the moment of freeze. Position changes have reduced/no effect during freeze.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 13.2 Spectral toggle — off
**Setup:** Spectral enabled and audible (test 13.1).  
**Steps:**
1. Disable Spectral toggle.

**Expected result:** Output returns to normal granular processing. Frozen spectral content disappears.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 13.3 Spectral Mode — Blur
**Setup:** Spectral enabled.  
**Steps:**
1. Set Mode = **Blur**.
2. Set Blur Amount = 50%.

**Expected result:** Output has a smeared, diffuse quality. Transients and fine spectral detail are softened/blended.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 13.4 Blur Amount — sweep
**Setup:** Spectral enabled, Mode = Blur.  
**Steps:**
1. Sweep Blur Amount from 0 to 100%.

**Expected result:** At 0%: minimal blur (near-normal output). At 100%: maximum spectral smearing — highly diffuse, resonant texture.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 13.5 Spectral reports no latency
**Setup:** DAW with latency compensation visible (check plug-in delay compensation readout).  
**Steps:**
1. Enable Spectral.
2. Read the reported plugin latency in the DAW.

**Expected result:** DAW reports 0 samples. (Spectral is an offline transform of the source; it adds no audio-path delay. beta.1/beta.2 reported 2048 samples, which shifted the track 42 ms early.)  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 13.6 Spectral latency cleared when disabled
**Setup:** Spectral was enabled (test 13.5).  
**Steps:**
1. Disable Spectral.
2. Check reported latency.

**Expected result:** Reported latency stays 0 samples.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

## 14. Monitor

### 14.1 Waveform renders after sample load
**Setup:** No sample loaded.  
**Steps:**
1. Load a WAV file (any length).

**Expected result:** Monitor renders the sample amplitude envelope. Shape is recognizable and matches the audio file's content.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 14.2 Grain particle visualization
**Setup:** Sample loaded. Density = 30 g/s. Host playing.  
**Steps:**
1. Observe the monitor.

**Expected result:** Grain dots appear on the waveform at the current grain read positions, with a short read trace. The grain count under the waveform and in the status strip updates in real time.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 14.3 Particle density matches Density fader
**Setup:** Sample loaded. Host playing.  
**Steps:**
1. Set Density = 5 g/s. Observe particle count.
2. Set Density = 80 g/s. Observe particle count.

**Expected result:** Visibly more particles at 80 g/s than at 5 g/s. Particle activity scales with the Density parameter.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 14.4 Position Jitter scatters particle positions
**Setup:** Sample loaded. Host playing. Position = 50%.  
**Steps:**
1. Set Position Jitter = 0%. Observe particle positions.
2. Set Position Jitter = 100%. Observe.

**Expected result:** At 0%: particles cluster tightly around the read head. At 100%: the jitter span covers the full width and particles scatter across it.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

## 15. State Persistence

### 15.1 Plugin state saves with DAW project
**Setup:** Plugin loaded with a sample and non-default parameters.  
**Steps:**
1. Set distinctive parameters (e.g., Pitch Shift = +12st, Density = 80, Grain Size = 30ms, MOTION enabled with Wow Depth = 70%).
2. Save the DAW project.
3. Close the project.
4. Reopen the project.

**Expected result:** All parameter values are restored exactly. All loaded slots reload (filenames in the slot strips, waveform in the monitor). Module On states and snapshots are correct. Output sounds identical to before saving.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 15.2 Plugin state restores in a new DAW instance
**Setup:** DAW project saved with Grano state (test 15.1).  
**Steps:**
1. Fully close the DAW (not just the project).
2. Reopen the DAW.
3. Open the saved project.
4. Play the track.

**Expected result:** Grano initializes with the saved state. No manual reconfiguration needed. Output matches the saved session.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 15.3 Multiple plugin instances maintain independent state
**Setup:** Two Grano instances on two separate tracks in the same project.  
**Steps:**
1. Configure instance A with Grain Size = 10ms, Pitch = +12st.
2. Configure instance B with Grain Size = 400ms, Pitch = −12st.
3. Save project, close, reopen.

**Expected result:** Each instance restores its own independent state. Instance A and B remain distinct.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

## 16. Performance & Stability

### 16.1 CPU at nominal load (200 grains, all modules on)
**Setup:** Sample loaded. Density = 100 g/s (param max), Grain Size = 50ms, MOTION + COLOR + PATTERN enabled, Feedback on (Gain = 0.5), Spectral off. Sub-grain Depth = 0.  
**Steps:**
1. Run for 60 seconds.
2. Monitor DAW CPU meter or system CPU usage.

**Expected result:** CPU usage ≤ 15% on a modern (2020-era) multi-core CPU. No xruns or audio dropouts.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`  
**Observed CPU:** ____%

---

### 16.2 CPU at worst case (1000 grains, Sub-grain Depth 2, Feedback on, Spectral on)
**Setup:** Sample loaded. Maximum Density, Sub-grain Depth = 2, Feedback on (Gain = 0.5), Spectral on. All modules enabled.  
**Steps:**
1. Run for 30 seconds.
2. Monitor CPU.

**Expected result:** CPU usage ≤ 40% on a modern (2020-era) multi-core CPU. Plugin remains stable — no crash, no audio blowup.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`  
**Observed CPU:** ____%

---

### 16.3 No audio dropout during rapid parameter automation
**Setup:** Sample loaded. Automate Grain Size, Density, and Pitch Shift simultaneously with fast LFOs.  
**Steps:**
1. Assign LFO 1 (8Hz) → Grain Size and LFO 2 (5Hz) → Pitch Shift.
2. Automate Density in the DAW from 1 to 100 g/s over 4 bars.
3. Run for 16 bars.

**Expected result:** Continuous audio output. No xruns, pops, or dropouts. CPU stays within bounds.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 16.4 Memory usage with 4 samples loaded (MultiSampleBank)
**Setup:** Load 4 large samples (> 30MB each), one per slot strip.  
**Steps:**
1. Load four distinct large samples into all four bank slots.
2. Monitor RAM usage via OS task manager or DAW diagnostics.

**Expected result:** Total plugin resident memory ≤ 200MB with all slots filled.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`  
**Observed RAM:** ____MB

---

### 16.5 No crash on rapid bypass toggling
**Setup:** Sample loaded, all three modules (MOTION, COLOR, PATTERN) enabled.  
**Steps:**
1. Rapidly toggle the On keys for MOTION, COLOR, and PATTERN in quick succession for 10 seconds.

**Expected result:** No crash or audio blowup. Bypass toggles cleanly with at most a brief audio artifact on the toggle edge.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 16.6 Silence on muted track
**Setup:** Grano on a track. Track muted in the DAW.  
**Steps:**
1. Mute the Grano track.
2. Listen to the master output.

**Expected result:** No audio from the Grano track passes through to the master. Complete silence from this track.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 16.7 Test-tone fallback (no usable sample)
**Setup:** Fresh plugin instance, no sample loaded. Spectral and Feedback off.  
**Steps:**
1. Run the host transport and listen.
2. Load a sample into slot 1. Listen.
3. Drag slot 1's weight fader to 0 (all slot weights 0). Listen.
4. In the monitor's **No Source** selector, click **Silence**. Listen. Then click **Test tone**.
5. Save and reopen the project.
6. Ctrl/Cmd-click snapshot **A** to store. Set No Source to **Silence**, then click **A** to recall.
7. Set PATTERN Trigger to **Sync** and stop the host transport. Move slot 1's weight between 0 and above 0 a few times, watching the monitor.
8. Repeat step 7 with Trigger **Euclid**.
9. Set Trigger back to **Free**. Clear slot 1 (no sample loaded) and set No Source to **Silence**. Turn **Feedback** on, then off. Turn **Spectral** on, then off.

**Expected result:** Steps 1 and 3: grains of a 440 Hz sine (the built-in test tone), meter moving; the No Source selector (Test tone lit) replaces the grain count. Step 2: the sample plays, the selector disappears. Step 4: Silence = no output, meter idle; Test tone brings the sine back. Step 5: the chosen mode is restored. Step 6: Silence stays lit after the recall (snapshots do not change No Source). Steps 7–8: with nothing playing, the selector appears the moment the weight reaches 0 and disappears the moment it rises above 0 (no stale state while no grains fire). Step 9: turning Feedback or Spectral on hides the selector immediately, because that source replaces the slots even when it holds no audio (output stays silent: there is nothing to feed back or analyse, and no test tone in either No Source mode); turning it off brings the selector back. The setting does not appear as an automation lane in the host.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 16.8 Stability over extended run (soak test)
**Setup:** Sample loaded, moderate settings (Density = 30, Grain Size = 100ms, MOTION + COLOR on, Feedback Gain = 0.3, Spectral off).  
**Steps:**
1. Start host playback.
2. Leave running for 30 minutes without touching controls.

**Expected result:** Plugin remains stable throughout. No crash, no memory leak (check RAM at start vs. end), no progressive audio degradation.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`  
**RAM at start:** ____MB  **RAM at end:** ____MB

---

## 17. Editor (console)

> The editor was redesigned in beta.3. See README "The console" for the layout.

### 17.1 Window scaling
**Setup:** Plugin open in a DAW, then in Standalone.  
**Steps:**
1. Drag the window corner to the smallest size, then the largest.
2. Close and reopen the editor.

**Expected result:** Window scales 0.75×–1.6× keeping its aspect ratio. Text stays sharp and legible at the minimum size; nothing overlaps or clips. Size is consistent after reopen.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 17.2 Every control readable and resettable
**Setup:** Default state.  
**Steps:**
1. Hover several controls in each bank.
2. Drag a GRAIN fader, then hold Ctrl/Cmd and drag again.
3. Double-click the control.

**Expected result:** Each control shows name + value + unit on its strip, and a tooltip plus a status-strip hint on hover. Ctrl/Cmd-drag moves ~10× finer. Double-click resets to default. The command line shows the full name, previous value and default.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 17.3 Keyboard and screen reader
**Setup:** Default state. OS screen reader optional.  
**Steps:**
1. Press Tab repeatedly.
2. Use the arrow keys on a focused control.

**Expected result:** Focus moves through the controls; arrows step the focused value. With a screen reader, each control announces its name and value text.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 17.4 Pattern contextual controls
**Setup:** PATTERN on.  
**Steps:**
1. Select Free, Sync, Euclid, Audio in turn.

**Expected result:** Free: info line, no Div. Sync: Div select + info line. Euclid: Div, ring, Pulses / Steps / Rotation; the ring updates as you edit them. Audio: Sensitivity encoder. Probability / Reverse / Spray and Scale are always present.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 17.5 Slots: show, weight, clear
**Setup:** Samples loaded in slots 1 and 2.  
**Steps:**
1. Click slot number 2, then 1.
2. Drag slot 2's weight fader.
3. Click slot 2's clear (×) key.

**Expected result:** The lit number and monitor follow the selection. The weight changes and is automatable. Clear empties the strip ("click to load", weight dimmed) and the command line reports it. The status strip counts loaded slots.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 17.6 Snapshot clear and live state
**Setup:** Snapshots A and B stored.  
**Steps:**
1. Right-click B → **Clear**.
2. Change a parameter, then recall A.

**Expected result:** B reads EMPTY. A lights white (LIVE) after recall. The right-click menu offers Store current / Recall / Clear.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 17.7 Master meter and clip LED
**Setup:** Sample loaded, host playing.  
**Steps:**
1. Watch the MASTER meter.
2. Raise Master Volume to +6 dB with dense settings until the output clips.

**Expected result:** The L/R meter follows the output level. The dB readout matches the fader. When the output exceeds 0 dBFS the clip LED lights and holds ~1.5 s.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 17.8 Modulation marks
**Setup:** LFO 1 Depth 70%.  
**Steps:**
1. Route LFO 1 → Grain Size at +50% in matrix row 1.
2. Route LFO 2 → Saturate in row 2.

**Expected result:** Grain Size's strip gets a blue marker and its fader a range bracket; Saturate's encoder gets a blue arc. The marks disappear when the routes are cleared.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 17.9 Editor survives reopen and project reload
**Setup:** Samples in slots 1–3, non-default values, snapshot A stored.  
**Steps:**
1. Close and reopen the editor.
2. Save, close and reopen the project.

**Expected result:** Waveform, slot filenames, weights, values, the live snapshot and module On states all reappear as they were.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

### 17.10 Drag and drop (Windows Explorer)
**Setup:** Windows, Grano in a DAW and in Standalone.  
**Steps:**
1. Drag a WAV from Explorer over the window and drop it.

**Expected result:** Copy cursor and amber outline while dragging; the file loads into the slot shown in the monitor.  
**Win (VST3/DAW):** `[ ]` &nbsp;&nbsp; **Win (Standalone):** `[ ]`  
**macOS (VST3):** `[ ]` &nbsp;&nbsp; **macOS (AU):** `[ ]` &nbsp;&nbsp; **macOS (Standalone):** `[ ]`

---

## Sign-off

| Platform | Tester | Date | Result |
|----------|--------|------|--------|
| Windows (VST3 + Standalone) | | | ○ Pending |
| macOS (VST3 + AU + Standalone) | | | ○ Pending |

**Release gate:** Both rows must read ✅ Pass before v1.0.0 final is tagged.

---

*End of TESTING.md — v1.3*
