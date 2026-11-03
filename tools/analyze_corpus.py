#!/usr/bin/env python3
"""Makes the learned tables from the song log and the ratings (SPEC-15).

Writes:
  transitions.learned.json  The numeral-to-numeral weights for major and minor keys (T15-02).
  strums.learned.json       A weight for each library strum pattern (T15-03).

Usage:
  analyze_corpus.py [--songlog DIR] [--ratings FILE] [--library DIR] [--out DIR]
  analyze_corpus.py --check DIR      Checks the song log files and writes nothing.

The script uses only the Python standard library.
"""
import argparse
import csv
import datetime
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
HOME = Path.home() / "Algo-Rythm"

DIATONIC = range(1, 8)
LAPLACE = 0.5
MIN_STRUM_WEIGHT = 0.05
RATING_WEIGHT = {5: 2.0, 4: 1.0, 3: 0.5, 2: 0.0, 1: 0.0}
SONG_WEIGHT = 1.0
HIGH_RATING = 4

NUMERAL = re.compile(r"^(b|#)?([1-7])([A-Za-z0-9#]*)(/(b|#)?[1-7])?$")
STRUM = re.compile(r"^[DUxc-]+$")
REQUIRED = ["title", "artist", "key", "mode", "bpm", "timeSignature", "form", "sections"]


class SongLogError(Exception):
    pass


def parse_numeral(text):
    """Returns the scale degree (1 to 7) of a diatonic numeral, or None for a numeral with an accidental."""
    match = NUMERAL.match(text.strip())
    if not match:
        raise SongLogError(f"'{text}' is not a Nashville number")
    if match.group(1):
        return None
    return int(match.group(2))


def check_entry(entry, name):
    """Raises SongLogError if the entry does not obey data/songlog/schema.json."""
    if not isinstance(entry, dict):
        raise SongLogError(f"{name}: the entry must be an object")
    for field in REQUIRED:
        if field not in entry:
            raise SongLogError(f"{name}: the field '{field}' is missing")
    if entry["mode"] not in ("major", "minor"):
        raise SongLogError(f"{name}: the mode must be major or minor")
    if not re.match(r"^[A-G](#|b)?$", str(entry["key"])):
        raise SongLogError(f"{name}: the key must be a root such as G or F#, not a chord name")
    if not isinstance(entry["bpm"], int) or not 30 <= entry["bpm"] <= 300:
        raise SongLogError(f"{name}: the BPM must be an integer from 30 to 300")
    if not re.match(r"^[0-9]+/(2|4|8|16)$", str(entry["timeSignature"])):
        raise SongLogError(f"{name}: the time signature must look like 4/4 or 6/8")
    if not isinstance(entry["sections"], list) or not entry["sections"]:
        raise SongLogError(f"{name}: the song needs 1 or more sections")
    for section in entry["sections"]:
        for field in ("name", "bars", "numerals"):
            if field not in section:
                raise SongLogError(f"{name}: a section has no '{field}'")
        if not isinstance(section["numerals"], list) or not section["numerals"]:
            raise SongLogError(f"{name}: the section '{section['name']}' needs 1 or more numerals")
        for numeral in section["numerals"]:
            parse_numeral(str(numeral))
        if "strum" in section and not STRUM.match(section["strum"]):
            raise SongLogError(f"{name}: the strum of '{section['name']}' must use only D, U, -, x and c")


def load_songlog(folder):
    """Returns (name, entry) for each song log file, in name order. schema.json is not an entry."""
    folder = Path(folder)
    songs = []
    if not folder.is_dir():
        return songs
    for path in sorted(folder.glob("*.json")):
        if path.name == "schema.json":
            continue
        with open(path, encoding="utf-8") as stream:
            try:
                entry = json.load(stream)
            except json.JSONDecodeError as error:
                raise SongLogError(f"{path.name}: {error}") from error
        check_entry(entry, path.name)
        songs.append((path.name, entry))
    return songs


def minor_scales(scales_file):
    with open(scales_file, encoding="utf-8") as stream:
        data = json.load(stream)
    return {scale["name"] for scale in data["scales"] if "minor" in scale.get("tags", [])}


def parse_progressions(text):
    """'Verse: 1 5 6m 4 | Chorus: 4 1 5 6m' gives [['1', '5', '6m', '4'], ['4', '1', '5', '6m']]."""
    loops = []
    for part in text.split("|"):
        if ":" in part:
            part = part.split(":", 1)[1]
        numerals = part.split()
        if numerals:
            loops.append(numerals)
    return loops


def load_ratings(path, minor):
    """Returns (mode, loops, weight) for each line of ratings.csv with a positive weight, and the line count."""
    rows = []
    count = 0
    if path is None or not Path(path).is_file():
        return rows, count
    with open(path, encoding="utf-8", newline="") as stream:
        for line in csv.DictReader(stream):
            count += 1
            try:
                rating = int(line["rating"])
            except (KeyError, ValueError):
                continue
            weight = RATING_WEIGHT.get(rating, 0.0)
            if weight <= 0.0:
                continue
            mode = "minor" if line.get("scale", "") in minor else "major"
            rows.append((mode, parse_progressions(line.get("progressions", "")), weight))
    return rows, count


