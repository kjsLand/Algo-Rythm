# SPEC-12: Audio Playback

Milestone: M3
Source: Coding Spec, Section 7.1 (Playback)
Depends on: SPEC-09, SPEC-11

## 1. Purpose

This spec lets the user hear the song starter in the app.
A plucked-string synthesizer plays the rendered events.
The app does not need a sample library.

## 2. Scope

In scope:

- The JUCE audio modules and the audio device setup.
- The plucked-string voice (Karplus-Strong method).
- The sequencer, the transport controls and the click.
- An offline render for tests.

Out of scope:

- Sampled guitar sounds. This is an open question in the Coding Spec, Section 11.
- Host sync in a plugin. Refer to SPEC-16.

## 3. Requirements

| ID | Requirement |
| --- | --- |
| REQ-12-1 | The audio thread must not call the generator, allocate memory or lock a mutex. |
| REQ-12-2 | Each note must start within 1 ms of its event time. |
| REQ-12-3 | The user must be able to play, stop and loop one section. |
| REQ-12-4 | The user must be able to turn the click on and off during playback. |
| REQ-12-5 | A new song starter must replace the old events with no audio click or gap. |

## 4. Tasks

### T12-01: Add the audio modules and device setup

- **Status:** [ ] Open
- **Prerequisites:** T11-01
- **Files:** `app/CMakeLists.txt`, `app/Source/AudioEngine.h`, `app/Source/AudioEngine.cpp`

**Procedure:**

1. Link `juce_audio_basics`, `juce_audio_devices`, `juce_audio_formats` and `juce_audio_utils`.
2. Write the class `AudioEngine` with a `juce::AudioDeviceManager`.
3. Open the default output device with 0 inputs and 2 outputs.
4. Connect an `juce::AudioSourcePlayer` to the device.
5. If no device opens, show a message in the status bar. Do not stop the app.

**Acceptance:**

- [ ] The app starts on a computer with no audio device and shows the message.

### T12-02: Write the plucked-string voice

- **Status:** [ ] Open
- **Prerequisites:** T12-01
- **Files:** `app/Source/PluckVoice.h`, `app/Source/PluckVoice.cpp`

**Procedure:**

1. Write a `juce::SynthesiserVoice` that uses the Karplus-Strong method.
2. Set the delay length to the sample rate divided by the note frequency.
3. Fill the delay line with noise at the note start. Scale the noise by the note velocity.
4. Use a two-point average filter in the feedback loop.
5. Set the decay factor so that a note at E2 decays by 60 dB in about 4 seconds.
6. Fill the delay line with noise from a fixed seed. This gives the same sound on each play.
7. Allocate the delay line in `prepareToPlay` only.

**Acceptance:**

- [ ] A test renders A4 offline and finds a frequency of 440 Hz within 2 Hz.
- [ ] The voice makes no allocation in `renderNextBlock`.

### T12-03: Write the sequencer

- **Status:** [ ] Open
- **Prerequisites:** T12-02, T09-03
- **Files:** `app/Source/Sequencer.h`, `app/Source/Sequencer.cpp`

**Procedure:**

1. Write the class `Sequencer` as a `juce::AudioSource`.
2. Convert the `EventList` to a sorted list of note-on and note-off events in samples.
3. Prepare the list on the message thread.
4. Give the list to the audio thread with an atomic pointer swap.
5. Free the old list on the message thread after the swap.
6. In `getNextAudioBlock`, send each event to the synthesizer at its sample offset in the block.

**Acceptance:**

- [ ] REQ-12-1 and REQ-12-2 are true in the test from T12-06.

### T12-04: Write the transport controls

- **Status:** [ ] Open
- **Prerequisites:** T12-03, T11-09
- **Files:** `app/Source/TransportBar.h`, `app/Source/TransportBar.cpp`

**Procedure:**

1. Add the buttons Play, Stop and Loop Section.
2. Add a toggle for the click.
3. Connect the Space key to Play and Stop.
4. When Loop Section is on, play the selected section again and again.
5. Show the playback position as a line that moves in the song overview.
6. When the user generates a new song starter during playback, start the new events at the next bar.

**Acceptance:**

- [ ] REQ-12-3, REQ-12-4 and REQ-12-5 are true.

### T12-05: Add the audio settings dialog

- **Status:** [ ] Open
- **Prerequisites:** T12-01
- **Files:** `app/Source/AudioSettingsDialog.cpp`

**Procedure:**

1. Add a menu item "Audio Settings".
2. Show a `juce::AudioDeviceSelectorComponent` in a dialog window.
3. Save the device settings with `createStateXml`.
4. Load the saved settings when the app starts.

**Acceptance:**

- [ ] The selected output device stays selected after the app restarts.

### T12-06: Add the offline timing test

- **Status:** [ ] Open
- **Prerequisites:** T12-03
- **Files:** `tests/app/sequencer_timing_test.cpp`, `app/CMakeLists.txt`

**Procedure:**

1. Add a test target `algo_app_tests` that links JUCE and the app sources, except `Main.cpp`.
2. Render 8 bars of a known song starter offline at 48 kHz with a block size of 512.
3. Record the sample index of each note-on.
4. Compare each index to the expected time of its event.
5. Run the test with block sizes 64, 512 and 1024.

**Acceptance:**

- [ ] Each note starts within 48 samples (1 ms) of its expected time at all 3 block sizes.

## 5. Done criteria

- [ ] All tasks in this spec are Done.
- [ ] You play a song starter for 5 minutes with no audio dropout.
