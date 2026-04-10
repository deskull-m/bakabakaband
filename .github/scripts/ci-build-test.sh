#!/bin/sh
# autotools ビルドテストスクリプト
# GitHub Actions の build_test_japanese ジョブ（pull-request-status-check.yml）と同等のビルドをローカルで実行する。
#
# 使い方:
#   sh .github/scripts/ci-build-test.sh [オプション]
#
# オプション:
#   --cxx <コンパイラ>        使用するコンパイラ (デフォルト: g++)
#   --cxxflags <フラグ>       コンパイルフラグ (デフォルト: -pipe -O3 -Werror -Wall -Wextra)
#   --configure-opts <オプション>  configure に渡す追加オプション (デフォルト: --disable-pch)
#   --jobs <数>               並列ビルド数 (デフォルト: nproc の値)
#
# 前提:
#   以下のパッケージがインストールされていること:
#     sudo apt-get install libncursesw5-dev libcurl4-openssl-dev nkf libc++-15-dev libc++abi-15-dev

set -eu

CXX="${CXX:-g++}"
CXXFLAGS="${CXXFLAGS:--pipe -O3 -Werror -Wall -Wextra}"
CONFIGURE_OPTS="--disable-pch"
JOBS=$(nproc 2>/dev/null || echo 4)

# 引数パース
while [ $# -gt 0 ]; do
    case "$1" in
        --cxx)
            CXX="$2"; shift 2 ;;
        --cxxflags)
            CXXFLAGS="$2"; shift 2 ;;
        --configure-opts)
            CONFIGURE_OPTS="$2"; shift 2 ;;
        --jobs)
            JOBS="$2"; shift 2 ;;
        *)
            echo "不明なオプション: $1"
            exit 1 ;;
    esac
done

echo "[ci-build-test] コンパイラ : ${CXX}"
echo "[ci-build-test] フラグ     : ${CXXFLAGS}"
echo "[ci-build-test] configure  : ${CONFIGURE_OPTS}"
echo "[ci-build-test] 並列数     : ${JOBS}"

# bootstrap (configure スクリプト生成)
echo "[ci-build-test] bootstrap を実行しています..."
./bootstrap

# configure
echo "[ci-build-test] configure を実行しています..."
CXX="${CXX}" CXXFLAGS="${CXXFLAGS}" ./configure ${CONFIGURE_OPTS}

# ビルド
echo "[ci-build-test] make を実行しています..."
make -j"${JOBS}" >/dev/null

echo "[ci-build-test] OK: ビルドが成功しました。"
exit 0
