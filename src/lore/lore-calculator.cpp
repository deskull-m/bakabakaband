#include "lore/lore-calculator.h"
#include "lore/lore-util.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/mspell-damage-calculator.h"
#include "system/creature-entity.h"
#include "system/monrace/monrace-definition.h"
#include <fmt/format.h>

std::string get_skill_damage(CreatureEntity &creature, lore_type *lore_ptr, MonsterAbilityType ms_type)
{
    return fmt::format("({})", describe_monspell_race_damage(creature, ms_type, lore_ptr->monrace_id));
}

/*!
 * @brief lore_ptrにダメージを与えるスキルの情報を追加する
 * @param lore_ptr 知識情報
 * @param ms_type スキル
 * @param msg スキルを表す文字列の書式 ({} にダメージの情報が入る)
 * @param color 表示する文字色
 */
void add_lore_of_damage_skill(CreatureEntity &creature, lore_type *lore_ptr, MonsterAbilityType ms_type, fmt::format_string<std::string_view> msg, byte color)
{
    if (!lore_ptr->is_details_known() && !lore_ptr->know_everything) {
        // ダメージ量の情報なし
        lore_ptr->lore_msgs.emplace_back(fmt::format(msg, std::string_view()), color);
        return;
    }

    lore_ptr->lore_msgs.emplace_back(fmt::format(msg, std::string_view(get_skill_damage(creature, lore_ptr, ms_type))), color);
}

void set_flags_for_full_knowledge(lore_type *lore_ptr)
{
    if (!lore_ptr->know_everything) {
        return;
    }

    lore_ptr->drop_gold = lore_ptr->drop_item = ((lore_ptr->monrace->drop_flags.has(MonsterDropType::DROP_4D2) ? 8 : 0) + (lore_ptr->monrace->drop_flags.has(MonsterDropType::DROP_3D2) ? 6 : 0) + (lore_ptr->monrace->drop_flags.has(MonsterDropType::DROP_2D2) ? 4 : 0) + (lore_ptr->monrace->drop_flags.has(MonsterDropType::DROP_1D2) ? 2 : 0) + (lore_ptr->monrace->drop_flags.has(MonsterDropType::DROP_90) ? 1 : 0) + (lore_ptr->monrace->drop_flags.has(MonsterDropType::DROP_60) ? 1 : 0));

    if (lore_ptr->monrace->drop_flags.has(MonsterDropType::ONLY_GOLD)) {
        lore_ptr->drop_item = 0;
    }

    if (lore_ptr->monrace->drop_flags.has(MonsterDropType::ONLY_ITEM)) {
        lore_ptr->drop_gold = 0;
    }

    lore_ptr->ability_flags = lore_ptr->monrace->ability_flags;
    lore_ptr->aura_flags = lore_ptr->monrace->aura_flags;
    lore_ptr->behavior_flags = lore_ptr->monrace->behavior_flags;
    lore_ptr->resistance_flags = lore_ptr->monrace->resistance_flags;
    lore_ptr->feature_flags = lore_ptr->monrace->feature_flags;
    lore_ptr->drop_flags = lore_ptr->monrace->drop_flags;
    lore_ptr->special_flags = lore_ptr->monrace->special_flags;
    lore_ptr->misc_flags = lore_ptr->monrace->misc_flags;
}

/*!
 * @brief 種族定義の種族分類 (MonsterKindType) を思い出へ写す
 * @param lore_ptr 知識情報
 * @details
 * フラグを 1 つずつ列挙して写すと MonsterKindType を追加したときに配線漏れが起き、
 * 表示側 (display_monster_kind() 等) がデッドコードになる。
 * 実際に 76 種が写されないまま表示側だけ存在していたため、一括で写す形にしてある。
 * ここは種族定義 (MonraceDefinition::kind_flags) をそのまま写すので「覚えた分」の
 * 選別は行わない。撃破数による開示判定は呼出側 (set_race_flags()) が担う。
 */
void set_known_kind_flags(lore_type *lore_ptr)
{
    lore_ptr->kind_flags.set(lore_ptr->monrace->kind_flags);
}
