#!/usr/bin/env python3
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
PARAMS = ROOT / "params.hpp"
MAIN = ROOT / "main.cpp"
OUT = ROOT / "submission.cpp"


def strip_main_includes(text: str) -> str:
    lines = []
    for line in text.splitlines():
        s = line.strip()
        if s == '#include <bits/stdc++.h>':
            continue
        if s == '#include "params.hpp"':
            continue
        lines.append(line)
    return "\n".join(lines).rstrip() + "\n"


def strip_params_header(text: str) -> str:
    lines = []
    for line in text.splitlines():
        if line.strip() == "#pragma once":
            continue
        lines.append(line)
    return "\n".join(lines).strip() + "\n"


def main() -> int:
    if not PARAMS.exists():
        raise FileNotFoundError(f"missing: {PARAMS}")
    if not MAIN.exists():
        raise FileNotFoundError(f"missing: {MAIN}")

    params_text = strip_params_header(PARAMS.read_text())
    main_text = MAIN.read_text()
    body = strip_main_includes(main_text)

    merged = (
        "#include <bits/stdc++.h>\n"
        "// BEGIN params.hpp\n"
        f"{params_text.rstrip()}\n"
        "// END params.hpp\n"
        f"{body}"
    )
    OUT.write_text(merged)
    print(f"wrote: {OUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
