#!/bin/bash
# Heuristic Contest Visualizer - Project Setup Script
# Usage: bash setup.sh [output_dir]
#   output_dir: ビジュアライザの出力先 (default: ./vis)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SKILL_DIR="$(dirname "$SCRIPT_DIR")"
TEMPLATE_DIR="$SKILL_DIR/assets/template"
OUTPUT_DIR="${1:-./vis}"

echo "=== Heuristic Visualizer Setup ==="
echo "Output: $OUTPUT_DIR"

if [ -d "$OUTPUT_DIR" ]; then
  echo "Error: $OUTPUT_DIR already exists"
  exit 1
fi

# テンプレートをコピー
cp -r "$TEMPLATE_DIR" "$OUTPUT_DIR"

# npm install
cd "$OUTPUT_DIR"
npm install

echo ""
echo "=== Setup Complete ==="
echo ""
echo "Next steps:"
echo "  1. wasm/src/lib.rs に tools/ の Rust コードを移植"
echo "  2. wasm/Cargo.toml に必要な依存を追加"
echo "  3. 問題に合わせて src/components/Visualizer.tsx を作成"
echo "  4. src/types.ts を問題に合わせて更新"
echo "  5. npm run wasm:build"
echo "  6. npm run dev"
