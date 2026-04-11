#include "mspell/improper-mspell-remover.h"
#include "game-option/birth-options.h"
#include "monster/smart-learn-types.h"
#include "mspell/element-resistance-checker.h"
#include "mspell/high-resistance-checker.h"
#include "mspell/smart-mspell-util.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"

static void add_cheat_remove_flags(CreatureEntity &creature, msr_type *msr_ptr)
{
    if (!smart_cheat) {
        return;
    }

    add_cheat_remove_flags_element(creature, msr_ptr);
    add_cheat_remove_flags_others(creature, msr_ptr);
}

/*!
 * @brief モンスターの魔法一覧から戦術的に適さない魔法を除外する /
 * Remove the "bad" spells from a spell list
 * @param m_idx モンスターの構造体参照ポインタ
 * @param f4p モンスター魔法のフラグリスト1
 * @param f5p モンスター魔法のフラグリスト2
 * @param f6p モンスター魔法のフラグリスト3
 */
void remove_bad_spells(MONSTER_IDX m_idx, CreatureEntity &creature, EnumClassFlagGroup<MonsterAbilityType> &ability_flags)
{
    msr_type tmp_msr(creature, m_idx, ability_flags);
    auto *msr_ptr = &tmp_msr;
    if (msr_ptr->r_ptr->behavior_flags.has(MonsterBehaviorType::STUPID)) {
        return;
    }

    if (!smart_cheat && !smart_learn) {
        return;
    }

    auto &monster = creature.get_floor()->get_monster(m_idx);
    if (smart_learn) {
        /* 時々学習情報を忘れる */
        if (one_in_(100)) {
            monster.get_monster_profile().smart.clear();
        }

        msr_ptr->smart_flags = monster.get_monster_profile().smart;
    }

    add_cheat_remove_flags(creature, msr_ptr);
    if (msr_ptr->smart_flags.none()) {
        return;
    }

    check_element_resistance(msr_ptr);
    check_high_resistances(creature, msr_ptr);
    ability_flags = msr_ptr->ability_flags;
}
