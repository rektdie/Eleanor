#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_INPUT = SCRIPT_DIR / "spsa_output.txt"
DEFAULT_TARGET = SCRIPT_DIR.parent / "source" / "tunables.h"

VALUE_RE = re.compile(r"^\s*([A-Za-z0-9_]+)\s*,\s*([^\s,]+)\s*$")
LINE_RE = re.compile(
    r"^(?P<indent>\s*)X_(?P<kind>INT|DOUBLE)\("
    r"(?P<name>[A-Za-z0-9_]+),\s*"
    r"(?P<default>[^,]+),\s*"
    r"(?P<rest>.+)\)\s*(?P<continuation>\\)?$"
)


def update_line(line: str, updated_values: dict[str, str]) -> tuple[str, bool]:
    match = LINE_RE.match(line)
    if not match:
        return line, False

    name = match.group("name")
    if name not in updated_values:
        return line, False

    continuation = " \\" if match.group("continuation") else ""
    new_line = (
        f"{match.group('indent')}X_{match.group('kind')}("
        f"{name}, {updated_values[name]}, {match.group('rest')}){continuation}\n"
    )
    return new_line, True


def load_values(path: Path) -> dict[str, str]:
    values = {}

    for line in path.read_text().splitlines():
        match = VALUE_RE.match(line)
        if not match:
            continue
        values[match.group(1)] = match.group(2)

    return values


def apply_updates(text: str, updated_values: dict[str, str]) -> tuple[str, set[str]]:
    updated_lines: list[str] = []
    seen: set[str] = set()

    for line in text.splitlines(keepends=True):
        new_line, changed = update_line(line, updated_values)
        if changed:
            match = LINE_RE.match(line)
            if match:
                seen.add(match.group("name"))
        updated_lines.append(new_line)

    return "".join(updated_lines), seen


def main() -> int:
    input_path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_INPUT
    target = Path(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_TARGET

    if not input_path.exists():
        print(f"Input file not found: {input_path}", file=sys.stderr)
        return 1

    updated_values = load_values(input_path)
    if not updated_values:
        print(f"No tunable values found in {input_path}", file=sys.stderr)
        return 1

    if not target.exists():
        print(f"Target file not found: {target}", file=sys.stderr)
        return 1

    text = target.read_text()
    updated_text, seen = apply_updates(text, updated_values)

    missing = sorted(set(updated_values) - seen)
    if missing:
        print("Missing tunables:", ", ".join(missing), file=sys.stderr)
        return 1

    target.write_text(updated_text)
    print(f"Updated {len(seen)} tunables in {target} from {input_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
