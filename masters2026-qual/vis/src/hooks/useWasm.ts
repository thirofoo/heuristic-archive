import { useState, useEffect } from "react";

// TODO: wasm-pack ビルド後に正しいパスに更新する
// import init, * as wasmModule from "../wasm-pkg/vis_wasm";

export function useWasm() {
  const [ready, setReady] = useState(false);
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  const [wasm, setWasm] = useState<any>(null);

  useEffect(() => {
    (async () => {
      try {
        // TODO: wasm-pkg のパスを実際のビルド出力に合わせる
        const mod = await import("../wasm-pkg/vis_wasm");
        await mod.default(); // init()
        setWasm(mod);
        setReady(true);
      } catch (e) {
        console.error("WASM init failed:", e);
      }
    })();
  }, []);

  return { wasm, ready };
}
