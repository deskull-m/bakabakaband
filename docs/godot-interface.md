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
│   ├── stack-trace-godot.cpp          # スタックトレース (Godot 版スタブ)
│   └── win-exception-tracer.cpp       # Windows 例外トレーサ
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
| Godot Engine | 4.3 以上 (プロジェクト設定・CI は 4.6) | 実行・エディタ |
| SCons | 4.x 以上 | ビルドシステム |
| Python | 3.8 以上 | SCons 実行環境 |
| C++ コンパイラ | C++20 対応 (MSVC v143+ / g++ / clang++) | コンパイル |

SCons が未インストールの場合は次のいずれかで導入する。

```bash
# Ubuntu / Debian
sudo apt-get install scons python3
# または pip 経由 (任意の OS)
pip install scons
```

### 1. サブモジュールの取得

`godot-cpp` (branch 4.3) をサブモジュールとして取得する。`SConstruct` は
先頭で `godot-cpp/SConstruct` を読み込むため、これが無いとビルドできない。

```bash
git submodule update --init godot-cpp
```

### 2. GDExtension ライブラリのビルド

`SConstruct` のオプションは godot-cpp 準拠 (`platform` / `target` /
`arch` 等)。並列ビルドは標準の `-j` を付与する。

```bash
# Windows デバッグビルド (既定: platform=windows target=template_debug)
scons platform=windows target=template_debug

# Linux ビルド
scons platform=linux target=template_debug

# リリースビルド
scons platform=linux target=template_release

# 並列ビルド (推奨)
scons platform=linux target=template_debug -j$(nproc)
```

ビルド成果物は `bin/bakabakaband.<platform>.<target>.x86_64.<so|dll>` に出力され、
`godot_project/bakabakaband.gdextension` の `[libraries]` パス
(`res://../bin/bakabakaband.<platform>.<target>.x86_64.<so|dll>`) と一致する。
`bin/` は `.gitignore` 対象。

`SConstruct` はゲームコアを glob で収集し、端末版バックエンド
(`main-win` / `main-unix` / `main-x11` / `main-gcu` / `main-cap`)・`src/net/`
(libcurl 依存)・`src/test/`・autotools の Makefile.am 未収録の孤立ソースを
除外する (ただし `main-unix/unix-user-ids.cpp` だけはゲームコアが Windows 以外で
参照するため Linux 等では個別に加える)。ソースを追加・除外する際は `SConstruct` の `exclude_dirs` /
`exclude_files` / `exclude_paths` を合わせて更新すること。プラットフォーム
別のチャーセット・例外・マクロ設定 (Windows: `/source-charset:utf-8` /
CP932 実行文字コード / `WINDOWS`・`WIN32`・`JP`・`SJIS`、Linux: `-fexceptions`
再付与 / POSIX フィーチャマクロ) も同スクリプトに集約されている。

### 3. Godot からの起動

`godot_project/project.godot` を Godot エディタ (4.3 以上) で開いて実行するか、

```bash
godot --path godot_project
```

`.gdextension` の `compatibility_minimum` は 4.3。ライブラリ (`bin/`) が
未ビルドだと Godot 起動時に GDExtension のロードに失敗するため、先に
ステップ 2 を済ませておくこと。

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
  - `csp`/`msp` → `get_current_mp()`/`get_max_mp()` (提案 6 で改名済み)、`au` → `get_au()`
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

## ワールドスナップショット (観測可能ワールドモデル)

