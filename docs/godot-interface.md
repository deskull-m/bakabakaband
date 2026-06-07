# Bakabakaband Godot インターフェース (GDExtension 移植)

[hengband-godot](https://github.com/deskull-m/hengband-godot)（変愚蛮怒 Godot 版）の
GDExtension バックエンドを bakabakaband のコードベースへ移植したものです。
Win32 GDI / WinMM に依存していた描画・音声・入力処理を
[Godot 4.x GDExtension](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/)
に置き換え、ゲームロジック (`src/` 以下) はそのまま利用します。

## アーキテクチャ

```
┌──────────────────────────────────────────────┐
│               Godot Engine                   │
│   (レンダリング / オーディオ / 入力)         │
├──────────────────────────────────────────────┤
│                GDExtension API               │
│  ┌────────────────────────────────────────┐  │
│  │  src/main-godot/  (C++ バックエンド)   │  │
│  │  ・GodotTerminal / GodotTileLayer      │  │
│  │  ・GodotInputHandler                   │  │
│  │  ・GodotAudioManager                   │  │
│  │  ・term_type フック (godot-term-hooks) │  │
│  └────────────────────────────────────────┘  │
├──────────────────────────────────────────────┤
│   src/  (Bakabakaband ゲームコア)            │
│   ※ 専用ゲームスレッドで実行                 │
└──────────────────────────────────────────────┘
```

ゲームコアは UI ブロッキングを避けるため専用スレッド (`run_game_thread()`) で実行され、
`term_type` フック経由で Godot 側のターミナル / タイル / 入力 / 音声と連携する。

## ディレクトリ構成

```
bakabakaband/
├── src/main-godot/          # Godot バックエンド (GDExtension)
│   ├── hengband-gdextension.{cpp,h}   # GDExtension エントリ・HengbandGame ノード
│   ├── godot-init.{cpp,h}             # ゲーム初期化・スレッド起動
│   ├── godot-term-hooks.{cpp,h}       # term_type フック実装
│   ├── godot-terminal.{cpp,h}         # テキストターミナル描画
│   ├── godot-tile-layer.{cpp,h}       # タイル描画
│   ├── godot-input-handler.{cpp,h}    # キーボード・マウス入力
│   ├── godot-audio-manager.{cpp,h}    # 効果音・BGM
│   ├── godot-player-status.{cpp,h}    # プレイヤーステータスのブリッジ
│   ├── save-file-scanner.{cpp,h}      # タイトル画面用セーブファイル一覧
│   ├── term-color-map.{cpp,h}         # 端末カラー → Godot Color 変換
│   └── stack-trace-godot.cpp          # スタックトレース (Godot 版スタブ)
├── godot_project/           # Godot プロジェクト
│   ├── project.godot
│   ├── bakabakaband.gdextension
│   ├── scenes/              # title / main / terminal_pane シーン
│   ├── scripts/             # GDScript (UI ロジック)
│   └── assets/
├── godot-cpp/               # godot-cpp サブモジュール (branch 4.3)
├── bin/                     # ビルド成果物 (.so / .dll, .gitignore 対象)
└── SConstruct               # GDExtension ビルドスクリプト
```

## ビルド手順

### 必要環境

| ツール | バージョン | 用途 |
|---|---|---|
| Godot Engine | 4.3 以上 | 実行・エディタ |
| SCons | 4.x 以上 | ビルドシステム |
| Python | 3.8 以上 | SCons 実行環境 |
| C++ コンパイラ | C++20 対応 (MSVC v143+ / g++ / clang++) | コンパイル |

### 1. サブモジュールの取得

```bash
git submodule update --init godot-cpp
```

### 2. GDExtension ライブラリのビルド

```bash
# Windows デバッグビルド (既定)
scons platform=windows target=template_debug

# Linux ビルド
scons platform=linux target=template_debug

# リリースビルド
scons target=template_release
```

ビルド成果物は `bin/bakabakaband.<platform>.<target>.x86_64.<so|dll>` に出力される。

### 3. Godot からの起動

`godot_project/project.godot` を Godot エディタで開いて実行するか、

```bash
godot --path godot_project
```

## bakabakaband への移植にあたっての改変点

hengband-godot は変愚蛮怒 (上流) のコードを前提としているため、bakabakaband の
CreatureEntity 統合リファクタリングに合わせて以下を変換した
(変換規則は `CLAUDE.md`「変愚蛮怒（上流）からのマージ指針」節に準拠)。

- `p_ptr` → `PlayerType::get_instance()` / `PlayerType::get_instance_ptr()`
  (`godot-init.cpp`, `godot-term-hooks.cpp`)
- `init_angband(p_ptr, …)` / `play_game(p_ptr, …)` の引数を
  `CreatureEntity &` 形式に対応
- `player_ptr->current_floor_ptr` → `creature.get_floor()`
- プレイヤーステータス読取りを CreatureEntity の virtual アクセサ経由に変更
  (`godot-player-status.cpp`):
  - `lev` → `get_level()`、`chp`/`mhp` → `get_current_hp()`/`get_max_hp()`
  - `csp`/`msp` → `get_csp()`/`get_msp()`、`au` → `get_au()`
  - `exp`/`max_exp` → `get_exp()`/`get_max_exp()`、`pspeed` → `get_speed()`
  - `dis_ac`/`dis_to_a` → `get_dis_ac()`/`get_dis_to_a()`
  - `stat_use[]`/`stat_top[]` → `get_stat_use(i)`/`get_stat_top(i)`
  - `rp_ptr`/`cp_ptr` → `creature.get_race_info()`/`get_class_info()`
- `godot-player-status.h` に `#include <cstdint>` を追加 (GCC で `uint64_t` を使うため)
- `SConstruct` の Linux ブランチで godot-cpp 既定の `-fno-exceptions` を除去し
  `-fexceptions` を付与 (ゲームコアが例外を使用するため)
- ライブラリ名・アプリ名を `bakabakaband` にリブランド
  (GDExtension の内部クラス名・エントリシンボルは互換性のため hengband のまま)

### GODOT_RICH_UI による端末ステータス表示の抑制

`GODOT_RICH_UI` マクロ定義時、各種ステータス値の端末描画を抑制し、Godot 側の
StatusPanel に表示を委ねる。以下の関数に `#ifdef GODOT_RICH_UI` ガードを追加:

| ファイル | 関数 |
|---|---|
| `src/core/window-redrawer.cpp` | `print_dungeon` |
| `src/world/world-turn-processor.cpp` | `print_time` |
| `src/window/main-window-left-frame.cpp` | `print_depth` |
| `src/window/main-window-stat-poster.cpp` | `print_speed` / `print_study` / `print_imitation` / `print_status` |

これらのガードは通常 (autotools / MSVC) ビルドでは無効 (マクロ未定義) のため、
端末版の挙動には影響しない。

## 既知の制限・今後の課題

- **セーブファイル形式**: `save-file-scanner.cpp` はタイトル画面のセーブ一覧表示用に
  セーブファイルヘッダを独自に走査する。bakabakaband のセーブデータ
  バージョン (現在 51) は上流と形式が異なるため、表示内容の検証・調整が必要。
- **ネットワーク機能**: `src/net/` (libcurl 依存) は GDExtension ビルドから除外している。
- **状態バー (`print_status`)**: 現状は端末描画を抑制するのみで、Godot StatusPanel への
  移行は未実装。
- GDExtension ビルドは通常の autotools / MSVC ビルドとは独立しており、
  CI には組み込んでいない。
