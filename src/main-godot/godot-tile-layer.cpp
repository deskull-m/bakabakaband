/*!
 * @file godot-tile-layer.cpp
 * @brief Godot タイル描画レイヤー実装
 */

#include "godot-tile-layer.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

using namespace godot;
using namespace hengband_godot;

// ---------------------------------------------------------------------------
// マスク合成
// ---------------------------------------------------------------------------

Ref<Image> GodotTileLayer::apply_mask(Ref<Image> src, Ref<Image> mask)
{
    // src を RGBA8 に変換
    if (src->get_format() != Image::FORMAT_RGBA8) {
        src->convert(Image::FORMAT_RGBA8);
    }
    if (mask->get_format() != Image::FORMAT_RGBA8) {
        mask->convert(Image::FORMAT_RGBA8);
    }

    const int w = src->get_width();
    const int h = src->get_height();
    const int mw = mask->get_width();
    const int mh = mask->get_height();

    // ピクセル単位でアルファ合成:
    //   マスクが白(bright) → 透明, 黒 → 不透明
    //   alpha = 255 - mask_gray
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (x >= mw || y >= mh) {
                break;
            }
            const Color m = mask->get_pixel(x, y);
            // グレースケール輝度 (0.0〜1.0)
            const float gray = m.get_luminance();
            Color p = src->get_pixel(x, y);
            p.a = 1.0f - gray; // 白=透明, 黒=不透明
            src->set_pixel(x, y, p);
        }
    }
    return src;
}

// ---------------------------------------------------------------------------
// GodotTileLayer 実装
// ---------------------------------------------------------------------------

void GodotTileLayer::_ready()
{
    // ピクセルアートタイルにバイリニア補間がかかると色が薄く・グレー混じりになるため
    // ニアレストネイバーフィルタを使用する
    set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
    resize_grid();
}

void GodotTileLayer::_draw()
{
    if (!texture_.is_valid()) {
        return;
    }

    // グリッドのスナップショットを取得
    std::vector<TileCell> snapshot;
    int snap_cols, snap_rows;
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        snapshot = grid_;
        snap_cols = cols_;
        snap_rows = rows_;
    }

    // bigtile モード時は描画先幅を2倍にする (Windows版 tw2 *= 2 相当)
    const int draw_w = bigtile_ ? tile_w_ * 2 : tile_w_;

    for (int y = 0; y < snap_rows; ++y) {
        for (int x = 0; x < snap_cols; ++x) {
            const auto &cell = snapshot[y * snap_cols + x];
            if (!cell.valid) {
                continue;
            }
            // AF_BIGTILE2 (0xf0) は右半分ダミーセル — 描画スキップ
            if ((cell.fg_row & 0xF0) == 0xF0) {
                continue;
            }
            draw_one_tile(x * tile_w_, y * tile_h_, cell.bg_row, cell.bg_col, draw_w);
            if (cell.fg_row != cell.bg_row || cell.fg_col != cell.bg_col) {
                draw_one_tile(x * tile_w_, y * tile_h_, cell.fg_row, cell.fg_col, draw_w);
            }
        }
    }
}

void GodotTileLayer::draw_one_tile(int dst_x, int dst_y,
    uint8_t row, uint8_t col, int dst_w)
{
    const Rect2 dst(
        static_cast<float>(dst_x),
        static_cast<float>(dst_y),
        static_cast<float>(dst_w),
        static_cast<float>(tile_h_));

    const Rect2 src(
        static_cast<float>(col * cell_w_),
        static_cast<float>(row * cell_h_),
        static_cast<float>(cell_w_),
        static_cast<float>(cell_h_));

    draw_texture_rect_region(texture_, dst, src);
}

bool GodotTileLayer::load_tileset(const std::string &tileset_path,
    const std::string &mask_path,
    int cell_w, int cell_h,
    int tile_w, int tile_h)
{
    cell_w_ = cell_w;
    cell_h_ = cell_h;
    tile_w_ = tile_w;
    tile_h_ = tile_h;

    // タイルセット画像を読み込む
    Ref<Image> img = Image::load_from_file(tileset_path.c_str());
    if (!img.is_valid()) {
        return false;
    }

    // マスクを適用する
    if (!mask_path.empty()) {
        Ref<Image> mask_img = Image::load_from_file(mask_path.c_str());
        if (mask_img.is_valid()) {
            img = apply_mask(img, mask_img);
        }
    } else {
        // マスクなし: RGB → RGBA (α=1.0 で不透明)
        if (img->get_format() != Image::FORMAT_RGBA8) {
            img->convert(Image::FORMAT_RGBA8);
        }
    }

    texture_ = ImageTexture::create_from_image(img);
    return texture_.is_valid();
}

void GodotTileLayer::set_tile_size(int tw, int th)
{
    tile_w_ = tw;
    tile_h_ = th;
    call_deferred("queue_redraw");
}

void GodotTileLayer::set_bigtile(bool enabled)
{
    bigtile_ = enabled;
    call_deferred("queue_redraw");
}

void GodotTileLayer::set_grid_size(int cols, int rows)
{
    if (cols == cols_ && rows == rows_) {
        return;
    }
    cols_ = cols;
    rows_ = rows;
    resize_grid();
    call_deferred("queue_redraw");
}

void GodotTileLayer::draw_tiles(int x, int y, int n,
    const uint8_t *ap, const char *cp,
    const uint8_t *tap, const char *tcp)
{
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        for (int i = 0; i < n; ++i) {
            const int col = x + i;
            if (col >= cols_ || y >= rows_) {
                break;
            }
            auto &cell = grid_[cell_idx(col, y)];
            cell.fg_row = ap[i] & 0x7F;
            cell.fg_col = static_cast<uint8_t>(cp[i]) & 0x7F;
            cell.bg_row = tap[i] & 0x7F;
            cell.bg_col = static_cast<uint8_t>(tcp[i]) & 0x7F;
            cell.valid = true;
        }
    }
    call_deferred("queue_redraw");
}

void GodotTileLayer::wipe_tiles(int x, int y, int n)
{
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        for (int i = 0; i < n && (x + i) < cols_; ++i) {
            grid_[cell_idx(x + i, y)].valid = false;
        }
    }
    call_deferred("queue_redraw");
}

void GodotTileLayer::clear_all()
{
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        for (auto &cell : grid_) {
            cell.valid = false;
        }
    }
    call_deferred("queue_redraw");
}

void GodotTileLayer::resize_grid()
{
    std::lock_guard<std::mutex> lock(grid_mutex_);
    grid_.assign(static_cast<size_t>(cols_ * rows_), TileCell{});
}

void GodotTileLayer::_bind_methods()
{
    // load_tileset は C++ 側 (HengbandGame) からのみ呼ばれるため bind 不要
    // (std::string は GDExtension 型システム外)
    ClassDB::bind_method(D_METHOD("set_grid_size", "cols", "rows"),
        &GodotTileLayer::set_grid_size);
    ClassDB::bind_method(D_METHOD("clear_all"),
        &GodotTileLayer::clear_all);
    ClassDB::bind_method(D_METHOD("is_tileset_loaded"),
        &GodotTileLayer::is_tileset_loaded);
    ClassDB::bind_method(D_METHOD("get_cell_width"),
        &GodotTileLayer::get_cell_width);
    ClassDB::bind_method(D_METHOD("get_cell_height"),
        &GodotTileLayer::get_cell_height);
}
