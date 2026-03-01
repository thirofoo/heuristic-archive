---
name: heuristic-visualizer
description: >
  競技プログラミングのヒューリスティック/マラソン系コンテスト問題のビジュアライザ Web アプリを構築する。
  PROBLEM.md の問題文と tools/ ディレクトリの Rust 実装 (入力生成・スコア計算) を読み解き、
  wasm-pack で WASM にビルドし、React + Vite + SVG でステップ再生可能なビジュアライザを生成する。
  使用タイミング: ユーザーが「ビジュアライザを作って」「visを作りたい」「問題を可視化したい」
  「ステップ再生できるようにしたい」などと要求したとき。
  対象: AtCoder Heuristic Contest, HTTF, Masters 等のマラソン系コンテストの問題。
---

# Heuristic Visualizer

PROBLEM.md と tools/ の Rust コードから、Web ベースのステップ再生ビジュアライザを構築する。

## ワークフロー

1. **問題理解** — PROBLEM.md と tools/ を読み解く
2. **プロジェクトセットアップ** — setup.sh でテンプレートを展開
3. **WASM 移植** — tools/ の Rust コードを wasm クレートに移植
4. **Visualizer 実装** — 問題に合わせた SVG コンポーネントを作成
5. **ビルド & 動作確認** — wasm-pack ビルド → npm run dev

## Step 1: 問題理解

PROBLEM.md と tools/ を読み、以下を特定する:

- **入力形式**: `Input` 構造体のフィールドと意味
- **出力形式**: `Output` 構造体 (アクション列など)
- **状態表現**: グリッド/座標/グラフなど、何をどう可視化するか
- **スコア計算**: `compute_score()` のロジック
- **入力生成**: `gen()` のロジックとバリアント (A/B/C 等)
- **状態遷移**: 各アクションで状態がどう変化するか

### tools/ の典型的な構造

```
tools/
├── Cargo.toml
├── src/
│   ├── lib.rs        # Input, Output, gen(), compute_score()
│   └── bin/
│       ├── gen.rs    # CLI 入力生成
│       └── score.rs  # CLI スコア計算
```

lib.rs が中核。`gen()` と `compute_score()` の両方を WASM にエクスポートする。

## Step 2: プロジェクトセットアップ

setup.sh を実行してテンプレートを展開:

```bash
bash <skill_dir>/scripts/setup.sh [output_dir]
```

デフォルトで `./vis` に以下が生成される:

```
vis/
├── package.json, index.html, tsconfig.json, vite.config.ts
├── src/
│   ├── App.tsx              # メインレイアウト (TODO箇所あり)
│   ├── types.ts             # 型定義 (カスタマイズ必要)
│   ├── hooks/
│   │   ├── useWasm.ts       # WASM 読み込み
│   │   └── usePlayback.ts   # ステップ再生ロジック
│   └── components/
│       ├── Controls.tsx     # 再生コントロール (そのまま使える)
│       ├── InputPanel.tsx   # 入力パネル (バリアント名をカスタマイズ)
│       └── ScorePanel.tsx   # スコア表示 (問題固有情報を追加)
└── wasm/
    ├── Cargo.toml           # 依存追加が必要
    └── src/lib.rs           # tools/ から移植する
```

テンプレートの `Controls.tsx` と `usePlayback.ts` はそのまま使える。
`InputPanel.tsx` は問題のバリアント名 (select の option) を変更する。

## Step 3: WASM 移植

tools/src/lib.rs のコードを vis/wasm/src/lib.rs に移植する。
詳細は [references/wasm-integration.md](references/wasm-integration.md) を参照。

### 移植の要点

1. **依存の調整**: tools/Cargo.toml から必要な crate をコピー。`proconio` は削除。`getrandom = { version = "0.2", features = ["js"] }` を追加
2. **パース書き換え**: `proconio` マクロ → 手動文字列パース
3. **I/O 排除**: stdin/stdout/ファイル → 引数と戻り値
4. **3 つのエクスポート関数**:
   - `parse_input(text: &str) -> JsValue` — 入力テキストをパース
   - `generate(seed: u64, problem_type: &str) -> JsValue` — 入力生成
   - `simulate(input_text: &str, output_text: &str) -> JsValue` — シミュレーション実行
5. **simulate の拡張**: `compute_score()` の各ステップで中間状態を `Vec<State>` に記録し、スコアと一緒に返す

### ビルド

```bash
cd vis
npm run wasm:build
# = cd wasm && wasm-pack build --target web --out-dir ../src/wasm-pkg
```

## Step 4: Visualizer 実装

問題に合わせた `src/components/Visualizer.tsx` を新規作成する。
パターン集は [references/visualizer-ui.md](references/visualizer-ui.md) を参照。

### 問題タイプ別の方針

**グリッド問題** (最も一般的):
- N×N の `<rect>` + `<text>` で描画
- セルの色で状態を表現 (空/壁/オブジェクト等)
- プレイヤー位置をハイライト

**座標問題** (TSP, 配置系):
- 点を `<circle>`, 辺を `<line>` で描画
- SVG viewBox で座標系をマッピング

**スケジューリング問題**:
- ガントチャート風に横棒で描画
- 時間軸をスライダーと連動

### 実装手順

1. `SimulationState` の型を `types.ts` に定義
2. `Visualizer.tsx` を作成 (SVG 描画ロジック)
3. `App.tsx` の TODO 箇所を置換して Visualizer を配置
4. `ScorePanel.tsx` に問題固有の情報を追加
5. `InputPanel.tsx` のバリアント select を問題に合わせる

## Step 5: ビルド & 動作確認

```bash
cd vis
npm run wasm:build   # WASM ビルド
npm run dev          # 開発サーバー起動
```

### 確認項目

- [ ] seed 指定で入力が生成される
- [ ] ファイルアップロードで入力/出力を読み込める
- [ ] Simulate ボタンでスコアが表示される
- [ ] ステップ再生 (前後) が動作する
- [ ] 自動再生 + 速度調整が動作する
- [ ] SVG が問題の状態を正しく描画している

## Resources

### scripts/
- `setup.sh` — プロジェクトテンプレートの展開スクリプト

### references/
- [wasm-integration.md](references/wasm-integration.md) — Rust → WASM 移植の詳細手順
- [visualizer-ui.md](references/visualizer-ui.md) — React コンポーネントと SVG 描画パターン

### assets/
- `template/` — React + Vite + WASM プロジェクトの完全なテンプレート
