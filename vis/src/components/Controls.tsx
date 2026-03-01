import { useState } from "react";

interface ControlsProps {
  step: number;
  maxStep: number;
  playing: boolean;
  speed: number;
  onPlay: () => void;
  onPause: () => void;
  onStepForward: () => void;
  onStepBackward: () => void;
  onJumpTo: (n: number) => void;
  onSpeedChange: (s: number) => void;
  onJumpToStart: () => void;
  onJumpToEnd: () => void;
}

export function Controls({
  step,
  maxStep,
  playing,
  speed,
  onPlay,
  onPause,
  onStepForward,
  onStepBackward,
  onJumpTo,
  onSpeedChange,
  onJumpToStart,
  onJumpToEnd,
}: ControlsProps) {
  const [editingStep, setEditingStep] = useState(false);
  const [stepInput, setStepInput] = useState("");

  const handleStepSubmit = () => {
    const n = parseInt(stepInput, 10);
    if (!isNaN(n)) onJumpTo(n);
    setEditingStep(false);
  };

  return (
    <div className="bg-[var(--surface)] border-t border-[var(--primary)] px-4 py-2 flex items-center gap-3">
      {/* Playback buttons */}
      <div className="flex gap-0.5">
        <button
          onClick={onJumpToStart}
          disabled={step === 0}
          className="ctrl-btn rounded-l-md"
          title="Jump to start (Home)"
        >
          <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor"><path d="M6 6h2v12H6zm3.5 6l8.5 6V6z"/></svg>
        </button>
        <button
          onClick={onStepBackward}
          disabled={step === 0}
          className="ctrl-btn"
          title="Step backward (Left Arrow)"
        >
          <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor"><path d="M11 18V6l-8.5 6 8.5 6z"/></svg>
        </button>
        {playing ? (
          <button
            onClick={onPause}
            className="ctrl-btn-accent"
            title="Pause (Space)"
          >
            <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M6 19h4V5H6v14zm8-14v14h4V5h-4z"/></svg>
          </button>
        ) : (
          <button
            onClick={onPlay}
            disabled={step >= maxStep}
            className="ctrl-btn-accent"
            title="Play (Space)"
          >
            <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M8 5v14l11-7z"/></svg>
          </button>
        )}
        <button
          onClick={onStepForward}
          disabled={step >= maxStep}
          className="ctrl-btn"
          title="Step forward (Right Arrow)"
        >
          <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor"><path d="M13 6v12l8.5-6L13 6z"/></svg>
        </button>
        <button
          onClick={onJumpToEnd}
          disabled={step >= maxStep}
          className="ctrl-btn rounded-r-md"
          title="Jump to end (End)"
        >
          <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor"><path d="M6 18l8.5-6L6 6v12zM16 6v12h2V6h-2z"/></svg>
        </button>
      </div>

      {/* Seek slider */}
      <input
        type="range"
        min={0}
        max={maxStep}
        value={step}
        onChange={(e) => onJumpTo(Number(e.target.value))}
        className="flex-1 accent-[var(--accent)] h-1.5"
      />

      {/* Step counter (click to edit) */}
      {editingStep ? (
        <form
          onSubmit={(e) => { e.preventDefault(); handleStepSubmit(); }}
          className="flex items-center"
        >
          <input
            type="number"
            autoFocus
            value={stepInput}
            onChange={(e) => setStepInput(e.target.value)}
            onBlur={handleStepSubmit}
            min={0}
            max={maxStep}
            className="w-16 px-1 py-0.5 bg-[var(--bg)] border border-[var(--accent)] rounded text-xs text-center font-mono"
          />
          <span className="text-xs text-[var(--text-muted)] ml-1">/ {maxStep}</span>
        </form>
      ) : (
        <button
          onClick={() => { setStepInput(String(step)); setEditingStep(true); }}
          className="text-xs text-[var(--text-muted)] min-w-[90px] text-center font-mono hover:text-[var(--text)] transition cursor-text"
          title="Click to enter step number"
        >
          {step} / {maxStep}
        </button>
      )}

      {/* Speed control */}
      <div className="flex items-center gap-1.5 text-xs border-l border-[var(--primary)] pl-3">
        <svg width="12" height="12" viewBox="0 0 24 24" fill="var(--text-muted)"><path d="M13 2.05v2.02c3.95.49 7 3.85 7 7.93 0 3.73-2.56 6.86-6.02 7.75l.72 1.93A9.96 9.96 0 0 0 23 12c0-5.18-3.95-9.45-9-9.95zM12 19c-3.87 0-7-3.13-7-7s3.13-7 7-7v14zM1 12C1 5.93 5.84 1.02 11.87 1.02V3.04C6.95 3.04 3 7.04 3 12s3.95 8.96 8.87 8.96v2.02C5.84 22.98 1 18.07 1 12z"/></svg>
        <input
          type="range"
          min={1}
          max={60}
          value={speed}
          onChange={(e) => onSpeedChange(Number(e.target.value))}
          className="w-16 accent-[var(--accent)] h-1"
        />
        <span className="text-[var(--text-muted)] min-w-[28px] font-mono">{speed}x</span>
      </div>
    </div>
  );
}
