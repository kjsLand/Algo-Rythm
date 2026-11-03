# Algo-Rythm
Auto generate MIDI files to give music producers a jumping off point during the creation of a beat. 

## Build and test

You need CMake 3.22+, Git and a C++17 compiler. CMake downloads JUCE, Catch2 and nlohmann/json at the versions pinned in [cmake/Dependencies.cmake](cmake/Dependencies.cmake).

- **Windows:** Install Visual Studio 2022 with the "Desktop development with C++" workload. Use Command Prompt.
- **macOS:** Install the Xcode Command Line Tools with `xcode-select --install`.
- **Ubuntu:** Install the system packages listed in [plugin/README.md](plugin/README.md#ubuntu).

```bash
git clone https://github.com/kjsLand/Algo-Rythm.git
cd Algo-Rythm
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

The first build compiles JUCE and takes a few minutes.

## Run the app

Run the command for your operating system from the repository root:

| Operating system | Command |
| --- | --- |
| Windows | `build\plugin\AlgoRythm_artefacts\Debug\Algo-Rythm.exe` |
| macOS | `./build/plugin/AlgoRythm_artefacts/Debug/Algo-Rythm.app/Contents/MacOS/Algo-Rythm` |
| Ubuntu | `./build/plugin/AlgoRythm_artefacts/Debug/Algo-Rythm` |

After a code change, run the build command again. `plugin/run.bat` and `plugin/run.sh` do the configure and build steps.

## Build options

Add these options to the first `cmake` command, for example `-DALGO_BUILD_APP=OFF` to build only the tests without JUCE.

| Option | Default | Target |
| --- | --- | --- |
| `ALGO_BUILD_APP` | ON | The desktop app |
| `ALGO_BUILD_CLI` | ON | The `algo-cli` tool |
| `ALGO_BUILD_TESTS` | ON | The unit tests |
| `ALGO_BUILD_PLUGIN` | OFF | The audio plugin |

## Tests and code format

CI runs on each push and each pull request to `main`. It must pass on Ubuntu, macOS and Windows.

- **Property tests:** `forEachSeed` in [tests/support/PropertyTest.h](tests/support/PropertyTest.h) runs 1,000 seeds by default. CI sets `ALGO_PROP_SEEDS=10000`.
- **Code format:** Use the clang-format version pinned in [.github/workflows/ci.yml](.github/workflows/ci.yml). CI fails if a C++ file does not obey [.clang-format](.clang-format).
