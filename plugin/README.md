# JUCEProject

A default JUCE GUI application project.

## Project Structure

```
JUCEProject/
├── CMakeLists.txt          # CMake build system (recommended)
├── JUCEProject.jucer       # Projucer project file (alternative)
├── README.md
└── Source/
    ├── Main.cpp            # Application entry point
    ├── MainComponent.h     # Main window component header
    └── MainComponent.cpp   # Main window component implementation
```

## Building with CMake (Recommended)

### Prerequisites
- CMake 3.22+
- A C++17 compiler (MSVC, GCC, Clang)
- JUCE (clone it into the project root)

### Steps

```bash
# 1. Clone JUCE into the project directory
git clone https://github.com/juce-framework/JUCE.git

# 2. Configure
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# 3. Build
cmake --build build --config Debug

# 4. Run
# macOS/Linux: ./build/JUCEProject_artefacts/Debug/JUCEProject
# Windows:     build\JUCEProject_artefacts\Debug\JUCEProject.exe
```

## Building with Projucer (Alternative)

1. Download JUCE from https://juce.com/get-juce/
2. Open `JUCEProject.jucer` in Projucer
3. Set the JUCE path to your JUCE installation
4. Export to your IDE (Xcode, Visual Studio, Linux Makefile)
5. Build from your IDE

## Modules Used

| Module               | Purpose                          |
|----------------------|----------------------------------|
| juce_core            | Core utilities, strings, files   |
| juce_data_structures | ValueTree, UndoManager           |
| juce_events          | Message loop, timers, threads    |
| juce_graphics        | 2D graphics, fonts, images       |
| juce_gui_basics      | Components, windows, layout      |
| juce_gui_extra       | Extra widgets (WebView, etc.)    |

## Next Steps

- Add UI components in `MainComponent`
- Add audio processing by including `juce_audio_basics`, `juce_audio_devices`, `juce_audio_processors`
- Add DSP by including `juce_dsp`
