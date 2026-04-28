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
}: ControlsProps) {
  return (
    <div className="bg-[var(--surface)] border-t border-[var(--primary)] p-3 flex items-center gap-3">
      <div className="flex gap-1">
        <button
          onClick={onStepBackward}
          disabled={step === 0}
          className="px-3 py-1.5 bg-[var(--primary)] rounded hover:brightness-125 disabled:opacity-40 transition"
        >
          ⏮
        </button>
        {playing ? (
          <button
            onClick={onPause}
            className="px-3 py-1.5 bg-[var(--accent)] rounded hover:brightness-125 transition"
          >
            ⏸
          </button>
        ) : (
          <button
            onClick={onPlay}
            disabled={step >= maxStep}
            className="px-3 py-1.5 bg-[var(--accent)] rounded hover:brightness-125 disabled:opacity-40 transition"
          >
            ▶
          </button>
        )}
        <button
          onClick={onStepForward}
          disabled={step >= maxStep}
          className="px-3 py-1.5 bg-[var(--primary)] rounded hover:brightness-125 disabled:opacity-40 transition"
        >
          ⏭
        </button>
      </div>

      <input
        type="range"
        min={0}
        max={maxStep}
        value={step}
        onChange={(e) => onJumpTo(Number(e.target.value))}
        className="flex-1 accent-[var(--accent)]"
      />

      <span className="text-sm text-[var(--text-muted)] min-w-[80px] text-center">
        {step} / {maxStep}
      </span>

      <div className="flex items-center gap-2 text-sm">
        <span className="text-[var(--text-muted)]">Speed</span>
        <input
          type="range"
          min={1}
          max={60}
          value={speed}
          onChange={(e) => onSpeedChange(Number(e.target.value))}
          className="w-20 accent-[var(--accent)]"
        />
        <span className="text-[var(--text-muted)] min-w-[30px]">{speed}x</span>
      </div>
    </div>
  );
}
