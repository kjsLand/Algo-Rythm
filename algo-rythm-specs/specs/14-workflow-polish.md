# SPEC-14: Workflow Polish

Milestone: M4
Source: Coding Spec, Section 3 (US-2, US-8, US-9) and Section 10 (M4)
Depends on: SPEC-11, SPEC-12

## 1. Purpose

This spec makes the app fast to use in daily practice.
The user can lock parts, reroll one part, save favourites and rate results.
It also adds release builds for macOS and Windows.

## 2. Scope

In scope:

- Locks and per-section rerolls.
- The favourites library and ratings.
- Saved app preferences.
- Release builds from CI.

Out of scope:

- Table changes that use the ratings. Refer to SPEC-15.

## 3. Requirements

| ID | Requirement |
| --- | --- |
| REQ-14-1 | A locked item must not change when the user clicks Generate. |
| REQ-14-2 | A reroll must change only the item and section that the user selected. |
| REQ-14-3 | Each saved song starter must reload to the same result. |
| REQ-14-4 | A rating must be an integer from 1 to 5. |
| REQ-14-5 | CI must make an installable build for macOS and Windows from each version tag. |

## 4. Tasks

### T14-01: Add the lock fields to the settings

- **Status:** [ ] Open
- **Prerequisites:** T05-03, T09-07
- **Files:** `core/include/algo/Settings.h`, `core/src/engine.cpp`, `tests/locks_test.cpp`

**Procedure:**

1. Write the struct `Locks` with the fields key, scale, tempo and form.
2. Add a list of section locks with the fields chords, voicings and strum.
3. Add the field `previous` to the settings. It holds the last song starter.
4. In `generate`, if a field has a lock, copy its value from `previous`. Skip that stage for that item.
5. If a lock exists and `previous` is empty, ignore the lock.

**Acceptance:**

- [ ] REQ-14-1 is true over 1,000 seeds for each lock field.

### T14-02: Add the lock controls to the app

- **Status:** [ ] Open
- **Prerequisites:** T14-01, T11-10
- **Files:** `app/Source/ControlsPanel.cpp`, `app/Source/SongView.cpp`, `app/Source/LockButton.h`

**Procedure:**

1. Write a small `LockButton` component with an open lock icon and a closed lock icon.
2. Add a lock button next to the key, scale, tempo and form controls.
3. Add 3 lock buttons to each section tab for chords, voicings and strum.
4. When the user clicks a lock button, update `Locks` in `AppState`.

**Acceptance:**

- [ ] A locked verse progression stays the same after 10 clicks on Generate.

### T14-03: Add the per-section rerolls

- **Status:** [ ] Open
- **Prerequisites:** T14-01, T05-06
- **Files:** `core/include/algo/Engine.h`, `core/src/engine.cpp`, `app/Source/SongView.cpp`

**Procedure:**

1. Add a reroll count for each stage and section to the settings.
2. Write `reroll(starter, stage, sectionIndex)`. The function increases the count and runs that stage for that section only.
3. If the stage is Chords, also run the Voicing stage for that section.
4. Add the buttons "New progression", "New voicing" and "New strum" to each section tab.
5. Store the reroll counts in the song starter JSON.

**Acceptance:**

- [ ] REQ-14-2 is true over 1,000 seeds.
- [ ] A saved song starter with reroll counts reloads to the same result.

### T14-04: Write the favourites library

- **Status:** [ ] Open
- **Prerequisites:** T09-08, T11-10
- **Files:** `app/Source/Library.h`, `app/Source/Library.cpp`, `app/Source/LibraryPanel.cpp`

**Procedure:**

1. Set the library folder to `~/Algo-Rythm/library/`.
2. Make the folder if it does not exist.
3. Add a "Save" button.
4. Write the current song starter as JSON with its default file name.
5. Add a side panel that lists the saved files.
6. Show the key, BPM and date.
7. When the user double-clicks a file, load it into `AppState`.
8. Add a "Delete" button that moves the file to the system trash.

**Acceptance:**

- [ ] REQ-14-3 is true for 20 saved song starters.

### T14-05: Add the ratings

- **Status:** [ ] Open
- **Prerequisites:** T14-04
- **Files:** `core/include/algo/Types.h`, `app/Source/RatingControl.cpp`, `app/Source/Library.cpp`

**Procedure:**

1. Add the optional field `rating` to `SongStarter`.
2. Add a control with 5 stars next to the Save button.
3. When the user sets a rating, save the song starter to the library.
4. Add one line to `~/Algo-Rythm/ratings.csv` with date, seed, difficulty, rating and the progressions.
5. If `ratings.csv` does not exist, write the header line first.

**Acceptance:**

- [ ] REQ-14-4 is true.
- [ ] Each rating adds one line to `ratings.csv`.

### T14-06: Save the app preferences

- **Status:** [ ] Open
- **Prerequisites:** T11-10
- **Files:** `app/Source/Preferences.h`, `app/Source/Preferences.cpp`

**Procedure:**

1. Use `juce::PropertiesFile` to save the preferences.
2. Save the last settings, the window size and the window position.
3. Save the click state and the last export folder.
4. Load the preferences when the app starts.

**Acceptance:**

- [ ] The app opens with the same settings and window size after a restart.

### T14-07: Add the release builds

- **Status:** [ ] Open
- **Prerequisites:** T02-03, T11-10
- **Files:** `.github/workflows/release.yml`, `docs/RELEASE.md`

**Procedure:**

1. Write a workflow that starts when you push a tag in the form `v*.*.*`.
2. Build a universal binary (arm64 and x86_64) for macOS.
3. Package the macOS app as a `.dmg` file.
4. Build the Windows app.
5. Package the Windows app as a `.zip` file.
6. Attach both files to a GitHub release.
7. Write `docs/RELEASE.md` with the steps for code signing and notarization.
8. If you have no Apple Developer ID, make unsigned builds. Write this limit in the release notes.

**Acceptance:**

- [ ] REQ-14-5 is true for a test tag `v0.1.0-rc1`.

## 5. Done criteria

- [ ] All tasks in this spec are Done.
- [ ] You use the app for one week of practice with no data loss.
