/*!
 * @file godot-term-hooks.cpp
 * @brief term_type フック関数の Godot 実装
 */

#include "godot-term-hooks.h"
#include "godot-audio-manager.h"
#include "godot-input-handler.h"
#include "godot-player-status.h"
#include "godot-terminal.h"
#include "godot-tile-layer.h"

#include "core/visuals-reseter.h"
#include "game-option/runtime-arguments.h"
#include "game-option/special-options.h"
#include "locale/japanese.h"
#include "system/floor/floor-info.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "target/target-checker.h"
#include "term/gameterm.h"
#include "term/z-term.h"
#include "window/main-window-util.h"

#include <atomic>
#include <chrono>
#include <thread>

using namespace hengband_godot;

/// シーン内の GodotInputHandler への参照（HengbandGame が設定する）
static GodotInputHandler *s_input_handler = nullptr;
/// term_data_godot 配列への参照（TERM_XTRA_REACT / resize_hook 用）
static term_data_godot *s_term_data = nullptr;
static int s_term_count = 0;

/// Godot スレッドからタイルモードが変更されたことをゲームスレッドに伝えるフラグ
/// apply_tile_mode() がセット → TERM_XTRA_REACT がクリアして reset_visuals() を呼ぶ
static std::atomic<bool> s_graphics_mode_changed{ false };

/// ビッグタイルの ON/OFF が変化したことをゲームスレッドに伝えるフラグ
/// apply_bigtile_mode() がセット → TERM_XTRA_REACT がクリアして resize_map() を呼ぶ
static std::atomic<bool> s_bigtile_mode_changed{ false };

void set_input_handler(GodotInputHandler *handler)
{
    s_input_handler = handler;
}

void set_term_data_array(term_data_godot *arr, int count)
{
    s_term_data = arr;
    s_term_count = count;
}

// ---------------------------------------------------------------------------
// resize_hook: ゲームがターミナルサイズを変更した時のコールバック
// ---------------------------------------------------------------------------
void term_resize_hook_godot()
{
    if (!game_term || !game_term->data) {
        return;
    }
    auto *td = reinterpret_cast<term_data_godot *>(game_term->data);
    const int new_cols = static_cast<int>(game_term->wid);
    const int new_rows = static_cast<int>(game_term->hgt);
    if (td->cols == new_cols && td->rows == new_rows) {
        return;
    }
    td->cols = new_cols;
    td->rows = new_rows;
    if (td->terminal) {
        td->terminal->set_grid_size(new_cols, new_rows);
    }
    if (td->tile_layer) {
        td->tile_layer->set_grid_size(new_cols, new_rows);
    }
}

// ---------------------------------------------------------------------------
// text_hook: テキスト描画
// ---------------------------------------------------------------------------
errr term_text_godot(int x, int y, int n, TERM_COLOR a, concptr s)
{
    auto *term = get_terminal(game_term);
    if (!term) {
        return 1;
    }
#ifdef JP
    // z-term から渡される s は SJIS(CP932) エンコード、n はバイト数 = セル数。
    // Godot のフォント描画は UTF-8 を要求するため変換する。
    // n バイト分だけを変換対象とし、null 終端に依存しない。
    const auto utf8 = sys_to_utf8(std::string_view(s, static_cast<size_t>(n)));
    const char *str = utf8 ? utf8->c_str() : s;
    term->draw_text(x, y, n, static_cast<uint8_t>(a), str);
#else
    term->draw_text(x, y, n, static_cast<uint8_t>(a), s);
#endif
    return 0;
}

// ---------------------------------------------------------------------------
// wipe_hook: セル消去
// ---------------------------------------------------------------------------
errr term_wipe_godot(int x, int y, int n)
{
    auto *term = get_terminal(game_term);
    if (!term) {
        return 1;
    }
    term->wipe_cells(x, y, n);
    return 0;
}

// ---------------------------------------------------------------------------
// curs_hook: カーソル描画
// ---------------------------------------------------------------------------
errr term_curs_godot(int x, int y)
{
    auto *term = get_terminal(game_term);
    if (!term) {
        return 1;
    }
    term->set_cursor(x, y);
    return 0;
}

