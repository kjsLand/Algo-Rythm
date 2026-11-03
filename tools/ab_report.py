#!/usr/bin/env python3
"""Compares the ratings of the rule tables and the learned tables (T15-06).

Usage: ab_report.py [ratings.csv]

Reads the lines of ratings.csv that have a table type, then prints the count, the mean rating and the
standard deviation of each table type, and a two-sided Mann-Whitney U test. The script uses only the
Python standard library.
"""
import csv
import math
import statistics
import sys
from pathlib import Path

GROUPS = ("rules", "learned")
MIN_RELIABLE = 20


def read_groups(path):
    groups = {name: [] for name in GROUPS}
    with open(path, encoding="utf-8", newline="") as stream:
        for line in csv.DictReader(stream):
            table = (line.get("tables") or "").strip()
            if table not in groups:
                continue
            try:
                groups[table].append(int(line["rating"]))
            except (KeyError, ValueError):
                continue
    return groups


def mann_whitney(a, b):
    """Returns (U of a, two-sided p) with the normal approximation, a tie correction and a continuity correction."""
    n1, n2 = len(a), len(b)
    if n1 == 0 or n2 == 0:
        return float("nan"), float("nan")

    values = sorted([(value, 0) for value in a] + [(value, 1) for value in b])
    ranks = [0.0] * len(values)
    ties = 0.0
    i = 0
    while i < len(values):
        j = i
        while j + 1 < len(values) and values[j + 1][0] == values[i][0]:
            j += 1
        rank = (i + j) / 2.0 + 1.0
        for k in range(i, j + 1):
            ranks[k] = rank
        size = j - i + 1
        ties += size ** 3 - size
        i = j + 1

    rank_sum_a = sum(rank for rank, (_, group) in zip(ranks, values) if group == 0)
    u_a = rank_sum_a - n1 * (n1 + 1) / 2.0
    n = n1 + n2
    mean = n1 * n2 / 2.0
    variance = n1 * n2 / 12.0 * ((n + 1) - ties / (n * (n - 1)))
    if variance <= 0.0:
        return u_a, 1.0
    z = (abs(u_a - mean) - 0.5) / math.sqrt(variance)
    p = math.erfc(max(z, 0.0) / math.sqrt(2.0))
    return u_a, min(1.0, p)


def summary(values):
    if not values:
        return 0, float("nan"), float("nan")
    std = statistics.stdev(values) if len(values) > 1 else 0.0
    return len(values), statistics.mean(values), std


def report(path):
    groups = read_groups(path)
    lines = []
    lines.append(f"{'Tables':<10}{'Count':>7}{'Mean':>8}{'SD':>8}")
    for name in GROUPS:
        count, mean, std = summary(groups[name])
        lines.append(f"{name:<10}{count:>7}{mean:>8.2f}{std:>8.2f}")
    u, p = mann_whitney(groups["rules"], groups["learned"])
    lines.append("")
    lines.append(f"Mann-Whitney U (rules) = {u:.1f}, two-sided p = {p:.4g}")
    for name in GROUPS:
        if len(groups[name]) < MIN_RELIABLE:
            lines.append(f"Warning: '{name}' has fewer than {MIN_RELIABLE} ratings. The result is not reliable.")
    return "\n".join(lines), groups, u, p


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    path = Path(argv[0]) if argv else Path.home() / "Algo-Rythm" / "ratings.csv"
    if not path.is_file():
        print(f"There is no ratings file at {path}", file=sys.stderr)
        return 2
    text, _, _, _ = report(path)
    print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
