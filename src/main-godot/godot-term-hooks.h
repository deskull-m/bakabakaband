#pragma once
/*!
 * @file godot-term-hooks.h
 * @brief term_type フック関数（Godot バックエンド）
 *
 * main-win.cpp の term_text_win / term_wipe_win / term_curs_win 相当を
 * Godot API で実装する。
 *
 * 使い方:
 *   term_data_godot td;
 *   td.terminal = <GodotTerminal ノードへのポインタ>;
 *   term_type *t = &td.t;
 *   term_init(t, cols, rows, KEY_QUEUE_SIZE);
 *   t->text_hook  = term_text_godot;
 *   t->wipe_hook  = term_wipe_godot;
 *   t->curs_hook  = term_curs_godot;
 *   t->xtra_hook  = term_xtra_godot;
 *   t->data       = (vptr)&td;
 */

#include "term/z-term.h"

namespace hengband_godot {
class GodotTerminal;
class GodotTileLayer;
class GodotInputHandler;
} // namespace hengband_godot

/*!
 * @brief Godot バックエンド用の term_data 構造体
 *
 * game_term->data にキャストして使用する。
 */
struct term_data_godot {
    term_type t{};
    hengband_godot::GodotTerminal *terminal{ nullptr }; ///< テキスト描画ノード
    hengband_godot::GodotTileLayer *tile_layer{ nullptr }; ///< タイル描画ノード
    int cols{ 80 };
    int rows{ 24 };
    int cell_w{ 8 };
    int cell_h{ 16 };
};

/// GodotInputHandler をグローバル参照として登録する
void set_input_handler(hengband_godot::GodotInputHandler *handler);

/// term_data_godot 配列をグローバル参照として登録する (TERM_XTRA_REACT 用)
void set_term_data_array(term_data_godot *arr, int count);

/// 2倍幅タイルモードを切り替える
/// arg_bigtile を設定し term_resize() で use_bigtile と同期させる
void apply_bigtile_mode(bool enabled);

/// タイル描画の有効/無効を切り替える
/// use_graphics / ANGBAND_GRAF / higher_pict を一括設定し、
/// ゲームスレッドが起動済みであれば reset_visuals() を呼び出す。
/// @param enabled  true = タイルモード、false = テキストモード
/// @param graf_name  "old"(8x8) / "new"(16x16) / "ne2"(32x32) / "ascii"
void apply_tile_mode(bool enabled, const char *graf_name = "old");

/// resize_hook: ターミナルサイズ変更時のコールバック
void term_resize_hook_godot();

/// text_hook: テキスト描画
errr term_text_godot(int x, int y, int n, TERM_COLOR a, concptr s);

/// wipe_hook: 消去
errr term_wipe_godot(int x, int y, int n);

/// curs_hook: カーソル描画
errr term_curs_godot(int x, int y);

/// xtra_hook: 拡張アクション（イベント処理・音声等）
errr term_xtra_godot(int n, int v);

/// init_hook: ターミナル初期化
void term_init_godot(term_type *t);

/// nuke_hook: ターミナル破棄
void term_nuke_godot(term_type *t);

/// pict_hook: タイル描画
errr term_pict_godot(TERM_LEN x, TERM_LEN y, int n,
    const TERM_COLOR *ap, concptr cp,
    const TERM_COLOR *tap, concptr tcp);

/// ターミナルデータから GodotTerminal ポインタを取得するヘルパー
inline hengband_godot::GodotTerminal *get_terminal(term_type *t)
{
    return reinterpret_cast<term_data_godot *>(t->data)->terminal;
}

/// ターミナルデータから GodotTileLayer ポインタを取得するヘルパー
inline hengband_godot::GodotTileLayer *get_tile_layer(term_type *t)
{
    return reinterpret_cast<term_data_godot *>(t->data)->tile_layer;
}
