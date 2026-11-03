# SPEC-06: Song Plan and Chord Progressions

Milestone: M2
Source: Coding Spec, Section 6.2 and Section 6.3
Depends on: SPEC-04, SPEC-05

## 1. Purpose

This spec writes the first two generator stages.
Stage 1 (Plan) selects the key, scale, tempo and form.
Stage 2 (Chords) makes a chord progression for each section.

## 2. Scope

In scope:

- The functions `planSong` and `makeChords`.
- The transition data file `transitions.json`.
- Section rules, colour chords and harmonic rhythm.

Out of scope:

- Voicings. Refer to SPEC-07.
- Learned transition values. Refer to SPEC-15.

## 3. Requirements

| ID | Requirement |
| --- | --- |
| REQ-06-1 | If the key is "any", the key choice must use the weights for the difficulty. |
| REQ-06-2 | Each section must have a loop of 4 chords. |
| REQ-06-3 | The chorus progression must not be the same as the verse progression. |
| REQ-06-4 | Each loop must end on a chord that leads back to its first chord. |
| REQ-06-5 | A chord must be diatonic, unless it has the tag "borrowed" or "secondary". |
| REQ-06-6 | Beginner progressions must contain only major and minor triads. |
| REQ-06-7 | The stage must make a valid result in 50 attempts or fewer. If not, it must use a library progression. |

## 4. Tasks

### T06-01: Write the key and scale choice

- **Status:** [x] Done
- **Prerequisites:** T04-02, T05-03
- **Files:** `core/include/algo/Plan.h`, `core/src/plan.cpp`, `tests/plan_test.cpp`

**Procedure:**

1. Write the struct `SongPlan` with key, BPM, time signature, capo and a list of section names and bar counts.
2. Write the mood table: Bright to Major, Sad to Natural Minor, Hopeful to Dorian, Bluesy to Mixolydian.
3. If the settings give a key, use it.
4. Make a list of Beginner keys: G, C, D, A and E major, and A, E and D minor.
5. If the key is "any" and the difficulty is Beginner, choose from the Beginner keys.
6. If the key is "any" and the difficulty is not Beginner, choose from all 12 roots with equal weight.
7. If the root has no open chord shapes and the difficulty is Beginner, set a capo suggestion.

**Acceptance:**

- [x] Over 10,000 Beginner seeds, each key is one of the 8 permitted keys.
- [x] A key and a scale from the settings never change.

### T06-02: Write the tempo and form choice

- **Status:** [x] Done
- **Prerequisites:** T06-01
- **Files:** `core/src/plan.cpp`, `core/data/forms.json`, `tests/plan_test.cpp`

**Procedure:**

1. Choose the BPM with `uniformInt` in the settings range.
2. Round the BPM to the nearest even number.
3. Keep it in the range.
4. Write `forms.json` with the templates Verse-Chorus, Verse-Chorus-Bridge and Loop.
5. Set the bar counts: Verse 8, Chorus 8, Bridge 4 and Loop 4.
6. Write `planSong(settings, rng)`. The function returns a `SongPlan`.

**Acceptance:**

- [x] The BPM is always even and in the range.
- [x] The template Verse-Chorus-Bridge gives 3 sections with 8, 8 and 4 bars.

### T06-03: Write the transition data file

- **Status:** [x] Done
- **Prerequisites:** T04-04
- **Files:** `core/data/transitions.json`, `core/include/algo/Transitions.h`, `core/src/transitions.cpp`

**Procedure:**

1. Write one table for major keys and one table for minor keys.
2. For each numeral, write the weights of the numerals that can follow it.
3. In the major table, give 5 after 1 the weight 0.31. This value comes from the Hooktheory data.
4. Give the other values by ear.
5. Mark the file with `"source": "hand-authored v1"`.
6. Write the class `TransitionTable` that loads the file and returns the next numeral with `weightedIndex`.
7. Make sure that the weights for each numeral add up to 1.0 within 0.001.

**Acceptance:**

- [x] Each diatonic numeral has a row in each table.
- [x] Data validation fails if a row does not add up to 1.0.

