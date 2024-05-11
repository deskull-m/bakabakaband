#!/bin/sh

sudo apt-get update >/dev/null
sudo apt-get install clang-format-15 >/dev/null

SRC_FILES=$(find src/ -type f ! -path "src/external-lib/*" \( -name \*.cpp -or -name \*.h \))

clang-format-15 -style=file:.github/scripts/check-clang-format-style -i $SRC_FILES

DIFF_FILE=$(mktemp)
git diff >$DIFF_FILE

if [ -s $DIFF_FILE ]; then
    echo "Some source code files are not properly formatted."
    cat $DIFF_FILE
    exit 1
fi

exit 0
