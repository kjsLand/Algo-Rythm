# Algo-Rythm App

The JUCE desktop application for Algo-Rythm. All music logic comes from the core library in `core/`.

## Project Structure

```
app/
├── CMakeLists.txt          # The app, the algo_ui library and the algo_app_tests target
├── README.md
├── run.bat                 # Build script for Windows
├── run.sh                  # Build script for macOS and Linux
└── Source/
    ├── Main.cpp            # The application, the window and the preferences
    ├── AppState.*          # Settings, the current song starter and the background generation job
    ├── MainComponent.*     # The layout, the menu and the keyboard shortcuts
    ├── ControlsPanel.*     # The settings controls and the locks
    ├── SongView.*          # Section tabs, bar cells, rerolls and drag to DAW
    ├── ChordDiagram.*      # The chord box
    ├── FretboardView.*     # The fretboard of the selected bar
    ├── StrumView.*         # The strum pattern of the selected section
    ├── ExportPanel.*       # Export, Save and the rating stars
    ├── LibraryPanel.*      # The favourites library
    ├── TransportBar.*      # Play, Stop, Loop Section and Click
    ├── AudioEngine.*       # The audio device and the synthesizer
    ├── Sequencer.*         # The event schedule and the audio-thread playback
    ├── PluckVoice.*        # The Karplus-Strong voice and the click voice
    └── Theme.h             # The colours (contrast 4.5:1 or more)
```

The plugin in `plugin/` uses the same sources through the `algo_ui` library (T16-01).

## Building

CMake is the only build path. Run all commands from the repository root. CMake downloads JUCE at the version pinned in [cmake/Dependencies.cmake](../cmake/Dependencies.cmake).

### Prerequisites

- CMake 3.22+
- Git
- A C++17 compiler (see your operating system below)

### macOS

Install the Xcode Command Line Tools with `xcode-select --install`.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
./build/app/AlgoRythm_artefacts/Debug/Algo-Rythm.app/Contents/MacOS/Algo-Rythm
```

### Windows

Install Visual Studio 2022 with the "Desktop development with C++" workload.
Run these commands in Command Prompt.

```bat
cmake -B build
cmake --build build --config Debug
build\app\AlgoRythm_artefacts\Debug\Algo-Rythm.exe
```

`app\run.bat` does the configure and build steps.

If the JUCE download fails with "Filename too long", move the repository to a shorter path.

### Ubuntu

Install the compiler and the JUCE system packages:

```bash
sudo apt update
sudo apt install g++ cmake git pkg-config libasound2-dev libfreetype-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libglu1-mesa-dev mesa-common-dev
```

Build and run the app:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
./build/app/AlgoRythm_artefacts/Debug/Algo-Rythm
```

## Modules Used

| Module               | Purpose                          |
|----------------------|----------------------------------|
| juce_core            | Core utilities, strings, files   |
| juce_data_structures | ValueTree, UndoManager           |
| juce_events          | Message loop, timers, threads    |
| juce_graphics        | 2D graphics, fonts, images       |
| juce_gui_basics      | Components, windows, layout      |
| juce_gui_extra       | Extra widgets                    |
| juce_audio_basics    | The synthesizer and MIDI buffers |
| juce_audio_devices   | The audio output device          |
| juce_audio_utils     | The audio settings dialog        |
