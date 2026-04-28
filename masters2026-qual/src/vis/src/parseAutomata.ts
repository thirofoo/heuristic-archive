import type { RobotAutomaton } from "./types";

export function parseAutomata(outputText: string): RobotAutomaton[] | null {
  try {
    const tokens = outputText.trim().split(/\s+/);
    let idx = 0;
    const next = () => tokens[idx++];

    const K = parseInt(next(), 10);
    if (isNaN(K) || K < 1) return null;

    const robots: RobotAutomaton[] = [];
    for (let k = 0; k < K; k++) {
      const m = parseInt(next(), 10);
      const i0 = parseInt(next(), 10);
      const j0 = parseInt(next(), 10);
      const d0 = next();
      if (isNaN(m) || isNaN(i0) || isNaN(j0) || !d0) return null;

      const transitions: RobotAutomaton["transitions"] = [];
      for (let s = 0; s < m; s++) {
        const a0 = next();
        const b0 = parseInt(next(), 10);
        const a1 = next();
        const b1 = parseInt(next(), 10);
        if (!a0 || isNaN(b0) || !a1 || isNaN(b1)) return null;
        transitions.push({ a0, b0, a1, b1 });
      }
      robots.push({ m, i0, j0, d0, transitions });
    }
    return robots;
  } catch {
    return null;
  }
}
