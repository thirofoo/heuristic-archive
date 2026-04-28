# WASM Integration Guide

tools/ の Rust コードを wasm-pack でブラウザ向けにビルドする手順。

## ディレクトリ構成

```
vis/
├── wasm/                  # wasm-pack クレート (tools/ から派生)
│   ├── Cargo.toml
│   └── src/
│       └── lib.rs         # wasm-bindgen エクスポート
├── src/                   # React アプリ
├── package.json
└── vite.config.ts
```

## Step 1: wasm クレートを作成

tools/ の lib.rs から `Input`, `Output`, `gen()`, `compute_score()` を流用し、wasm-bindgen でエクスポートする。

### Cargo.toml

```toml
[package]
name = "vis-wasm"
edition = "2021"

[lib]
crate-type = ["cdylib", "rlib"]

[dependencies]
wasm-bindgen = "0.2"
serde = { version = "1", features = ["derive"] }
serde_json = "1"
# tools/ の依存から必要なものをコピー (rand, rand_chacha, etc.)
# ただし getrandom は js feature が必要
getrandom = { version = "0.2", features = ["js"] }

[profile.release]
opt-level = "s"    # サイズ最適化
lto = true
```

### 重要: getrandom の js feature

ブラウザ環境では `getrandom` が `crypto.getRandomValues` を使う必要がある。
`getrandom = { version = "0.2", features = ["js"] }` を必ず追加する。

### lib.rs パターン

```rust
use wasm_bindgen::prelude::*;
use serde::{Serialize, Deserialize};

// tools/src/lib.rs から Input, Output, gen, compute_score を移植

#[derive(Serialize, Deserialize)]
pub struct WasmInput {
    // Input のフィールドを JSON シリアライズ可能にする
}

#[derive(Serialize, Deserialize)]
pub struct WasmResult {
    pub score: i64,
    pub error: Option<String>,
    pub states: Vec<WasmState>,  // 各ステップの状態
}

#[derive(Serialize, Deserialize)]
pub struct WasmState {
    // 問題に応じた状態表現 (グリッド, 座標, etc.)
}

#[wasm_bindgen]
pub fn generate(seed: u64, problem_type: &str) -> JsValue {
    let input = gen(seed, problem_type);
    serde_wasm_bindgen::to_value(&WasmInput::from(input)).unwrap()
}

#[wasm_bindgen]
pub fn simulate(input_json: &str, output_text: &str) -> JsValue {
    // 1. Input, Output をパース
    // 2. 各ステップの状態を記録しながらシミュレーション
    // 3. スコアと全ステップの状態を返す
    let result = WasmResult { /* ... */ };
    serde_wasm_bindgen::to_value(&result).unwrap()
}
```

### 移植時の注意点

1. **stdin/stdout の排除**: `proconio` など stdin 系マクロは使えない。文字列パースに書き換える
2. **ファイルI/O の排除**: 全てメモリ上で完結させる
3. **eprintln!/println! の排除**: `web_sys::console::log_1` か、戻り値で返す
4. **compute_score の拡張**: 各ステップの中間状態を `Vec<State>` として記録し返す
5. **SVG 生成をRust側でやる場合**: `svg` クレートをそのまま使い、文字列として返す

## Step 2: ビルド

```bash
cd vis/wasm
wasm-pack build --target web --out-dir ../src/wasm-pkg
```

`--target web` で ES module 形式の出力になる。

## Step 3: Vite 連携

### vite.config.ts

```typescript
import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import wasm from 'vite-plugin-wasm';
import topLevelAwait from 'vite-plugin-top-level-await';

export default defineConfig({
  plugins: [react(), wasm(), topLevelAwait()],
});
```

### React での読み込み

```typescript
import init, { generate, simulate } from './wasm-pkg/vis_wasm';

// 初期化 (1回だけ)
await init();

// 使用
const input = generate(BigInt(seed), problemType);
const result = simulate(JSON.stringify(input), outputText);
```

## よくあるエラーと対処

| エラー | 原因 | 対処 |
|--------|------|------|
| `unreachable` at runtime | panic が発生 | `console_error_panic_hook` を追加 |
| `random` not supported | getrandom の js feature 未設定 | Cargo.toml に追加 |
| `memory access out of bounds` | 大きなデータ | wasm メモリを増やす |
| import エラー | target 不一致 | `--target web` を確認 |

### panic デバッグの有効化

```rust
#[wasm_bindgen(start)]
pub fn init_panic_hook() {
    console_error_panic_hook::set_once();
}
```

`console_error_panic_hook = "0.1"` を依存に追加する。
