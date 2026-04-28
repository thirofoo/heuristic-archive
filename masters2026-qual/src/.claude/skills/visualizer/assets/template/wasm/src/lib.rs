use wasm_bindgen::prelude::*;

// パニック時にブラウザコンソールにスタックトレースを出力
#[wasm_bindgen(start)]
pub fn init_panic_hook() {
    console_error_panic_hook::set_once();
}

// TODO: tools/src/lib.rs から以下を移植:
//   - Input 構造体 (文字列パースに書き換え)
//   - Output 構造体 (文字列パースに書き換え)
//   - gen() 関数
//   - compute_score() 関数 (中間状態を記録するよう拡張)
//
// 移植時の注意:
//   - proconio マクロを削除し、手動パースに置き換える
//   - println!/eprintln! を削除
//   - ファイルI/O を削除
//   - 戻り値は serde_wasm_bindgen::to_value() で JsValue に変換

/// 入力テキストをパースして JSON として返す
#[wasm_bindgen]
pub fn parse_input(input_text: &str) -> JsValue {
    // TODO: Input::from_str(input_text) を実装
    JsValue::NULL
}

/// seed と問題タイプから入力を生成して JSON として返す
#[wasm_bindgen]
pub fn generate(seed: u64, problem_type: &str) -> JsValue {
    // TODO: gen(seed, problem_type) を呼び出し
    JsValue::NULL
}

/// 入力テキストと出力テキストからシミュレーションを実行
/// 各ステップの状態とスコアを JSON として返す
#[wasm_bindgen]
pub fn simulate(input_text: &str, output_text: &str) -> JsValue {
    // TODO: compute_score() を拡張し、各ステップの状態を記録
    // {
    //   "score": 12345,
    //   "error": null,
    //   "states": [
    //     { "step": 0, ... },
    //     { "step": 1, ... },
    //   ]
    // }
    JsValue::NULL
}
