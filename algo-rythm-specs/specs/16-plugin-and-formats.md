# SPEC-16: Plugin, Advanced Formats and Extensions

Milestone: M6
Source: Coding Spec, Section 3 (Out of scope), Section 7.3, Section 7.4 and Section 10 (M6)
Depends on: SPEC-11, SPEC-12

## 1. Purpose

This spec adds the features that come after the 12-month plan.
It makes a VST3 and AU plugin from the app components.
It adds MusicXML, Guitar Pro, 6/8 time, alternate tunings and model experiments.

## 2. Scope

In scope:

- The plugin target, host sync and MIDI output.
- MusicXML and Guitar Pro export.
- 6/8 time, fingerpicking patterns, alternate tunings and a left-handed view.
- Experiments with external symbolic music models.

Out of scope:

- Audio-to-tab transcription.
- A web or mobile version.

## 3. Requirements

| ID | Requirement |
| --- | --- |
| REQ-16-1 | The plugin must use the same core library and UI components as the app. |
| REQ-16-2 | The plugin must follow the host tempo and position. |
| REQ-16-3 | The plugin must pass `pluginval` at strictness level 5. |
| REQ-16-4 | MusicXML files must open in MuseScore with a notation staff and a tab staff. |
| REQ-16-5 | Before you use an external model, you must record its licence. Non-commercial models must not ship in a release. |

## 4. Tasks

### T16-01: Make the plugin target

- **Status:** [x] Done
- **Prerequisites:** T11-10, T12-03
- **Files:** `plugin/CMakeLists.txt`, `plugin/Source/PluginProcessor.*`, `plugin/Source/PluginEditor.*`

**Procedure:**

1. Write `plugin/CMakeLists.txt` with `juce_add_plugin`.
2. Set the formats to VST3, AU and Standalone.
3. Set `IS_SYNTH` to TRUE and `NEEDS_MIDI_OUTPUT` to TRUE.
4. Move the shared UI components from `app/Source/` to a library target `algo_ui`.
5. Link the app and the plugin to `algo_ui`.
6. Write `PluginEditor` with the same panels as the app.
7. Build the plugin only when `ALGO_BUILD_PLUGIN` is ON.

**Acceptance:**

- [ ] The plugin loads in Reaper and in Ableton Live.
- [x] REQ-16-1 is true.

**Note:** A JUCE VST3 host on Ubuntu loads the plugin, plays it and opens its editor. Check Reaper and Ableton Live by hand.

### T16-02: Write the host sync and MIDI output

- **Status:** [x] Done
- **Prerequisites:** T16-01
- **Files:** `plugin/Source/PluginProcessor.cpp`

**Procedure:**

1. In `processBlock`, read the tempo and the PPQ position from `getPlayHead()`.
2. If the host transport stops, send note-off events for all notes that sound.
3. Convert the PPQ position to ticks at 480 PPQ.
4. Send the events for the current block to the MIDI output buffer.
5. Play the events through the plucked-string synthesizer if the user turns on "Preview Sound".
6. Save the current song starter JSON in `getStateInformation`.

**Acceptance:**

- [x] REQ-16-2 is true at 70, 120 and 170 BPM.
- [x] A reopened DAW project restores the same song starter.

**Note:** `tests/app/host_sync_test.cpp` checks 70, 120 and 170 BPM. A test host restored the saved state.

### T16-03: Run pluginval in CI

- **Status:** [x] Done
- **Prerequisites:** T16-02, T02-03
- **Files:** `.github/workflows/ci.yml`

**Procedure:**

1. Add a CI job that builds the plugin on macOS and Windows.
2. Download a pinned version of `pluginval`.
3. Run `pluginval` on the VST3 file at strictness level 5.

**Acceptance:**

- [ ] REQ-16-3 is true in CI.

**Note:** Check the result of the `plugin` job in the next CI run.

### T16-04: Write the MusicXML export

- **Status:** [x] Done
- **Prerequisites:** T09-08
- **Files:** `core/include/algo/MusicXml.h`, `core/src/musicxml.cpp`, `tests/musicxml_test.cpp`

