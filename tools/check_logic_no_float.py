#!/usr/bin/env python3

import pathlib
import re
import sys


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: check_logic_no_float.py <logic_dir>")
        return 2

    logic_dir = pathlib.Path(sys.argv[1])
    if not logic_dir.exists():
        print(f"Path does not exist: {logic_dir}")
        return 2

    pattern = re.compile(r"\b(float|double)\b")
    violations = []

    for path in sorted(logic_dir.rglob("*")):
        if path.suffix.lower() not in {".h", ".hpp", ".hh", ".c", ".cc", ".cpp", ".cxx", ".inl"}:
            continue

        text = path.read_text(encoding="utf-8", errors="ignore")
        for line_number, line in enumerate(text.splitlines(), start=1):
            if pattern.search(line):
                violations.append((path, line_number, line.strip()))

    if violations:
        print("Found forbidden floating-point types in src/logic:")
        for path, line_number, line in violations:
            print(f"- {path}:{line_number}: {line}")
        return 1

    print("No float/double usage found in src/logic")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
