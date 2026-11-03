# SPEC-15: Learning From Data

Milestone: M5
Source: Coding Spec, Section 10 (M5) and the learning guide, Sections 6.5 and 9
Depends on: SPEC-10, SPEC-14

## 1. Purpose

This spec replaces the hand-authored transition values with values from data.
The data comes from your song log and from your ratings.
An A/B test compares the rule tables and the learned tables.

## 2. Scope

In scope:

- The song log format.
- The analysis script that makes `transitions.json` and strum weights.
- The table selection in the CLI and the app.
- The A/B test and its report.

Out of scope:

- Neural models. Refer to SPEC-16.
- Data from copyrighted tab files.

## 3. Requirements

| ID | Requirement |
| --- | --- |
| REQ-15-1 | The song log must store progressions as Nashville numbers, not as chord names. |
| REQ-15-2 | Each learned file must record its sources, its date and the number of songs. |
| REQ-15-3 | Each learned transition row must add up to 1.0 within 0.001. |
| REQ-15-4 | The user must be able to select the rule tables or the learned tables. |
| REQ-15-5 | The A/B test must hide the table type from the user until the user saves the rating. |

## 4. Tasks

### T15-01: Define the song log format

- **Status:** [x] Done
- **Prerequisites:** T04-04
- **Files:** `data/songlog/README.md`, `data/songlog/schema.json`, `data/songlog/example.json`

**Procedure:**

1. Write a JSON schema for one song entry.
2. Include title, artist, key, mode, BPM, time signature and form.
3. For each section, include the name, the bar count, the Nashville numerals and the strum pattern.
4. Write one example entry for a song that you learned in the guitar plan.
5. Write a README that tells you how to add a song after you learn it.

**Acceptance:**

- [x] The example entry passes the schema check.
- [x] REQ-15-1 is true for the schema.

### T15-02: Write the transition analysis script

- **Status:** [x] Done
- **Prerequisites:** T15-01, T14-05
- **Files:** `tools/analyze_corpus.py`, `tools/requirements.txt`, `tests/tools/test_analyze_corpus.py`

**Procedure:**

1. Read all song log entries and `ratings.csv`.
2. Count each numeral-to-numeral change in each section loop.
3. Include the change from the last chord to the first chord.
4. Count changes in major keys and minor keys separately.
5. Give each rated song starter a weight.
6. Use rating 5 = 2.0, 4 = 1.0, 3 = 0.5, and 1 or 2 = 0.
7. Add 0.5 to each count (Laplace smoothing) for the diatonic numerals.
8. Divide each row by its total.
9. Write `transitions.learned.json` with the fields from REQ-15-2.

**Acceptance:**

- [x] A test with 3 known songs gives the expected values.
- [x] REQ-15-3 is true for the output.

### T15-03: Write the strum weight analysis

- **Status:** [x] Done
- **Prerequisites:** T15-02
- **Files:** `tools/analyze_corpus.py`

**Procedure:**

1. Count how often each library strum pattern occurs in the song log and in high-rated song starters.
2. Write a weight for each pattern to `strums.learned.json`.
3. Give each pattern a minimum weight of 0.05 so that no pattern disappears.

**Acceptance:**

- [x] Each pattern in `strums.json` has a weight of 0.05 or more.

### T15-04: Add the table selection

- **Status:** [x] Done
- **Prerequisites:** T15-02, T10-02, T11-03
- **Files:** `core/src/transitions.cpp`, `core/src/strum.cpp`, `cli/SettingsArgs.h`, `app/Source/ControlsPanel.cpp`

**Procedure:**

1. Add the setting `tables` with the values "rules" and "learned".
2. If `tables` is "learned" and the learned files exist, load them.
3. If the learned files do not exist, use the rule tables and show a warning.
4. Add the option `--tables` to the CLI.
5. Add a "Tables" combo box to the app settings menu.

**Acceptance:**

- [x] REQ-15-4 is true in the CLI and in the app.

### T15-05: Write the A/B test mode

- **Status:** [x] Done
- **Prerequisites:** T15-04, T14-05
- **Files:** `app/Source/AbTest.cpp`, `app/Source/ControlsPanel.cpp`

**Procedure:**

1. Add a menu item "A/B Test Mode".
2. In this mode, choose "rules" or "learned" at random for each Generate click.
3. Hide the table type in the user interface.
4. After the user saves a rating, add the table type to the line in `ratings.csv`.

**Acceptance:**

- [x] REQ-15-5 is true.
- [x] Over 100 clicks, each table type occurs 40 to 60 times.

### T15-06: Write the A/B report

- **Status:** [x] Done
- **Prerequisites:** T15-05
- **Files:** `tools/ab_report.py`

**Procedure:**

1. Read `ratings.csv`.
2. Keep only the lines with a table type.
3. Calculate the count, mean rating and standard deviation for each table type.
4. Calculate a two-sided Mann-Whitney U test between the two groups.
5. Print the result as a short table.
6. If a group has fewer than 20 ratings, print a warning that the result is not reliable.

**Acceptance:**

- [x] The report runs on a test file with 40 ratings and prints the expected values.

## 5. Done criteria

- [x] All tasks in this spec are Done.
- [ ] You collect 40 or more A/B ratings and decide which table type to use by default.
