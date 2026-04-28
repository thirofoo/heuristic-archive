#!/usr/bin/env python3
import argparse
import csv
import json
import math
import os
from collections import defaultdict
from dataclasses import dataclass
from typing import Dict, Iterable, List, Tuple

os.environ.setdefault("MPLCONFIGDIR", "/tmp/matplotlib")
os.makedirs(os.environ["MPLCONFIGDIR"], exist_ok=True)

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np


@dataclass
class Row:
    seed: int
    score: float
    n: int
    m: int
    t: int
    u: int


def read_input_header(path: str) -> Tuple[int, int, int, int]:
    with open(path, "r", encoding="utf-8") as f:
        line = f.readline().strip()
    parts = line.split()
    if len(parts) != 4:
        raise ValueError(f"invalid header: {path}")
    return int(parts[0]), int(parts[1]), int(parts[2]), int(parts[3])


def load_rows(best_scores_path: str, in_dir: str) -> List[Row]:
    with open(best_scores_path, "r", encoding="utf-8") as f:
        obj = json.load(f)
    rows: List[Row] = []
    for key, value in obj.items():
        try:
            seed = int(key)
        except ValueError:
            continue
        path = os.path.join(in_dir, f"{seed:04d}.txt")
        if not os.path.exists(path):
            continue
        n, m, t, u = read_input_header(path)
        rows.append(Row(seed=seed, score=float(value), n=n, m=m, t=t, u=u))
    rows.sort(key=lambda x: x.seed)
    return rows


def ensure_dir(path: str) -> None:
    os.makedirs(path, exist_ok=True)


def percent(arr: np.ndarray, p: float) -> float:
    if arr.size == 0:
        return float("nan")
    return float(np.percentile(arr, p))


def summarize(values: Iterable[float]) -> Dict[str, float]:
    arr = np.array(list(values), dtype=float)
    if arr.size == 0:
        return {
            "count": 0.0,
            "mean": float("nan"),
            "median": float("nan"),
            "std": float("nan"),
            "min": float("nan"),
            "max": float("nan"),
            "p10": float("nan"),
            "p25": float("nan"),
            "p75": float("nan"),
            "p90": float("nan"),
        }
    return {
        "count": float(arr.size),
        "mean": float(arr.mean()),
        "median": float(np.median(arr)),
        "std": float(arr.std(ddof=0)),
        "min": float(arr.min()),
        "max": float(arr.max()),
        "p10": percent(arr, 10),
        "p25": percent(arr, 25),
        "p75": percent(arr, 75),
        "p90": percent(arr, 90),
    }


def write_rows_csv(rows: List[Row], out_path: str) -> None:
    with open(out_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["seed", "score", "N", "M", "T", "U"])
        for r in rows:
            w.writerow([r.seed, int(r.score), r.n, r.m, r.t, r.u])


