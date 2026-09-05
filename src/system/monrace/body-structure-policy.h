/*!
 * @brief 体構造ごとの装備スロット可否ポリシー
 * @details 各 BodyStructureType に対して、どの inventory_slot_type
 *          (INVEN_MAIN_HAND..INVEN_TOTAL の範囲) を許可するかを
 *          ビットマスクで保持する。
 *          詳細は docs/monster-body-structure-equipment-slots.md 参照。
 */
#pragma once

#include "inventory/inventory-slot-types.h"
#include "system/angband.h"
#include "system/monrace/body-structure-types.h"
#include "system/monrace/extended-slot.h"
#include <bitset>
#include <string_view>
#include <vector>

/*!
 * @brief 体構造ごとの装備スロット可否ポリシー
 * @note std::bitset のメンバ関数は C++23 まで constexpr ではないため、
 *       本クラスでは constexpr を使用しない (MSVC 互換性のため)。
 */
class BodySlotPolicy {
public:
    //! 装備スロット数 (INVEN_MAIN_HAND..INVEN_TOTAL-1)
    static constexpr int SLOT_COUNT = INVEN_TOTAL - INVEN_MAIN_HAND;
    using SlotMask = std::bitset<SLOT_COUNT>;

    BodySlotPolicy(SlotMask mask = SlotMask{}, std::vector<ExtendedSlotType> extended = {})
        : allowed_slots(mask)
        , extended_slots(std::move(extended))
    {
    }

    //! 指定スロット (INVEN_MAIN_HAND..INVEN_TOTAL-1) が許可されているか
    bool is_allowed(int slot) const
    {
        if (slot < INVEN_MAIN_HAND || slot >= INVEN_TOTAL) {
            return false;
        }
        return this->allowed_slots.test(slot - INVEN_MAIN_HAND);
    }

    //! 拡張スロット一覧 (Phase 2)
    const std::vector<ExtendedSlotType> &get_extended_slots() const
    {
        return this->extended_slots;
    }

private:
    SlotMask allowed_slots;
    std::vector<ExtendedSlotType> extended_slots; //!< 拡張装備スロット (Phase 2)
};

/*!
 * @brief 体構造から装備スロット可否ポリシーを取得する
 * @param type 体構造
 * @return ポリシー (static 寿命)
 */
const BodySlotPolicy &get_body_slot_policy(BodyStructureType type);

/*!
 * @brief 体構造の表示名を取得する
 * @param type 体構造
 * @return ローカライズされた表示名 (静的文字列)
 * @details c コマンドのステータス表示と r コマンドの思い出表示で共用し、
 *          呼称が食い違わないようにする。
 */
std::string_view body_structure_name(BodyStructureType type);

/*!
 * @brief 体構造の表示色を取得する
 * @param type 体構造
 * @return 表示色
 */
TERM_COLOR body_structure_color(BodyStructureType type);
