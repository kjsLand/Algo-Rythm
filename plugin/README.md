# Algo-Rythm App

The JUCE GUI application for Algo-Rythm.

## Project Structure

```
plugin/
├── CMakeLists.txt          # The app target. The root CMakeLists.txt adds this folder.
├── README.md
├── run.bat                 # Build script for Windows
├── run.sh                  # Build script for macOS and Linux
└── Source/
    ├── Main.cpp            # Application entry point
    ├── MainComponent.h     # Main window component header
    ├── MainComponent.cpp   # Main window component implementation
    ├── Fretboard.h         # Tight chord voicing (the model is in core/)
    ├── Fretboard.cpp
    └── StrumPattern.h      # Strum pattern generator
```

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
./build/plugin/AlgoRythm_artefacts/Debug/Algo-Rythm.app/Contents/MacOS/Algo-Rythm
```

### Windows

Install Visual Studio 2022 with the "Desktop development with C++" workload.
Run these commands in Command Prompt.

```bat
cmake -B build
cmake --build build --config Debug
build\plugin\AlgoRythm_artefacts\Debug\Algo-Rythm.exe
```

`plugin\run.bat` does the configure and build steps.

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
./build/plugin/AlgoRythm_artefacts/Debug/Algo-Rythm
```

## Modules Used

| Module               | Purpose                          |
|----------------------|----------------------------------|
| juce_core            | Core utilities, strings, files   |
| juce_data_structures | ValueTree, UndoManager           |
| juce_events          | Message loop, timers, threads    |
| juce_graphics        | 2D graphics, fonts, images       |
| juce_gui_basics      | Components, windows, layout      |
| juce_gui_extra       | Extra widgets (WebView, etc.)    |
