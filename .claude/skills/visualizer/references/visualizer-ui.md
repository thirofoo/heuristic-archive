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
│   ├── usePlayback.ts    # ステップ再生ロジック
│   └── useSimulation.ts   # シミュレーション状態管理
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

```tsx
function InputPanel({ onInput, onGenerate, wasm }) {
  const [seed, setSeed] = useState(0);
  const [problemType, setProblemType] = useState('A');

  const handleFileUpload = (e: React.ChangeEvent<HTMLInputElement>, type: 'input' | 'output') => {
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
        <button onClick={() => onGenerate(seed, problemType)}>Generate</button>
      </div>
      <div>
        <label>Input: <input type="file" onChange={e => handleFileUpload(e, 'input')} /></label>
        <label>Output: <input type="file" onChange={e => handleFileUpload(e, 'output')} /></label>
      </div>
      <div>
        <label>Output (paste):
          <textarea onChange={e => onInput('output', e.target.value)} rows={4} />
        </label>
      </div>
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
