# SPEC-08: Strum Engine

Milestone: M2
Source: Coding Spec, Section 2 (defect 3) and Section 6.5
Depends on: SPEC-04, SPEC-05

## 1. Purpose

This spec moves the strum generator to the core and extends it.
The engine selects a strum pattern for each section.
It uses the pattern library and the Euclidean method.

## 2. Scope

In scope:

- The function `makeStrums` and the `StrumPattern` value type.
- Grid size, downbeat rules and rest-mask rotation.
- Library patterns, section density, accents, mutes and chucks.

Out of scope:

- Conversion of strokes to MIDI notes. Refer to SPEC-09.
- Fingerpicking patterns and 6/8 time. Refer to SPEC-16.

## 3. Requirements

| ID | Requirement |
| --- | --- |
| REQ-08-1 | Slot 0 of each pattern must be a Down stroke. |
| REQ-08-2 | An even slot must be Down or a silent slot. An odd slot must be Up or a silent slot. |
| REQ-08-3 | The grid must have 8 steps at Beginner and 16 steps at other levels. Above 140 BPM, the grid must have 8 steps. |
| REQ-08-4 | At Beginner, slot `steps/2` (beat 3) must not be a rest. |
| REQ-08-5 | A chorus pattern must have 1 or 2 more hits than the verse pattern, if the grid permits it. |
| REQ-08-6 | Only Advanced patterns can contain Mute or Chuck strokes. |

## 4. Tasks

### T08-01: Move the strum generator to the core

- **Status:** [x] Done
- **Prerequisites:** T04-06, T05-01
- **Files:** `core/include/algo/Strum.h`, `core/src/strum.cpp`, `plugin/Source/StrumPattern.h`

**Procedure:**

1. Move the logic of `StrumPattern::generate` to `core/src/strum.cpp`.
2. Return the core type `algo::StrumPattern` with a `Stroke` value for each slot.
3. Use `algo::Rng`.
4. Remove `std::random_device` and `std::uniform_int_distribution`.
5. Keep the two-layer method: a strum layer and a rest mask.
6. Change the app to call the core function.
7. Delete `plugin/Source/StrumPattern.h`.

**Acceptance:**

- [x] The app shows strum patterns from the core.
- [x] The banned-name check from T05-04 passes.

### T08-02: Write the grid and downbeat rules

- **Status:** [x] Done
- **Prerequisites:** T08-01
- **Files:** `core/src/strum.cpp`, `tests/strum_test.cpp`

**Procedure:**

1. Write `gridSteps(difficulty, bpm)` with the rule in REQ-08-3.
2. Set slot 0 of the strum layer to a hit.
3. Rotate the rest mask until slot 0 is not a rest.
4. At Beginner, continue the rotation until slot `steps/2` is also not a rest.
5. If no rotation meets the rules, remove rests from the mask one at a time.

**Acceptance:**

- [x] Over 10,000 seeds, REQ-08-1 and REQ-08-4 are true.
- [x] The defect test from T01-05 passes with the core function.

### T08-03: Add the library patterns

- **Status:** [x] Done
- **Prerequisites:** T03-09, T08-02
- **Files:** `core/src/strum.cpp`, `core/include/algo/StrumLibrary.h`

**Procedure:**

1. Write the class `StrumLibrary` that loads `strums.json`.
2. Write `pickLibraryPattern(difficulty, steps, targetHits, rng)`. The function filters by difficulty, step count and hit count.
3. In `makeStrums`, use a library pattern with probability 0.5.
4. Otherwise, use the Euclidean method from T08-02.
5. If no library pattern passes the filter, use the Euclidean method.

**Acceptance:**

- [x] Over 10,000 Beginner seeds, 30% or more of the patterns come from the library.

### T08-04: Write the section density and accents

- **Status:** [x] Done
- **Prerequisites:** T08-03
- **Files:** `core/src/strum.cpp`, `tests/strum_test.cpp`

**Procedure:**

1. Choose one base hit count for the song.
2. Use the Strum stream with section index 0 and reroll count 0.
3. For a verse or a bridge, use the base hit count as the target.
4. For a chorus, use the base hit count plus 1 or 2 as the target.
5. Use the target hit count in the Euclidean method and in the library filter.
6. Set the accent of beats 2 and 4 to 1.0.
7. Set the accent of other hits to 0.6.
8. Store the accents in the `accent` field of `StrumPattern`.

**Acceptance:**

- [x] Over 10,000 seeds, REQ-08-5 is true when the grid has space for more hits.
- [x] Each accent value is in the range 0 to 1.

### T08-05: Add the Mute and Chuck strokes

- **Status:** [x] Done
- **Prerequisites:** T08-04
- **Files:** `core/src/strum.cpp`, `tests/strum_test.cpp`

**Procedure:**

1. At Advanced, change each rest on beat 2 or beat 4 to a Chuck with probability 0.3.
2. At Advanced, change each odd-slot hit to a Mute with probability 0.15.
3. Do not change slot 0.
4. Do not change strokes at Beginner or Intermediate.

**Acceptance:**

- [x] Over 10,000 seeds, REQ-08-6 is true.
- [x] Over 10,000 Advanced seeds, 1 or more patterns contain a Chuck.

### T08-06: Write the strum stage function and property tests

- **Status:** [x] Done
- **Prerequisites:** T08-05, T06-02, T02-05
- **Files:** `core/src/strum.cpp`, `tests/strum_property_test.cpp`

**Procedure:**

1. Write `makeStrums(plan, rngFactory)`. The function returns one pattern for each section.
2. Use the random stream for the Strum stage and the section index.
3. Set the field `strings` to 6 for Beginner and to 4 or 6 for other levels.
4. With `forEachSeed`, check REQ-08-1 to REQ-08-6.

**Acceptance:**

- [x] The "Strum" property test from Coding Spec Section 9.2 passes for 10,000 seeds.
- [x] A reroll of the verse strum does not change the chorus strum or the base hit count.

## 5. Done criteria

- [x] All tasks in this spec are Done.
- [x] No generated pattern has a silent beat 1.
