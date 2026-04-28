# Visualizer UI Patterns

React + Vite + TypeScript でビジュアライザ UI を構成するパターン集。

## コンポーネント構成

```
src/
├── App.tsx                # メインレイアウト
├── components/
│   ├── Visualizer.tsx     # SVG 描画メイン
│   ├── Controls.tsx       # 再生コントロール
│   ├── InputPanel.tsx     # 入力生成 & ファイル読み込み
│   └── ScorePanel.tsx     # スコア表示
├── hooks/
│   ├── useWasm.ts         # WASM 初期化・呼び出し
│   └── usePlayback.ts    # ステップ再生ロジック
├── wasm-pkg/              # wasm-pack 出力
└── types.ts               # 型定義
```

## 型定義パターン (types.ts)

問題ごとにカスタマイズする。以下は汎用的な骨格:

```typescript
// WASM から返される状態
export interface SimulationState {
  step: number;
  // 問題固有のフィールド (grid, positions, etc.)
  [key: string]: unknown;
}

export interface SimulationResult {
  score: number;
  error?: string;
  states: SimulationState[];
}

export interface ProblemInput {
  // 問題固有 (N, M, grid, etc.)
  [key: string]: unknown;
}
```

## WASM Hook (useWasm.ts)

```typescript
import { useState, useEffect } from 'react';

export function useWasm() {
  const [ready, setReady] = useState(false);
  const [wasm, setWasm] = useState<typeof import('./wasm-pkg/vis_wasm') | null>(null);

  useEffect(() => {
    (async () => {
      const mod = await import('./wasm-pkg/vis_wasm');
      await mod.default();  // init()
      setWasm(mod);
      setReady(true);
    })();
  }, []);

  return { wasm, ready };
}
```

## Playback Hook (usePlayback.ts)

```typescript
import { useState, useRef, useCallback } from 'react';

export function usePlayback(maxStep: number) {
  const [step, setStep] = useState(0);
  const [playing, setPlaying] = useState(false);
  const [speed, setSpeed] = useState(5); // steps per second
  const timerRef = useRef<number | null>(null);

  const play = useCallback(() => {
    setPlaying(true);
    const tick = () => {
      setStep(s => {
        if (s >= maxStep) { setPlaying(false); return s; }
        return s + 1;
      });
      timerRef.current = window.setTimeout(tick, 1000 / speed);
    };
    tick();
  }, [maxStep, speed]);

  const pause = useCallback(() => {
    setPlaying(false);
    if (timerRef.current) clearTimeout(timerRef.current);
  }, []);

  const stepForward = useCallback(() => {
    setStep(s => Math.min(s + 1, maxStep));
  }, [maxStep]);

  const stepBackward = useCallback(() => {
    setStep(s => Math.max(s - 1, 0));
  }, []);

  const jumpTo = useCallback((n: number) => {
    setStep(Math.max(0, Math.min(n, maxStep)));
  }, [maxStep]);

  return { step, playing, speed, setSpeed, play, pause, stepForward, stepBackward, jumpTo };
}
```

## SVG 描画パターン

### グリッド問題

```tsx
interface GridVisualizerProps {
  grid: string[][];
  cellSize: number;
  highlight?: { row: number; col: number };
}

function GridVisualizer({ grid, cellSize, highlight }: GridVisualizerProps) {
  const rows = grid.length;
  const cols = grid[0].length;
  return (
    <svg width={cols * cellSize} height={rows * cellSize} xmlns="http://www.w3.org/2000/svg">
      {grid.map((row, r) =>
        row.map((cell, c) => (
          <g key={`${r}-${c}`}>
            <rect
              x={c * cellSize} y={r * cellSize}
              width={cellSize} height={cellSize}
              fill={getCellColor(cell)}
              stroke="#ccc" strokeWidth={0.5}
            />
            {cell !== '.' && (
              <text
                x={c * cellSize + cellSize / 2}
                y={r * cellSize + cellSize / 2}
                textAnchor="middle" dominantBaseline="central"
                fontSize={cellSize * 0.6}
              >
                {cell}
              </text>
            )}
            {highlight?.row === r && highlight?.col === c && (
              <rect
                x={c * cellSize} y={r * cellSize}
                width={cellSize} height={cellSize}
                fill="none" stroke="red" strokeWidth={2}
              />
            )}
          </g>
        ))
      )}
    </svg>
  );
}
```

