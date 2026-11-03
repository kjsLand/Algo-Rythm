#!/usr/bin/env python3
"""Writes a Guitar Pro 5 file from a song starter JSON file (T16-05).

Usage: export_gp.py starter.json [--out starter.gp5]

The GP5 song has one guitar track in the tuning of the song starter, with the capo. Each strum slot is one
beat with the string and the fret of each note. A Mute is a dead note. A Rest or a Chuck is a rest beat.
Each section name is a marker. Needs PyGuitarPro (tools/requirements.txt).
"""
import argparse
import json
import sys
from pathlib import Path

import guitarpro

NUM_STRINGS = 6
NOTE_NAMES = ["C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"]


def duration_for(steps, numerator, denominator):
    """The Duration of one slot. A 4/4 bar of 8 steps has eighth notes and 16 steps has sixteenth notes."""
    whole_fraction = numerator / denominator  # The length of a bar in whole notes
    value = round(steps / whole_fraction)     # 1 = whole, 2 = half, 4 = quarter, 8 = eighth
    return guitarpro.models.Duration(value=value)


def pluck_string(frets, char):
    """The string index (0 = low E) of a Pluck, as in pluckStringIndex in core/src/strum.cpp."""
    sounding = [s for s in range(NUM_STRINGS) if frets[s] >= 0]
    if not sounding:
        return None
    lowest = sounding[0]
    if char == "b":
        return lowest
    if char == "a":
        above = [s for s in sounding if s >= lowest + (1 if lowest >= 1 else 2)]
        return above[0] if above else lowest
    wanted = NUM_STRINGS - int(char)
    for d in range(NUM_STRINGS):
        for s in (wanted + d, wanted - d):
            if s in sounding:
                return s
    return None


def load(path):
    with open(path, encoding="utf-8") as stream:
        starter = json.load(stream)
    if starter.get("schemaVersion", 0) < 1 or "sections" not in starter:
        raise ValueError(f"{path} is not a song starter JSON file")
    return starter


def build_song(starter):
    song = guitarpro.models.Song()
    key = NOTE_NAMES[starter["key"]["root"] % 12]
    song.title = f"{key} {starter['key']['scale']} song starter"
    song.artist = "Algo-Rythm"
    song.comments = [f"Seed {starter['seed']}, {starter['difficulty']}, {starter['form']}"]
    song.tempo = int(starter["bpm"])

    track = song.tracks[0]
    track.name = "Guitar"
    track.offset = int(starter.get("capo", 0))
    # GP strings count from the high E string. The song starter lists the low E string first.
    tuning = starter["tuning"]["strings"]
    track.strings = [guitarpro.models.GuitarString(number=n + 1, value=tuning[NUM_STRINGS - 1 - n]) for n in range(NUM_STRINGS)]
    track.channel.instrument = 25  # Steel string guitar

    numerator = starter["timeSig"]["numerator"]
    denominator = starter["timeSig"]["denominator"]

    bars = [(section, bar_index, bar) for section in starter["sections"] for bar_index, bar in enumerate(section["bars"])]
    for number, (section, bar_index, bar) in enumerate(bars):
        if number > 0:
            header = guitarpro.models.MeasureHeader()
            song.addMeasureHeader(header)
        header = song.measureHeaders[number]
        header.timeSignature.numerator = numerator
        header.timeSignature.denominator.value = denominator
        if bar_index == 0:
            header.marker = guitarpro.models.Marker(title=section["name"])

    for track_measure_count in range(len(track.measures), len(song.measureHeaders)):
        track.measures.append(guitarpro.models.Measure(track, song.measureHeaders[track_measure_count]))

    for number, (section, _, bar) in enumerate(bars):
        measure = track.measures[number]
        voice = measure.voices[0]
        voice.beats = []
        strum = section["strum"]
        steps = strum["steps"]
        sounding = strum.get("strings", NUM_STRINGS)
        voicings = bar["voicings"]

        for slot, stroke in enumerate(strum["slots"]):
            beat = guitarpro.models.Beat(voice, duration=duration_for(steps, numerator, denominator))
            if stroke in "-c" or not voicings:
                beat.status = guitarpro.models.BeatStatus.rest
            else:
                beat.status = guitarpro.models.BeatStatus.normal
                frets = voicings[min(len(voicings) - 1, slot * len(voicings) // steps)]["frets"]
                plucked = pluck_string(frets, stroke) if stroke in "123456ba" else None
                for s in range(NUM_STRINGS - sounding, NUM_STRINGS):
                    if frets[s] < 0 or (stroke in "123456ba" and s != plucked):
                        continue
                    note = guitarpro.models.Note(beat, value=frets[s], string=NUM_STRINGS - s, velocity=95)
                    note.type = guitarpro.models.NoteType.dead if stroke == "x" else guitarpro.models.NoteType.normal
                    beat.notes.append(note)
                if stroke in "DUx":
                    beat.effect.pickStroke = guitarpro.models.BeatStrokeDirection.up if stroke == "U" else guitarpro.models.BeatStrokeDirection.down
            voice.beats.append(beat)
    return song


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("starter", help="A song starter JSON file")
    parser.add_argument("--out", help="The GP5 file (default: the JSON file name with .gp5)")
    args = parser.parse_args(argv)

    try:
        starter = load(args.starter)
    except (OSError, ValueError, KeyError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 2

    out = args.out or str(Path(args.starter).with_suffix(".gp5"))
    guitarpro.write(build_song(starter), out, version=(5, 1, 0))
    print(out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
