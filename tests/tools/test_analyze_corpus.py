"""Tests for tools/analyze_corpus.py and tools/ab_report.py. Run: python3 -m unittest discover -s tests/tools"""
import json
import sys
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "tools"))

import ab_report  # noqa: E402
import analyze_corpus  # noqa: E402


def song(title, mode, numerals, strum="D-D-D-D-"):
    return {
        "title": title,
        "artist": "Test",
        "key": "C",
        "mode": mode,
        "bpm": 100,
        "timeSignature": "4/4",
        "form": "Loop",
        "sections": [{"name": "Loop", "bars": 4, "numerals": numerals, "strum": strum}],
    }


class AnalyzeCorpusTest(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory()
        self.root = Path(self.folder.name)
        self.songlog = self.root / "songlog"
        self.songlog.mkdir()
        songs = {
            "a.json": song("A", "major", ["1", "4", "5"]),
            "b.json": song("B", "major", ["1", "5", "6m", "4"], "D-DUD-DU"),
            "c.json": song("C", "minor", ["1", "6", "7"]),
        }
        for name, entry in songs.items():
            (self.songlog / name).write_text(json.dumps(entry))
        (self.songlog / "schema.json").write_text("{}")

    def tearDown(self):
        self.folder.cleanup()

    def run_tool(self, ratings=None, library=None):
        out = self.root / "out"
        transitions, strums = analyze_corpus.run(
            self.songlog, ratings, library, REPO / "core" / "data" / "strums.json", REPO / "core" / "data" / "scales.json", out, "2026-11-03"
        )
        return transitions, strums, out

    def rows(self, transitions, mode):
        return next(t for t in transitions["tables"] if t["name"] == mode)["rows"]

    def test_three_known_songs_give_the_expected_values(self):
        transitions, _, _ = self.run_tool()
        major = self.rows(transitions, "major")
        # Row 1: 1->4 and 1->5 occur once each. Laplace adds 0.5 to the 6 numerals: total 5.
        self.assertAlmostEqual(major["1"]["4"], 1.5 / 5)
        self.assertAlmostEqual(major["1"]["5"], 1.5 / 5)
        self.assertAlmostEqual(major["1"]["2"], 0.5 / 5)
        self.assertNotIn("1", major["1"])
        # Row 5: 5->1 (song A, loop end) and 5->6 (song B).
        self.assertAlmostEqual(major["5"]["1"], 1.5 / 5)
        self.assertAlmostEqual(major["5"]["6"], 1.5 / 5)
        # Row 2 has no data, so it is uniform.
        for value in major["2"].values():
            self.assertAlmostEqual(value, 1 / 6, places=5)
        minor = self.rows(transitions, "minor")
        self.assertAlmostEqual(minor["7"]["1"], 1.5 / 4)
        self.assertEqual(transitions["songs"], 3)
        self.assertEqual(transitions["date"], "2026-11-03")
        self.assertIn("songlog/a.json", transitions["sources"])

    def test_each_row_adds_up_to_one(self):
        transitions, _, _ = self.run_tool()
        for table in transitions["tables"]:
            for row in table["rows"].values():
                self.assertAlmostEqual(sum(row.values()), 1.0, delta=0.001)

    def test_ratings_have_weights(self):
        ratings = self.root / "ratings.csv"
        ratings.write_text(
            "date,seed,key,scale,difficulty,rating,progressions,tables\n"
            "2026-11-01,1,C,Major,Beginner,5,Verse: 2 3,\n"
            "2026-11-01,2,C,Major,Beginner,2,Verse: 2 7,\n"
            "2026-11-01,3,A,Natural Minor,Beginner,4,Verse: 2 3,rules\n"
        )
        transitions, _, _ = self.run_tool(ratings)
        major = self.rows(transitions, "major")
        # Rating 5 = 2.0 for 2->3 (and 3->2 at the loop end). Rating 2 = 0.
        self.assertAlmostEqual(major["2"]["3"], 2.5 / 5)
        self.assertAlmostEqual(major["2"]["7"], 0.5 / 5)
        minor = self.rows(transitions, "minor")
        self.assertAlmostEqual(minor["2"]["3"], 1.5 / 4)
        self.assertEqual(transitions["ratings"], 3)

    def test_strum_weights_have_a_minimum(self):
        _, strums, out = self.run_tool()
        library = json.loads((REPO / "core" / "data" / "strums.json").read_text())["strums"]
        self.assertEqual(set(strums["weights"]), {entry["name"] for entry in library})
        for weight in strums["weights"].values():
            self.assertGreaterEqual(weight, 0.05)
        self.assertGreater(strums["weights"]["Quarter Downs"], strums["weights"]["Funk Sixteenths"])
        self.assertTrue((out / "strums.learned.json").is_file())

    def test_a_chord_name_is_not_a_nashville_number(self):
        bad = song("Bad", "major", ["G", "C"])
        (self.songlog / "bad.json").write_text(json.dumps(bad))
        with self.assertRaises(analyze_corpus.SongLogError):
            analyze_corpus.load_songlog(self.songlog)

    def test_the_example_entry_passes_the_check(self):
        songs = analyze_corpus.load_songlog(REPO / "data" / "songlog")
        self.assertEqual([name for name, _ in songs], ["example.json"])
        try:
            import jsonschema
        except ImportError:
            return
        schema = json.loads((REPO / "data" / "songlog" / "schema.json").read_text())
        example = json.loads((REPO / "data" / "songlog" / "example.json").read_text())
        jsonschema.validate(example, schema)


class AbReportTest(unittest.TestCase):
    def write(self, rows):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        path = Path(folder.name) / "ratings.csv"
        lines = ["date,seed,key,scale,difficulty,rating,progressions,tables"]
        lines += [f"2026-11-01,{i},C,Major,Beginner,{rating},Verse: 1 5,{table}" for i, (rating, table) in enumerate(rows)]
        path.write_text("\n".join(lines) + "\n")
        return path

    def test_40_ratings_give_the_expected_values(self):
        rules = [1, 2, 3, 4, 5] * 4
        learned = [3, 4, 5, 5, 5] * 4
        rows = [(r, "rules") for r in rules] + [(r, "learned") for r in learned] + [(5, "")]
        text, groups, u, p = ab_report.report(self.write(rows))
        self.assertEqual(len(groups["rules"]), 20)
        self.assertEqual(len(groups["learned"]), 20)
        self.assertIn("rules          20    3.00    1.45", text)
        self.assertIn("learned        20    4.40    0.82", text)
        # U counts the pairs where rules wins, with a tie as one half.
        pairs = sum((1.0 if a > b else 0.5 if a == b else 0.0) for a in rules for b in learned)
        self.assertAlmostEqual(u, pairs)
        self.assertLess(p, 0.01)
        self.assertNotIn("Warning", text)

    def test_identical_groups_give_p_one_and_a_small_group_gives_a_warning(self):
        rows = [(4, "rules")] * 5 + [(4, "learned")] * 5
        text, _, u, p = ab_report.report(self.write(rows))
        self.assertAlmostEqual(u, 12.5)
        self.assertAlmostEqual(p, 1.0)
        self.assertIn("Warning: 'rules' has fewer than 20 ratings", text)

    def test_separated_groups_give_a_small_p(self):
        rows = [(3, "rules")] * 20 + [(4, "learned")] * 20
        _, _, u, p = ab_report.report(self.write(rows))
        self.assertEqual(u, 0.0)
        self.assertLess(p, 1e-6)


if __name__ == "__main__":
    unittest.main()
