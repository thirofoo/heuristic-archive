import { useMemo } from "react";
import type { SimulationResult } from "../types";

interface ScorePanelProps {
  result: SimulationResult | null;
  step: number;
  onRobotClick?: (robotIndex: number) => void;
}

const DIR_NAMES = ["U", "R", "D", "L"];

const ROBOT_COLORS = [
  "#ff6b6b", "#4ecdc4", "#45b7d1", "#f9ca24", "#6ab04c",
  "#eb4d4b", "#7ed6df", "#e056fd", "#f0932b", "#22a6b3",
  "#be2edd", "#ffbe76", "#badc58", "#ff7979", "#686de0",
  "#30336b", "#95afc0", "#f8c291", "#6a89cc", "#82ccdd",
];

function ProgressBar({ value, max, color }: { value: number; max: number; color: string }) {
  const pct = max > 0 ? (value / max) * 100 : 0;
  return (
    <div className="w-full h-1.5 bg-[var(--bg)] rounded-full overflow-hidden">
      <div
        className="h-full rounded-full transition-all duration-200"
        style={{ width: `${pct}%`, backgroundColor: color }}
      />
    </div>
  );
}

export function ScorePanel({ result, step, onRobotClick }: ScorePanelProps) {
  if (!result) return null;

  const currentState = result.states[step];
  const totalCells = result.n * result.n;

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

  const isAllPatrolled = patrolledCount === totalCells;

  return (
    <div className="w-72 bg-[var(--surface)] border-l border-[var(--primary)] flex flex-col overflow-hidden">
      {/* Score - prominent section */}
      <div className="px-4 py-3 border-b border-[var(--primary)]">
        <div className="flex items-baseline justify-between">
          <span className="text-[10px] text-[var(--text-muted)] uppercase tracking-wider font-semibold">Score</span>
          {result.error && (
            <span className="text-[10px] px-1.5 py-0.5 rounded bg-red-900/30 text-red-400 font-medium">
              WA
            </span>
          )}
        </div>
        <div className="text-3xl font-bold text-[var(--accent)] tabular-nums tracking-tight">
          {result.score.toLocaleString()}
        </div>
        {result.error && (
          <div className="text-[11px] text-red-400 mt-1">{result.error}</div>
        )}
      </div>

      {/* Scrollable content */}
      <div className="flex-1 overflow-y-auto px-4 py-3 space-y-4">
        {/* Coverage with progress bars */}
        <div>
          <div className="text-[10px] text-[var(--text-muted)] uppercase tracking-wider font-semibold mb-2">
            Coverage
          </div>
          <div className="space-y-2">
            <div>
              <div className="flex justify-between text-xs mb-0.5">
                <span className="text-[var(--text-muted)]">Visited</span>
                <span className="font-mono tabular-nums">
                  {visitedCount}<span className="text-[var(--text-muted)]"> / {totalCells}</span>
                </span>
              </div>
              <ProgressBar value={visitedCount} max={totalCells} color="#45b7d1" />
            </div>
            <div>
              <div className="flex justify-between text-xs mb-0.5">
                <span className="text-[var(--text-muted)]">Patrolled</span>
                <span className={`font-mono tabular-nums ${isAllPatrolled ? "text-green-400" : "text-red-400"}`}>
                  {patrolledCount}<span className="text-[var(--text-muted)]"> / {totalCells}</span>
                </span>
              </div>
              <ProgressBar value={patrolledCount} max={totalCells} color={isAllPatrolled ? "#4ecdc4" : "#e94560"} />
            </div>
          </div>
        </div>

        {/* Cost breakdown */}
        <div>
          <div className="text-[10px] text-[var(--text-muted)] uppercase tracking-wider font-semibold mb-1.5">
            Cost
          </div>
          <div className="text-lg font-semibold tabular-nums mb-1.5">
            V = {result.cost.toLocaleString()}
          </div>
          <div className="space-y-1 text-xs">
            <div className="flex justify-between">
              <span className="text-[var(--text-muted)] font-mono">
                AK({result.ak}) x (K-1)({result.num_robots - 1})
              </span>
              <span className="font-mono tabular-nums">{(result.ak * (result.num_robots - 1)).toLocaleString()}</span>
            </div>
            <div className="flex justify-between">
              <span className="text-[var(--text-muted)] font-mono">
                AM({result.am}) x M({result.total_states})
              </span>
              <span className="font-mono tabular-nums">{(result.am * result.total_states).toLocaleString()}</span>
            </div>
            <div className="flex justify-between">
              <span className="text-[var(--text-muted)] font-mono">
                AW({result.aw}) x W({result.num_new_walls})
              </span>
              <span className="font-mono tabular-nums">{(result.aw * result.num_new_walls).toLocaleString()}</span>
            </div>
          </div>
        </div>

        {/* Robot info */}
        <div>
          <div className="text-[10px] text-[var(--text-muted)] uppercase tracking-wider font-semibold mb-1.5">
            Robots ({result.num_robots})
          </div>
          <div className="space-y-0.5 max-h-52 overflow-y-auto">
            {currentState?.robots.map((robot, k) => {
              const phase = robotPhases[k];
              return (
                <button
                  key={k}
                  onClick={() => onRobotClick?.(k)}
                  className="w-full flex items-center gap-1.5 text-[11px] bg-[var(--bg)] rounded px-2 py-1 hover:bg-[var(--primary)] transition cursor-pointer text-left"
                  title="Click to view automaton"
                >
                  <span
                    className="w-2 h-2 rounded-full flex-shrink-0"
                    style={{ backgroundColor: ROBOT_COLORS[k % ROBOT_COLORS.length] }}
                  />
                  <span className="font-mono font-semibold text-[var(--text-muted)]">
                    #{k}
                  </span>
                  <span className="font-mono">
                    ({robot.i},{robot.j})
                  </span>
                  <span className="text-[var(--text-muted)]">{DIR_NAMES[robot.d]}</span>
                  <span className="text-[var(--text-muted)]">s{robot.s}</span>
                  <span
                    className={`ml-auto text-[9px] px-1 py-px rounded font-medium ${
                      phase?.isPeriodic
                        ? "bg-green-900/40 text-green-400"
                        : "bg-yellow-900/40 text-yellow-400"
                    }`}
                  >
                    {phase?.isPeriodic ? "cycle" : "init"}
                  </span>
                </button>
              );
            })}
          </div>
        </div>
      </div>

      {/* Legend - sticky at bottom */}
      <div className="px-4 py-2.5 border-t border-[var(--primary)] bg-[var(--surface)]">
        <div className="grid grid-cols-2 gap-x-3 gap-y-1 text-[10px]">
          <div className="flex items-center gap-1.5">
            <div className="w-2.5 h-2.5 rounded-sm" style={{ background: "#1e2740" }} />
            <span className="text-[var(--text-muted)]">Unvisited</span>
          </div>
          <div className="flex items-center gap-1.5">
            <div className="w-2.5 h-2.5 rounded-sm" style={{ background: "#2d5a3d" }} />
            <span className="text-[var(--text-muted)]">Visited</span>
          </div>
          <div className="flex items-center gap-1.5">
            <div className="w-2.5 h-2.5 rounded-sm" style={{ background: "#1a3a2a" }} />
            <span className="text-[var(--text-muted)]">Patrolled</span>
          </div>
          <div className="flex items-center gap-1.5">
            <div className="w-2.5 h-1 rounded-sm" style={{ background: "#ff6b35" }} />
            <span className="text-[var(--text-muted)]">New wall</span>
          </div>
        </div>
      </div>
    </div>
  );
}
