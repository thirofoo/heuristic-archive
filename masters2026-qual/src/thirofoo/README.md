# Sor4chi用

## Setup

### Pahcerのセットアップ

```bash
cargo install pahcer
```

## テスト方法

### Pahcerを使用したテスト

基本的にAve log10の増減で評価する
(Seedによってスコアレンジに差がある場合、対数和を評価した方が適しているため)

#### チェックラン（今後の基本運用）

ベストスコアを凍結して比較する通常チェック。

```bash
./check.sh
```

（同等コマンド）

```bash
pahcer run --freeze-best-scores
```

### A/B/C 個別設定

- A: `pahcer_config_a.toml` + `src/solve_a.cpp`
- B: `pahcer_config_b.toml` + `src/solve_b.cpp`
- C: `pahcer_config_c.toml` + `src/solve_c.cpp`

個別に実行する場合:

```bash
pahcer run --setting-file pahcer_config_a.toml --freeze-best-scores
pahcer run --setting-file pahcer_config_b.toml --freeze-best-scores
pahcer run --setting-file pahcer_config_c.toml --freeze-best-scores
```

### A/B/C まとめて実行

- freeze比較: `./check_all.sh`
- ベスト更新あり: `./run_all.sh`

#### 更新ラン

結果ファイルを更新するため、最終結果を出すために使用する。

```bash
pahcer run
```