### 座標・グラフ問題

```tsx
interface PointVisualizerProps {
  points: { x: number; y: number; label?: string }[];
  edges?: { from: number; to: number }[];
  width: number;
  height: number;
  activeIndex?: number;
}

function PointVisualizer({ points, edges, width, height, activeIndex }: PointVisualizerProps) {
  // 座標をSVG空間にマッピング
  const scale = (v: number, max: number, size: number) => (v / max) * (size - 40) + 20;

  const maxX = Math.max(...points.map(p => p.x));
  const maxY = Math.max(...points.map(p => p.y));

  return (
    <svg width={width} height={height} xmlns="http://www.w3.org/2000/svg">
      {edges?.map((e, i) => (
        <line key={i}
          x1={scale(points[e.from].x, maxX, width)}
          y1={scale(points[e.from].y, maxY, height)}
          x2={scale(points[e.to].x, maxX, width)}
          y2={scale(points[e.to].y, maxY, height)}
          stroke="#999" strokeWidth={1}
        />
      ))}
      {points.map((p, i) => (
        <circle key={i}
          cx={scale(p.x, maxX, width)}
          cy={scale(p.y, maxY, height)}
          r={i === activeIndex ? 6 : 4}
          fill={i === activeIndex ? 'red' : '#4a90d9'}
        />
      ))}
    </svg>
  );
}
```

## Controls コンポーネント

```tsx
interface ControlsProps {
  step: number;
  maxStep: number;
  playing: boolean;
  speed: number;
  onPlay: () => void;
  onPause: () => void;
  onStepForward: () => void;
  onStepBackward: () => void;
  onJumpTo: (n: number) => void;
  onSpeedChange: (s: number) => void;
}

function Controls(props: ControlsProps) {
  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: 8, padding: 8 }}>
      <button onClick={props.onStepBackward} disabled={props.step === 0}>⏮</button>
      {props.playing
        ? <button onClick={props.onPause}>⏸</button>
        : <button onClick={props.onPlay} disabled={props.step >= props.maxStep}>▶</button>
      }
      <button onClick={props.onStepForward} disabled={props.step >= props.maxStep}>⏭</button>
      <input
        type="range" min={0} max={props.maxStep}
        value={props.step}
        onChange={e => props.onJumpTo(Number(e.target.value))}
        style={{ flex: 1 }}
      />
      <span>{props.step} / {props.maxStep}</span>
      <label>
        Speed:
        <input type="range" min={1} max={60} value={props.speed}
          onChange={e => props.onSpeedChange(Number(e.target.value))}
        />
      </label>
    </div>
  );
}
```

## InputPanel パターン

**UX 方針**: Generate ボタンは不要。seed / problemType の変更で自動再生成する。
出力テキストエリアは常時表示し、ファイルアップロード時はテキストエリアに反映する (controlled textarea)。

```tsx
interface InputPanelProps {
  onInput: (type: "input" | "output", text: string) => void;
  onGenerate: (seed: number, problemType: string) => void;
  outputText: string; // controlled value for textarea
}

function InputPanel({ onInput, onGenerate, outputText }: InputPanelProps) {
  const [seed, setSeed] = useState(0);
  const [problemType, setProblemType] = useState('A');
  const onGenerateRef = useRef(onGenerate);
  onGenerateRef.current = onGenerate;

  // seed / problemType 変更で自動再生成 (Generate ボタン不要)
  useEffect(() => {
    onGenerateRef.current(seed, problemType);
  }, [seed, problemType]);

  const handleFile = (e: React.ChangeEvent<HTMLInputElement>, type: 'input' | 'output') => {
    const file = e.target.files?.[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = () => onInput(type, reader.result as string);
    reader.readAsText(file);
  };

  return (
    <div>
      <div>
        <label>Seed: <input type="number" value={seed} onChange={e => setSeed(Number(e.target.value))} /></label>
        <select value={problemType} onChange={e => setProblemType(e.target.value)}>
          {/* 問題タイプのオプション - 問題に応じて動的に変更 */}
        </select>
        {/* Generate ボタンなし — useEffect で自動実行 */}
      </div>
      <div>
        <label>Input: <input type="file" onChange={e => handleFile(e, 'input')} /></label>
        <label>Output: <input type="file" onChange={e => handleFile(e, 'output')} /></label>
      </div>
      {/* 出力テキストエリア — 常時表示、controlled */}
      <textarea
        placeholder="Output text (paste or load file)"
        rows={4}
        value={outputText}
        onChange={e => onInput('output', e.target.value)}
      />
    </div>
  );
}
```

