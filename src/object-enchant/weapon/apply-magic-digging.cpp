/*!
 * @brief 掘削武器に耐性等の追加効果を付与する処理
 * @date 2022/03/11
 * @author Hourier
 */

#include "object-enchant/weapon/apply-magic-digging.h"
#include "artifact/random-art-generator.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"

/*!
 * @brief 掘削武器強化クラスのコンストラクタ
 * @param creature クリーチャーへの参照
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
DiggingEnchanter::DiggingEnchanter(CreatureEntity &creature, ItemEntity *o_ptr, DEPTH level, int power)
    : AbstractWeaponEnchanter(o_ptr, level, power)
    , creature(creature)
{
}

/*!
 * @brief 掘削武器に生成ランクごとの強化を与えるサブルーチン
 * @details power > 2はデバッグ専用.
 */
void DiggingEnchanter::apply_magic()
{
    this->decide_skip();
    if (this->should_skip) {
        return;
    }

    this->give_killing_bonus();
    if (this->power > 1) {
        if ((this->power > 2) || one_in_(30)) {
            become_random_artifact(this->creature, this->o_ptr, false);
        } else {
            this->o_ptr->ego_idx = EgoType::DIGGING;
        }

        return;
    }

    if (this->power < -1) {
        this->o_ptr->pval = 0 - (5 + randint1(5));
        return;
    }

    if (this->power < 0) {
        this->o_ptr->pval = 0 - (this->o_ptr->pval);
        return;
    }
}
