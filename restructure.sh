#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SOURCE_DIR="$SCRIPT_DIR/../Heuristic"
TARGET_DIR="$SCRIPT_DIR"

cd "$TARGET_DIR"

# ==========================================
#  Step 1: git subtree で未取り込みのリポジトリを追加
# ==========================================
echo "=========================================="
echo " Step 1: Importing new directories"
echo "=========================================="

for dir_path in "$SOURCE_DIR"/*/; do
    repo_name=$(basename "$dir_path")

    if [ ! -d "${dir_path}.git" ]; then
        echo "[SKIP] $repo_name — not a Git repository"
        continue
    fi

    if [ -d "$TARGET_DIR/$repo_name" ]; then
        echo "[SKIP] $repo_name — already in monorepo"
        continue
    fi

    if ! (cd "$dir_path" && git rev-parse HEAD >/dev/null 2>&1); then
        echo "[SKIP] $repo_name — no commits"
        continue
    fi

    branch_name=$(cd "$dir_path" && git branch --show-current)

    if [ -z "$branch_name" ]; then
        echo "[SKIP] $repo_name — no branch"
        continue
    fi

    echo "[ADD]  $repo_name (branch: $branch_name)"
    git subtree add --prefix="$repo_name" "$dir_path" "$branch_name" -m "Add $repo_name via git subtree"
done

# ==========================================
#  Step 2: 各ディレクトリを src/ + meta.json 構成にリストラクチャ
#    - 全ファイルを src/ 以下に移動（README.md 含む）
#    - meta.json がなければ空テンプレートを生成
# ==========================================
echo ""
echo "=========================================="
echo " Step 2: Restructuring directories"
echo "=========================================="

for dir_path in "$TARGET_DIR"/*/; do
    repo_name=$(basename "$dir_path")
    cd "$dir_path"

    # 既にリストラクチャ済みかチェック:
    # src/ があり、かつ src/ と meta.json 以外のものがトップレベルにない場合はスキップ
    if [ -d "src" ]; then
        other_count=$(find . -maxdepth 1 -mindepth 1 -not -name 'src' -not -name 'meta.json' | wc -l)
        if [ "$other_count" -eq 0 ]; then
            echo "[SKIP] $repo_name — already restructured"
            cd "$TARGET_DIR"
            continue
        fi
        # src/ はあるがコードファイルもトップレベルにある → 既存 src/ を退避
        echo "[INFO] $repo_name — renaming existing src/ to _src_bak"
        git mv src _src_bak
    fi

    echo "[RESTRUCTURE] $repo_name"
    mkdir -p src

    # meta.json と src/ 以外を全て src/ に移動
    find . -maxdepth 1 -mindepth 1 -not -name 'src' -not -name 'meta.json' -print0 | \
        xargs -0 -I {} git mv {} src/

    # 退避した元の src/ を復元
    if [ -d "src/_src_bak" ]; then
        git mv src/_src_bak src/src
    fi

    # meta.json がなければテンプレートを生成
    if [ ! -f "meta.json" ]; then
        slug=$(echo "$repo_name" | tr '[:upper:]' '[:lower:]')
        cat > meta.json <<METAEOF
{
  "id": "$repo_name",
  "title": "",
  "date": "",
  "rank": null,
  "score": null,
  "performance": null,
  "tags": [],
  "visualizer": null,
  "problemUrl": "https://atcoder.jp/contests/$slug/tasks/${slug}_a",
  "description": ""
}
METAEOF
        git add meta.json
    fi

    cd "$TARGET_DIR"
    git add -A "$repo_name/"
    git commit -m "Restructure $repo_name: move files to src/"
done

echo ""
echo "=========================================="
echo " Done!"
echo "=========================================="
