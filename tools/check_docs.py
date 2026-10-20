"""Check repository Markdown file links; include dated records with --history."""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess
from urllib.parse import unquote, urlsplit


ROOT = Path(__file__).resolve().parents[1]
INLINE_LINK = re.compile(r"!?\[[^\]\n]*\]\((<[^>]+>|[^)\n]+)\)")
REFERENCE_LINK = re.compile(r"^\s*\[[^\]]+\]:\s*(<[^>]+>|\S+)")


def targets(text: str):
    fence = None
    for number, line in enumerate(text.splitlines(), 1):
        marker = re.match(r"\s*(`{3,}|~{3,})", line)
        if marker:
            char = marker[1][0]
            if fence is None:
                fence = char
            elif char == fence:
                fence = None
            continue
        if fence:
            continue
        matches = list(INLINE_LINK.finditer(line))
        reference = REFERENCE_LINK.match(line)
        if reference:
            matches.append(reference)
        for match in matches:
            raw = match[1].strip()
            yield number, raw[1:-1] if raw.startswith("<") else re.split(r'\s+["\']', raw, maxsplit=1)[0]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--history", action="store_true")
    args = parser.parse_args()
    names = subprocess.check_output(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"], cwd=ROOT
    ).decode().split("\0")
    errors = []
    checked = 0
    for name in sorted(set(names)):
        path = ROOT / name
        if not name.endswith(".md") or not path.is_file():
            continue
        if not args.history and "/docs/history/" in "/" + name:
            continue
        data = path.read_bytes()
        try:
            text = data.decode("utf-8-sig")
        except UnicodeDecodeError:
            text = data.decode("cp1252")
        checked += 1
        for line, target in targets(text):
            parsed = urlsplit(target)
            if parsed.scheme or parsed.netloc or not parsed.path:
                continue
            base = ROOT if parsed.path.startswith("/") else path.parent
            resolved = base / unquote(parsed.path).lstrip("/")
            if not resolved.exists():
                errors.append(f"{name}:{line}: {target}")
    for error in errors:
        print(error)
    print(f"Checked {checked} Markdown files; {len(errors)} missing file links.")
    return bool(errors)


if __name__ == "__main__":
    raise SystemExit(main())
