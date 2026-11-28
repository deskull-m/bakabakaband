#!/bin/sh
# 実ゲームのチェックスクリプト (tools/godot/*-runtime-check.gd) を headless で走らせ、
# 出力の "RESULT: OK" を見て終了コードを返す。
#
# チェックスクリプトはゲームのスレッドを止めずに抜けるため最後に自分のプロセスを強制終了する
# (シェルからは成否に関わらず異常終了に見える) ので、成否はこのラッパで終了コードに直す。
#
# 使い方: HB_SAVE=<使い捨てのセーブ> sh tools/godot/run-runtime-check.sh <チェックの .gd> [制限秒数]
#   環境変数 GODOT で Godot の実行ファイルを指定できる (既定: godot)
set -u

if [ $# -lt 1 ]; then
    echo "usage: HB_SAVE=<save> $0 <check-script.gd> [timeout-seconds]" >&2
    exit 2
fi

script=$1
limit=${2:-300}
godot=${GODOT:-godot}
root=$(cd "$(dirname "$0")/../.." && pwd)
case "$script" in
/*) ;;
*) script="$PWD/$script" ;;
esac

log=$(mktemp)
trap 'rm -f "$log"' EXIT
timeout "$limit" "$godot" --headless --path "$root/godot_project" --script "$script" >"$log" 2>&1
cat "$log"

if grep -q "^RESULT: OK" "$log"; then
    exit 0
fi
echo "runtime check failed: $(grep '^RESULT:' "$log" | tail -1)" >&2
exit 1