### T06-04: Write the library progression choice

- **Status:** [x] Done
- **Prerequisites:** T03-08, T06-02
- **Files:** `core/include/algo/Chords.h`, `core/src/chords.cpp`

**Procedure:**

1. Write `pickLibraryLoop(plan, sectionRole, difficulty, rng)`.
2. Filter the progression library by mode, role and difficulty.
3. If no progression passes the filter, remove the role filter.
4. Choose one progression with `uniformInt`.

**Acceptance:**

- [x] A Beginner chorus in a major key always gets a progression with the role "chorus".

### T06-05: Write the Markov walk

- **Status:** [x] Done
- **Prerequisites:** T06-03
- **Files:** `core/src/chords.cpp`, `tests/chords_test.cpp`

**Procedure:**

1. Write `walkLoop(plan, sectionRole, startNumeral, rng)`.
2. Start on the start numeral for the section role.
3. Use 1 for a verse, and 4, 6 or 1 for a chorus.
4. Use 4, 6 or 2 for a bridge.
5. Choose the next 3 numerals with the transition table.
6. Do not choose the same numeral two times in sequence.

**Acceptance:**

- [x] Each loop has 4 numerals with no direct repeats.
- [x] Each verse loop starts on 1.

### T06-06: Write the section rule check

- **Status:** [x] Done
- **Prerequisites:** T06-04, T06-05
- **Files:** `core/src/chords.cpp`, `tests/chords_test.cpp`

**Procedure:**

1. Write `checkLoop(loop, sectionRole, otherLoops)`. The function returns a list of rule failures.
2. Make sure that the last chord leads back to the first chord.
3. Permit 5 to 1, 4 to 1, b7 to 1, 4 to 6 and 5 to 6.
4. Make sure that a bridge ends on 5.
5. Make sure that the chorus loop is not the same as the verse loop.
6. Write `makeLoop`. The function uses the library with probability 0.5. Otherwise, it uses the Markov walk.
7. If `checkLoop` fails, try again. After 50 attempts, use a library progression that passes.

**Acceptance:**

- [x] Over 10,000 seeds, each loop passes `checkLoop`.
- [x] A test with a forced failure uses the library after 50 attempts.

### T06-07: Write the colour chord rules

- **Status:** [x] Done
- **Prerequisites:** T06-06
- **Files:** `core/src/chords.cpp`, `core/include/algo/ColourRules.h`, `tests/colour_test.cpp`

**Procedure:**

1. Put the probabilities in a struct `ColourRules`. Do not write them as literal values in the code.
2. For Beginner, make no change.
3. For Intermediate, change up to 2 chords for each section to sus2, sus4 or add9. Use probability 0.25 for each chord.
4. For Intermediate, add one borrowed chord (b7, 4m or b6) with probability 0.15. Tag it "borrowed".
5. For Advanced, change 2, 4 and 5 to 7th chords with probability 0.4.
6. For Advanced, add the secondary dominant of 6 (3 as Dom7) with probability 0.1. Tag it "secondary".

**Acceptance:**

- [x] Over 10,000 Beginner seeds, each chord is a major or minor triad.
- [x] Over 10,000 seeds, each non-diatonic chord has the tag "borrowed" or "secondary".

### T06-08: Write the harmonic rhythm and the stage function

- **Status:** [x] Done
- **Prerequisites:** T06-07
- **Files:** `core/src/chords.cpp`, `tests/chords_test.cpp`

**Procedure:**

1. Write `makeChords(plan, tables, rngFactory)`. The function makes one loop for each section.
2. Use the random stream for the Chords stage and the section index.
3. Fill the bars of a section by repetition of its loop.
4. For Advanced, or if the BPM is less than 80, put 2 chords in each bar.
5. Return a `ProgressionSet` with the chords for each bar.

**Acceptance:**

- [x] An 8-bar verse with 1 chord in each bar plays its 4-chord loop 2 times.
- [x] A reroll of section 1 does not change section 0.

## 5. Done criteria

- [x] All tasks in this spec are Done.
- [ ] The "In key" property test from Coding Spec Section 9.2 passes in CI.
