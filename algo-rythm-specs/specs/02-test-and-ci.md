# SPEC-02: Test Framework and Continuous Integration

Milestone: M0
Source: Coding Spec, Section 8 (Conventions) and Section 9 (Testing)
Depends on: SPEC-01 (T01-01)

## 1. Purpose

This spec adds a unit test framework and a CI pipeline.
All later specs add tests to this framework.
CI must build and test each commit on macOS, Windows and Ubuntu.

## 2. Scope

In scope:

- The top-level CMake project and its build options.
- Catch2 unit tests and the property-test helper.
- The GitHub Actions workflow.
- Code format rules.

Out of scope:

- The tests for each feature. Each feature spec contains its tests.
- Release builds and code signing. Refer to SPEC-14.

## 3. Requirements

| ID | Requirement |
| --- | --- |
| REQ-02-1 | One top-level CMake project must build the core, app, CLI, tests and plugin. Options must turn each target on or off. |
| REQ-02-2 | All external dependencies must have a pinned version. |
| REQ-02-3 | `ctest` must run all tests. |
| REQ-02-4 | CI must run on each push and each pull request to `main`. |
| REQ-02-5 | CI must fail if a test fails or the code format is incorrect. |
| REQ-02-6 | Property tests must run 1,000 seeds on a local build and 10,000 seeds in CI. |

## 4. Tasks

### T02-01: Add the top-level CMake project and Catch2

- **Status:** [x] Done
- **Prerequisites:** T01-01
- **Files:** `CMakeLists.txt`, `cmake/Dependencies.cmake`, `tests/CMakeLists.txt`, `tests/smoke_test.cpp`

**Procedure:**

1. Write a root `CMakeLists.txt` with C++17 and the project name `AlgoRythm`.
2. Add the options `ALGO_BUILD_APP`, `ALGO_BUILD_CLI`, `ALGO_BUILD_TESTS` and `ALGO_BUILD_PLUGIN`.
3. Set the default of `ALGO_BUILD_PLUGIN` to OFF.
4. Set the other defaults to ON.
5. In `cmake/Dependencies.cmake`, get Catch2 v3 with `FetchContent`. Pin a release tag.
6. Add `tests/CMakeLists.txt` with one target `algo_tests`.
7. Write `tests/smoke_test.cpp` with one test that passes.
8. Call `enable_testing()` and `catch_discover_tests(algo_tests)`.

**Acceptance:**

- [x] `cmake -B build -DALGO_BUILD_APP=OFF && cmake --build build && ctest --test-dir build` passes.
- [x] The build does not download JUCE when `ALGO_BUILD_APP` and `ALGO_BUILD_PLUGIN` are OFF.

### T02-02: Get JUCE with a pinned version

- **Status:** [ ] Open
- **Prerequisites:** T02-01, T01-04
- **Files:** `cmake/Dependencies.cmake`, `plugin/CMakeLists.txt`, `README.md`

**Procedure:**

1. In `cmake/Dependencies.cmake`, get JUCE with `FetchContent`. Pin a release tag.
2. Get JUCE only if `ALGO_BUILD_APP` or `ALGO_BUILD_PLUGIN` is ON.
3. Remove the `add_subdirectory(JUCE)` line from `plugin/CMakeLists.txt`.
4. Add the app folder from the root `CMakeLists.txt`.
5. In the root `README.md`, remove the step that tells the user to clone JUCE manually.

**Acceptance:**

- [ ] A clean clone builds the app with `cmake -B build && cmake --build build`.
- [ ] The JUCE version occurs in one location only.

### T02-03: Add the CI workflow

- **Status:** [ ] Open
- **Prerequisites:** T02-01
- **Files:** `.github/workflows/ci.yml`

**Procedure:**

1. Write a workflow that starts on `push` and `pull_request` to `main`.
2. Add a job matrix with `ubuntu-latest`, `macos-latest` and `windows-latest`.
3. On Ubuntu, install the JUCE system packages with `apt-get`.
4. Configure with CMake in Release mode.
5. Build all targets that are ON.
6. Run `ctest --output-on-failure`.
7. Set the environment variable `ALGO_PROP_SEEDS=10000` for the test step.
8. Cache the FetchContent download folder.

**Acceptance:**

- [ ] The workflow passes on all three operating systems.
- [ ] A test that you make fail on purpose causes the workflow to fail.

### T02-04: Add the code format rules and check

- **Status:** [ ] Open
- **Prerequisites:** T02-03
- **Files:** `.clang-format`, `.github/workflows/ci.yml`

**Procedure:**

1. Write a `.clang-format` file that matches the current code style.
2. Set `SpaceBeforeParens: Always`, `IndentWidth: 4` and `BreakBeforeBraces: Allman`.
3. Run `clang-format -i` on all C++ files.
4. Commit the result in a separate commit.
5. Add a CI job `format` that runs `clang-format --dry-run --Werror` on all C++ files.

**Acceptance:**

- [ ] The `format` job passes on `main`.
- [ ] A file with incorrect format causes the `format` job to fail.

### T02-05: Add the property-test helper

- **Status:** [ ] Open
- **Prerequisites:** T02-01
- **Files:** `tests/support/PropertyTest.h`, `tests/property_helper_test.cpp`

**Procedure:**

1. Write a function `propertySeedCount()`.
2. Make the function read the environment variable `ALGO_PROP_SEEDS`.
3. If the variable is not set, return 1000.
4. Write a function `forEachSeed(fn)` that calls `fn` with seeds 1 to `propertySeedCount()`.
5. If `fn` fails for a seed, write the seed in the failure message.

**Acceptance:**

- [ ] A failed property test shows the seed that caused the failure.
- [ ] With `ALGO_PROP_SEEDS=5`, `forEachSeed` calls the function 5 times.

### T02-06: Add compiler warnings for the core

- **Status:** [ ] Open
- **Prerequisites:** T02-01
- **Files:** `cmake/Warnings.cmake`

**Procedure:**

1. Write a CMake function `algo_set_warnings(target)`.
2. For GCC and Clang, add `-Wall -Wextra -Wpedantic -Werror`.
3. For MSVC, add `/W4 /WX`.
4. Do not apply these warnings to JUCE or to third-party code.

**Acceptance:**

- [ ] A target with `algo_set_warnings` fails to compile if it has an unused variable.
- [ ] The JUCE modules compile without errors from these flags.

## 5. Done criteria

- [ ] All tasks in this spec are Done.
- [ ] CI passes on all three operating systems.
- [ ] The README tells a new developer how to build and test in 5 commands or fewer.
