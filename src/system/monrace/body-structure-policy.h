/*!
 * @brief 体構造ごとの装備スロット可否ポリシー
 * @details 各 BodyStructureType に対して、どの inventory_slot_type
 *          (INVEN_MAIN_HAND..INVEN_TOTAL の範囲) を許可するかを
 *          ビットマスクで保持する。
 *          詳細は docs/monster-body-structure-equipment-slots.md 参照。
 */
#pragma once

#include "inventory/inventory-slot-types.h"
#include "system/monrace/body-structure-types.h"
#include <bitset>

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

    explicit BodySlotPolicy(SlotMask mask = SlotMask{})
        : allowed_slots(mask)
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

private:
    SlotMask allowed_slots;
};

/*!
 * @brief 体構造から装備スロット可否ポリシーを取得する
 * @param type 体構造
 * @return ポリシー (static 寿命)
 */
const BodySlotPolicy &get_body_slot_policy(BodyStructureType type);
