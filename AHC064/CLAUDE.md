# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Problem

AHC064 - Non-Crossing Railcar Rearrangement. R=10本の出発線・待避線間で車両移動し、各出発線に目標順序で並べる。同一ターン内の移動経路は交差不可。最大4000ターン。スコア=100R+4000-T（完全一致時）。

## Build & Run

```bash
# コンパイル
g++ -std=c++20 -O2 main.cpp

# 実行
./a.out < tools/in/0000.txt > tools/out/0000.txt 2> tools/err/0000.txt

# ビジュアライザ（スコア算出）
cd tools && cargo run --bin vis --release ./in/0000.txt ./out/0000.txt

# pahcer（一括テスト、seed 0-99）
pahcer
```

## Architecture

- `main.cpp`: 単一ファイル構成。C++20 + ACL (atcoder/all)
  - `utility::timer`: 経過時間計測（ms）
  - `rand_int/rand_double/gaussian`: Xorshift乱数
  - `temp/prob`: 焼きなまし用温度・採用確率
  - `Trace/restore`: ビームサーチ結果復元
  - `State`: 状態管理（id自動採番、score比較）
  - `Solver`: input→solve→output のエントリポイント
- `pahcer_config.toml`: pahcerテストランナー設定（seed 0-99、最大化問題）
- `tools/`: ジェネレータ・ビジュアライザ（Rust、.gitignore済）

## Constraints

- 実行時間制限: 2sec（TIME_LIMIT=2950ms）
- メモリ制限: 1024MiB
- 出発線容量: 15両、待避線容量: 20両
- 初期配置: 各出発線10両、待避線空
- 出力形式: スコアは標準エラーに `Score = {score}` で出力（pahcer score_regex対応）
