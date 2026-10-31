#include "system/monrace/body-structure-policy.h"
#include "locale/language-switcher.h"
#include "term/term-color-types.h"
#include "util/enum-converter.h"
#include <array>

namespace {

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
 *
 *          **通常装備スロット (INVEN_MAIN_HAND..INVEN_TOTAL-1) を持つのは
 *          人型系 3 種と竜体だけ**で、それ以外の体構造は 1 つも持たない。
 *          人間用の装具は人型の体に合わせた形をしているので、獣・鳥・蟲・蠕虫に
 *          「首と胴と頭だけ」のような部分的な人型スロットを与えるのは不整合
 *          (倒して身ぐるみを剥げるのは人型が主、という方針)。獣側の装飾は
 *          身体に合った**拡張スロット** (尾の指輪・両翼の装飾) で表現する。
 *          拡張スロットの中身は `drop_all_inventory()` の走査対象外なので、
 *          剥ぎ取れる装備としては扱われない。
 *
 *          第 3 引数は「武具を自ら調えて身に着ける体か」(武装度充填の対象か)。
 *          スロットの有無とは別軸の概念だが、現状の体構造では両者が一致する
 *          (通常スロットを持つ 4 種のみ true)。将来「首輪だけ着ける獣」等を
 *          足す場合に「スロットはあるが自分で武具は調えない」と表せる。
 */
const std::array<BodySlotPolicy, enum2i(BodyStructureType::MAX)> body_slot_policies = {
    // HUMANOID: 全 13 スロット有効、拡張なし
    BodySlotPolicy(make_all_mask(), {}, true),

    // BIPEDAL: 二足型 (恐竜型・翼手型)。人型用の装具が合わないため通常スロットなし
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, {}, false),

    // QUADRUPED: 四足獣。獣に人間用の鎧・兜は合わないため通常スロットなし
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, {}, false),

    // SERPENTINE: ヘビ型。通常スロットなし。装飾は尾の指輪で表す
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, { ExtendedSlotType::TAIL_RING }, false),

    // AMORPHOUS: 粘体型。擬足に指輪という扱いも人型スロットの流用なので持たせない
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, {}, false),

    // INCORPOREAL: 装備一切不可
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, {}, false),

    // DRACONIC: HUMANOID 装備可能 + 尾の指輪 + 両翼の装飾
    BodySlotPolicy(make_all_mask(), { ExtendedSlotType::TAIL_RING, ExtendedSlotType::WING_LEFT, ExtendedSlotType::WING_RIGHT }, true),

    // FORMLESS: 決まった形を持たない塊・雲・霧。装備枠一切なし (擬足の指輪も無い)
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, {}, false),

    // GASEOUS: 毒ガス・煙・蒸気。実体が希薄で装備枠一切なし
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, {}, false),

    // VORTEX: 渦・竜巻・奔流。回転する現象そのもので装備枠一切なし
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, {}, false),

    // XAREN: 岩を泳ぐ多腕多脚の異形。手足の形が人型と異なり装備枠一切なし
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, {}, false),

    // AVIAN: 鳥型。全身が鳥なので通常スロットなし。装飾は両翼で表す
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, { ExtendedSlotType::WING_LEFT, ExtendedSlotType::WING_RIGHT }, false),

    // WORM: 長胴型。手足を持たないため通常スロットなし。装飾は尾の指輪で表す
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, { ExtendedSlotType::TAIL_RING }, false),

    // INSECTOID: 節足型。外骨格と多数の脚に人間用の武具は合わないため通常スロットなし
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, {}, false),

    // WINGED_HUMANOID: 有翼人。背の翼が邪魔でクローク (体の上) だけ装備できない + 両翼の装飾
    BodySlotPolicy(make_all_mask_except({ INVEN_OUTER }), { ExtendedSlotType::WING_LEFT, ExtendedSlotType::WING_RIGHT }, true),

    // WING_ARMED_HUMANOID: 翼腕人。両腕が翼なので利き手・逆手が無い。体の上は羽織れる + 両翼の装飾
    BodySlotPolicy(make_all_mask_except({ INVEN_MAIN_HAND, INVEN_SUB_HAND }), { ExtendedSlotType::WING_LEFT, ExtendedSlotType::WING_RIGHT }, true),

    // SPHERE: 球体そのもの。手足も頭も首も無く、身に着けられる部位が存在しない
    BodySlotPolicy(BodySlotPolicy::SlotMask{}, {}, false),
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
    case BodyStructureType::SPHERE:
        return _("球体", "sphere");
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
    case BodyStructureType::SPHERE:
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
