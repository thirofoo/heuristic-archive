import { useState } from "react";

interface InputPanelProps {
  onInput: (type: "input" | "output", text: string) => void;
  onGenerate: (seed: number, problemType: string) => void;
  onSimulate: () => void;
  hasInput: boolean;
  hasOutput: boolean;
}

export function InputPanel({
  onInput,
  onGenerate,
  onSimulate,
  hasInput,
  hasOutput,
}: InputPanelProps) {
  const [seed, setSeed] = useState(0);
  // TODO: 問題のバリアント種別を問題に合わせて変更
  const [problemType, setProblemType] = useState("A");

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
    <div className="bg-[var(--surface)] border-b border-[var(--primary)] p-3 flex flex-wrap items-center gap-4">
      {/* Seed 生成 */}
      <div className="flex items-center gap-2">
        <label className="text-sm text-[var(--text-muted)]">Seed</label>
        <input
          type="number"
          value={seed}
          onChange={(e) => setSeed(Number(e.target.value))}
          className="w-20 px-2 py-1 bg-[var(--bg)] border border-[var(--primary)] rounded text-sm"
        />
        {/* TODO: 問題に合わせてオプションを変更 */}
        <select
          value={problemType}
          onChange={(e) => setProblemType(e.target.value)}
          className="px-2 py-1 bg-[var(--bg)] border border-[var(--primary)] rounded text-sm"
        >
          <option value="A">Problem A</option>
          <option value="B">Problem B</option>
          <option value="C">Problem C</option>
        </select>
        <button
          onClick={() => onGenerate(seed, problemType)}
          className="px-3 py-1 bg-[var(--primary)] rounded hover:brightness-125 text-sm transition"
        >
          Generate
        </button>
      </div>

      {/* ファイル読み込み */}
      <div className="flex items-center gap-2">
        <label className="text-sm text-[var(--text-muted)] cursor-pointer hover:text-[var(--text)]">
          Input
          <input
            type="file"
            onChange={(e) => handleFile(e, "input")}
            className="hidden"
            accept=".txt"
          />
        </label>
        <label className="text-sm text-[var(--text-muted)] cursor-pointer hover:text-[var(--text)]">
          Output
          <input
            type="file"
            onChange={(e) => handleFile(e, "output")}
            className="hidden"
            accept=".txt"
          />
        </label>
      </div>

      {/* シミュレート */}
      <button
        onClick={onSimulate}
        disabled={!hasInput || !hasOutput}
        className="px-4 py-1 bg-[var(--accent)] rounded hover:brightness-125 disabled:opacity-40 text-sm font-semibold transition"
      >
        Simulate
      </button>
    </div>
  );
}
