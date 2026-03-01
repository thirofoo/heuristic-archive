import { useState, useCallback, useEffect, useRef } from "react";
import { useWasm } from "./hooks/useWasm";
import { usePlayback } from "./hooks/usePlayback";
import { Controls } from "./components/Controls";
import { InputPanel } from "./components/InputPanel";
import { ScorePanel } from "./components/ScorePanel";
import { Visualizer } from "./components/Visualizer";
import type { SimulationResult, ProblemInput } from "./types";

export default function App() {
  const { wasm, ready } = useWasm();
  const [input, setInput] = useState<ProblemInput | null>(null);
  const [outputText, setOutputText] = useState<string>("");
  const [result, setResult] = useState<SimulationResult | null>(null);

  const maxStep = result ? result.states.length - 1 : 0;
  const playback = usePlayback(maxStep);

  // Refs for latest values (avoid stale closures)
  const outputTextRef = useRef(outputText);
  outputTextRef.current = outputText;
  const inputRef = useRef(input);
  inputRef.current = input;
  const wasmRef = useRef(wasm);
  wasmRef.current = wasm;
  const playbackRef = useRef(playback);
  playbackRef.current = playback;

  // Core simulate function using refs
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

  const handleGenerate = useCallback(
    (seed: number, problemType: string) => {
      if (!wasmRef.current) return;
      try {
        const generated = wasmRef.current.generate(
          BigInt(seed),
          problemType
        ) as unknown as ProblemInput;
        setInput(generated);
        if (outputTextRef.current) {
          doSimulate(generated.raw, outputTextRef.current);
        } else {
          setResult(null);
        }
      } catch (e) {
        console.error("Generate error:", e);
      }
    },
    [doSimulate]
  );

  const handleInputText = useCallback(
    (type: "input" | "output", text: string) => {
      if (type === "input" && wasmRef.current) {
        try {
          const parsed = wasmRef.current.parse_input(
            text
          ) as unknown as ProblemInput;
          setInput(parsed);
          if (outputTextRef.current) {
            doSimulate(parsed.raw, outputTextRef.current);
          } else {
            setResult(null);
          }
        } catch (e) {
          console.error("Parse error:", e);
        }
      } else if (type === "output") {
        setOutputText(text);
      }
    },
    [doSimulate]
  );

  // Auto-simulate when outputText changes
  useEffect(() => {
    if (inputRef.current && outputText) {
      doSimulate(inputRef.current.raw, outputText);
    } else if (!outputText) {
      setResult(null);
    }
  }, [outputText, doSimulate]);

  if (!ready) {
    return (
      <div className="flex items-center justify-center h-screen text-xl">
        Loading WASM...
      </div>
    );
  }

  return (
    <div className="min-h-screen flex flex-col">
      <header className="bg-[var(--surface)] p-3 text-center text-lg font-bold border-b border-[var(--primary)]">
        Periodic Patrol Automata - Visualizer
      </header>

      <InputPanel
        onInput={handleInputText}
        onGenerate={handleGenerate}
        outputText={outputText}
      />

      <div className="flex flex-1 overflow-hidden">
        <div className="flex-1 flex items-center justify-center p-4">
          {input ? (
            <Visualizer input={input} result={result} step={playback.step} />
          ) : (
            <div className="text-[var(--text-muted)]">
              Generate or load an input file to begin
            </div>
          )}
        </div>

        <ScorePanel result={result} step={playback.step} />
      </div>

      {result && result.states.length > 0 && (
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
