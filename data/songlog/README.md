# Song log

The song log is a set of JSON files, one for each song that you learned on guitar.
`tools/analyze_corpus.py` reads the song log and your ratings, and makes the learned tables (SPEC-15).

## Rules

- Write each progression as Nashville numbers, not as chord names (REQ-15-1). Use `1`, `4`, `5`, `6m`, `b7` or `5/7`.
- Write the numbers relative to the mode of the song: `major` or `minor`.
- Write one loop of each section. The analysis also counts the change from the last chord back to the first chord.
- Do not copy tab files. Write only the facts that you learned: the key, the tempo, the form and the numbers.

## Add a song after you learn it

1. Copy `example.json` to a new file, for example `my-song.json`, in this folder.
2. Change the title, the artist, the key, the mode, the BPM and the time signature.
3. For each section, write the name, the bar count, the numbers of one loop and the strum of one bar.
4. Check the file:

   ```bash
   python3 tools/analyze_corpus.py --check data/songlog
   ```

5. Make the learned tables again:

   ```bash
   python3 tools/analyze_corpus.py --songlog data/songlog --ratings ~/Algo-Rythm/ratings.csv
   ```

The script writes `transitions.learned.json` and `strums.learned.json` to `~/Algo-Rythm/learned/`.
Select the learned tables with `algo-cli generate --tables learned`, or in the app with Settings > Tables.

## Fields

`schema.json` defines each field. An editor that knows JSON Schema can check a file as you write it.

| Field | Meaning |
| --- | --- |
| `key` | The root of the key, for example `G` or `F#` |
| `mode` | `major` or `minor`. This selects the transition table. |
| `timeSignature` | For example `4/4` or `6/8` |
| `sections[].numerals` | One loop of the section as Nashville numbers |
| `sections[].strum` | One bar: `D` down, `U` up, `-` rest, `x` mute, `c` chuck |
