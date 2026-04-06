/*
 * @brief 重鎧に耐性等の追加効果を付与する処理
 * @date 2022/03/12
 * @author Hourier
 */

#include "object-enchant/protector/apply-magic-hard-armor.h"
#include "system/creature-entity.h"

/*
 * @brief コンストラクタ
 * @param creature クリーチャーへの参照
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
HardArmorEnchanter::HardArmorEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power)
    : ArmorEnchanter{ creature, o_ptr, level, power }
{
}

/*!
 * @brief power > 2 はデバッグ専用.
 */
void HardArmorEnchanter::apply_magic()
{
    if (this->power > 1) {
        this->give_ego_index();
        return;
    }

    if (this->power < -1) {
        this->give_cursed();
    }
}
