#!/usr/bin/env python
"""
Bakabakaband GDExtension ビルドスクリプト
Usage:
  scons                          # デフォルトビルド (platform=windows target=template_debug)
  scons platform=linux           # Linux向けビルド
  scons target=template_release  # リリースビルド
"""

import os
import sys

# godot-cpp の SConstruct を読み込む
env = SConscript("godot-cpp/SConstruct")

# --- Hengband ソースファイル設定 ---

# main-godot バックエンドのソースファイル
godot_backend_sources = Glob("src/main-godot/*.cpp")

# Hengband ゲームコア (main-win を除く全ソース)
def collect_hengband_sources():
    sources = []
    exclude_dirs = {
        "main-win",    # Windows バックエンド (今回は除外)
        "main-unix",   # Unix バックエンド (今回は除外)
        "main-godot",  # Godot バックエンド (godot_backend_sources で別途追加)
        "net",         # libcurl 依存のネットワーク機能 (Godot 版では不要)
        "test",        # 単体テスト (各自 main() を持つため DLL ビルドから除外)
    }
    exclude_files = {
        "main-x11.cpp",
        "main-gcu.cpp",
        "main-cap.cpp",
        "main.cpp",      # Unix エントリポイント (WinMain の代わりに main-godot を使用)
        "main-win.cpp",  # Windows ネイティブバックエンド (GDExtension では不要)
    }
    # autotools の src/Makefile.am に含まれない孤立 (死に) ソース。
    # ヘッダ欠落や未定義シンボル (ROOT_VARIANT_NAME 等) を含み、呼出箇所も無い。
    # glob ビルドから path 指定で除外し、autotools の扱いと揃える。
    # ※ 同名の生存ファイルを巻き込まないよう basename ではなく相対 path で判定する。
    exclude_paths = {
        "src/load/angband-version-comparer.cpp",
        "src/lore/lore-store.cpp",
        "src/market/arena-info-table.cpp",
        "src/monster-race/monster-era-type-name.cpp",
        "src/store/owner-insults.cpp",
        "src/window/display-sub-window-spells.cpp",
    }
    for root, dirs, files in os.walk("src"):
        # 除外ディレクトリをスキップ
        dirs[:] = [d for d in dirs if d not in exclude_dirs]
        for f in files:
            rel_path = os.path.join(root, f).replace("\\", "/")
            if f.endswith(".cpp") and f not in exclude_files and rel_path not in exclude_paths:
                sources.append(rel_path)
    return sources

hengband_core_sources = collect_hengband_sources()

# --- ビルド設定 ---

# CPPPATH と USE_GODOT は全ソースに共通 (godot-cpp との互換性に問題なし)
env.Append(CPPPATH=[
    "src",
    "src/external-lib",
    "src/external-lib/include",  # fmtlib, tl::optional 等
])

env.Append(CPPDEFINES=["USE_GODOT", "GODOT_RICH_UI"])

# --- Hengband 専用ビルド環境 ---
# godot-cpp は CCFLAGS に /utf-8 (=/source-charset:utf-8 /execution-charset:utf-8) を追加する。
# Hengband は z-term の SJIS モデルに合わせて /execution-charset:932 が必要なため、
# env をクローンして /utf-8 を除去し独自のチャーセットフラグを設定する。
hengband_env = env.Clone()

if env["platform"] == "windows":
    # h-config.h は WIN32 マクロで WINDOWS を判定するが、
    # x86_64 MSVC は _WIN32 のみ定義するため明示的に追加。
    # string-processor.cpp の iconv.h 分岐も WIN32 で判定するため必須。
    hengband_env.Append(CPPDEFINES=["WINDOWS", "WIN32", "JP", "SJIS"])
    # fmtlib v11 の consteval フォーマット文字列検証を無効化
    # (_() マクロがランタイム文字列を返すため compile-time check 不可)
    hengband_env.Append(CPPDEFINES=["FMT_USE_CONSTEVAL=0"])
    # /utf-8 は /source-charset:utf-8 /execution-charset:utf-8 の短縮形。
    # /source-charset:utf-8 と同時指定できないため除去して個別に指定する。
    hengband_env["CCFLAGS"] = [f for f in hengband_env.get("CCFLAGS", []) if f != "/utf-8"]
    # /source-charset:utf-8  : ソースファイルを UTF-8 として読む
    # /execution-charset は省略 → システムデフォルト (日本語 Windows = CP932) を使用
    #   → z-term の 1バイト/セル・SJIS モデルと一致させるため必須
    hengband_env.Append(CCFLAGS=["/source-charset:utf-8"])
    # godot-cpp が /std:c++17 を追加するため除去してから /std:c++20 を設定する
    hengband_env["CXXFLAGS"] = [f for f in hengband_env.get("CXXFLAGS", []) if f not in ("/std:c++17", "/std:c++20")]
    # C++ 例外ハンドラ有効化 (MSVC 既定では無効)
    hengband_env.Append(CXXFLAGS=["/std:c++20", "/EHsc"])
    # timeGetTime (record-play-movie.cpp) と DbgHelp (stack-trace) に必要
    # LIBS はリンクステップで使われるため、SharedLibrary を呼ぶ env に追加する
    env.Append(LIBS=["winmm", "DbgHelp"])
else:
    # godot-cpp は既定で例外を無効化 (-fno-exceptions) するが、
    # Bakabakaband のゲームコアは angband-exceptions.h 等で例外を使用するため
    # ゲームコア専用環境からは -fno-exceptions を除去し -fexceptions を付与する。
    # (godot-cpp 自身は例外を投げないため env 側は無効のままで問題ない)
    hengband_env["CCFLAGS"] = [f for f in hengband_env.get("CCFLAGS", []) if f != "-fno-exceptions"]
    hengband_env["CXXFLAGS"] = [f for f in hengband_env.get("CXXFLAGS", []) if f != "-fno-exceptions"]
    hengband_env.Append(CXXFLAGS=["-std=c++20", "-fexceptions"])
    # autotools の autoconf.h (configure 生成) が無いため、Linux で常に真となる
    # POSIX フィーチャマクロを明示的に定義する。HAVE_CONFIG_H は autoconf.h を
    # include させてしまうため定義しない。
    # HAVE_USLEEP は h-config.h が HAVE_CONFIG_H 未定義時に自動定義するため付与しない
    # (二重定義警告を避ける)。
    hengband_env.Append(CPPDEFINES=["HAVE_SYS_TIME_H", "HAVE_MKSTEMP"])

# --- ライブラリのビルド ---

# fmtlib のコンパイル済み実装 (ヘッダオンリーではなくコンパイルモードで使用)
fmt_sources = ["src/external-lib/fmt/format.cc"]

# Hengband 専用環境でオブジェクトファイルを生成
hengband_objects = hengband_env.SharedObject(
    list(godot_backend_sources) + hengband_core_sources + fmt_sources
)

# Linux の既定では SharedLibrary が "lib" プレフィックスを付与するが、
# .gdextension の libraries パスは "bakabakaband.<platform>...so" を期待するため
# プレフィックスを除去してファイル名を一致させる (Windows は元から空)。
env["SHLIBPREFIX"] = ""

library = env.SharedLibrary(
    "godot_project/../bin/bakabakaband{}{}".format(
        env["suffix"],
        env["SHLIBSUFFIX"]
    ),
    source=hengband_objects,
)

Default(library)
