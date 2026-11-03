"""Fails if a file in core/ uses a banned random function (REQ-05-2).

Usage: check_banned.py [repo_root]
"""
import re
import sys
from pathlib import Path

BANNED = {
    "std::random_device": re.compile(r"\brandom_device\b"),
    "std::uniform_int_distribution": re.compile(r"\buniform_int_distribution\b"),
    "std::uniform_real_distribution": re.compile(r"\buniform_real_distribution\b"),
    "std::shuffle": re.compile(r"\bstd\s*::\s*shuffle\b"),
    "rand()": re.compile(r"\brand\s*\("),
    "std::mt19937_64": re.compile(r"\bmt19937_64\b"),
}

# The Rng class is the only code that can name its engine.
PERMITTED = {("std::mt19937_64", "core/src/rng.cpp")}


def find_banned(root):
    """Returns the text of one line for each use of a banned name in root/core."""
    found = []
    for path in sorted((root / "core").rglob("*")):
        if not path.is_file():
            continue
        relative = path.relative_to(root).as_posix()
        text = path.read_text(encoding="utf-8", errors="replace")
        for number, line in enumerate(text.splitlines(), start=1):
            for name, pattern in BANNED.items():
                if pattern.search(line) and (name, relative) not in PERMITTED:
                    found.append(f"{relative}:{number}: {name}: {line.strip()}")
    return found


def main():
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent

    # A wrong path must not pass with no files checked.
    if not (root / "core").is_dir():
        print(f"There is no core folder in {root}")
        return 2

    found = find_banned(root)
    for text in found:
        print(text)
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
