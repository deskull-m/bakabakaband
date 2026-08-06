/*!
 * @brief モンスターの体構造列挙
 * @details 装備可能スロットを体構造単位で決定するための分類。
 *          HUMANOID をデフォルトとし、全装備スロット可能。
 *          詳細は docs/monster-body-structure-equipment-slots.md 参照。
 */
#pragma once

#include <cstdint>

enum class BodyStructureType : uint8_t {
    HUMANOID = 0, //!< 二足歩行・両手・頭・胴体: 全装備スロット有効 (デフォルト)
    BIPEDAL = 1, //!< 鳥型・恐竜型: 翼/前肢で武器装備不可、首/光源/胴体/頭/脚のみ
    QUADRUPED = 2, //!< 四足獣: 首/胴体/頭のみ
    SERPENTINE = 3, //!< ヘビ・うなぎ型: 首と胴体のみ
    AMORPHOUS = 4, //!< スライム・ジェル: リングのみ (擬足にはめる)
    INCORPOREAL = 5, //!< 幽霊・ベクター: 装備一切不可
    DRACONIC = 6, //!< ドラゴン: HUMANOID 装備可能 + 拡張スロット (尾の指輪)
    MAX,
};
