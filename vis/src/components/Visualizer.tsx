import { useMemo } from "react";
import type { ProblemInput, SimulationResult } from "../types";

interface VisualizerProps {
  input: ProblemInput;
  result: SimulationResult | null;
  step: number;
}

const ROBOT_COLORS = [
  "#ff6b6b", "#4ecdc4", "#45b7d1", "#f9ca24", "#6ab04c",
  "#eb4d4b", "#7ed6df", "#e056fd", "#f0932b", "#22a6b3",
  "#be2edd", "#ffbe76", "#badc58", "#ff7979", "#686de0",
  "#30336b", "#95afc0", "#f8c291", "#6a89cc", "#82ccdd",
];

const CELL_SIZE = 30;
const WALL_WIDTH = 3;
const PADDING = 4;

export function Visualizer({ input, result, step }: VisualizerProps) {
  const n = input.n;
  const size = n * CELL_SIZE;

  const wallV = result ? result.wall_v : input.wall_v;
  const wallH = result ? result.wall_h : input.wall_h;

  const currentState = result?.states[step];

  // Compute visited cells up to current step
  const visitedCells = useMemo(() => {
    if (!result || !result.states.length) return null;
    const visited: boolean[][] = Array.from({ length: n }, () =>
      Array(n).fill(false)
    );
    const maxS = Math.min(step, result.states.length - 1);
    for (let s = 0; s <= maxS; s++) {
      for (const robot of result.states[s].robots) {
        visited[robot.i][robot.j] = true;
      }
    }
    return visited;
  }, [result, step, n]);

  // Find which robots are on each cell (for current step)
  const robotsOnCell = useMemo(() => {
    if (!currentState) return null;
    const map = new Map<string, number[]>();
    currentState.robots.forEach((r, idx) => {
      const key = `${r.i},${r.j}`;
      if (!map.has(key)) map.set(key, []);
      map.get(key)!.push(idx);
    });
    return map;
  }, [currentState]);

  // Determine which robots are in the periodic phase at current step
  const periodicPhase = useMemo(() => {
    if (!result) return null;
    return result.routes.map((route) => step >= route.head.length);
  }, [result, step]);

  return (
    <svg
      width={size + 2 * PADDING}
      height={size + 2 * PADDING}
      viewBox={`${-PADDING} ${-PADDING} ${size + 2 * PADDING} ${size + 2 * PADDING}`}
      className="max-w-full max-h-full"
    >
      {/* Cell backgrounds */}
      {Array.from({ length: n }, (_, i) =>
        Array.from({ length: n }, (_, j) => {
          let fill = "#16213e";
          if (result) {
            if (result.patrolled[i][j]) fill = "#1a3a1a";
            if (visitedCells?.[i][j]) fill = "#264d26";
          }
          return (
            <rect
              key={`c-${i}-${j}`}
              x={j * CELL_SIZE}
              y={i * CELL_SIZE}
              width={CELL_SIZE}
              height={CELL_SIZE}
              fill={fill}
              stroke="#2a2a4a"
              strokeWidth={0.5}
            />
          );
        })
      )}

      {/* Vertical walls (between col j and col j+1) */}
      {wallV.flatMap((row, i) =>
        row.map((v, j) => {
          if (v === 0) return null;
          const isNew =
            result &&
            result.wall_v_new.length > 0 &&
            result.wall_v_new[i]?.[j] === 1 &&
            input.wall_v[i][j] === 0;
          return (
            <line
              key={`wv-${i}-${j}`}
              x1={(j + 1) * CELL_SIZE}
              y1={i * CELL_SIZE}
              x2={(j + 1) * CELL_SIZE}
              y2={(i + 1) * CELL_SIZE}
              stroke={isNew ? "#ff6b35" : "#c8c8d0"}
              strokeWidth={WALL_WIDTH}
              strokeLinecap="round"
            />
          );
        })
      )}

      {/* Horizontal walls (between row i and row i+1) */}
      {wallH.flatMap((row, i) =>
        row.map((v, j) => {
          if (v === 0) return null;
          const isNew =
            result &&
            result.wall_h_new.length > 0 &&
            result.wall_h_new[i]?.[j] === 1 &&
            input.wall_h[i]?.[j] === 0;
          return (
            <line
              key={`wh-${i}-${j}`}
              x1={j * CELL_SIZE}
              y1={(i + 1) * CELL_SIZE}
              x2={(j + 1) * CELL_SIZE}
              y2={(i + 1) * CELL_SIZE}
              stroke={isNew ? "#ff6b35" : "#c8c8d0"}
              strokeWidth={WALL_WIDTH}
              strokeLinecap="round"
            />
          );
        })
      )}

      {/* Outer border */}
      <rect
        x={0}
        y={0}
        width={size}
        height={size}
        fill="none"
        stroke="#c8c8d0"
        strokeWidth={WALL_WIDTH}
      />

      {/* Robots */}
      {currentState?.robots.map((robot, k) => {
        const cx = robot.j * CELL_SIZE + CELL_SIZE / 2;
        const cy = robot.i * CELL_SIZE + CELL_SIZE / 2;
        const color = ROBOT_COLORS[k % ROBOT_COLORS.length];

        // Check if multiple robots share this cell
        const cellKey = `${robot.i},${robot.j}`;
        const robotsHere = robotsOnCell?.get(cellKey) ?? [];
        const idxInCell = robotsHere.indexOf(k);
        const totalHere = robotsHere.length;

        // Offset for overlapping robots
        let ox = 0;
        let oy = 0;
        if (totalHere > 1) {
          const angle = (idxInCell / totalHere) * Math.PI * 2;
          const offsetR = CELL_SIZE * 0.15;
          ox = Math.cos(angle) * offsetR;
          oy = Math.sin(angle) * offsetR;
        }

        const r = CELL_SIZE * (totalHere > 1 ? 0.25 : 0.35);
        const fcx = cx + ox;
        const fcy = cy + oy;

        // Arrow polygon based on direction
        const arrowPoints: Record<number, string> = {
          0: `${fcx},${fcy - r} ${fcx - r * 0.7},${fcy + r * 0.5} ${fcx + r * 0.7},${fcy + r * 0.5}`, // U
          1: `${fcx + r},${fcy} ${fcx - r * 0.5},${fcy - r * 0.7} ${fcx - r * 0.5},${fcy + r * 0.7}`, // R
          2: `${fcx},${fcy + r} ${fcx - r * 0.7},${fcy - r * 0.5} ${fcx + r * 0.7},${fcy - r * 0.5}`, // D
          3: `${fcx - r},${fcy} ${fcx + r * 0.5},${fcy - r * 0.7} ${fcx + r * 0.5},${fcy + r * 0.7}`, // L
        };

        const isPeriodic = periodicPhase?.[k] ?? false;

        return (
          <g key={`robot-${k}`}>
            <polygon
              points={arrowPoints[robot.d]}
              fill={color}
              stroke={isPeriodic ? "#fff" : "#888"}
              strokeWidth={isPeriodic ? 1.5 : 0.8}
              opacity={0.9}
            />
          </g>
        );
      })}
    </svg>
  );
}
