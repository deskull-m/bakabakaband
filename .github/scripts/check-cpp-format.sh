#!/bin/sh

# 上流 (hengband) CI と format 出力を一致させるため、必ず clang-format-15 を使用する。
# clang-format-18 等の他バージョンは「&noexcept」のスペーシング等で出力差が出るため
# fallback には用いない (CI とローカルで diff が出るのを防ぐ)。
#
# clang-format-15 が未導入の環境では明示的に install する。CI (Ubuntu 24.04) では
# 標準では未インストールだが、universe リポジトリ経由で入手可能。
if ! command -v clang-format-15 >/dev/null 2>&1; then
    if command -v sudo >/dev/null 2>&1; then
        sudo apt-get update >/dev/null
        sudo apt-get install -y clang-format-15 >/dev/null
    else
        apt-get update >/dev/null
        apt-get install -y clang-format-15 >/dev/null
    fi
fi

if ! command -v clang-format-15 >/dev/null 2>&1; then
    echo "clang-format-15 is required but could not be installed."
    echo "Please install clang-format-15 manually (e.g. via 'apt-get install clang-format-15')."
    echo "Note: clang-format-18 や generic clang-format は出力差があるため使用不可。"
    exit 1
fi

SRC_FILES=$(find src/ -type f -regextype posix-egrep -regex ".*\.(cpp|h)" -not -path "src/external-lib/*")

clang-format-15 -style=file:.github/scripts/check-clang-format-style -i $SRC_FILES
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