def write_summary_csv(summary: Dict[Tuple[int, ...], Dict[str, float]], key_names: List[str], out_path: str) -> None:
    with open(out_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(key_names + ["count", "mean", "median", "std", "min", "max", "p10", "p25", "p75", "p90"])
        for key in sorted(summary.keys()):
            stats = summary[key]
            row = list(key) + [
                int(stats["count"]),
                stats["mean"],
                stats["median"],
                stats["std"],
                stats["min"],
                stats["max"],
                stats["p10"],
                stats["p25"],
                stats["p75"],
                stats["p90"],
            ]
            w.writerow(row)


def save_fig(path: str) -> None:
    plt.tight_layout()
    plt.savefig(path, dpi=160)
    plt.close()


def plot_hist_all(rows: List[Row], out_dir: str) -> None:
    scores = np.array([r.score for r in rows], dtype=float)
    plt.figure(figsize=(9, 5))
    plt.hist(scores, bins=20, color="#3a86ff", edgecolor="white")
    plt.xlabel("Score")
    plt.ylabel("Count")
    plt.title("Best Score Histogram (All Seeds)")
    save_fig(os.path.join(out_dir, "hist_all.png"))


def plot_ecdf_by_group(rows: List[Row], out_dir: str, attr: str, name: str) -> None:
    groups: Dict[int, List[float]] = defaultdict(list)
    for r in rows:
        groups[getattr(r, attr)].append(r.score)
    plt.figure(figsize=(9, 5))
    for g in sorted(groups.keys()):
        arr = np.sort(np.array(groups[g], dtype=float))
        if arr.size == 0:
            continue
        y = np.arange(1, arr.size + 1) / arr.size
        plt.step(arr, y, where="post", label=f"{name}={g}")
    plt.xlabel("Score")
    plt.ylabel("ECDF")
    plt.title(f"ECDF by {name}")
    plt.legend()
    save_fig(os.path.join(out_dir, f"ecdf_by_{name.lower()}.png"))


def plot_box_by_group(rows: List[Row], out_dir: str, attr: str, name: str) -> None:
    groups: Dict[int, List[float]] = defaultdict(list)
    for r in rows:
        groups[getattr(r, attr)].append(r.score)
    labels = sorted(groups.keys())
    data = [groups[k] for k in labels]
    plt.figure(figsize=(9, 5))
    plt.boxplot(data, labels=[str(x) for x in labels], showfliers=True)
    plt.xlabel(name)
    plt.ylabel("Score")
    plt.title(f"Score Distribution by {name}")
    save_fig(os.path.join(out_dir, f"box_by_{name.lower()}.png"))


def plot_violin_by_group(rows: List[Row], out_dir: str, attr: str, name: str) -> None:
    groups: Dict[int, List[float]] = defaultdict(list)
    for r in rows:
        groups[getattr(r, attr)].append(r.score)
    labels = sorted(groups.keys())
    data = [groups[k] for k in labels]
    plt.figure(figsize=(9, 5))
    parts = plt.violinplot(data, showmeans=True, showmedians=False, showextrema=True)
    for b in parts["bodies"]:
        b.set_alpha(0.6)
    plt.xticks(np.arange(1, len(labels) + 1), [str(x) for x in labels])
    plt.xlabel(name)
    plt.ylabel("Score")
    plt.title(f"Score Violin by {name}")
    save_fig(os.path.join(out_dir, f"violin_by_{name.lower()}.png"))


def plot_scatter_seed(rows: List[Row], out_dir: str) -> None:
    ms = sorted({r.m for r in rows})
    cmap = plt.cm.get_cmap("tab10", len(ms))
    m_to_color = {m: cmap(i) for i, m in enumerate(ms)}
    plt.figure(figsize=(10, 5))
    for m in ms:
        xs = [r.seed for r in rows if r.m == m]
        ys = [r.score for r in rows if r.m == m]
        plt.scatter(xs, ys, s=26, alpha=0.8, label=f"M={m}", color=m_to_color[m])
    plt.xlabel("Seed")
    plt.ylabel("Score")
    plt.title("Score by Seed (Colored by M)")
    plt.legend(ncol=min(4, len(ms)))
    save_fig(os.path.join(out_dir, "scatter_seed_by_m.png"))


def plot_mean_std_bar(rows: List[Row], out_dir: str, attr: str, name: str) -> None:
    groups: Dict[int, List[float]] = defaultdict(list)
    for r in rows:
        groups[getattr(r, attr)].append(r.score)
    labels = sorted(groups.keys())
    means = [np.mean(groups[k]) for k in labels]
    stds = [np.std(groups[k], ddof=0) for k in labels]
    x = np.arange(len(labels))
    plt.figure(figsize=(9, 5))
    plt.bar(x, means, yerr=stds, capsize=4, color="#4cc9f0")
    plt.xticks(x, [str(k) for k in labels])
    plt.xlabel(name)
    plt.ylabel("Mean Score ± Std")
    plt.title(f"Mean/Std by {name}")
    save_fig(os.path.join(out_dir, f"mean_std_by_{name.lower()}.png"))


def plot_heatmap(rows: List[Row], out_dir: str, metric: str, fname: str, title: str) -> None:
    ms = sorted({r.m for r in rows})
    us = sorted({r.u for r in rows})
    grid = np.full((len(us), len(ms)), np.nan, dtype=float)
    grouped: Dict[Tuple[int, int], List[float]] = defaultdict(list)
    for r in rows:
        grouped[(r.u, r.m)].append(r.score)
    for i, u in enumerate(us):
        for j, m in enumerate(ms):
            vals = grouped.get((u, m), [])
            if not vals:
                continue
            arr = np.array(vals, dtype=float)
            if metric == "mean":
                grid[i, j] = float(arr.mean())
            elif metric == "median":
                grid[i, j] = float(np.median(arr))
            elif metric == "std":
                grid[i, j] = float(arr.std(ddof=0))
            elif metric == "count":
                grid[i, j] = float(arr.size)
            else:
                raise ValueError(metric)

    plt.figure(figsize=(9, 6))
    masked = np.ma.masked_invalid(grid)
    im = plt.imshow(masked, cmap="viridis", aspect="auto")
    plt.colorbar(im)
    plt.xticks(np.arange(len(ms)), [str(m) for m in ms])
    plt.yticks(np.arange(len(us)), [str(u) for u in us])
    plt.xlabel("M")
    plt.ylabel("U")
    plt.title(title)
    for i in range(len(us)):
        for j in range(len(ms)):
            if math.isnan(grid[i, j]):
                continue
            txt = f"{grid[i, j]:.1f}" if metric != "count" else f"{int(grid[i, j])}"
            plt.text(j, i, txt, ha="center", va="center", color="white", fontsize=8)
    save_fig(os.path.join(out_dir, fname))


def plot_rank_curve(rows: List[Row], out_dir: str) -> None:
    sorted_rows = sorted(rows, key=lambda r: r.score)
    xs = np.arange(1, len(sorted_rows) + 1)
    ys = np.array([r.score for r in sorted_rows], dtype=float)
    plt.figure(figsize=(9, 5))
    plt.plot(xs, ys, color="#ff006e", linewidth=2)
    plt.xlabel("Rank (low -> high)")
    plt.ylabel("Score")
    plt.title("Ranked Best Scores")
    save_fig(os.path.join(out_dir, "rank_curve.png"))


def plot_top_bottom(rows: List[Row], out_dir: str, k: int = 20) -> None:
    sorted_rows = sorted(rows, key=lambda r: r.score)
    low = sorted_rows[:k]
    high = sorted_rows[-k:]
    fig, axes = plt.subplots(1, 2, figsize=(14, 6), sharey=True)
    axes[0].barh([f"{r.seed:04d}(M{r.m},U{r.u})" for r in low], [r.score for r in low], color="#ef476f")
    axes[0].set_title(f"Bottom {k}")
    axes[1].barh([f"{r.seed:04d}(M{r.m},U{r.u})" for r in high], [r.score for r in high], color="#06d6a0")
    axes[1].set_title(f"Top {k}")
    for ax in axes:
        ax.set_xlabel("Score")
    fig.suptitle("Top/Bottom Seeds in Best Scores")
    plt.tight_layout()
    plt.savefig(os.path.join(out_dir, "top_bottom_seeds.png"), dpi=160)
    plt.close()


def write_text_report(rows: List[Row], out_dir: str) -> None:
    scores = [r.score for r in rows]
    overall = summarize(scores)
    by_m: Dict[int, List[float]] = defaultdict(list)
    by_u: Dict[int, List[float]] = defaultdict(list)
    by_mu: Dict[Tuple[int, int], List[float]] = defaultdict(list)
    for r in rows:
        by_m[r.m].append(r.score)
        by_u[r.u].append(r.score)
        by_mu[(r.m, r.u)].append(r.score)

    lines: List[str] = []
    lines.append("# Best Score Distribution Report\n")
    lines.append(f"- Cases: {len(rows)}")
    lines.append(f"- Mean: {overall['mean']:.2f}")
    lines.append(f"- Median: {overall['median']:.2f}")
    lines.append(f"- Std: {overall['std']:.2f}")
    lines.append(f"- Min/Max: {overall['min']:.0f} / {overall['max']:.0f}\n")

    lines.append("## By M")
    for m in sorted(by_m.keys()):
        s = summarize(by_m[m])
        lines.append(f"- M={m}: n={int(s['count'])}, mean={s['mean']:.2f}, std={s['std']:.2f}, median={s['median']:.2f}")
    lines.append("")

    lines.append("## By U")
    for u in sorted(by_u.keys()):
        s = summarize(by_u[u])
        lines.append(f"- U={u}: n={int(s['count'])}, mean={s['mean']:.2f}, std={s['std']:.2f}, median={s['median']:.2f}")
    lines.append("")

    lines.append("## By (M, U)")
    for key in sorted(by_mu.keys()):
        s = summarize(by_mu[key])
        lines.append(
            f"- M={key[0]}, U={key[1]}: n={int(s['count'])}, "
            f"mean={s['mean']:.2f}, std={s['std']:.2f}, median={s['median']:.2f}"
        )
    lines.append("")

    out_path = os.path.join(out_dir, "report.md")
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--best-scores", default="pahcer/best_scores.json")
    parser.add_argument("--in-dir", default="tools/in")
    parser.add_argument("--out-dir", default="pahcer/best_score_analysis")
    parser.add_argument("--topk", type=int, default=20)
    args = parser.parse_args()

    rows = load_rows(args.best_scores, args.in_dir)
    if not rows:
        raise SystemExit("no rows loaded; check --best-scores and --in-dir")

    ensure_dir(args.out_dir)
    write_rows_csv(rows, os.path.join(args.out_dir, "rows.csv"))

    by_m: Dict[Tuple[int], Dict[str, float]] = {}
    by_u: Dict[Tuple[int], Dict[str, float]] = {}
    by_mu: Dict[Tuple[int, int], Dict[str, float]] = {}
    tmp_m: Dict[int, List[float]] = defaultdict(list)
    tmp_u: Dict[int, List[float]] = defaultdict(list)
    tmp_mu: Dict[Tuple[int, int], List[float]] = defaultdict(list)
    for r in rows:
        tmp_m[r.m].append(r.score)
        tmp_u[r.u].append(r.score)
        tmp_mu[(r.m, r.u)].append(r.score)
    for m, vals in tmp_m.items():
        by_m[(m,)] = summarize(vals)
    for u, vals in tmp_u.items():
        by_u[(u,)] = summarize(vals)
    for k, vals in tmp_mu.items():
        by_mu[k] = summarize(vals)

    write_summary_csv(by_m, ["M"], os.path.join(args.out_dir, "summary_by_m.csv"))
    write_summary_csv(by_u, ["U"], os.path.join(args.out_dir, "summary_by_u.csv"))
    write_summary_csv(by_mu, ["M", "U"], os.path.join(args.out_dir, "summary_by_m_u.csv"))

    write_text_report(rows, args.out_dir)
    plot_hist_all(rows, args.out_dir)
    plot_ecdf_by_group(rows, args.out_dir, "m", "M")
    plot_ecdf_by_group(rows, args.out_dir, "u", "U")
    plot_box_by_group(rows, args.out_dir, "m", "M")
    plot_box_by_group(rows, args.out_dir, "u", "U")
    plot_violin_by_group(rows, args.out_dir, "m", "M")
    plot_violin_by_group(rows, args.out_dir, "u", "U")
    plot_scatter_seed(rows, args.out_dir)
    plot_mean_std_bar(rows, args.out_dir, "m", "M")
    plot_mean_std_bar(rows, args.out_dir, "u", "U")
    plot_heatmap(rows, args.out_dir, "mean", "heatmap_mean_m_u.png", "Mean Score by (U, M)")
    plot_heatmap(rows, args.out_dir, "median", "heatmap_median_m_u.png", "Median Score by (U, M)")
    plot_heatmap(rows, args.out_dir, "std", "heatmap_std_m_u.png", "Std of Score by (U, M)")
    plot_heatmap(rows, args.out_dir, "count", "heatmap_count_m_u.png", "Case Count by (U, M)")
    plot_rank_curve(rows, args.out_dir)
    plot_top_bottom(rows, args.out_dir, args.topk)

    print(f"saved: {args.out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
