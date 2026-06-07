#pragma once
/*!
 * @file godot-tile-layer.h
 * @brief Godot タイル描画レイヤー (pict_hook 相当)
 *
 * Hengband の pict_hook を Godot の Node2D ベース描画 API で実装する。
 *
 * 設計:
 *  - 内部にタイルセルバッファ (cols × rows) を保持する
 *  - draw_tiles() でバッファを更新して queue_redraw() を呼ぶ
 *  - _draw() で draw_texture_rect_region() を使いバッチ描画する
 *  - タイルセット画像はマスクをアルファチャンネルに合成して ImageTexture に変換する
 *
 * タイル座標エンコーディング (Windows 版と同じ規約):
 *  - attr (= row) = a & 0x7F … タイルセット上の行
 *  - char (= col) = c & 0x7F … タイルセット上の列
 *
 * pict_hook の引数:
 *  pict_hook(x, y, n, ap[], cp[], tap[], tcp[])
 *  - (ap[i], cp[i])  … 前景タイル (row, col)
 *  - (tap[i], tcp[i]) … 背景タイル (terrain)
 */

#include <cstdint>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <mutex>
#include <string>
#include <vector>

namespace hengband_godot {

/// 1タイルセルのデータ
struct TileCell {
    uint8_t fg_row{ 0 }; ///< 前景タイルの行 (ap & 0x7F)
    uint8_t fg_col{ 0 }; ///< 前景タイルの列 (cp & 0x7F)
    uint8_t bg_row{ 0 }; ///< 背景タイルの行 (tap & 0x7F)
    uint8_t bg_col{ 0 }; ///< 背景タイルの列 (tcp & 0x7F)
    bool valid{ false }; ///< true = タイル描画あり
};

/*!
 * @brief Hengband タイル描画レイヤー
 *
 * GodotTerminal と重ねて配置する。
 * テキストモード時は非表示にしてテキストレイヤーを見せる。
 */
class GodotTileLayer : public godot::Node2D {
    GDCLASS(GodotTileLayer, godot::Node2D)

public:
    GodotTileLayer() = default;
    ~GodotTileLayer() override = default;

    void _ready() override;
    void _draw() override;

    // --- タイルセット設定 ---

    /*!
     * @brief タイルセット画像を読み込む
     * @param tileset_path  タイルセット画像のファイルパス（BMP/PNG）
     * @param mask_path     マスク画像のパス。空文字列なら透過なし。
     * @param cell_w        ソース画像上の1タイル幅
     * @param cell_h        ソース画像上の1タイル高さ
     * @param tile_w        描画先の1タイル幅
     * @param tile_h        描画先の1タイル高さ
     * @return true = 成功
     */
    bool load_tileset(const std::string &tileset_path,
        const std::string &mask_path,
        int cell_w, int cell_h,
        int tile_w, int tile_h);

    /// グリッドサイズを設定する
    void set_grid_size(int cols, int rows);

    // --- term_type pict_hook から呼ばれる ---

    /*!
     * @brief タイルを描画する (pict_hook 相当)
     * @param x     描画先列
     * @param y     描画先行
     * @param n     描画タイル数
     * @param ap    前景タイル row 配列
     * @param cp    前景タイル col 配列
     * @param tap   背景タイル row 配列
     * @param tcp   背景タイル col 配列
     */
    void draw_tiles(int x, int y, int n,
        const uint8_t *ap, const char *cp,
        const uint8_t *tap, const char *tcp);

    /// 指定領域をクリアする
    void wipe_tiles(int x, int y, int n);

    /// 全クリア
    void clear_all();

    /// タイルの描画先サイズを更新する（フォント変更時に呼ぶ）
    void set_tile_size(int tw, int th);

    /// 2倍幅タイルモードを設定する（use_bigtile に対応）
    void set_bigtile(bool enabled);

    bool is_tileset_loaded() const
    {
        return texture_.is_valid();
    }
    int get_cell_width() const
    {
        return cell_w_;
    }
    int get_cell_height() const
    {
        return cell_h_;
    }

protected:
    static void _bind_methods();

private:
    int cols_{ 80 };
    int rows_{ 24 };
    int cell_w_{ 32 }; ///< ソース上のタイル幅
    int cell_h_{ 32 }; ///< ソース上のタイル高さ
    int tile_w_{ 32 }; ///< 描画先タイル幅（1倍時）
    int tile_h_{ 32 }; ///< 描画先タイル高さ
    bool bigtile_{ false }; ///< 2倍幅モード (use_bigtile)

    godot::Ref<godot::ImageTexture> texture_; ///< マスク合成済みのタイルセットテクスチャ

    std::vector<TileCell> grid_; ///< タイルバッファ
    mutable std::mutex grid_mutex_; ///< スレッド安全グリッドアクセス用

    int cell_idx(int x, int y) const
    {
        return y * cols_ + x;
    }
    void resize_grid();

    /// タイルセット画像にマスクをアルファとして合成する
    static godot::Ref<godot::Image> apply_mask(
        godot::Ref<godot::Image> src,
        godot::Ref<godot::Image> mask);

    /// 単一タイルを draw_texture_rect_region で描画する
    void draw_one_tile(int dst_x, int dst_y,
        uint8_t row, uint8_t col, int dst_w);
};

} // namespace hengband_godot