## レイアウト例

```
┌─────────────────────────────────────────────┐
│  InputPanel (seed, file upload, paste)      │
├───────────────────────────┬─────────────────┤
│                           │  ScorePanel     │
│   SVG Visualizer          │  Step info      │
│   (メイン描画領域)         │  状態詳細       │
│                           │                 │
├───────────────────────────┴─────────────────┤
│  Controls (play/pause, slider, speed)       │
└─────────────────────────────────────────────┘
```

## スタイリング

Tailwind CSS を推奨。最小限の設定:

```bash
npm install -D tailwindcss @tailwindcss/vite
```

```typescript
// vite.config.ts
import tailwindcss from '@tailwindcss/vite';
export default defineConfig({
  plugins: [react(), tailwindcss(), wasm(), topLevelAwait()],
});
```

```css
/* src/index.css */
@import "tailwindcss";
```

## App.tsx 自動リアクティブパターン

**UX 方針**: ボタン操作なしで、入力変更 → 自動再生成、出力変更 → 自動シミュレートを実現する。
stale closure を回避するため、ref パターンで最新値を保持する。

```tsx
export default function App() {
  const { wasm, ready } = useWasm();
  const [input, setInput] = useState<ProblemInput | null>(null);
  const [outputText, setOutputText] = useState<string>("");
  const [result, setResult] = useState<SimulationResult | null>(null);

  const maxStep = result ? result.states.length - 1 : 0;
  const playback = usePlayback(maxStep);

  // Ref で最新値を保持 (stale closure 回避)
  const outputTextRef = useRef(outputText);
  outputTextRef.current = outputText;
  const inputRef = useRef(input);
  inputRef.current = input;
  const wasmRef = useRef(wasm);
  wasmRef.current = wasm;
  const playbackRef = useRef(playback);
  playbackRef.current = playback;

  // コアのシミュレート関数 (依存なし)
  const doSimulate = useCallback((inputRaw: string, outText: string) => {
    if (!wasmRef.current || !outText) return;
    try {
      const res = wasmRef.current.simulate(inputRaw, outText);
      setResult(res as unknown as SimulationResult);
      playbackRef.current.jumpTo(0);
    } catch (e) {
      console.error("Simulate error:", e);
    }
  }, []);

  // 入力生成ハンドラ — 出力があれば自動シミュレート
  const handleGenerate = useCallback((seed: number, problemType: string) => {
    if (!wasmRef.current) return;
    const generated = wasmRef.current.generate(BigInt(seed), problemType) as unknown as ProblemInput;
    setInput(generated);
    if (outputTextRef.current) {
      doSimulate(generated.raw, outputTextRef.current);
    } else {
      setResult(null);
    }
  }, [doSimulate]);

  // 入力テキスト / 出力テキスト変更ハンドラ
  const handleInputText = useCallback((type: "input" | "output", text: string) => {
    if (type === "input" && wasmRef.current) {
      const parsed = wasmRef.current.parse_input(text) as unknown as ProblemInput;
      setInput(parsed);
      if (outputTextRef.current) {
        doSimulate(parsed.raw, outputTextRef.current);
      } else {
        setResult(null);
      }
    } else if (type === "output") {
      setOutputText(text);
    }
  }, [doSimulate]);

  // 出力テキスト変更時に自動シミュレート (Simulate ボタン不要)
  useEffect(() => {
    if (inputRef.current && outputText) {
      doSimulate(inputRef.current.raw, outputText);
    } else if (!outputText) {
      setResult(null);
    }
  }, [outputText, doSimulate]);

  // ... render with InputPanel, Visualizer, ScorePanel, Controls
}
```

### ポイント

- `doSimulate` は依存配列が空 `[]` なので、先に定義可能
- `handleGenerate` / `handleInputText` は `doSimulate` のみに依存
- `useEffect` で `outputText` を監視し、変更時に自動シミュレート
- `InputPanel` に `outputText` を props で渡し、controlled textarea で管理
