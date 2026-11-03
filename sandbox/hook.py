"""Makes a melody hook with the steps of the C++ hook stage (T13-06).

The steps are the same as core/src/hook.cpp (T13-02 to T13-04):
  1. A random Fourier series gives the contour, as in rand_period.py.
  2. A 16-step Euclidean rhythm with 5 to 9 hits in each bar, rotated by 0 to 3 steps.
  3. The contour goes onto a range of 16 semitones around the 5th of the scale in octave 4.
     Beats 1 and 3 use chord tones. The other notes use scale notes.
  4. The last note is the root or the 3rd of the last chord. A note is not longer than 720 ticks.

Python random numbers are not the C++ Rng, so a seed gives a different hook here. Use the sandbox
to test new contour ideas before you change the C++ code.

Usage: python sandbox/hook.py --seed 1 [--key C] [--progression 1,5,6,4] [--out hook.mid]
"""
import argparse
import math
import random
import sys

import pretty_midi

PPQ = 480
STEP = PPQ // 4
BAR = 16 * STEP
RANGE = 16
MAX_NOTE = 720
LAST_NOTE = 480

NOTES = {"C": 0, "C#": 1, "Db": 1, "D": 2, "D#": 3, "Eb": 3, "E": 4, "F": 5, "F#": 6, "Gb": 6, "G": 7, "G#": 8, "Ab": 8, "A": 9, "A#": 10, "Bb": 10, "B": 11}
MAJOR = [0, 2, 4, 5, 7, 9, 11]


def bjorklund(hits, steps):
    """The Euclidean rhythm as in core/src/euclid.cpp."""
    pattern = [[1] for _ in range(hits)] + [[0] for _ in range(steps - hits)]
    while True:
        count = min(hits, len(pattern) - hits)
        if count <= 1 or len(pattern) - hits <= 1:
            break
        merged = [pattern[i] + pattern[len(pattern) - count + i] for i in range(count)]
        pattern = merged + pattern[count:len(pattern) - count]
        hits = count
    return [bit for group in pattern for bit in group]


def rotate(pattern, n):
    n %= len(pattern)
    return pattern[-n:] + pattern[:-n] if n else pattern


def random_contour(rng):
    n = rng.randint(3, 5)
    a0 = rng.randint(-3, 3)
    while True:
        a = [rng.randint(-3, 3) for _ in range(n)]
        b = [rng.randint(-3, 3) for _ in range(n)]
        if any(a) or any(b):
            return a0, a, b


def evaluate(contour, t):
    a0, a, b = contour
    x = 2 * math.pi * t
    return a0 + sum(a[k] * math.cos((k + 1) * x) + b[k] * math.sin((k + 1) * x) for k in range(len(a)))


def nearest(note, lo, hi, pcs):
    note = max(lo, min(hi, note))
    for d in range(hi - lo + 1):
        for candidate in (note - d, note + d):
            if lo <= candidate <= hi and candidate % 12 in pcs:
                return candidate
    return note


def triad(root, degree):
    scale = [(root + step) % 12 for step in MAJOR]
    return [scale[(degree - 1 + i) % 7] for i in (0, 2, 4)]


def make_hook(seed, key, progression):
    rng = random.Random(seed)
    scale = {(key + step) % 12 for step in MAJOR}
    bars = rng.randint(2, 4)

    starts = []
    for bar in range(bars):
        hits = rng.randint(5, 9)
        pattern = rotate(bjorklund(hits, 16), rng.randint(0, 3))
        starts += [bar * BAR + step * STEP for step, bit in enumerate(pattern) if bit]

    contour = random_contour(rng)
    values = [evaluate(contour, start / (bars * BAR)) for start in starts]
    low, high = min(values), max(values)
    center = 60 + (key + MAJOR[4]) % 12
    lo, hi = center - RANGE // 2, center + RANGE // 2

    notes = []
    for start, value in zip(starts, values):
        normal = (value - low) / (high - low) if high > low else 0.5
        raw = lo + round(normal * RANGE)
        strong = start % BAR in (0, 2 * PPQ)
        chord = triad(key, progression[(start // BAR) % len(progression)])
        pitch = nearest(raw, lo, hi, set(chord) if strong else scale)
        notes.append([pitch, start, 0, 90 if strong else 75])

    last_chord = triad(key, progression[(notes[-1][1] // BAR) % len(progression)])
    notes[-1][0] = nearest(notes[-1][0], lo, hi, {last_chord[0], last_chord[1]})
    for i, note in enumerate(notes):
        note[2] = min(notes[i + 1][1] - note[1], MAX_NOTE) if i + 1 < len(notes) else LAST_NOTE
    return notes, scale, (lo, hi), progression


def check(notes, scale, key, progression):
    """Returns the rules of REQ-13-1 to REQ-13-5 that the hook breaks."""
    failures = []
    if any(pitch % 12 not in scale for pitch, _, _, _ in notes):
        failures.append("REQ-13-1: a note is not in the scale")
    for pitch, start, _, _ in notes:
        if start % BAR in (0, 2 * PPQ) and pitch % 12 not in triad(key, progression[(start // BAR) % len(progression)]):
            failures.append("REQ-13-2: a note on beat 1 or 3 is not a chord tone")
    pitches = [pitch for pitch, _, _, _ in notes]
    if max(pitches) - min(pitches) > RANGE:
        failures.append("REQ-13-3: the range is more than a major 10th")
    last = triad(key, progression[(notes[-1][1] // BAR) % len(progression)])
    if notes[-1][0] % 12 not in (last[0], last[1]):
        failures.append("REQ-13-4: the last note is not the root or the 3rd")
    if any(length > MAX_NOTE for _, _, length, _ in notes):
        failures.append("REQ-13-5: a note is longer than a dotted quarter note")
    return failures


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--key", default="C", choices=sorted(NOTES))
    parser.add_argument("--progression", default="1,5,6,4", help="One diatonic numeral for each bar")
    parser.add_argument("--bpm", type=float, default=100.0)
    parser.add_argument("--out", default="hook.mid")
    args = parser.parse_args()

    key = NOTES[args.key]
    progression = [int(n) for n in args.progression.split(",")]
    notes, scale, _, _ = make_hook(args.seed, key, progression)

    midi = pretty_midi.PrettyMIDI(initial_tempo=args.bpm, resolution=PPQ)
    piano = pretty_midi.Instrument(program=0, name="Hook")
    seconds = 60.0 / (args.bpm * PPQ)
    for pitch, start, length, velocity in notes:
        piano.notes.append(pretty_midi.Note(velocity=velocity, pitch=pitch, start=start * seconds, end=(start + length) * seconds))
    midi.instruments.append(piano)
    midi.write(args.out)

    failures = check(notes, scale, key, progression)
    print(f"Wrote {args.out}: {len(notes)} notes")
    for failure in failures:
        print(failure)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
