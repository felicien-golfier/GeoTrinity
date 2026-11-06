"""Print the opening context for working on one AI/Pending file (backs the /implement skill).

Usage: python pending_context.py [Feature|substring]   (empty = first row of the README "Order" table)

Prints, in order: the Pending file, the CLAUDE.md files it lists under "Docs written", the nearest
CLAUDE.md of every Read-map file (and of its Public/Private twin folder), AI/CodingStyle.md, then every
Read-map file in full. Over the size budget, the remaining files are only listed (Read them yourself).
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
PENDING = ROOT / "AI" / "Pending"
BUDGET = 350_000  # chars inlined for Read-map source files
CODE_EXT = {".h", ".cpp", ".cs", ".py", ".ini", ".md", ".csv", ".txt", ".json", ".bat", ".usf", ".ush"}


def pick_file(arg: str) -> Path:
    files = sorted(p for p in PENDING.glob("*.md") if p.name != "README.md")
    if not arg:
        for line in (PENDING / "README.md").read_text(encoding="utf-8").splitlines():
            m = re.match(r"\|\s*\d\s*\|\s*`([^`]+)`", line)
            if m and (PENDING / m.group(1)).exists():
                return PENDING / m.group(1)
        sys.exit("No Pending file in the README Order table.")
    key = re.split(r"[\\/]", arg.strip().strip("\"'"))[-1].lower().removesuffix(".md")
    exact = [p for p in files if p.stem.lower() == key]
    hits = exact or [p for p in files if key in p.stem.lower()]
    if len(hits) != 1:
        sys.exit(f"{'No' if not hits else 'Ambiguous'} Pending file for '{arg}'. Available: "
                 + ", ".join(p.stem for p in files))
    return hits[0]


def section(text: str, title: str) -> str:
    m = re.search(rf"^##\s+{re.escape(title)}.*?\n(.*?)(?=^##\s|\Z)", text, re.M | re.S)
    return m.group(1) if m else ""


def expand(token: str) -> list[str]:
    """One path token -> candidate paths (handles `Public|Private` and `Foo.h/.cpp`)."""
    token = re.sub(r":\d+(-\d+)?$", "", token.strip())
    m = re.match(r"^(.*)\.(\w+)/\.(\w+)$", token)  # Foo.h/.cpp
    bases = [f"{m.group(1)}.{m.group(2)}", f"{m.group(1)}.{m.group(3)}"] if m else [token]
    out = []
    for b in bases:
        out += [b.replace("Public|Private", v) for v in ("Public", "Private")] if "Public|Private" in b else [b]
    return out


def resolve(cand: str) -> list[Path]:
    """Path relative to the root, to a module (entry written without `Source/<Module>/`), else by file name."""
    if "..." not in cand:
        for prefix in ("", "Source/GeoTrinity/", "Source/GeoTrinityUI/", "Source/GeoTrinityEditor/"):
            if (ROOT / (prefix + cand)).exists():
                return [ROOT / (prefix + cand)]
    name = cand.rsplit("/", 1)[-1]
    if Path(name).suffix in CODE_EXT and "/" in cand:
        want = [p for top in ("Source", "AI", "Config", "Data") if (ROOT / top).exists()
                for p in (ROOT / top).rglob(name)]
        head = cand.split("/...")[0] if "..." in cand else ""
        want = [p for p in want if rel(p).startswith(head)] if head else want
        return want if len(want) <= 2 else []
    return []


def read_map_paths(block: str) -> tuple[list[Path], list[str]]:
    found, unresolved = [], []
    for line in block.splitlines():
        if not line.lstrip().startswith("-"):
            continue
        head = re.split(r"\s+[—–]\s+|\s+-\s+", line.lstrip("- ").strip(), maxsplit=1)[0]
        tokens = re.findall(r"`([^`]+)`", head) or [head]
        for tok in tokens:
            tok = re.sub(r"\s*\(.*?\)", "", tok).strip()
            if "/" not in tok and "\\" not in tok and Path(tok).suffix not in CODE_EXT:
                continue  # a symbol name, not a path
            hit = False
            cands = expand(tok.replace("\\", "/"))
            for cand in cands:
                for p in resolve(cand):
                    hit = True
                    if p.is_file() and p not in found:
                        found.append(p)
                    elif p.is_dir():
                        for f in sorted(p.glob("*")):
                            if f.is_file() and f.suffix in CODE_EXT and f not in found:
                                found.append(f)
            if not hit:
                unresolved.append(tok)
    return found, unresolved


def nearest_claude(path: Path) -> list[Path]:
    """Nearest CLAUDE.md upward, plus the Public twin for a Private file (and vice versa)."""
    out = []
    variants = [path]
    s = path.as_posix()
    for a, b in (("/Private/", "/Public/"), ("/Public/", "/Private/")):
        if a in s:
            variants.append(Path(s.replace(a, b)))
    for v in variants:
        d = v.parent
        while d != ROOT.parent and d != d.parent:
            c = d / "CLAUDE.md"
            if c.exists():
                if c not in out:
                    out.append(c)
                break
            d = d.parent
    return out


def rel(p: Path) -> str:
    return p.relative_to(ROOT).as_posix()


def show(p: Path, label: str) -> None:
    print(f"\n===== {label}: {rel(p)} =====")
    print(p.read_text(encoding="utf-8", errors="replace").rstrip())


def main() -> None:
    pend = pick_file(sys.argv[1] if len(sys.argv) > 1 else "")
    text = pend.read_text(encoding="utf-8")
    seen: set[Path] = set()

    print(f"# Context for AI/Pending/{pend.name}")
    print("Step 4 of the workflow: the user's invocation of /implement is their go. Context loaded below = "
          "docs + Pending file + Read map, in full. Do not re-read them; start implementing.")
    show(pend, "PENDING FILE")
    seen.add(pend)

    docs = [ROOT / m for m in re.findall(r"`([^`]*CLAUDE\.md)`", re.search(r"^Docs written:.*$", text, re.M).group(0) if re.search(r"^Docs written:.*$", text, re.M) else "")]
    paths, unresolved = read_map_paths(section(text, "Read map"))
    claudes = list(docs)
    for p in paths:
        for c in nearest_claude(p):
            if c not in claudes:
                claudes.append(c)
    # always: root rules the step 4 relies on
    for p in claudes + [ROOT / "AI" / "CodingStyle.md"]:
        if p.is_file() and p not in seen:
            seen.add(p)
            show(p, "DOC")

    used, skipped = 0, []
    for p in paths:
        if p in seen:
            continue
        size = len(p.read_text(encoding="utf-8", errors="replace"))
        if used + size > BUDGET:
            skipped.append(p)
            continue
        used += size
        seen.add(p)
        show(p, "READ MAP")

    print("\n===== LOAD REPORT =====")
    print(f"Inlined {len(seen)} files. Not inlined (over budget, Read them if needed): "
          + (", ".join(rel(p) for p in skipped) or "none"))
    print("Read-map entries not found / not a path (fix the Read map if wrong): "
          + (", ".join(unresolved) or "none"))
    print("Next: implement the Pending items per the docs, tick them, build + quick tests per CLAUDE.md, "
          "delete the Pending file when empty. Mode: see AI/Output/mode.txt.")


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    main()
