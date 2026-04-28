import { useState, useCallback, useEffect, useRef, useMemo } from "react";
import { useWasm } from "./hooks/useWasm";
import { usePlayback } from "./hooks/usePlayback";
import { Controls } from "./components/Controls";
import { InputPanel } from "./components/InputPanel";
import { ScorePanel } from "./components/ScorePanel";
import { Visualizer } from "./components/Visualizer";
import { AutomatonPreview } from "./components/AutomatonPreview";
import { parseAutomata } from "./parseAutomata";
import type { SimulationResult, ProblemInput } from "./types";

const ROBOT_COLORS = [
  "#ff6b6b", "#4ecdc4", "#45b7d1", "#f9ca24", "#6ab04c",
  "#eb4d4b", "#7ed6df", "#e056fd", "#f0932b", "#22a6b3",
  "#be2edd", "#ffbe76", "#badc58", "#ff7979", "#686de0",
  "#30336b", "#95afc0", "#f8c291", "#6a89cc", "#82ccdd",
];

export default function App() {
  const { wasm, ready } = useWasm();
  const [input, setInput] = useState<ProblemInput | null>(null);
  const [outputText, setOutputText] = useState<string>("");
  const [result, setResult] = useState<SimulationResult | null>(null);
  const [selectedRobot, setSelectedRobot] = useState<number | null>(null);

  const maxStep = result ? result.states.length - 1 : 0;
  const playback = usePlayback(maxStep);

  // Parse automata from output text
  const automata = useMemo(() => {
    if (!outputText) return null;
    return parseAutomata(outputText);
  }, [outputText]);

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

  // Keyboard shortcuts
  useEffect(() => {
    const handler = (e: KeyboardEvent) => {
      const tag = (e.target as HTMLElement)?.tagName;
      if (tag === "INPUT" || tag === "TEXTAREA" || tag === "SELECT") return;

      const pb = playbackRef.current;
      switch (e.key) {
        case " ":
          e.preventDefault();
          pb.playing ? pb.pause() : pb.play();
          break;
        case "ArrowRight":
          e.preventDefault();
          pb.stepForward();
          break;
        case "ArrowLeft":
          e.preventDefault();
          pb.stepBackward();
          break;
        case "Home":
          e.preventDefault();
          pb.jumpToStart();
          break;
        case "End":
          e.preventDefault();
          pb.jumpToEnd();
          break;
      }
    };
    window.addEventListener("keydown", handler);
    return () => window.removeEventListener("keydown", handler);
  }, []);

  if (!ready) {
    return (
      <div className="flex items-center justify-center h-screen text-xl">
        Loading WASM...
      </div>
    );
  }

  // Get current internal state for selected robot
  const currentRobotState =
    selectedRobot !== null && result?.states[playback.step]
      ? result.states[playback.step].robots[selectedRobot]?.s ?? null
      : null;

  return (
    <div className="h-screen flex flex-col overflow-hidden">
      <InputPanel
        onInput={handleInputText}
        onGenerate={handleGenerate}
        outputText={outputText}
        input={input}
      />

      <div className="flex flex-1 min-h-0">
        <div className="flex-1 flex items-center justify-center p-4">
          {input ? (
            <Visualizer input={input} result={result} step={playback.step} />
          ) : (
            <div className="text-[var(--text-muted)] text-sm">
              Generate or load an input file to begin
            </div>
          )}
        </div>

        {result && (
          <ScorePanel
            result={result}
            step={playback.step}
            onRobotClick={(k) => setSelectedRobot(k)}
          />
        )}
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
          onJumpToStart={playback.jumpToStart}
          onJumpToEnd={playback.jumpToEnd}
        />
      )}

      {/* Automaton preview modal */}
      {selectedRobot !== null && automata && automata[selectedRobot] && (
        <AutomatonPreview
          robot={automata[selectedRobot]}
          robotIndex={selectedRobot}
          color={ROBOT_COLORS[selectedRobot % ROBOT_COLORS.length]}
          currentState={currentRobotState}
          onClose={() => setSelectedRobot(null)}
        />
      )}
    </div>
  );
}
