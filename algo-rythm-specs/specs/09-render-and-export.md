# SPEC-09: Engine Assembly, Render and Export

Milestone: M2
Source: Coding Spec, Section 6 (engine interface), Section 6.7 and Section 7.3
Depends on: SPEC-06, SPEC-07, SPEC-08

## 1. Purpose

This spec joins the generator stages into one engine function.
It converts a song starter into timed note events.
It writes the events and the song starter to MIDI, ASCII tab and JSON files.

## 2. Scope

In scope:

- The `generate` function and the `assemble` function.
- The `EventList` type and the `render` function.
- The Standard MIDI File (SMF) writer and a test reader.
- The ASCII tab writer and the JSON file functions.

Out of scope:

- The melody hook stage. Refer to SPEC-13. This spec makes space for the hook track.
- MusicXML and Guitar Pro. Refer to SPEC-16.

## 3. Requirements

| ID | Requirement |
| --- | --- |
| REQ-09-1 | `generate(settings)` must return a complete `SongStarter` or throw `GenerationError`. |
| REQ-09-2 | A Down stroke must sound its strings from low to high. An Up stroke must sound its strings from high to low. |
| REQ-09-3 | The time between strings in one stroke must be 8 to 20 ms. |
| REQ-09-4 | A strummed note must sound until the next stroke on the same string, or the end of the bar. |
| REQ-09-5 | The MIDI file must be SMF type 1 at 480 PPQ with tempo, time signature, key signature and section markers. |
| REQ-09-6 | The MIDI file must open in Logic Pro, Ableton Live, FL Studio and Reaper with the correct tempo. |
| REQ-09-7 | The ASCII tab must use the format in Coding Spec Section 7.3. |

## 4. Tasks

### T09-01: Define the event list

- **Status:** [x] Done
- **Prerequisites:** T03-02
- **Files:** `core/include/algo/Events.h`

**Procedure:**

1. Write the struct `NoteEvent` with track, channel, pitch, velocity, start tick and length in ticks.
2. Write the struct `MetaEvent` with tick, type and data. Types are tempo, time signature, key signature, marker and track name.
3. Write the struct `EventList` with note events, meta events and the total length in ticks.
4. Write `msToTicks(ms, bpm)`. The function uses ticks = ms × BPM × 480 / 60000. It rounds to the nearest tick.

**Acceptance:**

- [x] `msToTicks(15, 120)` returns 14.

### T09-02: Render the strum track

- **Status:** [x] Done
- **Prerequisites:** T09-01, T07-07, T08-06
- **Files:** `core/include/algo/Render.h`, `core/src/render.cpp`, `tests/render_test.cpp`

**Procedure:**

1. For each bar, get the voicing and the strum pattern of its section.
2. For each Down or Up slot, make one note for each string that sounds.
3. Use the top `strings` strings of the voicing only.
4. Count from string 1 (high E).
5. Put the strings in the order of REQ-09-2.
6. Add 12 ms between strings.
7. Use the Render stream to add a random change of up to 4 ms.
8. Set the velocity to 70 + 40 × accent.
9. Limit the result to the range 1 to 127.
10. Set the note length to the time until the next stroke, or to the end of the bar.

**Acceptance:**

- [x] A Down stroke on a 6-string voicing gives 6 notes. Each note has a higher pitch and a later start than the note before it.
- [x] REQ-09-3 and REQ-09-4 are true over 1,000 seeds.

### T09-03: Render mutes, chucks, click and markers

- **Status:** [x] Done
- **Prerequisites:** T09-02
- **Files:** `core/src/render.cpp`, `tests/render_test.cpp`

**Procedure:**

1. Render a Mute as notes on the same strings with a length of 30 ms and velocity 40.
2. Render a Chuck as one note on MIDI channel 10. Use pitch 39 (hand clap).
3. Add a click track on channel 10.
4. Use pitch 76 on beat 1 and pitch 77 on the other beats.
5. Add a tempo event, a time signature event and a key signature event at tick 0.
6. Add a marker event with the section name at the first tick of each section.
7. Add a track name event to each track: "Guitar", "Hook" and "Click".

**Acceptance:**

- [x] An 8-bar 4/4 section gives 32 click notes.
- [x] Each section has a marker at the correct tick.

### T09-04: Write the SMF writer

- **Status:** [x] Done
- **Prerequisites:** T09-01
- **Files:** `core/include/algo/Smf.h`, `core/src/smf_writer.cpp`, `tests/smf_test.cpp`