def add_loop(counts, loop, weight):
    """Counts each change of the loop, with the change from the last chord to the first chord."""
    degrees = []
    for numeral in loop:
        degrees.append(parse_numeral(numeral))
    for i, current in enumerate(degrees):
        following = degrees[(i + 1) % len(degrees)]
        if current is None or following is None or current == following:
            continue
        counts[current][following] += weight


def transition_tables(songs, ratings):
    counts = {mode: {a: {b: 0.0 for b in DIATONIC} for a in DIATONIC} for mode in ("major", "minor")}
    for _, entry in songs:
        for section in entry["sections"]:
            add_loop(counts[entry["mode"]], section["numerals"], SONG_WEIGHT)
    for mode, loops, weight in ratings:
        for loop in loops:
            add_loop(counts[mode], loop, weight)

    tables = []
    for mode in ("major", "minor"):
        rows = {}
        for a in DIATONIC:
            # Laplace smoothing: each diatonic numeral that can follow gets 0.5 more.
            smoothed = {b: counts[mode][a][b] + LAPLACE for b in DIATONIC if b != a}
            total = sum(smoothed.values())
            row = {str(b): round(value / total, 6) for b, value in smoothed.items()}
            # Put the rounding error on the largest weight, so the row adds up to 1.0.
            largest = max(row, key=lambda key: row[key])
            row[largest] = round(row[largest] + 1.0 - sum(row.values()), 6)
            rows[str(a)] = row
        tables.append({"name": mode, "rows": rows})
    return tables


def strum_weights(strums_file, songs, library_folder):
    """A weight for each library strum: how often it occurs in the song log and in high-rated song starters."""
    with open(strums_file, encoding="utf-8") as stream:
        library = json.load(stream)["strums"]
    by_slots = {entry["slots"]: entry["name"] for entry in library}
    counts = {entry["name"]: 0.0 for entry in library}

    for _, entry in songs:
        for section in entry["sections"]:
            name = by_slots.get(section.get("strum", ""))
            if name:
                counts[name] += SONG_WEIGHT

    starters = 0
    if library_folder is not None and Path(library_folder).is_dir():
        for path in sorted(Path(library_folder).glob("*.json")):
            try:
                with open(path, encoding="utf-8") as stream:
                    starter = json.load(stream)
            except (OSError, json.JSONDecodeError):
                continue
            rating = starter.get("rating")
            if not isinstance(rating, int) or rating < HIGH_RATING:
                continue
            starters += 1
            for section in starter.get("sections", []):
                name = section.get("strum", {}).get("name", "")
                if name in counts:
                    counts[name] += RATING_WEIGHT[rating]

    # With no data, each pattern has the weight 1.0. A pattern never falls below the minimum.
    total = sum(counts.values())
    size = len(counts)
    weights = {}
    for name, count in counts.items():
        share = (count + LAPLACE) / (total + LAPLACE * size) * size
        weights[name] = round(max(MIN_STRUM_WEIGHT, share), 6)
    return weights, starters


def metadata(sources, songs, rating_lines, date):
    return {
        "schemaVersion": 1,
        "source": f"learned from {songs} songs and {rating_lines} ratings on {date}",
        "sources": sources,
        "date": date,
        "songs": songs,
        "ratings": rating_lines,
    }


def run(songlog, ratings_file, library_folder, strums_file, scales_file, out, date):
    songs = load_songlog(songlog)
    ratings, rating_lines = load_ratings(ratings_file, minor_scales(scales_file))

    sources = [f"songlog/{name}" for name, _ in songs]
    if ratings_file is not None and Path(ratings_file).is_file():
        sources.append(str(ratings_file))

    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)

    transitions = metadata(sources, len(songs), rating_lines, date)
    transitions["tables"] = transition_tables(songs, ratings)
    with open(out / "transitions.learned.json", "w", encoding="utf-8") as stream:
        json.dump(transitions, stream, indent=2)
        stream.write("\n")

    weights, starters = strum_weights(strums_file, songs, library_folder)
    strums = metadata(sources, len(songs), rating_lines, date)
    strums["starters"] = starters
    strums["weights"] = weights
    with open(out / "strums.learned.json", "w", encoding="utf-8") as stream:
        json.dump(strums, stream, indent=2)
        stream.write("\n")
    return transitions, strums


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--songlog", default=str(REPO / "data" / "songlog"))
    parser.add_argument("--ratings", default=str(HOME / "ratings.csv"))
    parser.add_argument("--library", default=str(HOME / "library"))
    parser.add_argument("--strums", default=str(REPO / "core" / "data" / "strums.json"))
    parser.add_argument("--scales", default=str(REPO / "core" / "data" / "scales.json"))
    parser.add_argument("--out", default=str(HOME / "learned"))
    parser.add_argument("--date", default=datetime.date.today().isoformat())
    parser.add_argument("--check", metavar="DIR", help="Check the song log files in DIR and write nothing")
    args = parser.parse_args(argv)

    try:
        if args.check:
            songs = load_songlog(args.check)
            print(f"{len(songs)} song log files pass the check")
            return 0
        transitions, _ = run(args.songlog, args.ratings, args.library, args.strums, args.scales, args.out, args.date)
    except SongLogError as error:
        print(f"Song log error: {error}", file=sys.stderr)
        return 1

    print(transitions["source"])
    print(f"Wrote {Path(args.out) / 'transitions.learned.json'}")
    print(f"Wrote {Path(args.out) / 'strums.learned.json'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
