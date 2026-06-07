/*!
 * @file godot-terminal.cpp
 * @brief Godot ターミナル描画ノード実装
 */

#include "godot-terminal.h"
#include "term-color-map.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

using namespace godot;
using namespace hengband_godot;

// --- ユーティリティ ---

/// Unicode コードポイントが全角文字かどうかを判定する
/// (East Asian Width = Wide または Fullwidth に相当する範囲)
static bool is_fullwidth(char32_t cp)
{
    return (cp >= 0x1100 && cp <= 0x115F) // Hangul Jamo
           || (cp >= 0x2E80 && cp <= 0x303E) // CJK Radicals / Kangxi
           || (cp >= 0x3041 && cp <= 0x33BF) // Hiragana/Katakana/CJK Symbols
           || (cp >= 0x3400 && cp <= 0x4DBF) // CJK Extension A
           || (cp >= 0x4E00 && cp <= 0x9FFF) // CJK Unified Ideographs
           || (cp >= 0xAC00 && cp <= 0xD7AF) // Hangul Syllables
           || (cp >= 0xF900 && cp <= 0xFAFF) // CJK Compatibility Ideographs
           || (cp >= 0xFE10 && cp <= 0xFE6F) // CJK Compatibility Forms
           || (cp >= 0xFF01 && cp <= 0xFF60) // Fullwidth ASCII
           || (cp >= 0xFFE0 && cp <= 0xFFE6); // Fullwidth Signs
}

/// UTF-8 文字列を UTF-32 コードポイント列に変換する (バイト数上限付き)
static std::vector<char32_t> utf8_to_codepoints(const char *str, int max_chars)
{
    std::vector<char32_t> result;
    result.reserve(max_chars);
    const unsigned char *s = reinterpret_cast<const unsigned char *>(str);
    while (*s && static_cast<int>(result.size()) < max_chars) {
        char32_t cp = 0;
        if (*s < 0x80) {
            cp = *s++;
        } else if ((*s & 0xE0) == 0xC0) {
            cp = (*s++ & 0x1F) << 6;
            cp |= (*s++ & 0x3F);
        } else if ((*s & 0xF0) == 0xE0) {
            cp = (*s++ & 0x0F) << 12;
            cp |= (*s++ & 0x3F) << 6;
            cp |= (*s++ & 0x3F);
        } else if ((*s & 0xF8) == 0xF0) {
            cp = (*s++ & 0x07) << 18;
            cp |= (*s++ & 0x3F) << 12;
            cp |= (*s++ & 0x3F) << 6;
            cp |= (*s++ & 0x3F);
        } else {
            ++s; // 不正なシーケンスをスキップ
            cp = U'?';
        }
        result.push_back(cp);
    }
    return result;
}

// wall.bmp から抽出した 8×8 モノクロパターン（上から下の順、MSB が左端ピクセル）
// Windows 版では ASCII 127 ('DEL' / 壁文字) の描画にこのパターンブラシが使われる。
static const uint8_t kWallPattern[8] = {
    0xAA, // 10101010
    0x41, // 01000001
    0x96, // 10010110
    0x20, // 00100000
    0xAA, // 10101010
    0x05, // 00000101
    0xAA, // 10101010
    0x44, // 01000100
};

// --- GodotTerminal 実装 ---

GodotTerminal::GodotTerminal()
{
    resize_grid();
}

void GodotTerminal::_ready()
{
    resize_grid();
    create_wall_texture();
}

void GodotTerminal::_draw()
{
    if (!font_.is_valid()) {
        return;
    }

    // グリッドのスナップショットを取得 (ゲームスレッドとの競合防止)
    std::vector<CellData> snapshot;
    int snap_cols, snap_rows;
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        snapshot = grid_;
        snap_cols = cols_;
        snap_rows = rows_;
    }

    // 全セルを描画
    // NOTE: 全面黒塗りは行わない。タイルモード時は TileLayer0 (z_index=0) が
    // 背面に描画され、空白セル(U' ')は透過して見えるようにする。
    // 非タイルモード時は SubViewport 背景色(黒)が代わりに見える。
    for (int y = 0; y < snap_rows; ++y) {
        for (int x = 0; x < snap_cols; ++x) {
            draw_cell(x, y, snapshot[y * snap_cols + x]);
        }
    }

    // カーソル描画
    if (cursor_visible_) {
        draw_cursor_rect(cursor_x_, cursor_y_);
    }
}

void GodotTerminal::create_wall_texture()
{
    using namespace godot;
    Ref<Image> img = Image::create(8, 8, false, Image::FORMAT_RGBA8);
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            // MSB が左端ピクセル (col=0)
            const bool is_fg = (kWallPattern[row] >> (7 - col)) & 1;
            // 前景ビット → 白不透明 (描画時に fg カラーで変調)
            // 背景ビット → 完全透明 (黒背景が透けて見える)
            img->set_pixel(col, row, is_fg ? Color(1, 1, 1, 1) : Color(0, 0, 0, 0));
        }
    }
    wall_texture_ = ImageTexture::create_from_image(img);
}

