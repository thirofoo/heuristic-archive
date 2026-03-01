import { useState, useCallback } from "react";
import { useWasm } from "./hooks/useWasm";
import { usePlayback } from "./hooks/usePlayback";
import { Controls } from "./components/Controls";
import { InputPanel } from "./components/InputPanel";
import { ScorePanel } from "./components/ScorePanel";
import type { SimulationResult, ProblemInput } from "./types";

// TODO: 問題に合わせて Visualizer コンポーネントを作成し import する
// import { Visualizer } from "./components/Visualizer";

export default function App() {
  const { wasm, ready } = useWasm();
  const [input, setInput] = useState<ProblemInput | null>(null);
  const [outputText, setOutputText] = useState<string>("");
  const [result, setResult] = useState<SimulationResult | null>(null);

  const maxStep = result ? result.states.length - 1 : 0;
  const playback = usePlayback(maxStep);

  const handleGenerate = useCallback(
    (seed: number, problemType: string) => {
      if (!wasm) return;
      try {
        const generated = wasm.generate(BigInt(seed), problemType);
        setInput(generated as unknown as ProblemInput);
        setResult(null);
        playback.jumpTo(0);
      } catch (e) {
        console.error("Generate error:", e);
      }
    },
    [wasm, playback]
  );

  const handleInputText = useCallback(
    (type: "input" | "output", text: string) => {
      if (type === "input" && wasm) {
        try {
          const parsed = wasm.parse_input(text);
          setInput(parsed as unknown as ProblemInput);
          setResult(null);
          playback.jumpTo(0);
        } catch (e) {
          console.error("Parse error:", e);
        }
      } else if (type === "output") {
        setOutputText(text);
      }
    },
    [wasm, playback]
  );

  const handleSimulate = useCallback(() => {
    if (!wasm || !input) return;
    try {
      const res = wasm.simulate(input.raw, outputText);
      setResult(res as unknown as SimulationResult);
      playback.jumpTo(0);
    } catch (e) {
      console.error("Simulate error:", e);
    }
  }, [wasm, input, outputText, playback]);

  if (!ready) {
    return (
      <div className="flex items-center justify-center h-screen text-xl">
        Loading WASM...
      </div>
    );
  }

  const currentState = result?.states[playback.step] ?? null;

  return (
    <div className="min-h-screen flex flex-col">
      <header className="bg-[var(--surface)] p-3 text-center text-lg font-bold border-b border-[var(--primary)]">
        Heuristic Visualizer
      </header>

      <InputPanel
        onInput={handleInputText}
        onGenerate={handleGenerate}
        onSimulate={handleSimulate}
        hasInput={!!input}
        hasOutput={!!outputText}
      />

      <div className="flex flex-1 overflow-hidden">
        <div className="flex-1 flex items-center justify-center p-4">
          {/* TODO: 問題固有の Visualizer を配置 */}
          {currentState ? (
            <div className="text-[var(--text-muted)]">
              Step {playback.step}: Visualizer をここに実装
              <pre className="text-xs mt-2 max-h-80 overflow-auto">
                {JSON.stringify(currentState, null, 2)}
              </pre>
            </div>
          ) : (
            <div className="text-[var(--text-muted)]">
              入力を生成またはファイルを読み込んでください
            </div>
          )}
        </div>

        <ScorePanel result={result} step={playback.step} />
      </div>

      {result && (
        <Controls
          step={playback.step}
          maxStep={maxStep}
          playing={playback.playing}
          speed={playback.speed}
          onPlay={playback.play}
          onPause={playback.pause}
          onStepForward={playback.stepForward}
          onStepBackward={playback.stepBackward}
          onJumpTo={playback.jumpTo}
          onSpeedChange={playback.setSpeed}
        />
      )}
    </div>
  );
}
