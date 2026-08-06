/*!
 * @brief 矢類のアイテムを強化する処理
 * @date 2022/03/11
 * @author Hourier
 */

#include "object-enchant/weapon/apply-magic-arrow.h"
#include "artifact/random-art-generator.h"
#include "floor/floor-base-definitions.h"
#include "inventory/inventory-slot-types.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"

/*!
 * @brief 矢類強化クラスのコンストラクタ
 * @param creature クリーチャーへの参照
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
ArrowEnchanter::ArrowEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power)
    : AbstractWeaponEnchanter(o_ptr, level, power)
    , creature(creature)
{
}

/*!
 * @brief 矢類に生成ランクごとの強化を与えるサブルーチン
 * @details power > 2はデバッグ専用.
 */
void ArrowEnchanter::apply_magic()
{
    this->decide_skip();
    if (this->should_skip) {
        return;
    }

    this->give_killing_bonus();
    if (this->power > 1) {
        if (this->power > 2) {
            become_random_artifact(this->creature, this->o_ptr, false);
            return;
        }

        this->o_ptr->ego_idx = get_random_ego(INVEN_AMMO, true);
        auto &dice = this->o_ptr->damage_dice;
        while (one_in_(10 * dice.maxroll())) {
            dice.num++;
        }

        if (dice.num > 9) {
            dice.num = 9;
        }

        return;
    }

    if (this->power < -1) {
        if (randint0(MAX_DEPTH) < this->level) {
            this->o_ptr->ego_idx = get_random_ego(INVEN_AMMO, false);
        }
    }
}
