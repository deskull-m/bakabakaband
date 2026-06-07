#pragma once
/*!
 * @file hengband-gdextension.h
 * @brief Hengband GDExtension メインヘッダ
 *
 * Godot Engine の GDExtension として Hengband を統合するための
 * エントリポイントおよびコアクラス宣言。
 */

#include "godot-audio-manager.h"
#include "godot-input-handler.h"
#include "godot-player-status.h"
#include "godot-term-hooks.h"
#include "godot-terminal.h"
#include "godot-tile-layer.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>

/// ターミナル数（メイン + サブウィンドウ最大8）
static constexpr int HENGBAND_TERM_COUNT = 8;

namespace hengband_godot {

/*!
 * @brief Hengband ゲームのGodotノード
 *
 * Godot シーンに配置し、ゲームロジックのライフサイクルを管理する。
 * ゲームループは別スレッドで実行される（FR-07参照）。
 */
class HengbandGame : public godot::Node {
    GDCLASS(HengbandGame, godot::Node)

public:
    HengbandGame() = default;
    ~HengbandGame() override = default;

    void _ready() override;
    void _process(double delta) override;
    void _notification(int p_what);

    /// ゲームを開始する (Godot Thread を起動して play_game() を呼ぶ)
    /// @param lib_path  lib/ ディレクトリの絶対パス。省略時は実行ファイルの隣を使う。
    /// @param save_path セーブファイルのパス。空文字列 = 新規ゲーム。
    void start_game(const godot::String &lib_path = godot::String(),
        const godot::String &save_path = godot::String());

    /// ゲームスレッド本体 (Callable として Thread に渡す)
    void _game_thread_func(godot::String lib_path, godot::String save_path);

    /// フォントを設定する（GDScript から呼び出す）
    void set_game_font(const godot::Ref<godot::Font> &font, int size);

    /// サブウィンドウ(idx=1〜7)の表示/非表示を切り替える
    void set_sub_window_visible(int idx, bool visible);

    /// サブウィンドウのターミナルサイズを変更する
    void set_sub_window_size(int idx, int cols, int rows);

    /// ウィンドウレイアウト(表示状態・位置)をConfigFileに保存する
    void save_window_layout(const godot::String &path);

    /// ウィンドウレイアウトをConfigFileから復元する
    void load_window_layout(const godot::String &path);

    /// ビューポートのピクセルサイズに合わせてメインターミナルのグリッドを最大化する
    /// @param viewport_size ビューポートのピクセルサイズ (SubViewport のサイズと同値にして呼ぶこと)
    void fit_term_to_viewport(const godot::Vector2i &viewport_size);

    /// 2倍幅タイルモードを有効/無効にする（use_bigtile 相当）
    void set_bigtile_enabled(bool enabled);

    /// タイル描画を有効/無効にする
    /// enabled=false のときは TileLayer0 を非表示にして higher_pict を下げる
    void set_tile_rendering_enabled(bool enabled);

    /// ゲームスレッドが起動済みかどうかを返す（WM_CLOSE_REQUEST 判定用）
    bool is_game_started() const;

    /// lib/save/ 内のセーブファイルをスキャンしてキャラクタ情報の配列を返す
    /// @param lib_path lib/ ディレクトリの絶対パス
    /// @return Array[Dictionary{path,name,level,prace,pclass,ppersonality}]
    godot::Array scan_save_files(const godot::String &lib_path);

    /// GDScript 側で動的に作成したターミナルノードを登録する
    /// @param idx      ターミナルインデックス (0〜HENGBAND_TERM_COUNT-1)
    /// @param term_obj GodotTerminal ノード
    /// @param tile_obj GodotTileLayer ノード (null 可)
    void register_terminal(int idx, godot::Object *term_obj, godot::Object *tile_obj);

    /// 登録済みターミナルの参照を解除する（ペインクローズ時に呼ぶ）
    /// idx=0 は無視する
    void unregister_terminal(int idx);

    /// 現在のプレイヤーステータスを Dictionary で返す
    /// キー: hp, max_hp, sp, max_sp, level, dun_level, gold, exp, max_exp,
    ///       speed, ac, name, race, class, stat_str/int/wis/dex/con/chr,
    ///       stat_top_str/int/wis/dex/con/chr
    godot::Dictionary get_player_status();

protected:
    static void _bind_methods();

private:
    /// 各ターミナルの term_data_godot
    std::array<term_data_godot, HENGBAND_TERM_COUNT> term_data_{};

    /// サブウィンドウ(idx=1〜7)の Node2D ルートノード
    std::array<godot::Node2D *, HENGBAND_TERM_COUNT> sub_window_roots_{};

    /// ゲームスレッド (Phase 7)
    godot::Ref<godot::Thread> game_thread_;
    /// 入力ハンドラへのキャッシュ (停止通知用)
    GodotInputHandler *input_handler_{ nullptr };

    /// 使用するフォント
    godot::Ref<godot::Font> font_;
    int font_size_{ 14 };

    /// player_status_changed シグナル発行用: 最後に確認したスナップショットバージョン
    uint64_t last_status_version_{ 0 };

    /// タイルセットを読み込む（GDScript から呼び出す）
    /// @param graf_name  ANGBAND_GRAF に設定する値 ("old" / "new" / "ne2")
    bool load_tileset(const godot::String &tileset_path,
        const godot::String &mask_path,
        int cell_w, int cell_h,
        const godot::String &graf_name = godot::String());

    /// ターミナルを初期化して term_type フックを設定する
    void setup_terminal(int idx, GodotTerminal *term,
        GodotTileLayer *tiles, int cols, int rows);
};

} // namespace hengband_godot
