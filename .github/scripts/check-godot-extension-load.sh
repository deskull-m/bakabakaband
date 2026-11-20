#!/bin/sh
# Godot headless で GDExtension を読み込み、拡張のクラスが登録されたかを確認する。
# 使い方: sh check-godot-extension-load.sh <godot 実行ファイル> <godot_project ディレクトリ>

GODOT=$1
PROJECT=$2

if [ -z "$GODOT" ] || [ -z "$PROJECT" ]; then
    echo "usage: $0 <godot> <project-dir>"
    exit 2
fi

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
LOG=$(mktemp)

"$GODOT" --headless --path "$PROJECT" --script "$SCRIPT_DIR/check-godot-extension-load.gd" >"$LOG" 2>&1
STATUS=$?
cat "$LOG"

# GDExtension の読込失敗は Godot 本体がエラーを出すだけで終了コードに現れないことがあるため、ログも見る
if grep -E "Can't open dynamic library|GDExtension dynamic library not found|Failed to load GDExtension|Error loading extension" "$LOG" >/dev/null; then
    echo "GDExtension failed to load."
    STATUS=1
fi

rm -f "$LOG"
exit $STATUS
