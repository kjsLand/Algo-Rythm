# SPEC-01: Repository Cleanup

Milestone: M0
Source: Coding Spec, Section 2 (Current repo state) and Section 10 (M0)
Depends on: None

## 1. Purpose

This spec makes the current repository build on all three operating systems.
It removes duplicate files and generated files.
It fixes two defects that the new work does not replace.

## 2. Scope

In scope:

- File names, duplicate files and generated files.
- The CMake build and the build scripts.
- The beat-1 strum defect and the note-name search defect.
- The Python sandbox folder.

Out of scope:

- The new `core/` library. Refer to SPEC-03.
- The new strum engine. Refer to SPEC-08.

## 3. Requirements

| ID | Requirement |
| --- | --- |
| REQ-01-1 | The app must compile on macOS, Windows and Ubuntu from one CMake command. |
| REQ-01-2 | Each `#include` must use the exact case of the file name. |
| REQ-01-3 | The repository must not contain build outputs, caches or generated MIDI files. |
| REQ-01-4 | CMake must be the only build path. |
| REQ-01-5 | Slot 0 of each strum pattern must be a Down stroke. |
| REQ-01-6 | A note-name search must return only exact pitch-class matches. |

## 4. Tasks

### T01-01: Rename the registry headers

- **Status:** [x] Done
- **Prerequisites:** None
- **Files:** `plugin/Source/Scaleregistry.h`, `plugin/Source/Chordregistry.h`

**Procedure:**

1. Rename `Scaleregistry.h` to `ScaleRegistry.h` with `git mv`.
2. Rename `Chordregistry.h` to `ChordRegistry.h` with `git mv`.
3. Search all source files for the old names.
4. If you find an old name, change it to the new name.

**Acceptance:**

- [x] `g++ -std=c++17 -fsyntax-only Fretboard.cpp` passes on Ubuntu.
- [x] No file in the repository contains the text "Scaleregistry" or "Chordregistry".

### T01-02: Remove duplicate and stray source files

- **Status:** [x] Done
- **Prerequisites:** None
- **Files:** `plugin/Source/StrumPattern.cpp`, `plugin/Source/CMakeLists.txt`, `plugin/Source/bjorklund.cpp`, `tools/bjorklund_demo.cpp`

**Procedure:**

1. Delete `plugin/Source/StrumPattern.cpp`. The header `StrumPattern.h` already contains all its functions.
2. Delete `plugin/Source/CMakeLists.txt`. The root `plugin/CMakeLists.txt` is the correct file.
3. Make a folder `tools/` at the repository root.
4. Move `bjorklund.cpp` to `tools/bjorklund_demo.cpp` with `git mv`.
5. In the demo file, include `Bjorklund.h` and remove the duplicate `bjorklund()` function.

**Acceptance:**

- [x] The `plugin/Source/` folder contains no file with a `main()` function except `Main.cpp`.
- [x] `tools/bjorklund_demo.cpp` compiles and prints the 8 example rhythms.

### T01-03: Remove generated files and update .gitignore

- **Status:** [x] Done
- **Prerequisites:** None
- **Files:** `piano_melody.mid`, `__pycache__/`, `.gitignore`

**Procedure:**

1. Delete `piano_melody.mid` with `git rm`.
2. Delete the `__pycache__/` folder with `git rm -r`.
3. Add these lines to the root `.gitignore`: `.venv/`, `__pycache__/`, `*.pyc`, `build/`, `out/`, `*.mid`, `.DS_Store`.
4. Add `!tests/golden/*.mid` to `.gitignore` so that golden MIDI files stay in the repository.

**Acceptance:**

- [x] `git status` shows no untracked build output after a full build.
- [x] The repository contains no `.mid` file outside `tests/golden/`.

### T01-04: Make CMake the only build path

- **Status:** [ ] Open
- **Prerequisites:** T01-02
- **Files:** `plugin/JUCEProject.jucer`, `plugin/CMakeLists.txt`, `plugin/run.sh`, `plugin/run.bat`, `plugin/README.md`

**Procedure:**

