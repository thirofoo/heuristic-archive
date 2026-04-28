import type { SimulationResult } from "../types";

interface ScorePanelProps {
  result: SimulationResult | null;
  step: number;
}

export function ScorePanel({ result, step }: ScorePanelProps) {
  if (!result) return null;

  return (
    <div className="w-60 bg-[var(--surface)] border-l border-[var(--primary)] p-4 flex flex-col gap-3 overflow-y-auto">
      <div>
        <div className="text-xs text-[var(--text-muted)] uppercase tracking-wider">
          Score
        </div>
        <div className="text-2xl font-bold text-[var(--accent)]">
          {result.score.toLocaleString()}
        </div>
      </div>

      {result.error && (
        <div className="text-sm text-red-400 bg-red-900/20 p-2 rounded">
          {result.error}
        </div>
      )}

      <div>
        <div className="text-xs text-[var(--text-muted)] uppercase tracking-wider">
          Step
        </div>
        <div className="text-lg">
          {step} / {result.states.length - 1}
        </div>
      </div>

      {/* TODO: 問題固有の情報を追加 */}
      {/* 例: 配置済みの鉱石数、現在位置、残りアクション数など */}
    </div>
  );
}
