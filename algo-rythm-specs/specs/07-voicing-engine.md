# SPEC-07: Voicing and Fingering Engine

Milestone: M2
Source: Coding Spec, Section 2 (defect 4) and Section 6.4
Depends on: SPEC-04, SPEC-05

## 1. Purpose

This spec replaces `buildTightVoicing` with a voicing engine that gives only playable chords.
The engine selects a smooth sequence of voicings for each section.
It also assigns a finger to each fretted note.

## 2. Scope

In scope:

- Candidate voicings from chord shapes and from a fret search.
- The hard constraints and the cost function.
- Sequence selection with the Viterbi method and the 5 best sequences.
- Finger assignment and the candidate cache.

Out of scope:

- Display of chord diagrams. Refer to SPEC-11.
- Alternate tunings in the user interface. Refer to SPEC-16.

## 3. Requirements

| ID | Requirement |
| --- | --- |
| REQ-07-1 | Each voicing must have one note or no note on each string. |
| REQ-07-2 | The lowest note must be the chord root or the slash bass note. |
| REQ-07-3 | Each voicing must contain the root, the 3rd and the 7th of the chord, if the chord has them. |
| REQ-07-4 | The fret span of fretted notes must be 3 frets or fewer at Beginner, and 4 frets or fewer at other levels. |
| REQ-07-5 | A voicing must use 4 fingers or fewer. A barre counts as one finger. |
| REQ-07-6 | A strummed voicing must have 4 or more strings that sound. |
| REQ-07-7 | Only an Advanced voicing can have a muted string between two strings that sound. |
| REQ-07-8 | The engine must make all voicings for a 64-bar song starter in 100 ms or less. |

## 4. Tasks

### T07-01: Write the candidates from chord shapes

- **Status:** [ ] Open
- **Prerequisites:** T03-10, T04-05
- **Files:** `core/include/algo/Voicing.h`, `core/src/voicing_shapes.cpp`, `tests/voicing_shapes_test.cpp`

**Procedure:**

1. Write `shapeCandidates(chord, fretboard)`.
2. Select the shapes in `shapes.json` that have the same chord type.
3. For each movable shape, calculate the root fret on the shape's root string.
4. Add the root fret to each relative fret value.
5. If the highest fret is more than 15, reject the candidate.
6. Use an open shape only if its root is the same as the chord root.

**Acceptance:**

- [ ] G Major gives the open G shape and the E-shape barre at fret 3.
- [ ] Each candidate gives the correct pitch-class set.

### T07-02: Write the candidates from the fret search

- **Status:** [ ] Open
- **Prerequisites:** T04-05
- **Files:** `core/src/voicing_search.cpp`, `tests/voicing_search_test.cpp`

**Procedure:**

1. Write `searchCandidates(chord, fretboard, difficulty)`.
2. For each base fret from 0 to 12, make a window from the base fret to the base fret + 3.
3. For each string, list these options: muted, open, or a fret in the window with a chord tone.
4. Make all combinations of the options with a depth-first search.
5. Stop a branch early if it cannot pass a hard constraint.
6. Remove duplicate candidates.

**Acceptance:**

- [ ] The search for C Major includes the open C voicing x32010.
- [ ] The search for one chord takes 5 ms or less in a Release build.

### T07-03: Write the hard constraint filter

- **Status:** [ ] Open
- **Prerequisites:** T07-01, T07-02
- **Files:** `core/src/voicing_constraints.cpp`, `tests/voicing_constraints_test.cpp`

**Procedure:**

1. Write `violations(voicing, chord, difficulty)`. The function returns a list of rule names.
2. Write one check for each requirement from REQ-07-1 to REQ-07-7.
3. For REQ-07-3, use the `optional` intervals from `chords.json`.
4. For REQ-07-5, count a barre when the lowest fret covers 2 or more strings next to each other.
5. Write one test for each rule.
6. Use one voicing that passes and one voicing that fails.

**Acceptance:**

- [ ] Each of the 55 bad voicings from the defect test in Coding Spec Section 2 fails the filter.
- [ ] All shapes in `shapes.json` pass the filter at their own difficulty.

### T07-04: Write the cost function

- **Status:** [ ] Open
- **Prerequisites:** T07-03
- **Files:** `core/include/algo/VoicingCost.h`, `core/src/voicing_cost.cpp`, `tests/voicing_cost_test.cpp`

**Procedure:**

