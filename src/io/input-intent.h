#pragma once

/*!
 * @file input-intent.h
 * @brief 座標で指定する入力の意図 (3D 表示のクリック・タッチなど)
 * @details
 * キーで送ると「カーソルを何マス動かすか」に分解しなければならない入力を、座標のまま渡す。
 * 外部 (Godot のメインスレッドなど) が push_point_intent() で積み、ゲームが場所を尋ねる
 * ところ (point_target() / target_set()) で、用途が一致するものだけを take_point_intent() で取り出す。
 *
 * - 取り出されない意図は INTENT_LIFETIME で失効する (古いクリックが無関係な場所の指定に使われないように)
 * - 用途が違う意図は取り出さない
 * - キー入力・ボットの操作・リプレイには影響しない (意図を積まなければ何も変わらない)
 */

#include "util/point-2d.h"
#include <tl/optional.hpp>

/*!
 * @brief 座標を使う場面
 */
enum class PointIntentPurpose {
    POINT, //!< 場所の指定 (トラベルの行き先など。point_target())
    TARGET, //!< ターゲットの選択 (target_set()。見るコマンドは除く)
};

/// 座標の意図を積む (どのスレッドからでも呼べる。既に積まれていれば置き換える)
void push_point_intent(PointIntentPurpose purpose, const Pos2D &pos);

/// 用途が一致する失効していない意図を取り出す (ゲームスレッドから呼ぶ)
tl::optional<Pos2D> take_point_intent(PointIntentPurpose purpose);

/// 積まれている意図を捨てる
void clear_point_intent();

/// 失効していない意図が積まれたまま (まだ取り出されていない) か
bool has_point_intent();
