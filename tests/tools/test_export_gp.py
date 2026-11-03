"""Tests for tools/export_gp.py. The test skips if PyGuitarPro is not installed."""
import json
import sys
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "tools"))

try:
    import guitarpro
    import export_gp
except ImportError:  # pragma: no cover
    guitarpro = None


@unittest.skipIf(guitarpro is None, "PyGuitarPro is not installed")
class ExportGpTest(unittest.TestCase):
    def test_a_golden_starter_writes_a_gp5_file_with_the_chords_and_markers(self):
        source = REPO / "tests" / "golden" / "json" / "beginner-sad-vcb.json"
        starter = json.loads(source.read_text())
        with tempfile.TemporaryDirectory() as folder:
            out = Path(folder) / "song.gp5"
            self.assertEqual(export_gp.main([str(source), "--out", str(out)]), 0)
            song = guitarpro.parse(str(out))

        track = song.tracks[0]
        bars = sum(len(section["bars"]) for section in starter["sections"])
        self.assertEqual(len(track.measures), bars)
        self.assertEqual(song.tempo, starter["bpm"])
        self.assertEqual([s.value for s in reversed(track.strings)], starter["tuning"]["strings"])
        self.assertEqual(track.offset, starter["capo"])

        markers = [m.header.marker.title for m in track.measures if m.header.marker]
        self.assertEqual(markers, [section["name"] for section in starter["sections"]])

        # Slot 0 of bar 1 is a Down stroke with the frets of the first voicing.
        first = starter["sections"][0]["bars"][0]["voicings"][0]["frets"]
        beat = track.measures[0].voices[0].beats[0]
        played = {note.string: note.value for note in beat.notes}
        expected = {6 - s: fret for s, fret in enumerate(first) if fret >= 0}
        self.assertEqual(played, expected)
        self.assertEqual(len(track.measures[0].voices[0].beats), starter["sections"][0]["strum"]["steps"])


if __name__ == "__main__":
    unittest.main()
