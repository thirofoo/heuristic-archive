# 第一回マスターズ選手権-決勝-(オープンコンテスト) ビジュアライザ

## 使用方法
このアプリを動かすには以下の環境が必要です:
- Rustの実行環境
- wasm-pack (https://developer.mozilla.org/ja/docs/WebAssembly/Rust_to_Wasm)
- nodeの実行環境
- yarn

これらを用意するためには
```bash
cargo install wasm-pack
npm install -g yarn
```
などを実行する必要があります。

初回実行時には以下のコマンドを実行してください:
```bash
yarn  # nodeのモジュールのインストール
cd wasm && wasm-pack build --target web --out-dir ../public/wasm && cd .. # Rustのwasm化
yarn dev # サーバーの実行
```
これでローカルにwebアプリがホスティングされるので、コンソール上に表示されるURLにアクセスしてください。テンプレートの状態のビジュアライザが表示されると思います。

## ビジュアライザ開発の手順
初期状態ではコンテストの問題に固有の情報が含まれていないので、URLにアクセスしてもページの雛形しか出てきません。
Rustのテンプレートを適切に編集をして、問題ごとのビジュアライザを作っていきます。

具体的にはRustの関数を3つ実装する必要があります(wasm/src/lib.rs):
- `gen(seed: i32) -> String`: seedを与えてStringの形で入力ファイルを出力する関数
- `vis(_input: String, _output: String, turn: usize) -> Ret`: 入力・出力・ターン数を与えて、その時点のスコア・エラー文・SVGの画像を返す関数
- `get_max_turn(_input: String, _output: String) -> usize`: 入力・出力を与えたときに、その出力が何ターンからなるものかを計算する関数(スライダーで動かすときなどに必要)

これらを適切に実装して、wasmのディレクトリに移動し
```bash
wasm-pack build --target web --out-dir ../public/wasm
```
Rustの関数をJavaScriptから呼び出せるようにwasm化するとビジュアライザが動くようになります。

具体的な実装は、yukicoder-score-contest002ブランチやchokduai-contest-005ブランチを参考にしてください。

