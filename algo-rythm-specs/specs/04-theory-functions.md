# SPEC-04: Theory Functions and Fretboard Model

Milestone: M1
Source: Coding Spec, Section 5, Section 6.3 (step 1) and Section 9.1
Depends on: SPEC-03

## 1. Purpose

This spec adds the music theory functions that all generator stages use.
It moves the fretboard model from the app to the core.
It moves the Bjorklund (Euclidean rhythm) function to the core.

## 2. Scope

In scope:

- Note names, pitch classes and key-aware spelling.
- Scale degrees and the diatonic chord builder.
- Nashville number conversion.
- The fretboard model with tuning and capo.
- The Bjorklund function and pattern rotation.

Out of scope:

- Voicing search. Refer to SPEC-07.
- Strum generation. Refer to SPEC-08.

## 3. Requirements

| ID | Requirement |
| --- | --- |
| REQ-04-1 | The note-name parser must accept sharps, flats, lower case and an optional octave number. |
| REQ-04-2 | Note-name output must use flats in flat keys and sharps in sharp keys. |
| REQ-04-3 | The diatonic chord builder must give correct chords for every 7-note scale in the scale registry. |
| REQ-04-4 | Nashville conversion must support accidentals (b, #), chord qualities and slash bass notes. |
| REQ-04-5 | The fretboard model must support any 6-string tuning and a capo from 0 to 7. |
| REQ-04-6 | The core fretboard model must not print text. |

## 4. Tasks

### T04-01: Write the pitch and note-name functions

- **Status:** [x] Done
- **Prerequisites:** T03-02
- **Files:** `core/include/algo/Pitch.h`, `core/src/pitch.cpp`, `tests/pitch_test.cpp`

**Procedure:**

1. Write `parsePitchClass(name)`. The function accepts C, c, C#, Db, Cb, B#, E# and Fb.
2. Write `parseMidiNote(name)`. The function accepts an octave number after the name, for example "C4" is 60.
3. If the name is not valid, throw `std::invalid_argument` with the name.
4. Write `usesFlats(key)`. The function returns true for F, Bb, Eb, Ab, Db and Gb major and their relative minors.
5. Write `pitchClassName(pc, key)`. The function uses flats if `usesFlats(key)` is true.
6. Write `midiNoteName(note, key)`. The function adds the octave number. MIDI note 60 is C4.

**Acceptance:**

- [x] `parsePitchClass("Cb") == 11` and `parsePitchClass("B#") == 0`.
- [x] `pitchClassName(10, Key{5, "Major"})` returns "Bb".
- [x] `pitchClassName(10, Key{7, "Major"})` returns "A#".

### T04-02: Write the scale functions

- **Status:** [x] Done
- **Prerequisites:** T03-04, T04-01
- **Files:** `core/include/algo/Scale.h`, `core/src/scale.cpp`, `tests/scale_test.cpp`

**Procedure:**

1. Write `scalePitchClasses(key)`. The function returns the pitch classes in degree order. The first pitch class is the root.
2. Write `scaleDegreeOf(pc, key)`. The function returns the degree 1 to 7, or no value if the note is not in the scale.
3. Write `isInScale(pc, key)`.
4. Write `nearestScaleNote(midiNote, key)`. If two notes have the same distance, return the lower note.

**Acceptance:**

- [x] C major gives {0, 2, 4, 5, 7, 9, 11}.
- [x] A Minor Pentatonic gives {9, 0, 2, 4, 7}.
- [x] A test checks all 15 scales in all 12 keys.

### T04-03: Write the diatonic chord builder

- **Status:** [x] Done
- **Prerequisites:** T04-02, T03-05
- **Files:** `core/include/algo/Diatonic.h`, `core/src/diatonic.cpp`, `tests/diatonic_test.cpp`

**Procedure:**

1. Write `diatonicTriad(key, degree)`. The function uses the scale notes at degree d, d+2 and d+4.
2. Write `diatonicSeventh(key, degree)`. Also use the scale note at degree d+6.
3. Calculate the intervals above the root.
4. Find the chord type in the registry that has the same intervals.
5. If the scale does not have 7 notes, throw `std::invalid_argument`.
6. If no chord type has the same intervals, throw `DataError` with the intervals.

**Acceptance:**

- [x] Major gives Major, Minor, Minor, Major, Major, Minor, Dim.
- [x] Major 7th chords give Maj7, M7, M7, Maj7, Dom7, M7, M7b5.
- [x] In Dorian, degree 2 is Minor and degree 4 is Major.
- [x] A test builds triads and 7th chords for all 9 seven-note scales with no error.

### T04-04: Write the Nashville number conversion

- **Status:** [x] Done
- **Prerequisites:** T04-03
- **Files:** `core/include/algo/Nashville.h`, `core/src/nashville.cpp`, `tests/nashville_test.cpp`

**Procedure:**

1. Write `parseNumeral(text)`. The function accepts "1", "6m", "b7", "4maj7", "5/7" and "#4dim".
2. If the text has no quality, mark the quality as "diatonic".
3. Write `resolve(symbol, key)`. The function replaces a "diatonic" quality with the result of T04-03.
4. Write `chordRootPc(symbol, key)`. Apply the accidental to the scale degree.
5. Write `chordName(symbol, key)`. The function returns text such as "Em", "Bb" or "D/F#".
6. Write `numeralText(symbol)`. The function returns text such as "6m" or "b7".

**Acceptance:**

- [x] "b7" in C major gives the name "Bb".
- [x] "5/7" in G major gives the name "D/F#".
- [x] For all 12 major keys, "1 5 6 4" gives the correct 4 chord names.

### T04-05: Move the fretboard model to the core

- **Status:** [x] Done
- **Prerequisites:** T04-01
- **Files:** `core/include/algo/Fretboard.h`, `core/src/fretboard.cpp`, `tools/fretboard_print.cpp`, `tests/fretboard_test.cpp`

**Procedure:**

1. Move the grid and position functions from `plugin/Source/Fretboard.*` to the core.
2. Add a constructor that takes a `Tuning` and a capo fret.
3. Make `midiNoteAt(string, fret)` add the capo value.
4. Make `findNote(midi)` return only positions at or above the capo.
5. Remove the random note picker. SPEC-06 replaces it.
6. Move `printScale`, `printGrid` and `printChords` to `tools/fretboard_print.cpp`.

**Acceptance:**

- [x] With capo 2, fret 0 of string 0 gives MIDI note 42.
- [x] With drop D tuning, fret 0 of string 0 gives MIDI note 38.
- [x] No file in `core/` includes `<iostream>`.

### T04-06: Move the Bjorklund function to the core

- **Status:** [x] Done
- **Prerequisites:** T03-01
- **Files:** `core/include/algo/Euclid.h`, `core/src/euclid.cpp`, `tests/euclid_test.cpp`

**Procedure:**

1. Move `bjorklund(hits, steps)` from `plugin/Source/Bjorklund.h` to the core.
2. Write `rotate(pattern, n)`. A positive `n` moves each slot to the right.
3. Write `hitCount(pattern)`.
4. Update the app and `tools/bjorklund_demo.cpp` to include the core header.
5. Delete `plugin/Source/Bjorklund.h`.

**Acceptance:**

- [x] E(3,8) gives `X..X..X.` and E(5,8) gives `X.XX.XX.`.
- [x] For all values 0 ≤ k ≤ n ≤ 32, `hitCount(bjorklund(k, n)) == k`.
- [x] `rotate` does not change the hit count.

## 5. Done criteria

- [x] All tasks in this spec are Done.
- [ ] All unit tests from Coding Spec Section 9.1 pass.
- [x] The app still compiles and shows the fretboard.
