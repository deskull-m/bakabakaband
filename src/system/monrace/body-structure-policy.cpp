#include "system/monrace/body-structure-policy.h"
#include "util/enum-converter.h"
#include <array>

namespace {

/*!
 * @brief 指定スロット群を持つ SlotMask を生成
 * @note std::bitset::set() は C++23 まで constexpr ではないため、
 *       MSVC 互換性のため通常関数とする。
 */
BodySlotPolicy::SlotMask make_mask(std::initializer_list<inventory_slot_type> slots)
{
    BodySlotPolicy::SlotMask mask;
    for (auto slot : slots) {
        if (slot >= INVEN_MAIN_HAND && slot < INVEN_TOTAL) {
            mask.set(slot - INVEN_MAIN_HAND);
        }
    }
    return mask;
}

/*!
 * @brief 全スロット有効な SlotMask
 */
BodySlotPolicy::SlotMask make_all_mask()
{
    BodySlotPolicy::SlotMask mask;
    mask.set();
    return mask;
}

/*!
 * @brief 体構造別ポリシー定義
 * @details docs/monster-body-structure-equipment-slots.md の表に対応。
 *          HUMANOID は全許可、その他は構造ごとに減算した個別マスクを定義。
 */
const std::array<BodySlotPolicy, enum2i(BodyStructureType::MAX)> body_slot_policies = {
    // HUMANOID: 全 13 スロット有効
    BodySlotPolicy(make_all_mask()),

    // BIPEDAL: 武器/弓/指輪/腕装備なし。首・光源・胴体・頭・脚のみ
    BodySlotPolicy(make_mask({ INVEN_NECK, INVEN_LITE, INVEN_BODY, INVEN_HEAD, INVEN_FEET })),

    // QUADRUPED: 四足獣。首・胴体・頭のみ
    BodySlotPolicy(make_mask({ INVEN_NECK, INVEN_BODY, INVEN_HEAD })),

    // SERPENTINE: ヘビ型。首・胴体のみ
    BodySlotPolicy(make_mask({ INVEN_NECK, INVEN_BODY })),

    // AMORPHOUS: スライム型。リング 2 個のみ装備可
    BodySlotPolicy(make_mask({ INVEN_MAIN_RING, INVEN_SUB_RING })),

    // INCORPOREAL: 装備一切不可
    BodySlotPolicy(BodySlotPolicy::SlotMask{}),
};

}

const BodySlotPolicy &get_body_slot_policy(BodyStructureType type)
{
    const auto idx = enum2i(type);
    if (idx >= body_slot_policies.size()) {
        return body_slot_policies[enum2i(BodyStructureType::HUMANOID)];
    }
    return body_slot_policies[idx];
}
