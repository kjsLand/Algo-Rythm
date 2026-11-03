# Sandbox

This folder is for experiments only. The app and the tests do not use these files.

- `rand_period.py` makes a random trigonometric curve and plots it.
- `midi_converter.py` maps a curve to the notes of a scale and writes a MIDI file.

## Run

```bash
python -m venv .venv
.venv/bin/pip install -r sandbox/requirements.txt
.venv/bin/python sandbox/midi_converter.py
```

On Windows, use `.venv\Scripts\` in place of `.venv/bin/`.

The script writes `piano_melody.mid` in the current folder. Git ignores this file.