// ---------------------------------------------------------------------------
// xtra_hook: 拡張アクション
// ---------------------------------------------------------------------------
errr term_xtra_godot(int n, int v)
{
    switch (n) {
    case TERM_XTRA_CLEAR: {
        auto *term = get_terminal(game_term);
        if (term) {
            term->clear_all();
        }
        auto *tiles = get_tile_layer(game_term);
        if (tiles) {
            tiles->clear_all();
        }
        return 0;
    }
    case TERM_XTRA_FRESH:
    case TERM_XTRA_FROSH: {
        // 全ターミナルの再描画をメインスレッドにキュー
        auto *term = get_terminal(game_term);
        if (term) {
            term->call_deferred("queue_redraw");
        }
        auto *tiles = get_tile_layer(game_term);
        if (tiles) {
            tiles->call_deferred("queue_redraw");
        }
        // メインターミナル (term 0) の更新時にプレイヤーステータスを同期する
        if (game_term == term_screen) {
            player_status_push(PlayerType::get_instance_ptr());
        }
        return 0;
    }
    case TERM_XTRA_SHAPE:
        // カーソル表示/非表示 (v=0 で非表示)
        if (v == 0) {
            auto *term = get_terminal(game_term);
            if (term) {
                term->hide_cursor();
            }
        }
        return 0;
    case TERM_XTRA_DELAY:
        // v = ミリ秒
        // Phase 4: シングルスレッドのため sleep で実装
        // Phase 7: ゲームスレッド内での sleep に置き換える
        if (v > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(v));
        }
        return 0;
    case TERM_XTRA_EVENT:
        // v=true: キーが来るまで待機、v=false: ノンブロッキング
        if (s_input_handler) {
            if (v) {
                s_input_handler->wait_for_key(); // Phase 4: 即時リターン
            } else {
                s_input_handler->poll_events();
            }
        }
        return 0;
    case TERM_XTRA_FLUSH:
    case TERM_XTRA_BORED:
    case TERM_XTRA_ALIVE:
    case TERM_XTRA_LEVEL:
        return 0;
    case TERM_XTRA_REACT:
        // Windows 版 term_xtra_win_react() 相当:
        //   1. タイルモードが変化していれば prf ファイルを再ロード (reset_visuals)
        //      ※ このケースは必ずゲームスレッドから呼ばれるので安全
        //   2. 全ターミナルのサイズ同期
        if (s_graphics_mode_changed.exchange(false)) {
            reset_visuals(PlayerType::get_instance());
        }
        // ビッグタイル変更時: パネル境界のみ再計算し、スクロール幅を補正する。
        // resize_map() は内部で term_redraw() を呼ぶため、
        // do_cmd_redraw 内から TERM_XTRA_REACT に来た場合に二重クリアが起きる。
        // そのため verify_panel() だけ呼び、描画は do_cmd_redraw 本体に任せる。
        if (s_bigtile_mode_changed.exchange(false)) {
            auto &player = PlayerType::get_instance();
            const auto *floor = player.get_floor();
            panel_row_max = 0;
            panel_col_max = 0;
            panel_row_min = floor->height;
            panel_col_min = floor->width;
            verify_panel(player);
        }
        if (s_term_data) {
            for (int i = 0; i < s_term_count; ++i) {
                auto &td = s_term_data[i];
                const int w = static_cast<int>(td.t.wid);
                const int h = static_cast<int>(td.t.hgt);
                if (w > 0 && h > 0 && (td.cols != w || td.rows != h)) {
                    td.cols = w;
                    td.rows = h;
                    if (td.terminal) {
                        td.terminal->set_grid_size(w, h);
                    }
                    if (td.tile_layer) {
                        td.tile_layer->set_grid_size(w, h);
                    }
                }
            }
        }
        return 0;
    case TERM_XTRA_SOUND:
        hengband_godot::audio_play_sound(v);
        return 0;
    case TERM_XTRA_MUSIC_BASIC:
    case TERM_XTRA_MUSIC_DUNGEON:
    case TERM_XTRA_MUSIC_QUEST:
    case TERM_XTRA_MUSIC_TOWN:
    case TERM_XTRA_MUSIC_MONSTER:
        hengband_godot::audio_play_music(n, v);
        return 0;
    case TERM_XTRA_MUSIC_MUTE:
        hengband_godot::audio_stop_music();
        return 0;
    default:
        return 1; // 未サポート
    }
}

// ---------------------------------------------------------------------------
// pict_hook: タイル描画
// ---------------------------------------------------------------------------
errr term_pict_godot(TERM_LEN x, TERM_LEN y, int n,
    const TERM_COLOR *ap, concptr cp,
    const TERM_COLOR *tap, concptr tcp)
{
    auto *tiles = get_tile_layer(game_term);
    if (!tiles || !tiles->is_tileset_loaded()) {
        // タイルセット未ロード時はテキスト消去で代替
        return term_wipe_godot(x, y, n);
    }

    // タイルを描画する位置のテキスト層セルを空白にする。
    // GodotTerminal は空白セルを描画スキップするため、タイル層が透けて見える。
    auto *term = get_terminal(game_term);
    if (term) {
        term->wipe_cells(x, y, n);
    }

    tiles->draw_tiles(x, y, n,
        reinterpret_cast<const uint8_t *>(ap), cp,
        reinterpret_cast<const uint8_t *>(tap), tcp);
    return 0;
}

// ---------------------------------------------------------------------------
// init_hook / nuke_hook
// ---------------------------------------------------------------------------
void term_init_godot(term_type *t)
{
    // GodotTerminal の設定は HengbandGame::_ready() 側で行う
    (void)t;
}

void term_nuke_godot(term_type *t)
{
    // GodotTerminal はシーンのライフサイクルで管理されるため何もしない
    (void)t;
}

// ---------------------------------------------------------------------------
// タイルモード切り替えユーティリティ
// ---------------------------------------------------------------------------

void apply_bigtile_mode(bool enabled)
{
    arg_bigtile = enabled;
    use_bigtile = enabled;
    // ゲームスレッドに panel_col_max の再計算を依頼する
    s_bigtile_mode_changed.store(true);
    s_graphics_mode_changed.store(true);
}

void apply_tile_mode(bool enabled, const char *graf_name)
{
    // フラグ類の設定は Godot メインスレッドから呼ばれても安全な単純代入のみ行う。
    // reset_visuals() の呼び出しはゲームスレッドで TERM_XTRA_REACT が処理する。

    use_graphics = enabled;
    // graf_name が一時バッファを指している場合に備えて、
    // ANGBAND_GRAF には必ず静的な文字列リテラルを代入する。
    if (!enabled) {
        ANGBAND_GRAF = "ascii";
    } else if (strcmp(graf_name, "new") == 0) {
        ANGBAND_GRAF = "new";
    } else if (strcmp(graf_name, "ne2") == 0) {
        ANGBAND_GRAF = "ne2";
    } else {
        ANGBAND_GRAF = "old"; // "old" またはデフォルト
    }

    // term_screen の higher_pict フラグを更新する
    if (term_screen) {
        term_screen->higher_pict = enabled;
    }

    // ゲームスレッドへの通知: 次の TERM_XTRA_REACT で reset_visuals() を呼ばせる
    s_graphics_mode_changed.store(true);
}
