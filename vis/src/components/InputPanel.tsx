import { useEffect, useRef } from "react";
import type { ProblemInput } from "../types";

interface InputPanelProps {
  onInput: (type: "input" | "output", text: string) => void;
  onGenerate: (seed: number, problemType: string) => void;
  outputText: string;
  input: ProblemInput | null;
}

export function InputPanel({
  onInput,
  onGenerate,
  outputText,
  input,
}: InputPanelProps) {
  const seedRef = useRef(0);
  const problemTypeRef = useRef("A");
  const onGenerateRef = useRef(onGenerate);
  onGenerateRef.current = onGenerate;

  // Auto-generate on mount
  useEffect(() => {
    onGenerateRef.current(seedRef.current, problemTypeRef.current);
  }, []);

  const handleSeedChange = (val: number) => {
    seedRef.current = val;
    onGenerate(val, problemTypeRef.current);
  };

  const handleProblemTypeChange = (val: string) => {
    problemTypeRef.current = val;
    onGenerate(seedRef.current, val);
  };

  const handleFile = (
    e: React.ChangeEvent<HTMLInputElement>,
    type: "input" | "output"
  ) => {
    const file = e.target.files?.[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = () => onInput(type, reader.result as string);
    reader.readAsText(file);
  };

  return (
    <div className="bg-[var(--surface)] border-b border-[var(--primary)] px-4 py-2 flex flex-col gap-2">
      {/* Top row: controls */}
      <div className="flex items-center gap-4">
        {/* Title */}
        <h1 className="text-sm font-bold tracking-tight whitespace-nowrap">
          Periodic Patrol Automata
        </h1>

        <div className="w-px h-5 bg-[var(--primary)]" />

        {/* Seed & problem type */}
        <div className="flex items-center gap-2">
          <label className="text-[11px] text-[var(--text-muted)]">Seed</label>
          <input
            type="number"
            defaultValue={0}
            onChange={(e) => handleSeedChange(Number(e.target.value))}
            className="w-20 px-2 py-1 bg-[var(--bg)] border border-[var(--primary)] rounded text-xs font-mono"
          />
          <select
            defaultValue="A"
            onChange={(e) => handleProblemTypeChange(e.target.value)}
            className="px-2 py-1 bg-[var(--bg)] border border-[var(--primary)] rounded text-xs"
          >
            <option value="A">Problem A</option>
            <option value="B">Problem B</option>
            <option value="C">Problem C</option>
          </select>
        </div>

        <div className="w-px h-5 bg-[var(--primary)]" />

        {/* File upload */}
        <div className="flex items-center gap-2">
          <label className="text-[11px] px-2 py-1 bg-[var(--bg)] border border-[var(--primary)] rounded cursor-pointer hover:bg-[var(--primary)] transition">
            Input File
            <input
              type="file"
              onChange={(e) => handleFile(e, "input")}
              className="hidden"
              accept=".txt"
            />
          </label>
          <label className="text-[11px] px-2 py-1 bg-[var(--bg)] border border-[var(--primary)] rounded cursor-pointer hover:bg-[var(--primary)] transition">
            Output File
            <input
              type="file"
              onChange={(e) => handleFile(e, "output")}
              className="hidden"
              accept=".txt"
            />
          </label>
        </div>

        {/* Input params preview */}
        {input && (
          <>
            <div className="w-px h-5 bg-[var(--primary)]" />
            <div className="flex items-center gap-3 text-[11px] font-mono">
              <span>
                <span className="text-[var(--text-muted)]">N=</span>
                <span className="font-semibold">{input.n}</span>
              </span>
              <span>
                <span className="text-[var(--text-muted)]">AK=</span>
                <span className="font-semibold">{input.ak}</span>
              </span>
              <span>
                <span className="text-[var(--text-muted)]">AM=</span>
                <span className="font-semibold">{input.am}</span>
              </span>
              <span>
                <span className="text-[var(--text-muted)]">AW=</span>
                <span className="font-semibold">{input.aw}</span>
              </span>
            </div>
          </>
        )}

        {/* Shortcut hints */}
        <div className="ml-auto text-[10px] text-[var(--text-muted)] hidden lg:flex items-center gap-2">
          <kbd className="kbd">Space</kbd> Play
          <kbd className="kbd">&larr;&rarr;</kbd> Step
          <kbd className="kbd">Home/End</kbd> Jump
        </div>
      </div>

      {/* Output textarea - always visible */}
      <textarea
        placeholder="Paste output text here..."
        rows={3}
        value={outputText}
        onChange={(e) => onInput("output", e.target.value)}
        className="w-full px-2 py-1.5 bg-[var(--bg)] border border-[var(--primary)] rounded text-xs font-mono resize-y focus:border-[var(--accent)] focus:outline-none transition"
      />
    </div>
  );
}
