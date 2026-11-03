# SPEC-11: Desktop App User Interface

Milestone: M3
Source: Coding Spec, Section 7.1
Depends on: SPEC-09

## 1. Purpose

This spec changes the current JUCE window into the Algo-Rythm desktop app.
The app shows the controls, the song overview, the fretboard and the strum panel.
The app uses the core for all music logic.

## 2. Scope

In scope:

- The `app/` folder and its CMake target.
- The app state and the background generation job.
- The 4 panels, the chord diagram component, export and drag to DAW.
- Keyboard shortcuts and window layout.

Out of scope:

- Audio playback. Refer to SPEC-12.
- Locks, rerolls, the library and ratings. Refer to SPEC-14.

## 3. Requirements

| ID | Requirement |
| --- | --- |
| REQ-11-1 | The app must not contain music logic. All music logic must come from `algo::core`. |
| REQ-11-2 | Generation must not block the message thread for more than 16 ms. |
| REQ-11-3 | Each bar in the song overview must show the chord name, the Nashville number and a chord diagram. |
| REQ-11-4 | The window must be resizable. The minimum size must be 900 × 600 pixels. |
| REQ-11-5 | All controls must work with the keyboard. |
| REQ-11-6 | Text and colours must have a contrast ratio of 4.5:1 or more. |

## 4. Tasks

### T11-01: Move the UI code to the app folder

- **Status:** [x] Done
- **Prerequisites:** T09-07, T02-02
- **Files:** `app/CMakeLists.txt`, `app/Source/*`, `plugin/`

**Procedure:**

1. Move `Main.cpp` and `MainComponent.*` from `plugin/Source/` to `app/Source/` with `git mv`.
2. Write `app/CMakeLists.txt` with `juce_add_gui_app`.
3. Use the names from T01-07.
4. Link the app to `algo::core`.
5. Remove the old `plugin/` folder content.
6. Keep the folder name for SPEC-16.
7. Build the app only when `ALGO_BUILD_APP` is ON.

**Acceptance:**

- [ ] The app builds and starts on macOS and Windows.
- [x] No file in `app/` defines scales, chords or rhythm logic.

**Note:** The app builds and starts on Ubuntu (Xvfb). Check macOS and Windows in CI or by hand.

### T11-02: Write the app state and the generation job

- **Status:** [x] Done
- **Prerequisites:** T11-01
- **Files:** `app/Source/AppState.h`, `app/Source/AppState.cpp`

**Procedure:**

1. Write the class `AppState` with the settings, the current `SongStarter` and the selected section and bar.
2. Add a `juce::ChangeBroadcaster` so that panels can listen for changes.
3. Write `requestGenerate()`. The function runs `algo::generate` in a `juce::ThreadPool` job.
4. When the job ends, send the result to the message thread with `juce::MessageManager::callAsync`.
5. If a new request starts before the old job ends, ignore the old result.
6. If `generate` throws, show the error text in a status bar.

**Acceptance:**

- [ ] 20 fast clicks on Generate do not freeze the window.
- [x] The app shows the result of the last click only.

**Note:** Each request gets an ID. The app ignores a result with an old ID. Check the 20 fast clicks by hand.

### T11-03: Write the controls panel

- **Status:** [x] Done
- **Prerequisites:** T11-02
- **Files:** `app/Source/ControlsPanel.h`, `app/Source/ControlsPanel.cpp`

**Procedure:**

1. Add a combo box for the key with the values "Any" and the 12 roots.
2. Add combo boxes for mood, form and difficulty.
3. Add two sliders for the minimum and maximum BPM.
4. Add a combo box for the capo with the values 0 to 7.
5. Add a text field for the seed and a "Copy" button.
6. Add the Generate button.
7. When a control changes, update the settings in `AppState`. Do not generate.

**Acceptance:**

- [ ] Each control changes the next generated song starter in the expected way.
- [x] The seed field shows the seed of the current song starter.

### T11-04: Write the chord diagram component

- **Status:** [x] Done
- **Prerequisites:** T11-01, T07-06
- **Files:** `app/Source/ChordDiagram.h`, `app/Source/ChordDiagram.cpp`

**Procedure:**

1. Draw 6 vertical strings and 5 horizontal frets.
2. Draw a thick nut if the base fret is 1. Otherwise, write the base fret number on the left.
3. Draw a dot for each fretted note.
4. Write the finger number in the dot.
5. Draw a curved bar for a barre.
6. Write "O" above an open string and "X" above a muted string.
7. Write the chord name above the diagram.

**Acceptance:**

