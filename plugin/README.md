# Algo-Rythm Plugin

The VST3, AU and Standalone plugin (SPEC-16). It uses the core library and the UI library `algo_ui` from `app/` (REQ-16-1).

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DALGO_BUILD_PLUGIN=ON
cmake --build build --config Release --target AlgoRythmPlugin_All
```

The files are in `build/plugin/AlgoRythmPlugin_artefacts/Release/`: `VST3/Algo-Rythm.vst3`, `AU/Algo-Rythm.component` (macOS only) and `Standalone/`.
Copy the VST3 to your plugin folder, for example `~/Library/Audio/Plug-Ins/VST3/` on macOS or `C:\Program Files\Common Files\VST3\` on Windows.

## Use

1. Put the plugin on an instrument track.
2. Press Play in the host. The plugin follows the host tempo and position (REQ-16-2). The song starter repeats.
3. To record the MIDI, route the MIDI output of the plugin to another instrument track.
4. Turn "Preview Sound" off if you only want the MIDI output.

When the host stops, the plugin sends a note-off for each note that sounds. The DAW project saves the song starter and the settings, so a reopened project restores them.
The click track of the app is not in the MIDI output. Use the metronome of the host.

## Check the plugin

CI runs `pluginval` 1.0.4 at strictness level 5 on the VST3 on macOS and Windows (T16-03). To run it on your computer:

```bash
pluginval --strictness-level 5 --validate-in-process --validate build/plugin/AlgoRythmPlugin_artefacts/Release/VST3/Algo-Rythm.vst3
```
