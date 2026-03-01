import { useMemo } from "react";
import type { SimulationResult } from "../types";

interface ScorePanelProps {
  result: SimulationResult | null;
  step: number;
}

const DIR_NAMES = ["U", "R", "D", "L"];

export function ScorePanel({ result, step }: ScorePanelProps) {
  if (!result) return null;

  const currentState = result.states[step];

  // Check which robots are in periodic phase
  const robotPhases = useMemo(() => {
    return result.routes.map((route) => ({
      headLen: route.head.length,
      tailLen: route.tail.length,
      cycleLen: Math.max(route.tail.length - 1, 1),
      isPeriodic: step >= route.head.length,
    }));
  }, [result, step]);

  // Count visited cells up to this step
  const visitedCount = useMemo(() => {
    if (!result.states.length) return 0;
    const visited = new Set<string>();
    const maxS = Math.min(step, result.states.length - 1);
    for (let s = 0; s <= maxS; s++) {
      for (const robot of result.states[s].robots) {
        visited.add(`${robot.i},${robot.j}`);
      }
    }
    return visited.size;
  }, [result, step]);

  const patrolledCount = useMemo(() => {
    if (!result.patrolled.length) return 0;
    return result.patrolled.flat().filter(Boolean).length;
  }, [result]);

  return (
    <div className="w-64 bg-[var(--surface)] border-l border-[var(--primary)] p-4 flex flex-col gap-3 overflow-y-auto text-sm">
      {/* Score */}
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

      {/* Step */}
      <div>
        <div className="text-xs text-[var(--text-muted)] uppercase tracking-wider">
          Step
        </div>
        <div className="text-lg">
          {step} / {result.states.length - 1}
        </div>
      </div>

      {/* Cost breakdown */}
      <div>
        <div className="text-xs text-[var(--text-muted)] uppercase tracking-wider mb-1">
          Cost (V = {result.cost.toLocaleString()})
        </div>
        <div className="space-y-0.5 text-xs">
          <div className="flex justify-between">
            <span className="text-[var(--text-muted)]">
              AK * (K-1) = {result.ak} * {result.num_robots - 1}
            </span>
            <span>{(result.ak * (result.num_robots - 1)).toLocaleString()}</span>
          </div>
          <div className="flex justify-between">
            <span className="text-[var(--text-muted)]">
              AM * M = {result.am} * {result.total_states}
            </span>
            <span>{(result.am * result.total_states).toLocaleString()}</span>
          </div>
          <div className="flex justify-between">
            <span className="text-[var(--text-muted)]">
              AW * W = {result.aw} * {result.num_new_walls}
            </span>
            <span>{(result.aw * result.num_new_walls).toLocaleString()}</span>
          </div>
        </div>
      </div>

      {/* Coverage */}
      <div>
        <div className="text-xs text-[var(--text-muted)] uppercase tracking-wider mb-1">
          Coverage
        </div>
        <div className="space-y-0.5 text-xs">
          <div className="flex justify-between">
            <span className="text-[var(--text-muted)]">Visited (so far)</span>
            <span>
              {visitedCount} / {result.n * result.n}
            </span>
          </div>
          <div className="flex justify-between">
            <span className="text-[var(--text-muted)]">Patrolled (periodic)</span>
            <span
              className={
                patrolledCount === result.n * result.n
                  ? "text-green-400"
                  : "text-red-400"
              }
            >
              {patrolledCount} / {result.n * result.n}
            </span>
          </div>
        </div>
      </div>

      {/* Robot info */}
      <div>
        <div className="text-xs text-[var(--text-muted)] uppercase tracking-wider mb-1">
          Robots ({result.num_robots})
        </div>
        <div className="space-y-1 max-h-48 overflow-y-auto">
          {currentState?.robots.map((robot, k) => {
            const phase = robotPhases[k];
            return (
              <div
                key={k}
                className="flex items-center gap-2 text-xs bg-[var(--bg)] rounded px-2 py-1"
              >
                <span className="font-mono font-bold" style={{ color: ROBOT_COLORS[k] }}>
                  #{k}
                </span>
                <span>
                  ({robot.i},{robot.j}) {DIR_NAMES[robot.d]}
                </span>
                <span className="text-[var(--text-muted)]">s={robot.s}</span>
                <span
                  className={`ml-auto text-[10px] px-1 rounded ${
                    phase?.isPeriodic
                      ? "bg-green-900/40 text-green-400"
                      : "bg-yellow-900/40 text-yellow-400"
                  }`}
                >
                  {phase?.isPeriodic ? "cycle" : "init"}
                </span>
              </div>
            );
          })}
        </div>
      </div>

      {/* Legend */}
      <div className="mt-auto pt-2 border-t border-[var(--primary)]">
        <div className="text-xs text-[var(--text-muted)] uppercase tracking-wider mb-1">
          Legend
        </div>
        <div className="space-y-1 text-xs">
          <div className="flex items-center gap-2">
            <div className="w-3 h-3 rounded" style={{ background: "#16213e" }} />
            <span className="text-[var(--text-muted)]">Not visited</span>
          </div>
          <div className="flex items-center gap-2">
            <div className="w-3 h-3 rounded" style={{ background: "#1a3a1a" }} />
            <span className="text-[var(--text-muted)]">Patrolled (periodic)</span>
          </div>
          <div className="flex items-center gap-2">
            <div className="w-3 h-3 rounded" style={{ background: "#264d26" }} />
            <span className="text-[var(--text-muted)]">Visited (so far)</span>
          </div>
          <div className="flex items-center gap-2">
            <div className="w-3 h-1 rounded" style={{ background: "#c8c8d0" }} />
            <span className="text-[var(--text-muted)]">Original wall</span>
          </div>
          <div className="flex items-center gap-2">
            <div className="w-3 h-1 rounded" style={{ background: "#ff6b35" }} />
            <span className="text-[var(--text-muted)]">New wall</span>
          </div>
        </div>
      </div>
    </div>
  );
}

const ROBOT_COLORS = [
  "#ff6b6b", "#4ecdc4", "#45b7d1", "#f9ca24", "#6ab04c",
  "#eb4d4b", "#7ed6df", "#e056fd", "#f0932b", "#22a6b3",
  "#be2edd", "#ffbe76", "#badc58", "#ff7979", "#686de0",
  "#30336b", "#95afc0", "#f8c291", "#6a89cc", "#82ccdd",
];
