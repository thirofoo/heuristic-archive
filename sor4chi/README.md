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

#### ドライラン

結果ファイルを更新しないため、基本こちらでテストを行う。

```bash
pahcer run --freeze-best-scores --no-result-file
```

#### 更新ラン

結果ファイルを更新するため、最終結果を出すために使用する。

```bash
pahcer run
```
