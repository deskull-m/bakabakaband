#pragma once

/*
 * @file player-ability-types.h
 * @brief アビリティスコア定義 (順番はAD&D初版準拠)
 */

#include <cstddef>

enum player_ability_type : int {
    A_STR = 0,
    A_INT = 1,
    A_WIS = 2,
    A_DEX = 3,
    A_CON = 4,
    A_CHR = 5,
    A_MAX = 6,
    A_RANDOM = 7,
};

/*!
 * @brief 能力値の内部表現上の下限・上限 (10 内部単位 = 表示 1.0)
 * @details 表示 3.0〜200.0 = 内部 30〜2000。従来は 400 (40.0) が上限だったが、
 *          クリーチャープレイアブル実装に向けて拡張。
 */
constexpr int STAT_MIN_VALUE = 30; //!< 表示 3.0
constexpr int STAT_MAX_VALUE = 2000; //!< 表示 200.0

/*!
 * @brief 能力値修正テーブル (adj_*) のエントリ数
 * @details index 0 が 表示 3.0、以降は内部 10 単位ごと。
 *          内部 180 (18.0) までが index 0〜15、以降 2000 (200.0) までが
 *          index 16〜197。よって 198 エントリ必要。
 */
constexpr std::size_t STAT_TABLE_SIZE = 198;