1. Delete `plugin/JUCEProject.jucer`.
2. In `plugin/CMakeLists.txt`, add all `.h` files in `Source/` to `target_sources`.
3. Replace the text in `run.bat` with Windows batch commands. The current file contains bash syntax.
4. Remove the Projucer section from `plugin/README.md`.
5. Remove the old build steps from `plugin/README.md`.
6. Write the build steps in the README for macOS, Windows and Ubuntu.
7. For Ubuntu, list the JUCE system packages, for example `libasound2-dev`, `libfreetype6-dev` and `libx11-dev`.

**Acceptance:**

- [ ] A clean clone builds on Ubuntu, macOS and Windows with the README steps.
- [x] `run.bat` runs in Windows Command Prompt without errors.

### T01-05: Fix the silent beat-1 strum defect

- **Status:** [x] Done
- **Prerequisites:** T02-01
- **Files:** `plugin/Source/StrumPattern.h`, `tests/strum_regression_test.cpp`

**Procedure:**

1. Write a test that generates patterns for seeds 1 to 2000.
2. In the test, make sure that character 0 of each pattern is 'D'.
3. Run the test.
4. Make sure that it fails before the fix.
5. In `generate()`, rotate `restBits` one slot to the right until `restBits[0]` is 0.
6. Set `strumBits[0]` to 1.
7. Run the test again.

**Acceptance:**

- [x] The test passes for all 2000 seeds.
- [x] The number of rests in each pattern did not change after the rotation.

### T01-06: Fix the note-name search defect

- **Status:** [x] Done
- **Prerequisites:** T02-01
- **Files:** `plugin/Source/Fretboard.cpp`, `tests/fretboard_regression_test.cpp`

**Procedure:**

1. Write a test that calls `findNoteByName("C")`.
2. In the test, make sure that no result has the pitch class of C#.
3. Change `findNoteByName` to convert the name to a pitch class with `nameToPC`.
4. If the name contains an octave number, compare the full MIDI note.
5. If the name contains no octave number, compare only the pitch class.

**Acceptance:**

- [x] `findNoteByName("C")` returns only C notes.
- [x] `findNoteByName("C4")` returns only positions of MIDI note 60.
- [x] `findNoteByName("Db")` and `findNoteByName("C#")` return the same positions.

### T01-07: Set the product names and identifiers

- **Status:** [x] Done
- **Prerequisites:** T01-04
- **Files:** `plugin/CMakeLists.txt`, `plugin/Source/Main.cpp`

**Procedure:**

1. Change the CMake target name from `JUCEProject` to `AlgoRythm`.
2. Set `PRODUCT_NAME` to "Algo-Rythm".
3. Set `COMPANY_NAME` to your name or company name.
4. Set `BUNDLE_ID` to a reverse-domain value that you own, for example `com.kjsland.algorythm`.
5. Change `getApplicationName()` in `Main.cpp` to return "Algo-Rythm".

**Acceptance:**

- [x] The window title shows "Algo-Rythm".
- [x] No file contains the text "MyCompany" or "com.mycompany".

### T01-08: Move the Python files to the sandbox folder

- **Status:** [x] Done
- **Prerequisites:** T01-03
- **Files:** `midi_converter.py`, `rand_period.py`, `sandbox/requirements.txt`, `sandbox/README.md`

**Procedure:**

1. Make a folder `sandbox/`.
2. Move `midi_converter.py` and `rand_period.py` to `sandbox/` with `git mv`.
3. Write `sandbox/requirements.txt` with `sympy`, `numpy`, `pretty_midi` and `matplotlib`.
4. Pin the version of each package in `requirements.txt`.
5. In `rasterize()`, remove the line that replaces the `m` argument with a new `Melody`.
6. Normalize the curve values to the range -1 to 1 before you map them to notes.
7. Write `sandbox/README.md`. Tell the reader that the sandbox is for experiments only.

**Acceptance:**

- [x] `python sandbox/midi_converter.py` writes a MIDI file in which all notes are in the selected scale.
- [x] `rasterize()` uses the `Melody` object that the caller gives.

## 5. Done criteria

- [ ] All tasks in this spec are Done.
- [ ] The app builds and starts on macOS and Windows.
- [ ] The CI job from T02-03 passes on Ubuntu.
