/**
 * 問題固有の型定義
 * 各問題に合わせてカスタマイズする
 */

// WASM から返されるシミュレーション状態 (各ステップ)
export interface SimulationState {
  step: number;
  // 問題固有のフィールドをここに追加
  // 例: grid: string[][], position: { row: number; col: number }, score: number
}

// シミュレーション全体の結果
export interface SimulationResult {
  score: number;
  error?: string;
  states: SimulationState[];
}

// 問題の入力データ
export interface ProblemInput {
  raw: string; // 生テキスト (表示用)
  // 問題固有のフィールドをここに追加
  // 例: n: number, m: number, grid: string[][]
}
