# SPEC-10: Command-Line Tool (algo-cli)

Milestone: M2
Source: Coding Spec, Section 7.2
Depends on: SPEC-09

## 1. Purpose

This spec makes the command-line tool `algo-cli`.
You use the tool to generate song starters with no user interface.
CI uses the tool for smoke tests and for checks on saved files.

## 2. Scope

In scope:

- The commands `generate`, `render`, `batch` and `check`.
- The argument parser, the help text and the exit codes.

Out of scope:

- The data analysis tools. Refer to SPEC-15.

## 3. Requirements

| ID | Requirement |
| --- | --- |
| REQ-10-1 | The CLI must link only to `algo::core`. It must not link to JUCE. |
| REQ-10-2 | Each command must have help text that `--help` shows. |
| REQ-10-3 | The CLI must exit with code 0 on success, 1 on a check failure and 2 on a usage error. |
| REQ-10-4 | The CLI must write errors to standard error. It must write results to standard output or to files. |
| REQ-10-5 | The same command and seed must write the same files on each run. |

## 4. Tasks

### T10-01: Make the CLI target

- **Status:** [ ] Open
- **Prerequisites:** T09-07
- **Files:** `cli/CMakeLists.txt`, `cli/main.cpp`, `cmake/Dependencies.cmake`

**Procedure:**

1. Get CLI11 with `FetchContent`.
2. Pin the CLI11 version to a release tag.
3. Write `cli/CMakeLists.txt` with the executable target `algo-cli`.
4. Link the target to `algo::core` and CLI11 only.
5. Add the subcommands `generate`, `render`, `batch` and `check`. Leave each one empty.
6. Add the global options `--data <folder>` and `--version`.
7. Build the target only when `ALGO_BUILD_CLI` is ON.

**Acceptance:**

- [ ] `algo-cli --version` prints "0.1.0".
- [ ] `algo-cli --help` lists the 4 subcommands.

### T10-02: Write the generate command

- **Status:** [ ] Open
- **Prerequisites:** T10-01, T09-08
- **Files:** `cli/cmd_generate.cpp`, `cli/SettingsArgs.h`

**Procedure:**

1. Add the options `--key`, `--scale`, `--mood`, `--bpm`, `--form`, `--difficulty`, `--capo`, `--seed`, `--hook`, `--out` and `--formats`.
2. Accept a BPM range in the form "90-110" and a single value in the form "96".
3. Accept the form codes "vc", "vcb" and "loop".
4. Convert the options to `GenerationSettings`.
5. If `--seed` is not given, make a seed from the system clock. Print the seed to standard output.
6. Call `generate`, then `exportAll`.
7. Print the paths of the files that the command wrote.

**Acceptance:**

- [ ] The example command in Coding Spec Section 7.2 writes 3 files.
- [ ] Two runs with the same seed write identical files.
- [ ] An incorrect key name gives exit code 2 and a clear error.

### T10-03: Write the render command

- **Status:** [ ] Open
- **Prerequisites:** T10-01, T09-08
- **Files:** `cli/cmd_render.cpp`

**Procedure:**

1. Read a song starter JSON file from the first argument.
2. Add the option `--out <file>`.
3. Choose the format from the file extension of `--out`: ".mid" or ".txt".
4. If the extension is not known, exit with code 2.

**Acceptance:**

- [ ] `algo-cli render starter.json --out starter.mid` writes a MIDI file that the T09-05 reader can read.

### T10-04: Write the batch command

- **Status:** [ ] Open
- **Prerequisites:** T10-02
- **Files:** `cli/cmd_batch.cpp`

**Procedure:**

1. Add the options `--count`, `--start-seed` and the settings options from T10-02.
2. Generate `--count` song starters with seeds from `--start-seed` upward.
3. Write each song starter to `--out` in the formats from `--formats`.
4. Write a file `index.csv` with seed, key, scale, BPM, form and the verse progression.
5. Print the progress after each 50 song starters.

**Acceptance:**

- [ ] `algo-cli batch --count 200` writes 200 song starters and `index.csv` in less than 60 seconds.

### T10-05: Write the check command

- **Status:** [ ] Open
- **Prerequisites:** T10-01, T07-03, T09-08
- **Files:** `cli/cmd_check.cpp`, `core/include/algo/Validate.h`, `core/src/validate.cpp`

**Procedure:**

1. Move the property checks for voicing, chord, key, strum and hook into `core/src/validate.cpp`.
2. Write `validateStarter(starter)`. The function returns a list of failures with section, bar and rule.
3. Make the property tests call `validateStarter`.
4. In the `check` command, load each JSON file from the arguments.
5. Print each failure on its own line.
6. If one or more failures occur, exit with code 1.

**Acceptance:**

- [ ] A changed JSON file with two notes on one string fails with exit code 1.
- [ ] All golden JSON files pass.

### T10-06: Add CLI smoke tests and documentation

- **Status:** [ ] Open
- **Prerequisites:** T10-02, T10-03, T10-04, T10-05
- **Files:** `cli/CMakeLists.txt`, `README.md`

**Procedure:**

1. Add `add_test` entries that run each subcommand with a fixed seed.
2. Add a test that runs `generate` and then `check` on its output.
3. Write a "Command-line tool" section in the root README.
4. Give one example for each subcommand.

**Acceptance:**

- [ ] `ctest` runs the CLI tests in CI on all three operating systems.

## 5. Done criteria

- [ ] All tasks in this spec are Done.
- [ ] You generate 20 Beginner song starters with `batch` and play them on guitar.
