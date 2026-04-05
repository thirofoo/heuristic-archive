# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Problem

AHC063 (AtCoder Heuristic Contest 063) - "Colorful Ouroboros". Optimization problem: control a snake on an N×N grid to eat colored food in a target order, minimizing `T + 10000 × (E + 2(M-k))` where T=turns, E=color mismatches, k=final snake length.

## Build & Run

```bash
# Compile (C++20)
g++ -std=c++20 -O2 main.cpp

# Compile with AddressSanitizer
g++ -std=c++20 -O2 -fsanitize=address main.cpp -o main_asan

# Run on single test case
./a.out < _in > _out 2> _err

# Score a single case (requires Rust toolchain)
cd tools && cargo run --bin vis --release in/0000.txt out/0000.txt

# Run full test suite (1000 seeds) via pahcer
pahcer
```

## Architecture

Single-file C++ solution in `main.cpp` (~900 lines). Key components:

- **`SnakeState`**: Core simulation state (grid, snake body deque, color deque). Handles movement, eating, and bite-off (self-intersection) logic via `apply(d)`.
- **`Solver`**: Main solver class containing:
  - **Greedy phase**: Finds nearest food of target color via BFS, eats in desired order (`d[5], d[6], ...`)
  - **Beam search phase**: Explores multiple candidate states to optimize food collection order
  - **Recovery system**: After bite-off events, traces back along the old body to rebuild the color prefix
  - **Cut candidates**: Evaluates deliberate self-intersection moves to fix color mismatches
- **BFS pathfinding**: Two-priority system — first tries avoiding both food and body, then allows body overlap

## Tools

- `tools/`: Official AtCoder visualizer/generator (Rust). `vis.rs` scores solutions, `gen.rs` generates inputs.
- `pahcer_config.toml`: Test runner config — compiles, runs on seeds 0-999, scores via visualizer.
- `local_visualizer/`: Browser-based visualization (Vite/JS).
- `vis.html`: Standalone HTML visualizer.

## Constraints

- N: 8-16, M: N²/4 to 3N²/4, C: 3-7
- Max 100,000 turns per test case
- Time limit: 2 seconds (beam search budgeted at 1.9s, phase limit 1.6s)
