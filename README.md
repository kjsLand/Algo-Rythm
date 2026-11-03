# Algo-Rythm
Auto generate MIDI files to give music producers a jumping off point during the creation of a beat. 

## Run the app

You need CMake 3.22+, Git and a C++17 compiler. Run all commands from the `plugin/` folder.

Get JUCE once, the first time you build:

```bash
cd plugin
git clone --depth 1 --branch 8.0.13 https://github.com/juce-framework/JUCE.git
```

### Windows

Use Command Prompt. This needs Visual Studio 2022 with the "Desktop development with C++" workload.

```bat
run.bat
build\AlgoRythm_artefacts\Debug\Algo-Rythm.exe
```

### macOS

```bash
sh run.sh
./build/AlgoRythm_artefacts/Debug/Algo-Rythm.app/Contents/MacOS/Algo-Rythm
```

### Ubuntu

Install the system packages listed in [plugin/README.md](plugin/README.md#ubuntu) first.

```bash
sh run.sh
./build/AlgoRythm_artefacts/Debug/Algo-Rythm
```

The first build compiles JUCE and takes a few minutes. After a code change, run the same two commands again.