3D 表示などのために、端末のセルではなく**フロアの状態**を Godot 側へ渡す仕組み
(#9292、Epic #9290 の第 2 段)。

| 層 | ファイル | 役割 |
|---|---|---|
| ゲームコア (表示非依存) | `src/view/observable-world.{h,cpp}` | `build_observable_world(player)` がプレイヤーの知り得るフロアの状態を `ObservableWorld` に取り出す |
| ブリッジ | `src/main-godot/godot-world-snapshot.{h,cpp}` | ゲームスレッドが `TERM_XTRA_FRESH` (メイン端末) で作り、不変の `shared_ptr` で Godot メインスレッドへ渡す |
| GDScript API | `HengbandGame` | `set_world_snapshot_enabled(bool)` / `get_world_snapshot()` / シグナル `world_snapshot_changed(version)` |

- **既定は無効**。`set_world_snapshot_enabled(true)` の間だけフロア全体を走査する
  (利用者がいないときにコストを掛けない)。キャラクター作成中など
  `character_dungeon` が偽の間は作らない。
- **端末版の画面より多くの情報は含まない**:
  - 地形は `is_map_terrain_visible()` (端末の地図描画と同じ判定) が真のグリッドだけ。
    未知のグリッドは `terrain = -1` / `shape = UNKNOWN`
  - クリーチャーは `is_visible_on_map()` が真のものだけ。視界内か感知のみかは
    `OBSERVED_CREATURE_IN_VIEW` で区別する
  - 幻覚中はクリーチャーの種族・体力・状態を伏せる (ボット出力と同じ)
  - 死体は地形の分かるグリッドに限る (端末の地図は地形が未知でも描くが、
    3D では地形と切り離して置くことになるため)
- 各グリッドの表示シンボル (`symbols`、1 グリッド 4 バイト: 前景色・前景文字・
  背景色・背景文字) は `map_info()` の結果そのもの。タイル表示中はタイルの
  行・列になるので、2D タイルセットをビルボードに流用できる。
- 形状 (`shapes`) は `ObservedTerrainShape` (床・壁・扉・階段・液体・木・入口等)。
  地形フラグから C++ 側で分類するので、GDScript は地形定義を持たなくてよい。
- 動作確認用に **Ctrl+Shift+M** でデバッグ用ミニマップ
  (`godot_project/scripts/world_debug_minimap.gd`) を表示できる。表示中だけ
  スナップショットが有効になる。**Ctrl+Shift+V** で検証モード (下記) を切り替える。

### 差分更新 (#9293)

| 要素 | 内容 |
|---|---|
| 観測点 | `src/view/view-observer.{h,cpp}` の `ViewObserver`。`lite_spot()` が `on_grid_changed(pos)`、`print_map()` が `on_all_grids_changed()` を呼ぶ (端末の地図・`OverviewMapCache` が描き直すのと同じ 2 か所)。未登録なら null 判定 1 回で終わり、乱数もゲームの状態も触らない |
| ブリッジ | `godot-world-snapshot.cpp` の観測者が変わったかもしれないグリッドを集め、次の push でそのグリッドだけ `observe_grid()` で求め直す。中身が実際に変わったグリッドに「最後に変わった版」を記録する |
| 全体の作り直し | 初回・`print_map()` (階の移動・画面の再描画)・フロアの大きさ/ダンジョン/階層/広域マップ表示の切替・通知の溜まりすぎ (8192 件)・スナップショットが無効だった間に変化があったとき。この版を `floor_version` とする |
| GDScript | `get_world_snapshot_changes(since_version)` が `since_version` より後に変わったグリッドだけを `indices` (行優先の添字) と同じ並びの配列で返す。`since_version` が `floor_version` より前なら全体 (`full = true`) を返す |

- **途中の版を読み飛ばしても取りこぼさない**。シグナルはフレームごとに最新の版で
  1 回しか来ないが、グリッドごとの版で比べるので、利用者は「前回受け取った版」を
  渡すだけでよい。
- **クリーチャーは差分化せず毎回全体を返す** (体力・状態の変化は `lite_spot()` を
  通らないため。走査は `m_max` までなので安い)。
- 同じ個体を追うには `creatures[].id` (`CreatureEntity::get_display_id()`) を使う。
  `m_idx` はモンスター配列の圧縮で変わり、空いた添字は別の個体に再利用される。
  `id` は圧縮では実体ごと移り、`wipe()` (添字の再利用) で付け直される。
  ゲームの進行には使わず、セーブもしない (ロード後・階の移動後は新しい番号になる)。
- **検証モード** (`set_world_snapshot_verify(true)`) では、差分で作った結果を毎回
  フロア全体から作り直した結果と突き合わせ、食い違ったグリッドを `printerr` し
  `get_world_snapshot_verify_mismatches()` に数える。`lite_spot()` を通らない
  見え方の変化を見つけるためのもので、通常は無効にしておく (全体を作り直す分だけ重い)。

## 画面の状態 (#9294)

3D 表示は地図を見ている間だけ出し、メニューや全画面の文章は端末で見せるため、
ゲームがいま何を画面に出しているかを Godot へ知らせる。

| 層 | ファイル | 役割 |
|---|---|---|
| 判定 (表示非依存) | `src/view/screen-state.{h,cpp}` | `classify_screen_state()` が入力待ちの文脈 (`io/input-context.h`)・重ねた画面の深さ (`character_icky_depth`)・重ねてから画面全体を消したか から `ScreenState` を決める |
| 画面全体を消した印 | `src/term/screen-processor.cpp` | `term_clear()` / `clear_from(0)` (メイン画面) で印を付け、`screen_save()` / `screen_load()` で外す。表示の判定にだけ使う |
| ブリッジ | `src/main-godot/godot-screen-state.{h,cpp}` | 入力待ちに入る直前 (`TERM_XTRA_EVENT`) と画面の更新 (`TERM_XTRA_FRESH`) で求め、変わったときだけ公開する |
| GDScript | `HengbandGame.get_screen_state()` / シグナル `screen_state_changed(state)` | `state` / `input_kind` / `icky_depth` / `prompt` / `has_cursor` / `cursor_x` / `cursor_y` |
| 切り替え | `godot_project/scripts/view_switcher.gd` | 地図ビューが登録されていて `GameState.use_map_view` が真のとき、`MAP` / `MAP_WITH_PROMPT` / `TARGETING` の間だけ地図ビューを見せる (地図ビューが無い現状では何もしない) |

| 状態 | 判定 |
|---|---|
| `NO_GAME` | フロアが無い (`character_dungeon` が偽。タイトル・キャラクター作成) |
| `FULLSCREEN_TEXT` | ファイル表示・一覧の閲覧・オプション・自動拾いエディタ・建物・キャラクター作成・店のコマンド待ち。または重ねた画面を開いてから画面全体を消した (キャラクター情報・知識メニューなど) |
| `TARGETING` | ターゲットの選択・見るコマンド・場所の指定。カーソルのフロア座標を `cursor_x` / `cursor_y` で渡す |
| `MAP` | 重ねた画面が無く、コマンド待ち (または入力待ちの文脈が無い) |
| `MAP_WITH_PROMPT` | 重ねた画面が無く、1 行で答える入力 (`-more-`・方向・y/n・数量・文字列・一覧を出していないアイテムや呪文の選択) |
| `OVERLAY_MENU` | 重ねた画面があり、画面全体は消していない (持ち物・アイテムや呪文の一覧など) |

**判定が曖昧なケース** (いずれも端末を見せる側に倒れるか、実害が小さいもの):

- 持ち物の一覧 (`i`) は入力待ちの文脈を設定しないので `input_kind = -1`。重ねた画面の深さだけで `OVERLAY_MENU` になる。
- 重ねた画面の上で `term_clear()` / `clear_from(0)` を使わずに全行を書き換える表示は `OVERLAY_MENU` になる (全画面の表示だが、地図ビューを隠す側なので安全)。
- 見るコマンドの途中でモンスターの思い出などを重ねて表示し、その間に新しい入力待ちの文脈を設定しない場合は `TARGETING` のまま (地図ビューを見せたまま端末に説明が出る)。
- 入力待ちの文脈は「対応した入力待ちだけ」が設定するので、文脈の無い入力待ちは重ねた画面の深さだけで決まる。

`tools/godot/screen-state-runtime-check.gd` で実ゲームの遷移を確かめられる (地図 → 持ち物 → キャラクター情報 → 見る → 知識 → 投げる → 方向入力 → 再びキャラクター情報 → 持ち物 の 17 遷移が期待どおり)。

## 表示用のゲームイベント (#9295)

3D 表示のアニメーション・エフェクトのため、プレイヤーが知覚できた出来事を型付きで流す。

| 層 | ファイル | 役割 |
|---|---|---|
| イベントと受け手 | `src/view/view-event.{h,cpp}` | `ViewEvent` / `ViewEventSink`。未登録なら呼出側は `is_view_event_sink_active()` の 1 判定で何もしない。乱数・ゲーム状態には触れない |
| 発火点 | 下表 | 1 行の `notify_view_*()` |
| ブリッジ | `src/main-godot/godot-game-events.{h,cpp}` | 有効な間キューに溜め (最大 1024、溢れたら古いものを捨てる)、毎フレーム `game_events(Array)` シグナルで古い順に渡す |
| GDScript | `HengbandGame.set_game_events_enabled(bool)` / シグナル `game_events(events)` | 既定は無効 |

| 種類 | 発火点 | 知らせる条件 |
|---|---|---|
| `MELEE` (1) | プレイヤー: `exe_player_attack_to_monster()` の各打撃 / モンスター→プレイヤー: `MonsterAttackPlayer::process_monster_blows()` / モンスター同士: `monster-attack-monster.cpp` | どちらかが見えている。見えない側は `id = 0`・座標なし |
| `PROJECTION` (2) | `project()` (効果範囲が決まった直後) | 盲目でなく `PROJECT_HIDE` でない (端末版の描画と同じ)。軌跡・効果範囲はプレイヤーの視線が通るグリッドだけ。どちらも空なら知らせない |
| `DAMAGE` (3) | `CreatureEntity::apply_raw_damage()` (プレイヤー・モンスター共通のプリミティブ) | プレイヤー、または見えているモンスター |
| `DEATH` (4) | `monster_death()` の冒頭 / プレイヤーの `take_hit()` の死亡判定 | 同上 |
| `MOVE` (5) / `TELEPORT` (6) / `APPEAR` (7) / `DISAPPEAR` (8) | ブリッジがワールドスナップショットの前回との差から作る (コアからは流さない) | スナップショットも有効なとき、同じ階の中だけ |

- **移動をコアから流さない理由**: `set_position()` の呼出は 25 か所あり、生成・ロードも通るため。
  スナップショットのクリーチャーの表示用 ID と座標を比べれば、歩行も瞬間移動も出入りも分かる。
- **`TELEPORT` は「スナップショットの間に 2 グリッド以上動いた」の意味**。加速したモンスターが
  1 回の画面更新の間に 2 歩進んだ場合も `TELEPORT` になる (実測で確認)。演出側は距離で補間の仕方を決めること。
- **演出のためにゲームを待たせない** (非ブロッキング)。受け手は後追いで再生する。
  ボルトの軌跡を見せ切ってから次へ進めたい場合の待ち合わせは将来の拡張。
- `tools/godot/game-events-runtime-check.gd` で実ゲームのイベントを数えられる
  (地下 1 階で移動 24・瞬間移動 1・近接 22・被弾 12・死亡 1 を確認。呪文を撃たせていないので `PROJECTION` は未確認)。
- デバッグ用ミニマップ (Ctrl+Shift+M) の右側に直近のイベントを一覧する。

### 実ゲームでの確認

`tools/godot/world-snapshot-runtime-check.gd` が `main.tscn` を headless で起動して
セーブを読み込み、移動しながら差分を手元に反映して全体と突き合わせる。

```bash
HB_SAVE=<使い捨てのセーブファイル> godot --headless --path godot_project \
    --script "$PWD"/tools/godot/world-snapshot-runtime-check.gd
```

セーブは端末版のヘッドレス実行 (`tools/bot/README.md`) で作れる。端末版と Godot 版の
Linux ビルドで日本語/英語が違っても読み込める。実測 (地下 1 階、可視モンスター 19 体、
154 版): 差分 109 グリッドを反映した手元の配列が全体と一致、検証モードの食い違い 0、
クリーチャーの `id` は 19 体とも全期間で不変。

`GodotTerminal.get_row_text(y)` で端末の行を読めるので、スクリプトから画面を確認できる。

## 既知の制限・今後の課題

- **セーブファイル形式**: `save-file-scanner.cpp` はタイトル画面のセーブ一覧表示用に
  セーブファイルヘッダを独自に走査する。bakabakaband のセーブデータ
  バージョン (現在 63、`SAVEFILE_VERSION`) は上流と形式が異なるため、表示内容の検証・調整が必要。
- **ネットワーク機能**: `src/net/` (libcurl 依存) は GDExtension ビルドから除外している。
- **状態バー (`print_status`)**: 現状は端末描画を抑制するのみで、Godot StatusPanel への
  移行は未実装。
- **Windows (MSVC) 向けの GDExtension ビルドは CI 対象外**。CI は Linux 向けのみ
  (下記「CI」節)。

## CI

`.github/workflows/build-godot-gdextension.yml` が、`src/**` / `SConstruct` /
`godot-cpp` / `godot_project/**` を変更する PR で次を行う。

1. `scons platform=linux target=template_debug` で GDExtension ライブラリをビルド
   (`SCONS_CACHE` によるオブジェクトキャッシュ付き)
2. Godot (`GODOT_VERSION`、`project.godot` の `config/features` に合わせて 4.6) を
   headless で起動し、`.github/scripts/check-godot-extension-load.gd` で
   拡張のクラス (`HengbandGame` / `GodotTerminal` / `GodotTileLayer` /
   `GodotInputHandler` / `GodotAudioManager`) が登録されたことを確認する

2 はゲームスレッドもタイトル画面も起動しないので、ゲーム内容の動作確認ではない。
未定義シンボル・初期化時の例外などで**ライブラリが読み込めなくなる変更**を検知する。

ローカルで同じ確認をする場合:

```bash
scons platform=linux target=template_debug -j$(nproc)
sh .github/scripts/check-godot-extension-load.sh <godot 実行ファイル> godot_project
```

**Linux ビルドは英語版になる**点に注意 (`SConstruct` の Linux 分岐は `JP` を定義しない)。
日本語版の Godot ビルドは Windows 分岐 (`JP` / `SJIS`) のみで、CI では確認していない。