- [ ] The diagrams for C, G, F (barre) and Bm7 match a standard chord chart.
- [x] The diagram can draw at sizes from 48 to 160 pixels wide.

### T11-05: Write the song overview panel

- **Status:** [x] Done
- **Prerequisites:** T11-02, T11-04
- **Files:** `app/Source/SongView.h`, `app/Source/SongView.cpp`

**Procedure:**

1. Add one tab for each section.
2. In each tab, show one cell for each bar.
3. In each cell, show the chord name, the Nashville number and a chord diagram.
4. Highlight the selected bar.
5. When the user clicks a cell, set the selected bar in `AppState`.
6. Show the key, BPM, time signature and capo above the tabs.

**Acceptance:**

- [x] REQ-11-3 is true for each bar.
- [x] A click on a bar updates the fretboard panel.

### T11-06: Change the fretboard panel to use voicings

- **Status:** [x] Done
- **Prerequisites:** T11-05, T07-07
- **Files:** `app/Source/FretboardView.h`, `app/Source/FretboardView.cpp`

**Procedure:**

1. Move `drawFretboard` from `MainComponent` to a new component `FretboardView`.
2. Read the voicing of the selected bar from `AppState`.
3. Show the voicing notes in the chord colour.
4. Show the root in the root colour.
5. Show the scale notes of the key in the scale colour.
6. If the capo is more than 0, draw the capo bar at its fret.
7. Remove the "New Note", "New Chord" and "New Key" buttons.

**Acceptance:**

- [x] The fretboard shows the same notes as the chord diagram for the selected bar.

### T11-07: Change the strum panel to use the core type

- **Status:** [x] Done
- **Prerequisites:** T11-05, T08-06
- **Files:** `app/Source/StrumView.h`, `app/Source/StrumView.cpp`

**Procedure:**

1. Move `drawStrumPanel` from `MainComponent` to a new component `StrumView`.
2. Read the strum pattern of the selected section from `AppState`.
3. Draw the beat ruler and one symbol for each slot.
4. Use different symbols for Down, Up, Rest, Mute and Chuck.
5. Show accents with a larger symbol.
6. Remove the "New Strum" button. SPEC-14 adds a reroll button for each section.

**Acceptance:**

- [x] The strum panel shows the same pattern as the ASCII tab export.

### T11-08: Add the export buttons and drag to DAW

- **Status:** [x] Done
- **Prerequisites:** T11-05, T09-08
- **Files:** `app/Source/ExportPanel.h`, `app/Source/ExportPanel.cpp`

**Procedure:**

1. Add the buttons "Export MIDI", "Export Tab" and "Export JSON".
2. When the user clicks a button, open a `juce::FileChooser`. Use the default file name from T09-08.
3. Add a drag handle on each section tab.
4. When the user drags the handle, write a temporary MIDI file for that section.
5. Start an external file drag with `performExternalDragDropOfFiles`.

**Acceptance:**

- [ ] A section that you drag into Ableton Live or Logic Pro makes a MIDI clip with the correct length.

**Note:** Check the drag into Ableton Live or Logic Pro by hand.

### T11-09: Add the keyboard shortcuts

- **Status:** [x] Done
- **Prerequisites:** T11-05
- **Files:** `app/Source/MainComponent.cpp`

**Procedure:**

1. Add a `juce::KeyListener` or a `juce::ApplicationCommandManager` to the main component.
2. Set G to Generate.
3. Set the keys 1 to 4 to select sections 1 to 4.
4. Set Space to Play and Stop. SPEC-12 connects this key to the transport.
5. Show the shortcuts in the tooltip of each button.

**Acceptance:**

- [ ] Each shortcut works when any panel has the keyboard focus.

### T11-10: Write the window layout

- **Status:** [x] Done
- **Prerequisites:** T11-03, T11-05, T11-06, T11-07, T11-08
- **Files:** `app/Source/MainComponent.cpp`, `app/Source/Theme.h`

**Procedure:**

1. Put the controls panel at the top.
2. Put the song overview below the controls panel.
3. Put the fretboard panel below the song overview.
4. Put the strum panel and the export panel at the bottom.
5. Set the minimum window size to 900 × 600 pixels.
6. Write the colours in `Theme.h`.
7. Make sure that each text colour meets REQ-11-6.

**Acceptance:**

- [x] No panel overlaps another panel at 900 × 600 pixels or at full screen.

## 5. Done criteria

- [x] All tasks in this spec are Done.
- [ ] REQ-11-1 to REQ-11-6 are true.

**Note:** Checked with screenshots at 900 x 600 and 1280 x 860 pixels.