1. Write the struct `CostWeights` with the default values from Coding Spec Section 6.4.
2. Write `staticCost(voicing, difficulty, weights)` for barre, span, finger count, open strings and register.
3. Write `transitionCost(previous, next, weights)` for hand movement and common tones.
4. Define the hand position as the mean fret of the fretted notes. If the voicing has no fretted note, the hand position is 0.
5. Give a penalty if the lowest note is below E2 or above A3.

**Acceptance:**

- [ ] At Beginner, the open C voicing costs less than the C barre at fret 3.
- [ ] Two voicings with 3 common tones have a lower transition cost than two voicings with 0 common tones.

### T07-05: Write the sequence selection

- **Status:** [ ] Open
- **Prerequisites:** T07-04
- **Files:** `core/src/voicing_sequence.cpp`, `tests/voicing_sequence_test.cpp`

**Procedure:**

1. Write `chooseSequence(chords, candidates, weights, k)`.
2. Use the Viterbi method. The state is the candidate index for each chord.
3. Keep the `k` best paths at each step.
4. Set `k` to 5.
5. Return the `k` best sequences in order of total cost.
6. If a chord has no candidate, throw `GenerationError` with the chord name.

**Acceptance:**

- [ ] For G, D, Em, C at Beginner, the best sequence uses only open shapes.
- [ ] The function returns 5 sequences when 5 or more paths exist.
- [ ] A brute-force test on 4 chords gives the same best sequence.

### T07-06: Write the finger assignment

- **Status:** [ ] Open
- **Prerequisites:** T07-03
- **Files:** `core/src/fingering.cpp`, `tests/fingering_test.cpp`

**Procedure:**

1. Write `assignFingers(voicing)`.
2. If the voicing has a barre, give finger 1 to all strings at the barre fret.
3. Give the next free fingers to the other fretted notes in fret order. Without a barre, the first free finger is 1.
4. If two notes have the same fret, give the lower finger number to the lower string.
5. If a note needs a fifth finger, mark the voicing as not valid.

**Acceptance:**

- [ ] The open C voicing x32010 gets the fingers x32010 (finger 3 on string 5, finger 2 on string 4, finger 1 on string 2).
- [ ] The F barre 133211 gets the fingers 134211.

### T07-07: Write the voicing stage function and cache

- **Status:** [ ] Open
- **Prerequisites:** T07-05, T07-06, T06-08
- **Files:** `core/src/voicing_stage.cpp`, `core/include/algo/VoicingCache.h`

**Procedure:**

1. Write `voiceChords(progressions, fretboard, difficulty, rngFactory)`.
2. Get the candidates from T07-01 and T07-02.
3. Filter them with T07-03.
4. Store the candidates in a cache with the key (chord, difficulty, tuning, capo).
5. Run `chooseSequence` for each section.
6. Use the reroll count to select one of the 5 best sequences.
7. Assign the fingers for each voicing.

**Acceptance:**

- [ ] A reroll count of 0 to 4 gives 5 different sequences, if 5 exist.
- [ ] A second call with the same chords does not run the search again.

### T07-08: Remove the old voicing function

- **Status:** [ ] Open
- **Prerequisites:** T07-07, T11-06
- **Files:** `plugin/Source/Fretboard.cpp`, `plugin/Source/Fretboard.h`

**Procedure:**

1. Delete `buildTightVoicing` and the struct `VoicedChord`.
2. Delete the files `plugin/Source/Fretboard.*` if no code uses them.
3. Search the repository for `buildTightVoicing`.
4. Make sure that no result remains.

**Acceptance:**

- [ ] The app compiles and shows voicings from the new engine.

### T07-09: Add the voicing property and speed tests

- **Status:** [ ] Open
- **Prerequisites:** T07-07, T02-05
- **Files:** `tests/voicing_property_test.cpp`, `tests/voicing_bench_test.cpp`

**Procedure:**

1. With `forEachSeed`, generate a progression and its voicings for each difficulty.
2. Make sure that each voicing passes the filter from T07-03.
3. Make sure that the set of pitch classes that sound is the same as the chord tones. The optional intervals can be absent.
4. Write a speed test for a 64-bar Advanced song starter.
5. Run it in Release builds only.

**Acceptance:**

- [ ] The property test passes for 10,000 seeds in CI.
- [ ] The speed test meets REQ-07-8.

## 5. Done criteria

- [ ] All tasks in this spec are Done.
- [ ] The "Playable voicing" and "Correct chord" property tests from Coding Spec Section 9.2 pass.
- [ ] You play 20 random Beginner voicing sequences on guitar with no problem.
