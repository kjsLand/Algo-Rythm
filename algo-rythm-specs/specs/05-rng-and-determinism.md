# SPEC-05: Random Numbers, Settings and Determinism

Milestone: M1
Source: Coding Spec, Section 6.1 and Section 9.2 (the determinism and lock rows)
Depends on: SPEC-03

## 1. Purpose

This spec makes generation deterministic.
The same seed and the same settings must give the same song starter on all operating systems.
A reroll of one item must not change the other items.

## 2. Scope

In scope:

- The `Rng` class and its distribution functions.
- Seed derivation for each stage and section.
- The `GenerationSettings` type and its validation.
- A CI check for banned random functions.
- The cross-platform determinism test.

Out of scope:

- The generator stages. Refer to SPEC-06, SPEC-07, SPEC-08 and SPEC-13.

## 3. Requirements

| ID | Requirement |
| --- | --- |
| REQ-05-1 | The core must use only the `algo::Rng` class for random values. |
| REQ-05-2 | The core must not use `std::random_device`, `std::uniform_int_distribution`, `std::uniform_real_distribution`, `std::shuffle` or `rand()`. |
| REQ-05-3 | Each stage and section must have its own random stream. |
| REQ-05-4 | The same master seed and settings must give byte-identical JSON on macOS, Windows and Ubuntu. |
| REQ-05-5 | Settings that are not valid must cause an error before generation starts. |

## 4. Tasks

### T05-01: Write the Rng class

- **Status:** [ ] Open
- **Prerequisites:** T03-01
- **Files:** `core/include/algo/Rng.h`, `core/src/rng.cpp`, `tests/rng_test.cpp`

**Procedure:**

1. Write the class `Rng` around `std::mt19937_64`. The C++ standard defines the output of this engine.
2. Write `uniformInt(lo, hi)`. The function uses rejection sampling so that each value has the same probability.
3. Write `uniformReal()`. The function returns a value in [0, 1) from the top 53 bits of one engine output.
4. Write `chance(p)`. The function returns true with probability `p`.
5. Write `weightedIndex(weights)`. If all weights are 0, throw `std::invalid_argument`.
6. Write `shuffle(vector)` with the Fisher-Yates method and `uniformInt`.

**Acceptance:**

- [ ] For seed 42, the first 10 values of `uniformInt(0, 99)` match a stored list on all three CI systems.
- [ ] Over 100,000 calls, each value of `uniformInt(0, 9)` occurs 9,500 to 10,500 times.

### T05-02: Write the seed derivation function

- **Status:** [ ] Open
- **Prerequisites:** T05-01
- **Files:** `core/include/algo/Seeds.h`, `core/src/seeds.cpp`, `tests/seeds_test.cpp`

**Procedure:**

1. Write an enum `Stage` with the values Plan, Chords, Voicing, Strum, Hook and Render.
2. Write `deriveSeed(master, stage, sectionIndex, rerollCount)`.
3. Combine the 4 values with the SplitMix64 mix function.
4. Write `rngFor(master, stage, sectionIndex, rerollCount)`. The function returns a new `Rng`.

**Acceptance:**

- [ ] Different values for any one of the 4 inputs give different seeds in a test of 10,000 combinations.
- [ ] `deriveSeed` gives the same value on all three CI systems.

### T05-03: Define the generation settings

- **Status:** [ ] Open
- **Prerequisites:** T03-03
- **Files:** `core/include/algo/Settings.h`, `core/src/settings.cpp`, `tests/settings_test.cpp`

**Procedure:**

1. Write the struct `GenerationSettings` with the inputs from Coding Spec Section 3.
2. Include key or "any", mood, BPM minimum and maximum, form, difficulty, capo, time signature and master seed.
3. Include a `Locks` struct. Refer to SPEC-14 for its fields.
4. Write `validate(settings)`. The function returns a list of error texts.
5. Make sure that the BPM range is 40 to 220 and that the minimum is not more than the maximum.
6. Make sure that the capo is 0 to 7.
7. Add JSON serialization for `GenerationSettings`.

**Acceptance:**

- [ ] `validate` returns an error for BPM 30 to 50.
- [ ] Default settings return no errors.

### T05-04: Add a CI check for banned random functions

- **Status:** [ ] Open
- **Prerequisites:** T05-01, T02-03
- **Files:** `tools/check_banned.py`, `.github/workflows/ci.yml`

**Procedure:**

1. Write a script that searches all files in `core/` for the banned names in REQ-05-2.
2. Ignore `core/src/rng.cpp` for `std::mt19937_64` only.
3. If the script finds a banned name, print the file and line. Exit with code 1.
4. Add the script as a step in the CI workflow.

**Acceptance:**

- [ ] The check fails when you add `std::random_device` to a core file.
- [ ] The check passes on `main`.

### T05-05: Add the cross-platform determinism test

- **Status:** [ ] Open
- **Prerequisites:** T09-07
- **Files:** `tests/golden/determinism/*.json`, `tests/determinism_test.cpp`

**Procedure:**

1. Choose 5 settings that cover the 3 difficulties and 3 form templates.
2. Generate one song starter for each setting with a fixed seed.
3. Save each result as a golden JSON file.
4. Write a test that generates each song starter again.
5. Compare each result to its golden file byte for byte.
6. If a result differs, write the first different line in the failure message.

**Acceptance:**

- [ ] The test passes on all three CI systems.
- [ ] A change to one weight in `transitions.json` causes the test to fail.

### T05-06: Add the reroll isolation test

- **Status:** [ ] Open
- **Prerequisites:** T09-07
- **Files:** `tests/reroll_isolation_test.cpp`

**Procedure:**

1. Generate a song starter with 3 sections.
2. Increase the reroll count for the Chords stage of section 1 only.
3. Generate the song starter again.
4. Make sure that sections 0 and 2 did not change.
5. Make sure that the key, the tempo and the form did not change.

**Acceptance:**

- [ ] The test passes for 1,000 master seeds.

## 5. Done criteria

- [ ] All tasks in this spec are Done.
- [ ] T05-05 proves REQ-05-4 in CI.
