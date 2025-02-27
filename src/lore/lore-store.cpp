/*!
 * @brief モンスターの思い出を記憶する処理
 * @date 2020/06/09
 * @author Hourier
 */

#include "lore/lore-store.h"
#include "core/window-redrawer.h"
#include "monster/monster-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h" //!< @todo 違和感、m_ptr は外から与えることとしたい.
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "tracking/lore-tracker.h"

template <class T>
static int count_lore_mflag_group(const EnumClassFlagGroup<T> &flags, const EnumClassFlagGroup<T> &r_flags)
{
    auto result_flags = flags;
    auto num = result_flags.reset(r_flags).count();
    return num;
}

/*!
 * @brief モンスターの調査による思い出補完処理 / Learn about a monster (by "probing" it)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param r_idx 補完されるモンスター種族ID
 * @return 明らかになった情報の度数
 * @details
 * Return the number of new flags learnt.  -Mogami-
 */
int lore_do_probe(PlayerType *player_ptr, MonraceId r_idx)
{
    (void)player_ptr;
    int n = 0;
    auto *r_ptr = &monraces_info[r_idx];
    if (r_ptr->r_wake != MAX_UCHAR) {
        n++;
    }
    if (r_ptr->r_ignore != MAX_UCHAR) {
        n++;
    }
    r_ptr->r_wake = r_ptr->r_ignore = MAX_UCHAR;

    for (auto i = 0; i < 4; i++) {
        if (r_ptr->blows[i].effect != RaceBlowEffectType::NONE || r_ptr->blows[i].method != RaceBlowMethodType::NONE) {
            if (r_ptr->r_blows[i] != MAX_UCHAR) {
                n++;
            }
            r_ptr->r_blows[i] = MAX_UCHAR;
        }
    }

    using Mdt = MonsterDropType;
    byte tmp_byte = (r_ptr->drop_flags.has(Mdt::DROP_4D2) ? 8 : 0);
    tmp_byte += (r_ptr->drop_flags.has(Mdt::DROP_3D2) ? 6 : 0);
    tmp_byte += (r_ptr->drop_flags.has(Mdt::DROP_2D2) ? 4 : 0);
    tmp_byte += (r_ptr->drop_flags.has(Mdt::DROP_1D2) ? 2 : 0);
    tmp_byte += (r_ptr->drop_flags.has(Mdt::DROP_90) ? 1 : 0);
    tmp_byte += (r_ptr->drop_flags.has(Mdt::DROP_60) ? 1 : 0);

    if (r_ptr->drop_flags.has_not(Mdt::ONLY_GOLD)) {
        if (r_ptr->r_drop_item != tmp_byte) {
            n++;
        }
        r_ptr->r_drop_item = tmp_byte;
    }
    if (r_ptr->drop_flags.has_not(Mdt::ONLY_ITEM)) {
        if (r_ptr->r_drop_gold != tmp_byte) {
            n++;
        }
        r_ptr->r_drop_gold = tmp_byte;
    }

    if (r_ptr->r_cast_spell != MAX_UCHAR) {
        n++;
    }
    r_ptr->r_cast_spell = MAX_UCHAR;

    n += count_lore_mflag_group(r_ptr->resistance_flags, r_ptr->r_resistance_flags);
    n += count_lore_mflag_group(r_ptr->ability_flags, r_ptr->r_ability_flags);
    n += count_lore_mflag_group(r_ptr->behavior_flags, r_ptr->r_behavior_flags);
    n += count_lore_mflag_group(r_ptr->drop_flags, r_ptr->r_drop_flags);
    n += count_lore_mflag_group(r_ptr->feature_flags, r_ptr->r_feature_flags);
    n += count_lore_mflag_group(r_ptr->special_flags, r_ptr->r_special_flags);
    n += count_lore_mflag_group(r_ptr->misc_flags, r_ptr->r_misc_flags);

    r_ptr->r_resistance_flags = r_ptr->resistance_flags;
    r_ptr->r_ability_flags = r_ptr->ability_flags;
    r_ptr->r_behavior_flags = r_ptr->behavior_flags;
    r_ptr->r_drop_flags = r_ptr->drop_flags;
    r_ptr->r_feature_flags = r_ptr->feature_flags;
    r_ptr->r_special_flags = r_ptr->special_flags;
    r_ptr->r_misc_flags = r_ptr->misc_flags;

    if (!r_ptr->r_can_evolve) {
        n++;
    }

    r_ptr->r_can_evolve = true;
    if (LoreTracker::get_instance().is_tracking(r_idx)) {
        RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
    }

    return n;
}

/*!
 * @brief モンスターの撃破に伴うドロップ情報の記憶処理 / Take note that the given monster just dropped some treasure
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター情報のID
 * @param num_item 手に入れたアイテム数
 * @param num_gold 手に入れた財宝の単位数
 */
void lore_treasure(PlayerType *player_ptr, MONSTER_IDX m_idx, ITEM_NUMBER num_item, ITEM_NUMBER num_gold)
{
    auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    auto &monrace = monster.get_monrace();
    if (!monster.is_original_ap()) {
        return;
    }

    if (num_item > monrace.r_drop_item) {
        monrace.r_drop_item = num_item;
    }

    if (num_gold > monrace.r_drop_gold) {
        monrace.r_drop_gold = num_gold;
    }

    if (monrace.drop_flags.has(MonsterDropType::DROP_GOOD)) {
        monrace.r_drop_flags.set(MonsterDropType::DROP_GOOD);
    }

    if (monrace.drop_flags.has(MonsterDropType::DROP_GREAT)) {
        monrace.r_drop_flags.set(MonsterDropType::DROP_GREAT);
    }

    if (monrace.drop_flags.has(MonsterDropType::DROP_NASTY)) {
        monrace.r_drop_flags.set(MonsterDropType::DROP_NASTY);
    }

    if (LoreTracker::get_instance().is_tracking(monster.r_idx)) {
        RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
    }
}
