/*!
 * @file term-color-map.cpp
 * @brief TERM_COLOR → Godot Color マッピング実装
 */

#include "term-color-map.h"
#include "term/gameterm.h" // angband_color_table

namespace hengband_godot {

// フォールバック用デフォルト16色
// term-color-types.h のコメントにある (R,G,B) を 0-255 スケールに変換
// 元のコメントは "fourths of maximal (=255)" なので ×64 相当
const godot::Color DEFAULT_TERM_COLORS[16] = {
    godot::Color(0.00f, 0.00f, 0.00f), // TERM_DARK    黒
    godot::Color(1.00f, 1.00f, 1.00f), // TERM_WHITE   白
    godot::Color(0.50f, 0.50f, 0.50f), // TERM_SLATE   灰
    godot::Color(1.00f, 0.50f, 0.00f), // TERM_ORANGE  橙
    godot::Color(0.75f, 0.00f, 0.00f), // TERM_RED     赤
    godot::Color(0.00f, 0.50f, 0.25f), // TERM_GREEN   緑
    godot::Color(0.00f, 0.50f, 1.00f), // TERM_BLUE    青
    godot::Color(0.50f, 0.25f, 0.00f), // TERM_UMBER   琥珀
    godot::Color(0.25f, 0.25f, 0.25f), // TERM_L_DARK  暗灰
    godot::Color(0.75f, 0.75f, 0.75f), // TERM_L_WHITE 明灰
    godot::Color(1.00f, 0.00f, 1.00f), // TERM_VIOLET  紫
    godot::Color(1.00f, 1.00f, 0.00f), // TERM_YELLOW  黄
    godot::Color(1.00f, 0.00f, 0.00f), // TERM_L_RED   明赤
    godot::Color(0.00f, 1.00f, 0.00f), // TERM_L_GREEN 明緑
    godot::Color(0.00f, 1.00f, 1.00f), // TERM_L_BLUE  明青
    godot::Color(0.75f, 0.50f, 0.25f), // TERM_L_UMBER 明琥珀
};

godot::Color term_color_to_godot(uint8_t color_idx)
{
    // angband_color_table は [color_idx][4]: [0]=K, [1]=R, [2]=G, [3]=B (0-255)
    // angband_color_table は グローバル変数として gameterm.cpp で定義される
    const auto r = static_cast<float>(angband_color_table[color_idx][1]) / 255.0f;
    const auto g = static_cast<float>(angband_color_table[color_idx][2]) / 255.0f;
    const auto b = static_cast<float>(angband_color_table[color_idx][3]) / 255.0f;
    return godot::Color(r, g, b);
}

} // namespace hengband_godot
