# Algo-Rythm
Auto generate MIDI files to give music producers a jumping off point during the creation of a beat. 

## Build and test

You need CMake 3.22+, Git and a C++17 compiler. CMake downloads JUCE, Catch2 and nlohmann/json at the versions pinned in [cmake/Dependencies.cmake](cmake/Dependencies.cmake).

- **Windows:** Install Visual Studio 2022 with the "Desktop development with C++" workload. Use Command Prompt.
- **macOS:** Install the Xcode Command Line Tools with `xcode-select --install`.
- **Ubuntu:** Install the system packages listed in [app/README.md](app/README.md#ubuntu).

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
| Windows | `build\app\AlgoRythm_artefacts\Debug\Algo-Rythm.exe` |
| macOS | `./build/app/AlgoRythm_artefacts/Debug/Algo-Rythm.app/Contents/MacOS/Algo-Rythm` |
| Ubuntu | `./build/app/AlgoRythm_artefacts/Debug/Algo-Rythm` |

After a code change, run the build command again. `app/run.bat` and `app/run.sh` do the configure and build steps.

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

## Command-line tool

`algo-cli` generates song starters with no user interface. It links only to the core library (REQ-10-1).
After a build, it is at `build/cli/algo-cli` (`build\cli\Debug\algo-cli.exe` on Windows).
Each command has help text: `algo-cli <command> --help`. The exit code is 0 on success, 1 on a check failure and 2 on a usage error.

**generate** writes one song starter in each format. If you do not give `--seed`, the system clock gives one and the command prints it.

```bash
algo-cli generate --key G --mood bright --bpm 90-110 --form vcb --difficulty beginner --seed 81723 --out songs
```

Other options: `--scale`, `--capo`, `--time 6/8`, `--tuning DADGAD`, `--hook`, `--fingerpicking`, `--tables learned` and `--formats json,mid,tab,musicxml`.

**render** converts a song starter JSON file to MIDI (`.mid`), ASCII tab (`.txt`) or MusicXML (`.musicxml`):

```bash
algo-cli render songs/G-Major-96bpm-81723.json --out starter.mid
```

**batch** writes many song starters and an `index.csv` file with the seed, key, scale, BPM, form and verse progression:

```bash
algo-cli batch --count 200 --start-seed 1000 --difficulty beginner --out batch
```

**check** checks song starter JSON files against the voicing, chord, key, strum and hook rules. It prints each failure on its own line:

```bash
algo-cli check songs/*.json
```

The global options `--data <folder>` (other data files) and `--learned <folder>` (the learned tables) come before the command.

## Desktop app

The app is in `app/`. All music logic comes from the core library.

| Key | Action |
| --- | --- |
| G | Generate |
| 1 to 4 | Select section 1 to 4 |
| Space | Play and Stop |
| S | Save to the library |
| E | Export MIDI |
| Left and Right | Select a bar in the song overview |

- Drag the handle on a section tab into your DAW to make a MIDI clip of that section.
- A lock next to a control keeps its value when you click Generate. Each section tab has locks for chords, voicings and strum.
- "New progression", "New voicing" and "New strum" change only the selected section.
- The library is in `~/Algo-Rythm/library/`. A rating also adds a line to `~/Algo-Rythm/ratings.csv`.
- The Settings menu has Audio Settings, the table type (rules or learned), A/B Test Mode, Fingerpicking Patterns and Left-handed.
- If the computer has no audio device, the app still runs and shows a message in the status bar.

## Plugin

Build the VST3, AU (macOS) and Standalone plugin with `-DALGO_BUILD_PLUGIN=ON`. The plugin follows the host tempo and position, sends the song starter as MIDI and can play it with the plucked-string synthesizer ("Preview Sound"). CI runs `pluginval` at strictness level 5 on macOS and Windows. Refer to [plugin/README.md](plugin/README.md).

## Learning from data

1. Add a JSON file to [data/songlog](data/songlog/README.md) for each song that you learn.
2. Rate song starters in the app. Each rating goes into `~/Algo-Rythm/ratings.csv`.
3. Make the learned tables:

   ```bash
   python3 tools/analyze_corpus.py
   ```

4. Select the learned tables with `--tables learned` or Settings > Tables > Learned tables.
5. Turn on Settings > A/B Test Mode, rate 40 or more song starters, then compare the table types:

   ```bash
   python3 tools/ab_report.py ~/Algo-Rythm/ratings.csv
   ```

`tools/export_gp.py starter.json` writes a Guitar Pro 5 file. It needs the packages in [tools/requirements.txt](tools/requirements.txt).

## Releases

Push a tag such as `v0.1.0` to make a release with a macOS `.dmg` file and a Windows `.zip` file. Refer to [docs/RELEASE.md](docs/RELEASE.md).
