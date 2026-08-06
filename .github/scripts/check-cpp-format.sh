#!/bin/sh

# 上流 (hengband) CI と format 出力を一致させるため、clang-format-18 を使用する
# (上流 PR #4215 で 15 から 18 へ移行)。スタイル定義はリポジトリルートの
# .clang-format を参照する。
#
# clang-format-18 が未導入の環境では明示的に install する。
if ! command -v clang-format-18 >/dev/null 2>&1; then
    if command -v sudo >/dev/null 2>&1; then
        sudo apt-get update >/dev/null
        sudo apt-get install -y clang-format-18 >/dev/null
    else
        apt-get update >/dev/null
        apt-get install -y clang-format-18 >/dev/null
    fi
fi

if ! command -v clang-format-18 >/dev/null 2>&1; then
    echo "clang-format-18 is required but could not be installed."
    echo "Please install clang-format-18 manually (e.g. via 'apt-get install clang-format-18')."
    exit 1
fi

SRC_FILES=$(find src/ -type f -regextype posix-egrep -regex ".*\.(cpp|h)" -not -path "src/external-lib/*")

clang-format-18 -i $SRC_FILES
clang_format_result=$?

if [ $clang_format_result -ne 0 ]; then
    echo "Could not execute clang-format properly."
    exit $clang_format_result
fi

DIFF_FILE=$(mktemp)
git diff >$DIFF_FILE

if [ -s $DIFF_FILE ]; then
    echo "Some source code files are not properly formatted."
    cat $DIFF_FILE
    exit 1
fi

exit 0
