/*!
 * @brief 体構造別の拡張装備スロット種別
 * @details Phase 2 (extended slots) で追加される、既存 INVEN_*
 *          に収まらないモンスター固有の装備部位を表す。
 *          MonsterProfile::extended_inventory に対応する形で
 *          BodySlotPolicy::extended_slots に並び順で格納される。
 *          詳細は docs/monster-body-structure-equipment-slots.md 参照。
 */
#pragma once

#include "locale/localized-string.h"
#include <cstdint>
#include <string_view>

enum class ExtendedSlotType : uint8_t {
    TAIL_RING = 0, //!< 尾の指輪 (ドラゴン・ヘビ型)
    SECOND_NECK = 1, //!< 第二の首のアミュレット (ハイドラ系)
    THIRD_HEAD = 2, //!< 第三の頭の兜 (多頭獣)
    WING_LEFT = 3, //!< 左翼の装飾
    WING_RIGHT = 4, //!< 右翼の装飾
    MAX,
};

/*!
 * @brief 拡張スロットの表示名 (ローカライズ済み) を取得する
 * @param type 拡張スロット種別
 * @return 表示名 (string_view)
 */
std::string_view get_extended_slot_name(ExtendedSlotType type);
