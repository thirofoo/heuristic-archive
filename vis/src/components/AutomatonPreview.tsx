import { useEffect, useRef, useCallback } from "react";
import mermaid from "mermaid";
import type { RobotAutomaton } from "../types";

interface AutomatonPreviewProps {
  robot: RobotAutomaton;
  robotIndex: number;
  color: string;
  currentState: number | null;
  onClose: () => void;
}

mermaid.initialize({
  startOnLoad: false,
  theme: "dark",
  themeVariables: {
    primaryColor: "#0f3460",
    primaryTextColor: "#eee",
    primaryBorderColor: "#45b7d1",
    lineColor: "#999",
    secondaryColor: "#16213e",
    tertiaryColor: "#1a1a2e",
    fontFamily: "monospace",
    fontSize: "12px",
  },
  flowchart: {
    curve: "basis",
    padding: 12,
  },
});

function buildMermaidDef(robot: RobotAutomaton, currentState: number | null): string {
  const lines: string[] = ["stateDiagram-v2"];

  // initial transition
  lines.push(`  [*] --> s${0}`);

  for (let s = 0; s < robot.m; s++) {
    const t = robot.transitions[s];

    // Use slash format to avoid colon parsing issues
    lines.push(`  s${s} --> s${t.b0} : open / ${t.a0}`);
    lines.push(`  s${s} --> s${t.b1} : wall / ${t.a1}`);
  }

  // Highlight current state
  if (currentState !== null && currentState >= 0 && currentState < robot.m) {
    lines.push(`  classDef active fill:#e94560,color:#fff,stroke:#fff`);
    lines.push(`  class s${currentState} active`);
  }

  return lines.join("\n");
}

export function AutomatonPreview({
  robot,
  robotIndex,
  color,
  currentState,
  onClose,
}: AutomatonPreviewProps) {
  const containerRef = useRef<HTMLDivElement>(null);

  const renderDiagram = useCallback(async () => {
    if (!containerRef.current) return;
    const def = buildMermaidDef(robot, currentState);
    const id = `mermaid-robot-${robotIndex}-${Date.now()}`;
    try {
      const { svg } = await mermaid.render(id, def);
      if (containerRef.current) {
        containerRef.current.innerHTML = svg;
      }
    } catch (e) {
      console.error("Mermaid render error:", e);
      if (containerRef.current) {
        containerRef.current.innerHTML = `<div class="text-red-400 text-xs p-2">Failed to render diagram</div>`;
      }
    }
  }, [robot, robotIndex, currentState]);

  useEffect(() => {
    renderDiagram();
  }, [renderDiagram]);

  // Close on Escape
  useEffect(() => {
    const handler = (e: KeyboardEvent) => {
      if (e.key === "Escape") onClose();
    };
    window.addEventListener("keydown", handler);
    return () => window.removeEventListener("keydown", handler);
  }, [onClose]);

  const DIR_FULL: Record<string, string> = { U: "Up", R: "Right", D: "Down", L: "Left" };

  return (
    <div
      className="fixed inset-0 z-50 flex items-center justify-center bg-black/60"
      onClick={onClose}
    >
      <div
        className="bg-[var(--surface)] border border-[var(--primary)] rounded-lg shadow-2xl max-w-[700px] max-h-[90vh] overflow-auto"
        onClick={(e) => e.stopPropagation()}
      >
        {/* Header */}
        <div className="flex items-center justify-between px-4 py-3 border-b border-[var(--primary)]">
          <div className="flex items-center gap-2">
            <span
              className="w-3 h-3 rounded-full"
              style={{ backgroundColor: color }}
            />
            <span className="font-semibold text-sm">
              Robot #{robotIndex} Automaton
            </span>
            <span className="text-xs text-[var(--text-muted)]">
              {robot.m} states, start ({robot.i0},{robot.j0}) {DIR_FULL[robot.d0] ?? robot.d0}
            </span>
          </div>
          <button
            onClick={onClose}
            className="text-[var(--text-muted)] hover:text-[var(--text)] transition px-2 py-1 text-lg leading-none"
          >
            &times;
          </button>
        </div>

        {/* Diagram */}
        <div ref={containerRef} className="p-4 flex justify-center min-h-[200px]" />

        {/* Transition table */}
        <div className="px-4 pb-4">
          <div className="text-[10px] text-[var(--text-muted)] uppercase tracking-wider font-semibold mb-1">
            Transition Table
          </div>
          <div className="overflow-x-auto">
            <table className="w-full text-xs font-mono border-collapse">
              <thead>
                <tr className="text-[var(--text-muted)]">
                  <th className="text-left px-2 py-1 border-b border-[var(--primary)]">State</th>
                  <th className="text-left px-2 py-1 border-b border-[var(--primary)]">No Wall</th>
                  <th className="text-left px-2 py-1 border-b border-[var(--primary)]">Next</th>
                  <th className="text-left px-2 py-1 border-b border-[var(--primary)]">Wall</th>
                  <th className="text-left px-2 py-1 border-b border-[var(--primary)]">Next</th>
                </tr>
              </thead>
              <tbody>
                {robot.transitions.map((t, s) => (
                  <tr
                    key={s}
                    className={
                      currentState === s
                        ? "bg-[var(--accent)]/20"
                        : "hover:bg-[var(--bg)]"
                    }
                  >
                    <td className="px-2 py-1 border-b border-[var(--primary)]/30 font-semibold">
                      s{s}
                    </td>
                    <td className="px-2 py-1 border-b border-[var(--primary)]/30">
                      {t.a0}
                    </td>
                    <td className="px-2 py-1 border-b border-[var(--primary)]/30 text-[var(--text-muted)]">
                      &rarr; s{t.b0}
                    </td>
                    <td className="px-2 py-1 border-b border-[var(--primary)]/30">
                      {t.a1}
                    </td>
                    <td className="px-2 py-1 border-b border-[var(--primary)]/30 text-[var(--text-muted)]">
                      &rarr; s{t.b1}
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </div>
  );
}
