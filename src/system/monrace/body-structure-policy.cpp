#include "system/monrace/body-structure-policy.h"
#include "locale/language-switcher.h"
#include "term/term-color-types.h"
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
 * @brief 指定スロット群だけを除いた全スロット有効な SlotMask
 */
BodySlotPolicy::SlotMask make_all_mask_except(std::initializer_list<inventory_slot_type> slots)
{
    auto mask = make_all_mask();
    for (auto slot : slots) {
        if (slot >= INVEN_MAIN_HAND && slot < INVEN_TOTAL) {
            mask.reset(slot - INVEN_MAIN_HAND);
        }
    }
    return mask;
}

/*!
 * @brief 体構造別ポリシー定義
 * @details docs/monster-body-structure-equipment-slots.md の表に対応。
 *          HUMANOID は全許可、その他は構造ごとに減算した個別マスクを定義。
 */
const std::array<BodySlotPolicy, enum2i(BodyStructureType::MAX)> body_slot_policies = {
    // HUMANOID: 全 13 スロット有効、拡張なし
    BodySlotPolicy(make_all_mask(), {}),

    // BIPEDAL: 武器/弓/指輪/腕装備なし。首・光源・胴体・頭・脚のみ
    BodySlotPolicy(make_mask({ INVEN_NECK, INVEN_LITE, INVEN_BODY, INVEN_HEAD, INVEN_FEET }), {}),

    // QUADRUPED: 四足獣。首・胴体・頭のみ
    BodySlotPolicy(make_mask({ INVEN_NECK, INVEN_BODY, INVEN_HEAD }), {}),

    // SERPENTINE: ヘビ型。首・胴体のみ + 尾の指輪
    BodySlotPolicy(make_mask({ INVEN_NECK, INVEN_BODY }), { ExtendedSlotType::TAIL_RING }),

    // AMORPHOUS: スライム型。リング 2 個のみ装備可
    BodySlotPolicy(make_mask({ INVEN_MAIN_RING, INVEN_SUB_RING }), {}),

    // INCORPOREAL: 装備一切不可
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, {}),

    // DRACONIC: HUMANOID 装備可能 + 尾の指輪 + 両翼の装飾
    BodySlotPolicy(make_all_mask(), { ExtendedSlotType::TAIL_RING, ExtendedSlotType::WING_LEFT, ExtendedSlotType::WING_RIGHT }),

    // FORMLESS: 決まった形を持たない塊・雲・霧。装備枠一切なし (擬足の指輪も無い)
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, {}),

    // GASEOUS: 毒ガス・煙・蒸気。実体が希薄で装備枠一切なし
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, {}),

    // VORTEX: 渦・竜巻・奔流。回転する現象そのもので装備枠一切なし
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, {}),

    // XAREN: 岩を泳ぐ多腕多脚の異形。手足の形が人型と異なり装備枠一切なし
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, {}),

    // AVIAN: 鳥型。前肢が完全な翼で武器も光源も持てない。首・胴体・頭・脚 + 両翼の装飾
    BodySlotPolicy(make_mask({ INVEN_NECK, INVEN_BODY, INVEN_HEAD, INVEN_FEET }), { ExtendedSlotType::WING_LEFT, ExtendedSlotType::WING_RIGHT }),

    // WORM: 長胴型。手足を持たない長い胴が身体の主体。首・胴体 + 尾の指輪
    BodySlotPolicy(make_mask({ INVEN_NECK, INVEN_BODY }), { ExtendedSlotType::TAIL_RING }),

    // INSECTOID: 節足型。外骨格と多数の脚で人間用の武具が合わない。首・胴体・頭のみ
    BodySlotPolicy(make_mask({ INVEN_NECK, INVEN_BODY, INVEN_HEAD }), {}),

    // WINGED_HUMANOID: 有翼人。背の翼が邪魔でクローク (体の上) だけ装備できない + 両翼の装飾
    BodySlotPolicy(make_all_mask_except({ INVEN_OUTER }), { ExtendedSlotType::WING_LEFT, ExtendedSlotType::WING_RIGHT }),

    // WING_ARMED_HUMANOID: 翼腕人。両腕が翼なので利き手・逆手が無い。体の上は羽織れる + 両翼の装飾
    BodySlotPolicy(make_all_mask_except({ INVEN_MAIN_HAND, INVEN_SUB_HAND }), { ExtendedSlotType::WING_LEFT, ExtendedSlotType::WING_RIGHT }),
};

}

std::string_view body_structure_name(BodyStructureType type)
{
    switch (type) {
    case BodyStructureType::HUMANOID:
        return _("人型", "humanoid");
    case BodyStructureType::BIPEDAL:
        return _("二足型", "bipedal");
    case BodyStructureType::QUADRUPED:
        return _("四足型", "quadruped");
    case BodyStructureType::SERPENTINE:
        return _("蛇型", "serpentine");
    case BodyStructureType::AMORPHOUS:
        return _("粘体型", "amorphous");
    case BodyStructureType::INCORPOREAL:
        return _("非実体", "incorporeal");
    case BodyStructureType::DRACONIC:
        return _("竜体", "draconic");
    case BodyStructureType::FORMLESS:
        return _("不定形", "formless");
    case BodyStructureType::GASEOUS:
        return _("気体", "gaseous");
    case BodyStructureType::VORTEX:
        return _("ボルテックス", "vortex");
    case BodyStructureType::XAREN:
        return _("ザレン型", "xaren");
    case BodyStructureType::AVIAN:
        return _("鳥型", "avian");
    case BodyStructureType::WORM:
        return _("長胴型", "worm");
    case BodyStructureType::INSECTOID:
        return _("節足型", "insectoid");
    case BodyStructureType::WINGED_HUMANOID:
        return _("有翼人", "winged humanoid");
    case BodyStructureType::WING_ARMED_HUMANOID:
        return _("翼腕人", "wing-armed humanoid");
    case BodyStructureType::MAX:
        break;
    }

    return _("不明", "unknown");
}

TERM_COLOR body_structure_color(BodyStructureType type)
{
    switch (type) {
    case BodyStructureType::INCORPOREAL:
        return TERM_L_DARK;
    case BodyStructureType::DRACONIC:
        return TERM_ORANGE;
    case BodyStructureType::HUMANOID:
    case BodyStructureType::BIPEDAL:
    case BodyStructureType::QUADRUPED:
    case BodyStructureType::SERPENTINE:
    case BodyStructureType::AMORPHOUS:
    case BodyStructureType::FORMLESS:
    case BodyStructureType::GASEOUS:
    case BodyStructureType::VORTEX:
    case BodyStructureType::XAREN:
    case BodyStructureType::AVIAN:
    case BodyStructureType::WORM:
    case BodyStructureType::INSECTOID:
    case BodyStructureType::WINGED_HUMANOID:
    case BodyStructureType::WING_ARMED_HUMANOID:
    case BodyStructureType::MAX:
        break;
    }

    return TERM_L_BLUE;
}

const BodySlotPolicy &get_body_slot_policy(BodyStructureType type)
{
    const auto idx = enum2i(type);
    if (idx >= body_slot_policies.size()) {
        return body_slot_policies[enum2i(BodyStructureType::HUMANOID)];
    }
    return body_slot_policies[idx];
}
