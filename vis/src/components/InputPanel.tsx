import { useState, useEffect, useRef } from "react";

interface InputPanelProps {
  onInput: (type: "input" | "output", text: string) => void;
  onGenerate: (seed: number, problemType: string) => void;
  outputText: string;
}

export function InputPanel({
  onInput,
  onGenerate,
  outputText,
}: InputPanelProps) {
  const [seed, setSeed] = useState(0);
  const [problemType, setProblemType] = useState("A");
  const onGenerateRef = useRef(onGenerate);
  onGenerateRef.current = onGenerate;

  // Auto-generate on mount and whenever seed/problemType changes
  useEffect(() => {
    onGenerateRef.current(seed, problemType);
  }, [seed, problemType]);

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
    <div className="bg-[var(--surface)] border-b border-[var(--primary)] p-3 flex flex-col gap-2">
      <div className="flex flex-wrap items-center gap-4">
        {/* Seed & problem type */}
        <div className="flex items-center gap-2">
          <label className="text-sm text-[var(--text-muted)]">Seed</label>
          <input
            type="number"
            value={seed}
            onChange={(e) => setSeed(Number(e.target.value))}
            className="w-20 px-2 py-1 bg-[var(--bg)] border border-[var(--primary)] rounded text-sm"
          />
          <select
            value={problemType}
            onChange={(e) => setProblemType(e.target.value)}
            className="px-2 py-1 bg-[var(--bg)] border border-[var(--primary)] rounded text-sm"
          >
            <option value="A">Problem A</option>
            <option value="B">Problem B</option>
            <option value="C">Problem C</option>
          </select>
        </div>

        {/* File upload */}
        <div className="flex items-center gap-2">
          <label className="text-sm px-2 py-1 bg-[var(--bg)] border border-[var(--primary)] rounded cursor-pointer hover:brightness-125 transition">
            Input File
            <input
              type="file"
              onChange={(e) => handleFile(e, "input")}
              className="hidden"
              accept=".txt"
            />
          </label>
          <label className="text-sm px-2 py-1 bg-[var(--bg)] border border-[var(--primary)] rounded cursor-pointer hover:brightness-125 transition">
            Output File
            <input
              type="file"
              onChange={(e) => handleFile(e, "output")}
              className="hidden"
              accept=".txt"
            />
          </label>
        </div>
      </div>

      {/* Output text area - always visible */}
      <textarea
        placeholder="Output text (paste or load file)"
        rows={4}
        value={outputText}
        onChange={(e) => onInput("output", e.target.value)}
        className="w-full px-2 py-1 bg-[var(--bg)] border border-[var(--primary)] rounded text-xs font-mono resize-y"
      />
    </div>
  );
}
