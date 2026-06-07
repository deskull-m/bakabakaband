#pragma once
/*!
 * @file term-color-map.h
 * @brief TERM_COLOR → Godot Color マッピング
 *
 * angband_color_table (byte[256][4]) の値を Godot の Color に変換する。
 * [0]=未使用(darkness), [1]=R, [2]=G, [3]=B (各 0-255)
 */

#include "term/term-color-types.h"
#include <cstdint>
#include <godot_cpp/variant/color.hpp>

namespace hengband_godot {

/// angband_color_table から Godot Color を生成する
/// @param color_idx  TERM_COLOR 値 (0-255)
/// @return Godot Color (linear RGB)
godot::Color term_color_to_godot(uint8_t color_idx);

/// 16色デフォルトカラーテーブル（angband_color_table が未初期化な場合のフォールバック）
extern const godot::Color DEFAULT_TERM_COLORS[16];

} // namespace hengband_godot
