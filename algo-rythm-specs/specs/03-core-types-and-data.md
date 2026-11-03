# SPEC-03: Core Library, Types and Data Files

Milestone: M1
Source: Coding Spec, Section 5 (Data model) and Section 8 (Project layout)
Depends on: SPEC-01, SPEC-02

## 1. Purpose

This spec makes the JUCE-free core library `algo::core`.
It defines the value types for a song starter.
It moves the scale and chord registries into data files.
It adds the progression, strum and shape data files.

## 2. Scope

In scope:

- The `core/` folder, its CMake target and its public headers.
- The value types and their JSON format.
- The registries and the 6 data files.

Out of scope:

- Theory functions. Refer to SPEC-04.
- Random numbers and settings. Refer to SPEC-05.

## 3. Requirements

| ID | Requirement |
| --- | --- |
| REQ-03-1 | The core must not include JUCE headers. |
| REQ-03-2 | The core must not write to `std::cout` or `std::cerr`. |
| REQ-03-3 | Each value type must serialize to JSON and parse from JSON with no data loss. |
| REQ-03-4 | Each JSON document must contain `schemaVersion`. |
| REQ-03-5 | The registries must load from JSON data files. |
| REQ-03-6 | A registry must list its entries in the same order as the data file. |
| REQ-03-7 | The data files must compile into the binary. A command-line option can load a different data folder. |
| REQ-03-8 | If a data file is not valid, the loader must stop with an error that gives the file name and the entry. |

## 4. Tasks

### T03-01: Make the core library target

- **Status:** [ ] Open
- **Prerequisites:** T02-01, T02-06
- **Files:** `core/CMakeLists.txt`, `core/include/algo/Version.h`, `core/src/version.cpp`

**Procedure:**

1. Make the folders `core/include/algo/`, `core/src/` and `core/data/`.
2. Write `core/CMakeLists.txt` with a static library target `algo_core`.
3. Add the alias `algo::core`.
4. Call `algo_set_warnings(algo_core)`.
5. Add `core/include` as a public include folder.
6. Write `Version.h` with a function that returns the version text "0.1.0".
7. Link `algo_tests` to `algo::core`.
8. Add a test for the version function.

**Acceptance:**

- [ ] `algo_core` compiles with no warnings on the three CI systems.
- [x] A CI check fails if any file in `core/` contains `#include <juce`.

### T03-02: Define the value types

- **Status:** [ ] Open
- **Prerequisites:** T03-01
- **Files:** `core/include/algo/Types.h`

**Procedure:**

1. Write the types from Coding Spec Section 5 in namespace `algo`.
2. Include `PitchClass`, `MidiNote`, `Tick`, `Key`, `TimeSig`, `ChordSymbol` and `FretPos`.
3. Include `Voicing`, `Stroke`, `StrumPattern`, `MelodyNote`, `Bar`, `Section`, `Tuning` and `SongStarter`.
4. Add `constexpr int kPpq = 480;`.
5. Give each field a default value.
6. Add `operator==` to each type.

**Acceptance:**

- [x] A default `SongStarter` compiles and has `schemaVersion == 1`.
- [x] Two default objects of each type are equal.

### T03-03: Add JSON serialization for the value types

- **Status:** [ ] Open
- **Prerequisites:** T03-02
- **Files:** `cmake/Dependencies.cmake`, `core/include/algo/Json.h`, `core/src/json.cpp`, `tests/json_test.cpp`

**Procedure:**

1. Get nlohmann/json with `FetchContent`.
2. Pin the nlohmann/json version to a release tag.
3. Write `to_json` and `from_json` for each value type.
4. Write a `Stroke` as one character: 'D', 'U', '-', 'x' or 'c'.
5. Write a `StrumPattern` slot list as one string, for example "D-DU-UDU".
6. Write a muted string in `Voicing.frets` as -1.
7. In `from_json` for `SongStarter`, read `schemaVersion` first.
8. If `schemaVersion` is larger than the current version, stop with an error.

**Acceptance:**

- [x] For each type, JSON to object to JSON gives the same text.
- [ ] The example JSON in Coding Spec Section 5 parses with no error.

### T03-04: Move the scale registry to the core

- **Status:** [x] Done
- **Prerequisites:** T03-03
- **Files:** `core/include/algo/ScaleRegistry.h`, `core/src/scale_registry.cpp`, `core/data/scales.json`, `tests/scale_registry_test.cpp`

**Procedure:**

1. Write `scales.json` with the 15 scales from the current `ScaleRegistry.h`.
2. For each scale, write `name`, `steps` and `tags`. Example tags are "heptatonic" and "pentatonic".
3. Move the `ScaleRegistry` class to the core.
4. Load the scales from JSON.
5. Store the entries in a `std::vector`.
6. Use a separate map from lower-case name to index.
7. Make sure that the sum of the steps for each scale is 12.

**Acceptance:**

- [x] `allNames()` returns the 15 names in file order.
- [x] A scale with steps that do not add up to 12 causes a load error with the scale name.
- [x] Lookup is not case-sensitive.

### T03-05: Move the chord registry to the core and add chord types

- **Status:** [x] Done
- **Prerequisites:** T03-03
- **Files:** `core/include/algo/ChordRegistry.h`, `core/src/chord_registry.cpp`, `core/data/chords.json`, `tests/chord_registry_test.cpp`

