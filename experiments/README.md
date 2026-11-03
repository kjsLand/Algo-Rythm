# External model experiments (T16-08)

This folder is for experiments with external symbolic music models. The app and the release builds do not use this folder.

## Status

No experiment has run yet. Each experiment needs a person to choose the model, check its licence, run it and rate 20 results.

| Experiment | Model | Licence checked | Results rated | Decision |
| --- | --- | --- | --- | --- |
| Chord-aware MIDI model with generator progressions | Not chosen | No | 0 of 20 | Open |
| MIDI-to-tab model as a second opinion for the voicing engine | Not chosen | No | 0 of 20 | Open |

## Procedure

1. Before each experiment, write the model name, its licence and the licence of its training data in [LICENCES.md](LICENCES.md) (REQ-16-5).
2. If the licence is non-commercial, keep the model and its code in `experiments/` only. Do not ship it in a release.
3. Put each experiment in its own folder, for example `experiments/chord-model/`, with a `README.md` and a `requirements.txt` with pinned versions.
4. Make the inputs with the CLI, so that each input has a seed:

   ```bash
   algo-cli batch --count 20 --start-seed 5000 --formats json,mid --out experiments/inputs
   ```

5. **Chord-aware MIDI model:** give it the progression of each song starter (`index.csv` has the verse progression). Keep its output as MIDI.
6. **MIDI-to-tab model:** give it the MIDI files of the inputs. Compare its string and fret for each note with the voicings in the JSON files. Count the voicings that are different, and check them with `algo-cli check`.
7. Rate 20 results of each experiment with the A/B method from SPEC-15: turn on A/B Test Mode, or rate blind in a spreadsheet, then run `tools/ab_report.py`.
8. Write the results and one decision in the table above: **adopt**, **change** or **stop**.
