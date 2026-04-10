/*
 * @brief ドラゴン・スケイルメイルに耐性等の追加効果を付与する処理
 * @date 2022/03/12
 * @author Hourier
 */

#include "object-enchant/protector/apply-magic-dragon-armor.h"
#include "artifact/random-art-generator.h"
#include "object-enchant/protector/abstract-protector-enchanter.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"

/*
 * @brief コンストラクタ
 * @param creature クリーチャーへの参照
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
DragonArmorEnchanter::DragonArmorEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power)
    : AbstractProtectorEnchanter{ o_ptr, level, power }
    , creature(creature)
{
}

/*!
 * @brief power > 2 はデバッグ専用.
 */
void DragonArmorEnchanter::apply_magic()
{
    if ((this->power > 2) || one_in_(50)) {
        become_random_artifact(this->creature, this->o_ptr, false);
    }
}
