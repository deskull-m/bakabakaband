/*!
 * @brief 剣・鈍器・長柄/斧武器に耐性等の追加効果を付与する処理
 * @date 2022/03/22
 * @author Hourier
 */

#include "object-enchant/weapon/melee-weapon-enchanter.h"
#include "system/creature-entity.h"
#include "artifact/random-art-generator.h"
#include "system/item-entity.h"

MeleeWeaponEnchanter::MeleeWeaponEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power)
    : AbstractWeaponEnchanter(o_ptr, level, power)
    , creature(creature)
{
}

/*!
 * @brief 打撃系オブジェクトに生成ランクごとの強化を与えるサブルーチン
 * @details power > 2はデバッグ専用.
 */
void MeleeWeaponEnchanter::apply_magic()
{
    if (this->should_skip) {
        return;
    }

    if (this->power > 1) {
        this->strengthen();
        return;
    }

    if (this->power < -1) {
        this->give_cursed();
    }
}

/*!
 * @brief アーティファクト生成・ダイス強化処理
 * @details power > 2はデバッグ専用.
 */
void MeleeWeaponEnchanter::strengthen()
{
    if ((this->power > 2) || one_in_(40)) {
        become_random_artifact(this->creature, this->o_ptr, false);
        return;
    }

    this->give_ego_index();
    if (this->o_ptr->is_random_artifact()) {
        return;
    }

    auto &dice = this->o_ptr->damage_dice;
    while (one_in_(10 * dice.maxroll())) {
        dice.num++;
    }

    if (dice.num > 9) {
        dice.num = 9;
    }
}
