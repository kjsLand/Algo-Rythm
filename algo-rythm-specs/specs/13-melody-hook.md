# SPEC-13: Melody Hook

Milestone: M4
Source: Coding Spec, Section 6.6
Depends on: SPEC-06, SPEC-09

## 1. Purpose

This spec adds an optional melody hook of 2 to 4 bars.
It keeps the idea from `rand_period.py`: a random Fourier series gives the melody contour.
Music rules then put the contour on scale notes and chord tones.

## 2. Scope

In scope:

- The contour function in C++.
- The hook rhythm, the pitch map and the end of the hook.
- The hook track in render and export.

Out of scope:

- Vocal melodies and lyrics.
- Changes to the Python sandbox. These are optional (T13-06).

## 3. Requirements

| ID | Requirement |
| --- | --- |
| REQ-13-1 | Each hook note must be in the scale of the key. |
| REQ-13-2 | Each hook note on beat 1 or beat 3 must be a chord tone of the chord in that bar. |
| REQ-13-3 | The hook range must be a major 10th (16 semitones) or less. |
| REQ-13-4 | The last note must be the root or the 3rd of the last chord. |
| REQ-13-5 | A note must not be longer than a dotted quarter note (720 ticks). |
| REQ-13-6 | The hook must be off by default. |

## 4. Tasks

### T13-01: Write the contour function

- **Status:** [ ] Open
- **Prerequisites:** T05-01
- **Files:** `core/include/algo/Contour.h`, `core/src/contour.cpp`, `tests/contour_test.cpp`

**Procedure:**

1. Write the struct `Contour` with `a0` and the lists `a` and `b`.
2. Write `randomContour(n, rng)`. The function chooses each coefficient as an integer from -3 to 3, as in `rand_period.py`.
3. Choose `n` from 3 to 5.
4. Write `evaluate(contour, t)`. The function uses t in the range 0 to 1 and maps it to 0 to 2π.
5. If all coefficients except `a0` are 0, choose the coefficients again.

**Acceptance:**

- [ ] For the same seed, the C++ contour gives the same coefficients on all 3 CI systems.
- [ ] No contour is a flat line.

### T13-02: Write the hook rhythm

- **Status:** [ ] Open
- **Prerequisites:** T04-06, T13-01
- **Files:** `core/include/algo/Hook.h`, `core/src/hook.cpp`

**Procedure:**

1. Choose a hook length of 2, 3 or 4 bars.
2. For each bar, choose a hit count from 5 to 9.
3. Make a 16-step Euclidean pattern for each bar.
4. Rotate each pattern by a random value from 0 to 3.
5. Convert each hit to a start tick. One step is 120 ticks.

**Acceptance:**

- [ ] Each bar has 5 to 9 note starts.

### T13-03: Write the pitch map

- **Status:** [ ] Open
- **Prerequisites:** T13-02, T04-02, T06-08
- **Files:** `core/src/hook.cpp`

**Procedure:**

1. Evaluate the contour at each note start.
2. Normalize the values to the range 0 to 1 across the hook.
3. Find the 5th of the scale in octave 4.
4. Put the range center there.
5. Map each value to a MIDI note in a range of 16 semitones around the center.
6. If the note is on beat 1 or beat 3, move it to the nearest chord tone.
7. If the note is on another beat, move it to the nearest scale note.
8. If a note is now outside the range, move it to the nearest valid note inside the range.

**Acceptance:**

- [ ] REQ-13-1, REQ-13-2 and REQ-13-3 are true over 10,000 seeds.

### T13-04: Write the end of the hook and the note lengths

- **Status:** [ ] Open
- **Prerequisites:** T13-03
- **Files:** `core/src/hook.cpp`, `tests/hook_property_test.cpp`

**Procedure:**

1. Move the last note to the nearest root or 3rd of the last chord.
2. Set each note length to the time until the next note start.
3. If a length is more than 720 ticks, set it to 720 ticks.
4. Set the length of the last note to 480 ticks.
5. Set the velocity to 90 on beats 1 and 3 and to 75 on other positions.
6. Write `makeHook(plan, progressions, rngFactory)`. The function returns the hook for the chorus, or the first section if no chorus exists.

**Acceptance:**

- [ ] REQ-13-4 and REQ-13-5 are true over 10,000 seeds.

### T13-05: Add the hook to the engine, render and export

- **Status:** [ ] Open
- **Prerequisites:** T13-04, T09-07
- **Files:** `core/src/engine.cpp`, `core/src/render.cpp`, `core/src/tab.cpp`

**Procedure:**

1. Add the setting `hook` with the default value false.
2. If `hook` is true, call `makeHook` in `generate`.
3. Render the hook on track 2 with MIDI program 1 (piano) so that it is easy to hear.
4. Write the hook as single-note tab below the chord tab of its section.
5. Add a "Hook" check box to the controls panel.

**Acceptance:**

- [ ] REQ-13-6 is true.
- [ ] With the hook on, the MIDI file has a hook track with 10 or more notes.

### T13-06: Update the Python sandbox (optional)

- **Status:** [ ] Open
- **Prerequisites:** T13-04, T01-08
- **Files:** `sandbox/hook.py`

**Procedure:**

1. Write `sandbox/hook.py` with the same steps as T13-02 to T13-04.
2. Use `pretty_midi` to write the hook to a MIDI file.
3. Use the sandbox to test new contour ideas before you change the C++ code.

**Acceptance:**

- [ ] `python sandbox/hook.py --seed 1` writes a MIDI file that obeys REQ-13-1 to REQ-13-5.

## 5. Done criteria

- [ ] All tasks in this spec are Done.
- [ ] The "Hook" property test from Coding Spec Section 9.2 passes.
