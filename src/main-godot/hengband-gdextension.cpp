/*!
 * @file hengband-gdextension.cpp
 * @brief Hengband GDExtension 実装 + エントリポイント
 */

#include "hengband-gdextension.h"
#include "godot-audio-manager.h"
#include "godot-init.h"
#include "godot-input-handler.h"
#include "godot-player-status.h"
#include "godot-term-hooks.h"
#include "godot-terminal.h"
#include "godot-tile-layer.h"
#include "save-file-scanner.h"

#include "game-option/special-options.h"
#include "system/system-variables.h"

#include "term/gameterm.h"
#include "term/z-term.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <algorithm>
#include <filesystem>
#include <string>

#ifdef _WIN32
#include <windows.h>

/// UTF-8 std::string → Windows UTF-16 wstring
static std::wstring utf8_to_wstring(const std::string &utf8)
{
    if (utf8.empty()) {
        return {};
    }
    const int wlen = ::MultiByteToWideChar(
        CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (wlen <= 1) {
        return {};
    }
    std::wstring result(wlen - 1, L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, result.data(), wlen);
    return result;
}

/// Windows UTF-16 wstring → UTF-8 std::string
static std::string wstring_to_utf8(const std::wstring &wstr)
{
    if (wstr.empty()) {
        return {};
    }
    const int len = ::WideCharToMultiByte(
        CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) {
        return {};
    }
    std::string result(len - 1, '\0');
    ::WideCharToMultiByte(
        CP_UTF8, 0, wstr.c_str(), -1, result.data(), len, nullptr, nullptr);
    return result;
}

/// Godot String (UTF-8) → std::filesystem::path (Windows では wstring 経由)
static std::filesystem::path godot_string_to_path(const godot::String &s)
{
    const std::string utf8 = s.utf8().get_data();
    if (utf8.empty()) {
        return {};
    }
    return std::filesystem::path(utf8_to_wstring(utf8));
}

/// std::filesystem::path → Godot String
/// Windows: wchar_t* コンストラクタが UTF-16 として正しく処理される
static godot::String path_to_godot_string(const std::filesystem::path &p)
{
    return godot::String(p.wstring().c_str());
}
#else
static std::filesystem::path godot_string_to_path(const godot::String &s)
{
    return std::filesystem::path(s.utf8().get_data());
}
static godot::String path_to_godot_string(const std::filesystem::path &p)
{
    const auto u8 = p.u8string();
    return godot::String::utf8(reinterpret_cast<const char *>(u8.c_str()));
}
#endif

using namespace godot;
using namespace hengband_godot;

// ---------------------------------------------------------------------------
// HengbandGame 実装
// ---------------------------------------------------------------------------

void HengbandGame::_ready()
{
    // 全ターミナルをプレースホルダとして事前初期化する。
    // angband_terms[0..7] が有効なポインタを持つようにしつつ、
    // terminal/tile_layer は nullptr のままにしておく。
    // 実際のノードは GDScript から register_terminal() で登録される。
    for (int i = 0; i < HENGBAND_TERM_COUNT; ++i) {
        setup_terminal(i, nullptr, nullptr, 80, 24);
    }

    // InputHandler を取得して TERM_XTRA_EVENT に登録
    input_handler_ = Object::cast_to<GodotInputHandler>(
        get_node_or_null(NodePath("InputHandler")));
    if (input_handler_) {
        set_input_handler(input_handler_);
    }

    // term_data 配列を TERM_XTRA_REACT / resize_hook に登録
    set_term_data_array(term_data_.data(), HENGBAND_TERM_COUNT);
}

/// GodotPlayerStatusSnapshot の各フィールドを Dictionary に変換するヘルパー
static godot::Dictionary snapshot_to_dict(const GodotPlayerStatusSnapshot &s)
{
    godot::Dictionary d;
    d["hp"] = s.chp;
    d["max_hp"] = s.mhp;
    d["sp"] = s.csp;
    d["max_sp"] = s.msp;
    d["level"] = s.level;
    d["dun_level"] = s.dun_level;
    d["gold"] = static_cast<int64_t>(s.gold);
    d["exp"] = static_cast<int64_t>(s.exp);
    d["max_exp"] = static_cast<int64_t>(s.max_exp);
    d["speed"] = s.speed;
    d["ac"] = s.display_ac;
    d["name"] = godot::String::utf8(s.name.c_str());
    d["race"] = godot::String::utf8(s.race_name.c_str());
    d["class"] = godot::String::utf8(s.class_name.c_str());
    // 能力値 (0=STR, 1=INT, 2=WIS, 3=DEX, 4=CON, 5=CHR)
    static const char *const STAT_KEYS[6] = { "str", "int", "wis", "dex", "con", "chr" };
    for (int i = 0; i < 6; ++i) {
        d[godot::String("stat_") + STAT_KEYS[i]] = s.stat_use[i];
        d[godot::String("stat_top_") + STAT_KEYS[i]] = s.stat_top[i];
    }
    return d;
}

void HengbandGame::_process(double delta)
{
    (void)delta;
    // ゲームスレッドがステータスを更新したら player_status_changed シグナルを発行する
    const auto snap = player_status_get_snapshot();
    if (snap.is_valid && snap.version != last_status_version_) {
        last_status_version_ = snap.version;
        emit_signal("player_status_changed", snapshot_to_dict(snap));
    }
}

godot::Dictionary HengbandGame::get_player_status()
{
    const auto snap = player_status_get_snapshot();
    if (!snap.is_valid) {
        return godot::Dictionary();
    }
    return snapshot_to_dict(snap);
}

void HengbandGame::_notification(int p_what)
{
    // WM_CLOSE_REQUEST ではゲームスレッドを強制停止しない。
    // main.gd 側がキューに Ctrl+X を注入して Hengband にセーブ＆終了させ、
    // godot_quit_aux が tree->quit() を呼ぶことで自然終了する。

    if (p_what == NOTIFICATION_EXIT_TREE) {
        // ノードがシーンツリーから取り除かれる直前にスレッドを安全に終了させる。
        // 通常は godot_quit_aux → tree->quit() の流れでゲームスレッドが
        // 既に終了しているため、ここでは即座に返る。
        if (input_handler_) {
            input_handler_->request_stop();
        }
        if (game_thread_.is_valid() && game_thread_->is_started()) {
            game_thread_->wait_to_finish();
        }
        // スレッド停止後に全ターミナル参照を無効化する。
        // LayoutRoot / TerminalPane は HengbandGame より後に解放されるが、
        // ゲームスレッドが停止しているため queue_redraw 等の呼び出しは発生しない。
        for (auto &td : term_data_) {
            td.terminal = nullptr;
            td.tile_layer = nullptr;
        }
        for (auto &root : sub_window_roots_) {
            root = nullptr;
        }
    }
}

bool HengbandGame::is_game_started() const
{
    return game_thread_.is_valid() && game_thread_->is_started();
}

void HengbandGame::start_game(const String &lib_path, const String &save_path)
{
    if (game_thread_.is_valid() && game_thread_->is_started()) {
        return; // 既に起動中
    }

    // lib_path が空の場合は実行ファイルの隣の lib/ を使う
    String resolved_lib;
    if (lib_path.is_empty()) {
        const String exe = OS::get_singleton()->get_executable_path();
        resolved_lib = exe.get_base_dir().path_join("lib");
    } else {
        resolved_lib = lib_path;
    }

    game_thread_.instantiate();
    game_thread_->start(
        Callable(this, "_game_thread_func").bind(resolved_lib, save_path),
        Thread::PRIORITY_NORMAL);
}

/// ゲームスレッド本体 (Godot Thread から呼ばれる)
void HengbandGame::_game_thread_func(String lib_path, String save_path)
{
    const auto lib_fs = godot_string_to_path(lib_path);
    const auto save_fs = godot_string_to_path(save_path);
    run_game_thread(lib_fs, save_fs);
}

void HengbandGame::set_game_font(const Ref<Font> &font, int size)
{
    font_ = font;
    font_size_ = size;
    // 既に _ready() で初期化済みのターミナルにも反映する
    for (auto &td : term_data_) {
        if (td.terminal && font_.is_valid()) {
            td.terminal->set_terminal_font(font_, font_size_);
        }
    }

    // タイルモードが有効な場合、フォント変更後のセルサイズをタイルレイヤーに反映する
    auto *tiles = term_data_[0].tile_layer;
    auto *term0 = term_data_[0].terminal;
    if (use_graphics && tiles && term0) {
        const int tw = term0->get_cell_width();
        const int th = term0->get_cell_height();
        if (tw > 0 && th > 0) {
            tiles->set_tile_size(tw, th);
        }
    }
}

void HengbandGame::set_bigtile_enabled(bool enabled)
{
    auto *tiles = term_data_[0].tile_layer;
    if (tiles) {
        tiles->set_bigtile(enabled);
    }
    apply_bigtile_mode(enabled);
}

void HengbandGame::set_tile_rendering_enabled(bool enabled)
{
    auto &td0 = term_data_[0];
    if (td0.tile_layer) {
        if (!enabled) {
            td0.tile_layer->clear_all();
        }
        td0.tile_layer->set_visible(enabled);
    }
    // use_graphics / ANGBAND_GRAF / higher_pict / reset_visuals を一括設定
    apply_tile_mode(enabled);
}

bool HengbandGame::load_tileset(const String &tileset_path,
    const String &mask_path,
    int cell_w, int cell_h,
    const String &graf_name)
{
    // メインターミナルの TileLayer にタイルセットを読み込む
    // tile_w / tile_h はフォントサイズに合わせる（cell_w と同じ値でよい）
    auto *tiles = term_data_[0].tile_layer;
    if (!tiles) {
        return false;
    }
    // 描画先サイズはフォントが決定するターミナルのセルサイズに合わせる
    const auto *term = term_data_[0].terminal;
    const int tile_w = (term && term->get_cell_width() > 0) ? term->get_cell_width() : cell_w;
    const int tile_h = (term && term->get_cell_height() > 0) ? term->get_cell_height() : cell_h;

    const bool ok = tiles->load_tileset(
        tileset_path.utf8().get_data(),
        mask_path.utf8().get_data(),
        cell_w, cell_h, tile_w, tile_h);

    if (ok) {
        // 旧タイルセットの描画バッファを先にクリアする。
        // テクスチャ差し替え後に古い座標データで queue_redraw が走ると
        // 旧座標 × 新テクスチャで黒く描画されてしまうため。
        tiles->clear_all();
        tiles->set_visible(true);
        // graf_name 未指定の場合は "old" (8x8) をデフォルトとする。
        // utf8().get_data() は一時 CharString を指すため、std::string に格納して
        // 呼び出し中はポインタが有効であることを保証する。
        const std::string gname = graf_name.is_empty() ? "old" : graf_name.utf8().get_data();
        apply_tile_mode(true, gname.c_str());
    }
    return ok;
}

void HengbandGame::setup_terminal(int idx, GodotTerminal *term,
    GodotTileLayer *tiles, int cols, int rows)
{
    auto &td = term_data_[idx];
    td.terminal = term;
    td.tile_layer = tiles;
    td.cols = cols;
    td.rows = rows;

    if (term) {
        term->set_grid_size(cols, rows);
    }
    if (tiles) {
        tiles->set_grid_size(cols, rows);
    }

    term_type *t = &td.t;
    term_init(t, cols, rows, 256);
    t->soft_cursor = true;
    t->attr_blank = 1; // TERM_WHITE
    t->char_blank = ' ';
    t->init_hook = term_init_godot;
    t->nuke_hook = term_nuke_godot;
    t->text_hook = term_text_godot;
    t->wipe_hook = term_wipe_godot;
    t->curs_hook = term_curs_godot;
    t->pict_hook = term_pict_godot;
    t->xtra_hook = term_xtra_godot;
    t->resize_hook = term_resize_hook_godot;
    t->data = static_cast<vptr>(&td);

    if (idx < HENGBAND_TERM_COUNT) {
        angband_terms[idx] = t;
    }
}

void HengbandGame::set_sub_window_visible(int idx, bool visible)
{
    if (idx < 0 || idx >= HENGBAND_TERM_COUNT) {
        return;
    }
    auto *root = sub_window_roots_[idx];
    if (root) {
        root->set_visible(visible);
    }
    // TileLayer も同期
    auto *tiles = term_data_[idx].tile_layer;
    if (tiles) {
        tiles->set_visible(visible);
    }
}

void HengbandGame::register_terminal(int idx, Object *term_obj, Object *tile_obj)
{
    if (idx < 0 || idx >= HENGBAND_TERM_COUNT) {
        return;
    }
    auto *term = Object::cast_to<GodotTerminal>(term_obj);
    auto *tiles = Object::cast_to<GodotTileLayer>(tile_obj);
    if (!term) {
        return;
    }

    auto &td = term_data_[idx];
    td.terminal = term;
    td.tile_layer = tiles;
    sub_window_roots_[idx] = term;

    // 既存のグリッドサイズでノードを初期化する
    term->set_grid_size(td.cols, td.rows);
    if (tiles) {
        tiles->set_grid_size(td.cols, td.rows);
    }

    // フォントを適用する
    if (font_.is_valid()) {
        term->set_terminal_font(font_, font_size_);
    }

    // メインターミナルはアクティブにする
    if (idx == 0) {
        term_activate(&td.t);
    }
}

void HengbandGame::unregister_terminal(int idx)
{
    if (idx <= 0 || idx >= HENGBAND_TERM_COUNT) {
        return; // idx=0 (メイン) は解除しない
    }
    auto &td = term_data_[idx];
    td.terminal = nullptr;
    td.tile_layer = nullptr;
    sub_window_roots_[idx] = nullptr;
}

void HengbandGame::set_sub_window_size(int idx, int cols, int rows)
{
    if (idx < 0 || idx >= HENGBAND_TERM_COUNT) {
        return;
    }
    auto &td = term_data_[idx];
    if (cols <= 0 || rows <= 0) {
        return;
    }
    td.cols = cols;
    td.rows = rows;
    if (td.terminal) {
        td.terminal->set_grid_size(cols, rows);
    }
    if (td.tile_layer) {
        td.tile_layer->set_grid_size(cols, rows);
    }
    // term_resize で z-term 側にも反映
    term_type *saved = game_term;
    term_activate(&td.t);
    term_resize(cols, rows);
    if (saved) {
        term_activate(saved);
    }
}

void HengbandGame::save_window_layout(const String &path)
{
    Ref<ConfigFile> cfg;
    cfg.instantiate();

    for (int i = 0; i < HENGBAND_TERM_COUNT; ++i) {
        const std::string section = "window" + std::to_string(i);
        const String sec(section.c_str());

        auto *root = sub_window_roots_[i];
        cfg->set_value(sec, "visible", root ? root->is_visible() : (i == 0));
        cfg->set_value(sec, "cols", term_data_[i].cols);
        cfg->set_value(sec, "rows", term_data_[i].rows);
    }
    cfg->save(path);
}

void HengbandGame::load_window_layout(const String &path)
{
    Ref<ConfigFile> cfg;
    cfg.instantiate();
    if (cfg->load(path) != Error::OK) {
        return;
    }

    for (int i = 0; i < HENGBAND_TERM_COUNT; ++i) {
        const std::string section = "window" + std::to_string(i);
        const String sec(section.c_str());

        const bool visible = cfg->get_value(sec, "visible", i == 0);
        const int cols = cfg->get_value(sec, "cols", 80);
        const int rows = cfg->get_value(sec, "rows", 24);
        const int pos_x = cfg->get_value(sec, "pos_x", 0);
        const int pos_y = cfg->get_value(sec, "pos_y", 0);

        set_sub_window_visible(i, visible);
        if (cols > 0 && rows > 0) {
            set_sub_window_size(i, cols, rows);
        }
        auto *root = sub_window_roots_[i];
        if (root) {
            root->set_position(Vector2(static_cast<float>(pos_x),
                static_cast<float>(pos_y)));
        }
    }
}

void HengbandGame::fit_term_to_viewport(const Vector2i &viewport_size)
{
    auto *term0 = term_data_[0].terminal;
    if (!term0) {
        return;
    }
    const int cell_w = term0->get_cell_width();
    const int cell_h = term0->get_cell_height();
    if (cell_w <= 0 || cell_h <= 0) {
        return;
    }
    const int cols = std::max(10, viewport_size.x / cell_w);
    int rows = std::max(3, viewport_size.y / cell_h);
#ifdef GODOT_RICH_UI
    // 底部ステータス行 (ROW_SPEED=-1, ROW_DUNGEON=-2, ROW_DAY=-3) を
    // Godot StatusPanel に移したため terminal 0 の行数を 3 行削減する。
    // これによりマップ描画領域が拡大する。
    static constexpr int GODOT_RICH_UI_BOTTOM_ROWS = 3;
    rows = std::max(3, rows - GODOT_RICH_UI_BOTTOM_ROWS);
#endif
    set_sub_window_size(0, cols, rows);
}

Array HengbandGame::scan_save_files(const String &lib_path)
{
    const auto lib_fs = godot_string_to_path(lib_path);
    const auto save_dir = lib_fs / "save";

    const auto summaries = hengband_godot::scan_save_files(save_dir);

    Array result;
    result.resize(static_cast<int>(summaries.size()));
    for (int i = 0; i < static_cast<int>(summaries.size()); ++i) {
        const auto &s = summaries[i];

        Dictionary d;
        d["path"] = path_to_godot_string(s.path); // wstring → Godot String
        d["name"] = String::utf8(s.char_name.c_str()); // UTF-8 → Godot String
        d["level"] = s.level;
        d["prace"] = s.prace;
        d["pclass"] = s.pclass;
        d["ppersonality"] = s.ppersonality;
        result[i] = d;
    }
    return result;
}

void HengbandGame::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("start_game", "lib_path", "save_path"), &HengbandGame::start_game);
    ClassDB::bind_method(D_METHOD("set_game_font", "font", "size"),
        &HengbandGame::set_game_font);
    ClassDB::bind_method(
        D_METHOD("load_tileset", "tileset_path", "mask_path", "cell_w", "cell_h", "graf_name"),
        &HengbandGame::load_tileset);
    ClassDB::bind_method(
        D_METHOD("set_sub_window_visible", "idx", "visible"),
        &HengbandGame::set_sub_window_visible);
    ClassDB::bind_method(
        D_METHOD("set_sub_window_size", "idx", "cols", "rows"),
        &HengbandGame::set_sub_window_size);
    ClassDB::bind_method(
        D_METHOD("save_window_layout", "path"),
        &HengbandGame::save_window_layout);
    ClassDB::bind_method(
        D_METHOD("load_window_layout", "path"),
        &HengbandGame::load_window_layout);
    ClassDB::bind_method(
        D_METHOD("fit_term_to_viewport", "viewport_size"),
        &HengbandGame::fit_term_to_viewport);
    ClassDB::bind_method(
        D_METHOD("scan_save_files", "lib_path"),
        &HengbandGame::scan_save_files);
    ClassDB::bind_method(
        D_METHOD("set_tile_rendering_enabled", "enabled"),
        &HengbandGame::set_tile_rendering_enabled);
    ClassDB::bind_method(
        D_METHOD("set_bigtile_enabled", "enabled"),
        &HengbandGame::set_bigtile_enabled);
    ClassDB::bind_method(D_METHOD("is_game_started"), &HengbandGame::is_game_started);
    ClassDB::bind_method(
        D_METHOD("_game_thread_func", "lib_path", "save_path"),
        &HengbandGame::_game_thread_func);
    ClassDB::bind_method(
        D_METHOD("register_terminal", "idx", "term_obj", "tile_obj"),
        &HengbandGame::register_terminal);
    ClassDB::bind_method(
        D_METHOD("unregister_terminal", "idx"),
        &HengbandGame::unregister_terminal);
    ClassDB::bind_method(
        D_METHOD("get_player_status"),
        &HengbandGame::get_player_status);

    // シグナル: ゲームターン更新時にプレイヤーステータスを通知する
    // 引数: status (Dictionary) - get_player_status() と同じキー構成
    ADD_SIGNAL(MethodInfo("player_status_changed",
        PropertyInfo(Variant::DICTIONARY, "status")));
}

// ---------------------------------------------------------------------------
// GDExtension エントリポイント
// ---------------------------------------------------------------------------

void initialize_hengband_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    ClassDB::register_class<GodotTerminal>();
    ClassDB::register_class<GodotTileLayer>();
    ClassDB::register_class<GodotInputHandler>();
    ClassDB::register_class<GodotAudioManager>();
    ClassDB::register_class<HengbandGame>();
}

void uninitialize_hengband_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {

GDExtensionBool GDE_EXPORT hengband_gdextension_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    const GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization)
{
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
    init_obj.register_initializer(initialize_hengband_module);
    init_obj.register_terminator(uninitialize_hengband_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init_obj.init();
}

} // extern "C"