**Procedure:**

1. Write a MusicXML 4.0 score with one guitar part.
2. Add a notation staff and a tab staff with 6 lines and the tuning of the song starter.
3. Write the chords as notes with `<string>` and `<fret>` elements.
4. Write chord names with `<harmony>` elements.
5. Write the hook as a second part if it exists.
6. Add "musicxml" to the formats in `exportAll` and the CLI.

**Acceptance:**

- [ ] REQ-16-4 is true for 5 golden song starters.

**Note:** 5 golden MusicXML files are in `tests/golden/musicxml/`. Open them in MuseScore to check REQ-16-4.

### T16-05: Write the Guitar Pro export

- **Status:** [x] Done
- **Prerequisites:** T09-08, T01-08
- **Files:** `tools/export_gp.py`, `tools/requirements.txt`

**Procedure:**

1. Add PyGuitarPro to `tools/requirements.txt`.
2. Pin the PyGuitarPro version.
3. Read a song starter JSON file.
4. Make a GP5 song with one guitar track in the tuning of the song starter.
5. Write each stroke as a beat with the string and fret of each note.
6. Write the section names as markers.
7. Write the file with `guitarpro.write`.

**Acceptance:**

- [ ] TuxGuitar opens the GP5 file and plays the correct chords.

**Note:** PyGuitarPro reads the GP5 files back in CI. Check the playback in TuxGuitar by hand.

### T16-06: Add 6/8 time and fingerpicking patterns

- **Status:** [x] Done
- **Prerequisites:** T08-06, T09-02
- **Files:** `core/src/strum.cpp`, `core/data/strums.json`, `core/src/render.cpp`

**Procedure:**

1. Permit the time signature 6/8 in the settings.
2. Use a 12-step grid for 6/8.
3. Put the strong beats on slots 0 and 6.
4. Add 6/8 patterns to `strums.json`.
5. Add the stroke type `Pluck` with a string number.
6. Add p-i-m-a and Travis patterns to `strums.json`.
7. Render a `Pluck` as one note on its string.

**Acceptance:**

- [x] A 6/8 song starter exports MIDI with the time signature 6/8.
- [x] A Travis pattern alternates the bass note between two strings on each beat.

### T16-07: Add alternate tunings and a left-handed view

- **Status:** [x] Done
- **Prerequisites:** T07-07, T11-06
- **Files:** `core/data/tunings.json`, `app/Source/ControlsPanel.cpp`, `app/Source/FretboardView.cpp`, `app/Source/ChordDiagram.cpp`

**Procedure:**

1. Write `tunings.json` with Standard, Drop D, DADGAD, Open G, Open D and Half-step down.
2. Add a tuning combo box to the controls panel.
3. Disable the open shapes from `shapes.json` when the tuning is not Standard.
4. Add a "Left-handed" option that mirrors the fretboard view and the chord diagrams.

**Acceptance:**

- [x] In DADGAD, each voicing passes the filter from T07-03.
- [x] The left-handed view shows the low E string in the correct position.

### T16-08: Run the external model experiments

- **Status:** [ ] Open
- **Prerequisites:** T15-06
- **Files:** `experiments/README.md`, `experiments/LICENCES.md`

**Procedure:**

1. Before each experiment, write the model name, its licence and the licence of its data in `LICENCES.md`.
2. If the licence is non-commercial, keep the model in `experiments/` only.
3. Test a chord-aware MIDI model with the progressions from the generator as input.
4. Test a MIDI-to-tab model as a second opinion for the voicing engine.
5. Rate 20 results from each experiment with the A/B method from SPEC-15.
6. Write the results and a decision in `experiments/README.md`.

**Acceptance:**

- [ ] REQ-16-5 is true for each experiment.
- [ ] Each experiment has a written decision: adopt, change or stop.

## 5. Done criteria

- [ ] All tasks in this spec are Done.
- [ ] The plugin works in your main DAW for one full song session.

**Note:** The procedure and the licence register are in `experiments/`. A person must choose the models, run them and rate the results.