void GodotTerminal::draw_cell(int x, int y, const CellData &cell)
{
    if (cell.ch == U' ' || cell.ch == 0) {
        return;
    }

    const Color fg = term_color_to_godot(cell.color);
    // 全角文字は2セル幅、半角は1セル幅
    const int char_cell_w = is_fullwidth(cell.ch) ? (cell_w_ * 2) : cell_w_;

    const float px = static_cast<float>(x * cell_w_);
    const float py = static_cast<float>(y * cell_h_);
    const Rect2 cell_rect(px, py,
        static_cast<float>(char_cell_w),
        static_cast<float>(cell_h_));

    // セル背景を黒で塗る
    draw_rect(cell_rect, Color(0, 0, 0));

    // ASCII 127 (DEL) = Windows 版の wall.bmp パターンブラシ相当
    // 壁・暗闇タイルの描画に使用される
    if (cell.ch == 0x7F) {
        if (wall_texture_.is_valid()) {
            draw_texture_rect(wall_texture_, cell_rect, true, fg);
        }
        return;
    }

    // 文字描画 (baseline = py + ascent)
    const float baseline = font_.is_valid()
                               ? font_->get_ascent(font_size_)
                               : static_cast<float>(font_size_);
    const Vector2 pos(px, py + baseline);
    font_->draw_char(get_canvas_item(), pos, static_cast<int64_t>(cell.ch), font_size_, fg);
}

void GodotTerminal::draw_cursor_rect(int x, int y)
{
    // 黄色の矩形カーソルを描画
    const Rect2 rc(
        static_cast<float>(x * cell_w_),
        static_cast<float>(y * cell_h_),
        static_cast<float>(cell_w_),
        static_cast<float>(cell_h_));
    draw_rect(rc, Color(1.0f, 1.0f, 0.0f), false, 1.5f);
}

void GodotTerminal::set_grid_size(int cols, int rows)
{
    if (cols == cols_ && rows == rows_) {
        return;
    }
    cols_ = cols;
    rows_ = rows;
    resize_grid();
    call_deferred("queue_redraw");
}

void GodotTerminal::set_cell_size(int w, int h)
{
    cell_w_ = w;
    cell_h_ = h;
    call_deferred("queue_redraw");
}

void GodotTerminal::set_terminal_font(const Ref<Font> &font, int font_size)
{
    font_ = font;
    font_size_ = font_size;

    if (font_.is_valid()) {
        // 半角ASCII文字の幅を cell_w_ に設定
        const float char_w = font_->get_char_size('M', font_size_).x;
        cell_w_ = std::max(1, static_cast<int>(std::round(char_w)));
        // 行の高さ = ascent + descent
        const float ascent = font_->get_ascent(font_size_);
        const float descent = font_->get_descent(font_size_);
        cell_h_ = std::max(1, static_cast<int>(std::round(ascent + descent)));
    }

    call_deferred("queue_redraw");
}

void GodotTerminal::draw_text(int x, int y, int n, uint8_t color, const char *str)
{
    if (y < 0 || y >= rows_ || x < 0) {
        return;
    }

    // n はセル数（半角=1、全角=2）。UTF-8文字列を全て変換してからセル数で打ち切る
    const auto codepoints = utf8_to_codepoints(str, 512);
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        int col = x;
        const int col_end = x + n; // セル上限
        for (const char32_t cp : codepoints) {
            if (col >= col_end || col >= cols_) {
                break;
            }
            auto &cell = grid_[cell_idx(col, y)];
            cell.ch = cp;
            cell.color = color;
            if (is_fullwidth(cp)) {
                // 全角文字は2セル占有: 次のセルをプレースホルダにする
                ++col;
                if (col < col_end && col < cols_) {
                    auto &cell2 = grid_[cell_idx(col, y)];
                    cell2.ch = 0; // 描画スキップ用プレースホルダ
                    cell2.color = color;
                }
            }
            ++col;
        }
    }
    call_deferred("queue_redraw");
}

void GodotTerminal::wipe_cells(int x, int y, int n)
{
    if (y < 0 || y >= rows_) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        for (int i = 0; i < n && (x + i) < cols_; ++i) {
            auto &cell = grid_[cell_idx(x + i, y)];
            cell.ch = U' ';
            cell.color = 0; // TERM_DARK
        }
    }
    call_deferred("queue_redraw");
}

void GodotTerminal::set_cursor(int x, int y)
{
    cursor_x_ = x;
    cursor_y_ = y;
    cursor_visible_ = true;
    call_deferred("queue_redraw");
}

void GodotTerminal::hide_cursor()
{
    cursor_visible_ = false;
    call_deferred("queue_redraw");
}

void GodotTerminal::clear_all()
{
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        for (auto &cell : grid_) {
            cell.ch = U' ';
            cell.color = 0;
        }
    }
    cursor_visible_ = false;
    call_deferred("queue_redraw");
}

void GodotTerminal::resize_grid()
{
    std::lock_guard<std::mutex> lock(grid_mutex_);
    grid_.assign(static_cast<size_t>(cols_ * rows_), CellData{});
}

void GodotTerminal::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_grid_size", "cols", "rows"),
        &GodotTerminal::set_grid_size);
    ClassDB::bind_method(D_METHOD("set_cell_size", "width", "height"),
        &GodotTerminal::set_cell_size);
    ClassDB::bind_method(D_METHOD("set_terminal_font", "font", "size"),
        &GodotTerminal::set_terminal_font);
    // draw_text / wipe_cells / set_cursor は C++ 側 (term hooks) からのみ呼ばれるため
    // GDScript への bind は不要 (std::string / const char* は GDExtension 型システム外)
    ClassDB::bind_method(D_METHOD("hide_cursor"),
        &GodotTerminal::hide_cursor);
    ClassDB::bind_method(D_METHOD("clear_all"),
        &GodotTerminal::clear_all);
    ClassDB::bind_method(D_METHOD("get_cols"), &GodotTerminal::get_cols);
    ClassDB::bind_method(D_METHOD("get_rows"), &GodotTerminal::get_rows);
    ClassDB::bind_method(D_METHOD("get_cell_width"), &GodotTerminal::get_cell_width);
    ClassDB::bind_method(D_METHOD("get_cell_height"), &GodotTerminal::get_cell_height);
}
