#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import math
import os
import re
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any

try:
    import tomllib
except ModuleNotFoundError:  # pragma: no cover
    import tomli as tomllib  # type: ignore


SCORE_RE = re.compile(r"(?m)^\s*Score\s*=\s*(\d+)\s*$")


@dataclass
class CaseResult:
    seed4: str
    score: int
    rel: float
    ms: int


def read_toml(path: Path) -> dict[str, Any]:
    with path.open("rb") as f:
        return tomllib.load(f)


def ensure_parent(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)


def format_seed_path(template: str, seed4: str, root: Path) -> Path:
    return (root / template.replace("{SEED04}", seed4)).resolve()


def run_cmd(cmd: list[str], cwd: Path | None = None) -> None:
    proc = subprocess.run(cmd, cwd=str(cwd) if cwd else None)
    if proc.returncode != 0:
        raise RuntimeError(f"command failed: {' '.join(cmd)} (exit={proc.returncode})")


def run_one_case(
    root: Path,
    test_step: dict[str, Any],
    seed4: str,
    best: dict[str, int],
) -> CaseResult:
    stdin_path = format_seed_path(test_step["stdin"], seed4, root)
    stdout_path = format_seed_path(test_step["stdout"], seed4, root)
    stderr_path = format_seed_path(test_step["stderr"], seed4, root)
    ensure_parent(stdout_path)
    ensure_parent(stderr_path)

    current_dir = (root / test_step.get("current_dir", ".")).resolve()
    program = test_step["program"]
    args = list(test_step.get("args", []))

    if program == "cargo":
        tester = current_dir / "target" / "release" / "tester"
        if not tester.exists():
            raise RuntimeError(f"tester binary not found: {tester}")
        run_argv = [str(tester), "../a.out"]
    else:
        run_argv = [program] + args

    t0 = time.perf_counter()
    with stdin_path.open("rb") as fin, stdout_path.open("wb") as fout, stderr_path.open("wb") as ferr:
        proc = subprocess.run(run_argv, cwd=str(current_dir), stdin=fin, stdout=fout, stderr=ferr)
    ms = int((time.perf_counter() - t0) * 1000)
    if proc.returncode != 0:
        raise RuntimeError(f"case {seed4} failed (exit={proc.returncode})")

    err_text = stderr_path.read_text(errors="replace")
    m = SCORE_RE.search(err_text)
    if not m:
        raise RuntimeError(f"score parse failed for case {seed4}")
    score = int(m.group(1))
    ref = best.get(seed4, score)
    rel = (100.0 * score / max(1, ref))
    return CaseResult(seed4=seed4, score=score, rel=rel, ms=ms)


def print_header(total: int) -> None:
    print("| Progress  | Seed |      Case Score       |      Average Score       |   Exec.   |")
    print("|           |      |   Score    | Relative |     Score     | Relative |   Time    |")
    print("|-----------|------|------------|----------|---------------|----------|-----------|")


def print_row(i: int, total: int, case: CaseResult, avg_score: float, avg_rel: float) -> None:
    print(
        f"| {i:>3d} / {total:<3d} | {case.seed4} | "
        f"{case.score:>10,} | {case.rel:>8.3f} | "
        f"{avg_score:>13,.2f} | {avg_rel:>8.3f} | {case.ms:>6,d} ms |"
    )


def cmd_run(args: argparse.Namespace) -> int:
    root = Path.cwd()
    cfg_path = (root / args.config).resolve()
    cfg = read_toml(cfg_path)
    test_cfg = cfg.get("test", {})
    compile_steps = test_cfg.get("compile_steps", [])
    test_steps = test_cfg.get("test_steps", [])
    if not compile_steps or not test_steps:
        raise RuntimeError("pahcer config is missing compile_steps/test_steps")

    # compile
    for step in compile_steps:
        program = step["program"]
        argv = [program] + list(step.get("args", []))
        cwd = (root / step.get("current_dir", ".")).resolve()
        run_cmd(argv, cwd=cwd)

    start_seed = int(test_cfg.get("start_seed", 0))
    end_seed = int(test_cfg.get("end_seed", 100))
    total = max(0, end_seed - start_seed)

    best_path = root / "pahcer" / "best_scores.json"
    best: dict[str, int] = {}
    if args.freeze_best_scores and best_path.exists():
        best = {str(k): int(v) for k, v in json.loads(best_path.read_text()).items()}

    test_step = test_steps[0]
    print_header(total)
    sum_score = 0.0
    sum_rel = 0.0
    max_ms = 0
    accepted = 0
    for idx, seed in enumerate(range(start_seed, end_seed), start=1):
        seed4 = f"{seed:04d}"
        res = run_one_case(root, test_step, seed4, best)
        accepted += 1
        sum_score += res.score
        sum_rel += res.rel
        max_ms = max(max_ms, res.ms)
        print_row(idx, total, res, sum_score / idx, sum_rel / idx)

    avg_score = (sum_score / total) if total else 0.0
    avg_rel = (sum_rel / total) if total else 0.0
    avg_log10 = 0.0
    if total:
        # reuse case scores from generated stderr files
        vals = []
        for seed in range(start_seed, end_seed):
            seed4 = f"{seed:04d}"
            err_path = format_seed_path(test_step["stderr"], seed4, root)
            text = err_path.read_text(errors="replace")
            m = SCORE_RE.search(text)
            vals.append(int(m.group(1)) if m else 0)
        avg_log10 = sum((0.0 if v <= 0 else math.log10(v)) for v in vals) / total

    print(f"Average Score          : {avg_score:,.2f}")
    print(f"Average Score (log10)  : {avg_log10:.5f}")
    print(f"Average Relative Score : {avg_rel:.3f}")
    print(f"Accepted               : {accepted} / {total}")
    print(f"Max Execution Time     : {max_ms:,d} ms")
    return 0


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(prog="pahcer")
    sub = p.add_subparsers(dest="command", required=True)

    run_p = sub.add_parser("run")
    run_p.add_argument("-c", "--config", default="pahcer_config.toml")
    run_p.add_argument("--freeze-best-scores", action="store_true")
    run_p.set_defaults(func=cmd_run)
    return p


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    try:
        return int(args.func(args))
    except Exception as e:  # noqa: BLE001
        print(f"pahcer-local error: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
