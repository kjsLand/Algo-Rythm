"""STE (ASD-STE100, 80% format) lint for the spec files.

Checks:
- Procedural step sentences: 20 words or fewer.
- Descriptive sentences (paragraphs, acceptance checks, requirement cells): 25 words or fewer.
- Banned words: should, shall, ensure, utilize, implement.
- "-ing" words that are not in the technical-name allow list.
- Steps with more than one instruction (two imperative sentences).
- Possible passive voice (for manual review).
- Task IDs: unique, prerequisites exist, no cycles.
"""
import re
import sys
from pathlib import Path

SPEC_DIR = Path(sys.argv[1] if len(sys.argv) > 1 else "specs")

ALLOWED_ING = {
    "string", "strings", "voicing", "voicings", "thing", "nothing", "something", "during",
    "bring", "spring", "ring", "sampling", "smoothing", "fingering", "rendering", "picking",
    "fingerpicking", "padding", "setting", "settings", "testing", "warning", "warnings",
    "heading", "spelling", "building", "pending", "mixing", "routing", "king", "ceiling",
    "timing", "tuning", "tunings", "reading", "string's", "rating", "ratings",
    # document and technical names
    "coding", "learning", "operating", "signing", "running", "meaning",
}
BANNED = {"should", "shall", "ensure", "ensures", "utilize", "utilise", "implement", "implements"}
IMPERATIVE_START = {
    "add", "write", "make", "use", "set", "move", "delete", "remove", "run", "read", "check",
    "keep", "put", "give", "choose", "get", "link", "call", "change", "show", "draw", "store",
    "convert", "sort", "rename", "search", "build", "open", "load", "save", "start", "stop",
    "replace", "compare", "record", "count", "divide", "print", "fill", "find", "rotate",
    "connect", "send", "test", "update", "filter", "return", "mark", "ignore", "include",
    "accept", "calculate", "evaluate", "normalize", "map", "assign", "define", "allocate",
    "attach", "package", "download", "disable", "permit", "hide", "commit", "cache", "pin",
    "limit", "measure", "copy", "increase", "skip", "highlight", "free", "prepare", "split",
}
ING_RE = re.compile(r"\b([A-Za-z]+ing)\b")
PASSIVE_RE = re.compile(r"\b(is|are|was|were|be|been)\s+(\w+ed|written|given|made|done|set|shown|built)\b", re.I)


def strip_md(text: str) -> str:
    text = re.sub(r"`[^`]*`", "X", text)          # code spans count as one word
    text = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", text)
    text = text.replace("**", "")
    return text.strip()


def sentences(text: str):
    text = strip_md(text)
    parts = re.split(r"(?<=[.!?])\s+(?=[A-Z0-9\"'(])", text)
    return [p for p in parts if p]


def words(s: str):
    return [w for w in re.split(r"\s+", s) if re.search(r"[A-Za-z0-9]", w)]


def main():
    issues = []
    tasks = {}
    prereqs = {}
    for path in sorted(SPEC_DIR.glob("*.md")):
        lines = path.read_text(encoding="utf-8").splitlines()
        mode = None
        in_code = False
        current = None
        for n, raw in enumerate(lines, 1):
            line = raw.rstrip()
            if line.startswith("```"):
                in_code = not in_code
                continue
            if in_code or not line.strip():
                continue
            m = re.match(r"### (T\d\d-\d\d):", line)
            if m:
                current = m.group(1)
                if current in tasks:
                    issues.append((path.name, n, f"duplicate task ID {current}"))
                tasks[current] = path.name
                mode = None
                continue
            if line.startswith("#"):
                mode = None
                continue
            if line.startswith("**Procedure:**"):
                mode = "proc"
                continue
            if line.startswith("**Acceptance:**"):
                mode = "acc"
                continue
            pm = re.match(r"- \*\*Prerequisites:\*\* (.*)", line)
            if pm and current:
                prereqs[current] = re.findall(r"T\d\d-\d\d", pm.group(1))
                continue
            if line.startswith("- **"):
                continue
            # Classify the text
            text = line
            limit = 25
            kind = "desc"
            sm = re.match(r"\s*\d+\.\s+(.*)", line)
            if sm and mode == "proc":
                text, limit, kind = sm.group(1), 20, "step"
            elif line.startswith("|"):
                cells = [c.strip() for c in line.strip("|").split("|")]
                if all(set(c) <= set("- :") for c in cells):
                    continue
                text = cells[-1]
            else:
                text = re.sub(r"^\s*(- \[[ x]\]|-|\d+\.)\s*", "", line)
            for s in sentences(text):
                wc = len(words(s))
                lim = limit
                if kind == "step" and not words(s)[0].lower().strip("\"'") in IMPERATIVE_START and not s.lower().startswith(("if ", "when ", "for ", "in ", "on ", "at ", "with ", "over ", "before ", "after ", "without ", "do not", "until ", "otherwise", "from ", "below ", "above ", "each ", "this ", "the ", "a ", "an ", "it ", "these ", "one ", "only ")):
                    pass
                if kind == "step":
                    # descriptive helper sentences inside a step use the 25-word limit
                    first = words(s)[0].lower()
                    if first in ("the", "a", "an", "it", "this", "these", "one", "each", "only"):
                        lim = 25
                if wc > lim:
                    issues.append((path.name, n, f"{wc} words > {lim}: {s[:80]}"))
            low = strip_md(text).lower()
            if path.name.startswith("00") and ("Do not" in text or "Example:" in text):
                low = ""
            for w in re.findall(r"[a-z]+", low):
                if w in BANNED:
                    issues.append((path.name, n, f"banned word '{w}'"))
            for w in ING_RE.findall(strip_md(text)):
                if w.lower() not in ALLOWED_ING:
                    issues.append((path.name, n, f"-ing word '{w}'"))
            if kind == "step":
                ss = sentences(text)
                imps = [s for s in ss if words(s)[0].lower() in IMPERATIVE_START]
                if len(imps) > 1:
                    issues.append((path.name, n, f"REVIEW two instructions in one step: {strip_md(text)[:90]}"))
            for m2 in ([] if (path.name.startswith("00") and "Do not" in text) else PASSIVE_RE.finditer(strip_md(text))):
                issues.append((path.name, n, f"REVIEW possible passive '{m2.group(0)}'"))
    # prerequisite checks
    for t, ps in prereqs.items():
        for p in ps:
            if p not in tasks:
                issues.append((tasks[t], 0, f"{t}: prerequisite {p} does not exist"))
    # cycle check
    state = {}

    def visit(t, stack):
        if state.get(t) == 1:
            issues.append((tasks.get(t, "?"), 0, "cycle: " + " -> ".join(stack + [t])))
            return
        if state.get(t) == 2:
            return
        state[t] = 1
        for p in prereqs.get(t, []):
            visit(p, stack + [t])
        state[t] = 2

    for t in tasks:
        visit(t, [])
    for f, n, msg in issues:
        print(f"{f}:{n}: {msg}")
    print(f"\n{len(tasks)} tasks checked, {len(issues)} issues")
    return 1 if any(not m.startswith("REVIEW") for _, _, m in issues) else 0


if __name__ == "__main__":
    sys.exit(main())
