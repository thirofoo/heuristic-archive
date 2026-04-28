import { useState, useRef, useCallback, useEffect } from "react";

export function usePlayback(maxStep: number) {
  const [step, setStep] = useState(0);
  const [playing, setPlaying] = useState(false);
  const [speed, setSpeed] = useState(5);
  const timerRef = useRef<number | null>(null);

  const clearTimer = useCallback(() => {
    if (timerRef.current !== null) {
      clearTimeout(timerRef.current);
      timerRef.current = null;
    }
  }, []);

  useEffect(() => {
    if (!playing) return;
    const tick = () => {
      setStep((s) => {
        if (s >= maxStep) {
          setPlaying(false);
          return s;
        }
        return s + 1;
      });
      timerRef.current = window.setTimeout(tick, 1000 / speed);
    };
    tick();
    return clearTimer;
  }, [playing, maxStep, speed, clearTimer]);

  const play = useCallback(() => setPlaying(true), []);
  const pause = useCallback(() => {
    setPlaying(false);
    clearTimer();
  }, [clearTimer]);

  const stepForward = useCallback(() => {
    setStep((s) => Math.min(s + 1, maxStep));
  }, [maxStep]);

  const stepBackward = useCallback(() => {
    setStep((s) => Math.max(s - 1, 0));
  }, []);

  const jumpTo = useCallback(
    (n: number) => {
      setStep(Math.max(0, Math.min(n, maxStep)));
    },
    [maxStep]
  );

  return {
    step,
    playing,
    speed,
    setSpeed,
    play,
    pause,
    stepForward,
    stepBackward,
    jumpTo,
  };
}