**Procedure:**

1. Write `chords.json` with the 12 chord types from the current `ChordRegistry.h`.
2. Add these chord types: 6, m6, 9, m9, add11, 5 (power chord), 7sus4, mMaj7 and Maj7#5.
3. Make sure that mMaj7 and Maj7#5 are present. The harmonic and melodic minor scales need them in T04-03.
4. For each chord, write `name`, `symbol`, `intervals`, `optional` and `tags`.
5. In `optional`, list the intervals that a voicing can omit, for example the 5th.
6. Move the `ChordRegistry` class to the core.
7. Load the chords from JSON.
8. Keep the file order, as in T03-04.

**Acceptance:**

- [x] The registry contains 21 chord types.
- [x] `Dom7` on root 7 (G) gives the pitch classes {7, 11, 2, 5}.
- [x] A chord with a duplicate name causes a load error.

### T03-06: Validate the data files

- **Status:** [x] Done
- **Prerequisites:** T03-04, T03-05
- **Files:** `core/include/algo/DataError.h`, `core/src/data_validation.cpp`, `tests/data_validation_test.cpp`

**Procedure:**

1. Write an exception class `DataError` with the fields `file`, `entry` and `message`.
2. For each data file, check that each required field is present.
3. Check that each field has the correct JSON type.
4. Check that each name is unique in its file.
5. Check that each reference to a scale or chord name exists in its registry.
6. Write a test for each type of error.

**Acceptance:**

- [x] Each error message gives the file name, the entry name and the problem.
- [x] All bundled data files pass validation.

### T03-07: Compile the data files into the binary

- **Status:** [x] Done
- **Prerequisites:** T03-06
- **Files:** `cmake/EmbedData.cmake`, `core/include/algo/Data.h`, `core/src/data.cpp`

**Procedure:**

1. Write a CMake function that converts each file in `core/data/` into a C++ byte array.
2. Write a function `loadBundledData()` that loads all registries from the byte arrays.
3. Write a function `loadDataFolder(path)` that loads all registries from a folder.
4. If a file is not in the folder, use the bundled file.
5. Run the CMake function again when a data file changes.

**Acceptance:**

- [x] A test calls `loadBundledData()` in a folder that has no `core/data/` subfolder. All registries load.
- [x] A change to `chords.json` appears in the next build with no manual step.

### T03-08: Write the progression data file

- **Status:** [ ] Open
- **Prerequisites:** T03-06
- **Files:** `core/data/progressions.json`, `core/include/algo/ProgressionLibrary.h`, `core/src/progression_library.cpp`

**Procedure:**

1. For each progression, write `name`, `numerals`, `mode`, `tags`, `roles` and `difficulty`.
2. Write `numerals` as Nashville numbers with optional accidentals and qualities, for example `["1", "5", "6m", "4"]`.
3. Add the 9 progressions from the learning guide, Section 3.5.
4. Add 11 more common loops so that the file has 20 or more entries.
5. Set `roles` to one or more of "verse", "chorus", "bridge" and "loop".
6. Set `difficulty` to the lowest difficulty that can use the progression.
7. Write a `ProgressionLibrary` class that filters by mode, role, tag and difficulty.

**Acceptance:**

- [x] The file contains 20 or more progressions.
- [x] A filter for "chorus" and "Beginner" in a major key returns 5 or more progressions.

### T03-09: Write the strum data file

- **Status:** [ ] Open
- **Prerequisites:** T03-06
- **Files:** `core/data/strums.json`

**Procedure:**

1. For each pattern, write `name`, `steps`, `slots`, `accents`, `difficulty` and `tags`.
2. Add the 7 patterns from the learning guide, Section 4.3.
3. Add 5 more patterns so that the file has 12 or more entries.
4. Make sure that the length of `slots` is equal to `steps`.
5. Make sure that slot 0 of each pattern is 'D'.

**Acceptance:**

- [x] The file contains 12 or more patterns.
- [x] Each Beginner pattern has 8 steps.

### T03-10: Write the chord shape data file

- **Status:** [x] Done
- **Prerequisites:** T03-06
- **Files:** `core/data/shapes.json`

**Procedure:**

1. For each shape, write `name`, `chord`, `rootString`, `frets`, `fingers`, `barre` and `difficulty`.
2. Write `frets` relative to the root fret.
3. Use "x" for a muted string.
4. Add the open shapes C, A, G, E, D, Am, Em and Dm.
5. Add the open shapes A7, D7, E7, G7, B7, Cadd9, Dsus2 and Dsus4.
6. Add the E-shape and A-shape barres for Major, Minor, Dom7 and M7.
7. Add the power chord shapes on strings 6 and 5.
8. Add major and minor triads on the string sets 1-3, 2-4 and 3-5.
9. Mark open shapes with `"open": true`. These shapes cannot move.

**Acceptance:**

- [x] The file contains 40 or more shapes.
- [x] A test plays each shape at its root fret and gets the correct pitch-class set.

## 5. Done criteria

- [ ] All tasks in this spec are Done.
- [x] The core compiles with no JUCE and no warnings.
- [ ] All data files load and pass validation in CI.