**Procedure:**

1. Write `writeSmf(events, stream)`. The function writes the header chunk "MThd" with format 1 and division 480.
2. Write a tempo track (track 0) with the meta events.
3. Write one track chunk "MTrk" for each of the guitar, hook and click tracks.
4. Write the delta times as variable-length quantities.
5. Write a program change at tick 0.
6. Use program 25 (steel string guitar) for the guitar track.
7. Write each note as a note-on event and a note-off event. Do not use running status.
8. Sort the events in each track by tick.
9. Put note-off events before note-on events at the same tick.
10. End each track with the meta event FF 2F 00.

**Acceptance:**

- [x] A test compares a small known song starter to a stored golden `.mid` file byte for byte.
- [x] `python -c "import mido; mido.MidiFile('out.mid')"` reads the file with no error.

### T09-05: Write the SMF test reader

- **Status:** [x] Done
- **Prerequisites:** T09-04
- **Files:** `tests/support/SmfReader.h`, `tests/support/smf_reader.cpp`, `tests/smf_roundtrip_test.cpp`

**Procedure:**

1. Write a small SMF reader for tests only. Do not add it to the core library.
2. Read the header, the tracks, the meta events and the note events.
3. Write a round-trip test: render, write, read, then compare the note count and the tempo.

**Acceptance:**

- [x] The round-trip test passes over 1,000 seeds.

### T09-06: Write the ASCII tab writer

- **Status:** [x] Done
- **Prerequisites:** T07-07, T08-06
- **Files:** `core/include/algo/Tab.h`, `core/src/tab.cpp`, `tests/tab_test.cpp`, `tests/golden/tab/*.txt`

**Procedure:**

1. Write a header line for each section with the section name, the BPM and the key.
2. Write a chord chart line with the chord names and the Nashville numbers.
3. Write a strum line, for example "Strum: D - D U - U D U".
4. Write 6 tab lines for each bar.
5. Put the high E string at the top.
6. Write the fret number at each stroke position.
7. Write "x" for a Mute.
8. If a fret number has 2 digits, make all columns in that bar 2 characters wide.
9. Write a chord diagram for each new chord at the end of the section.

**Acceptance:**

- [ ] The output for the example bar in Coding Spec Section 7.3 matches the example.
- [x] 5 golden tab files match the output byte for byte.

**Note:** The Coding Spec Section 7.3 example is not in this repository. `tests/tab_test.cpp` checks a G chord example bar in the same format.

### T09-07: Write the engine function

- **Status:** [x] Done
- **Prerequisites:** T06-08, T07-07, T08-06
- **Files:** `core/include/algo/Engine.h`, `core/src/engine.cpp`, `tests/engine_test.cpp`

**Procedure:**

1. Write `generate(settings, data)`. The function calls `validate(settings)` first.
2. If `validate` returns errors, throw `GenerationError` with all error texts.
3. Call `planSong`, `makeChords`, `voiceChords` and `makeStrums` in this order.
4. If settings ask for a hook, call `makeHook`. If SPEC-13 is not complete, leave the hook empty.
5. Call `assemble` to make the `SongStarter`.
6. Copy the settings into it.
7. Measure the time for each stage in Debug builds.
8. Write the times into a `GenerationStats` struct.

**Acceptance:**

- [x] `generate` with default settings returns a song starter with 2 or more sections.
- [x] A 64-bar Advanced song starter takes 200 ms or less in a Release build.

### T09-08: Write the JSON file functions

- **Status:** [x] Done
- **Prerequisites:** T09-07, T03-03
- **Files:** `core/include/algo/Files.h`, `core/src/files.cpp`, `tests/files_test.cpp`

**Procedure:**

1. Write `saveStarter(starter, path)`. The function uses 2-space indentation and keeps the field order.
2. Write `loadStarter(path)`.
3. Write `defaultFileName(starter)`. The function uses the form `<key>-<scale>-<bpm>bpm-<seed>`, for example `G-Major-96bpm-81723`.
4. Write `exportAll(starter, folder, formats)` for the formats "json", "mid" and "tab".
5. If the folder does not exist, make it.

**Acceptance:**

- [x] Save then load gives an equal `SongStarter`.
- [x] `exportAll` with the 3 formats writes 3 files with the same base name.

## 5. Done criteria

- [x] All tasks in this spec are Done.
- [x] The "Round trip" property test from Coding Spec Section 9.2 passes.
- [ ] You open one exported MIDI file in your DAW and hear the correct tempo and strum feel.
