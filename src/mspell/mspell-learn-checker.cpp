#include "mspell/mspell-learn-checker.h"
#include "grid/grid.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/monster-entity.h"
#include "world/world.h"

/*!
 * @brief モンスターの唱えた呪文を青魔法で学習できるか判定する /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @return プレイヤーが青魔法で学習できるならTRUE、そうでなければFALSEを返す。
 *
 * モンスターが特技を使う前にプレイヤーがモンスターを視認できているかどうかの判定用。
 */
bool spell_learnable(CreatureEntity &creature, MONSTER_IDX m_idx)
{
    const auto &floor = *creature.current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];
    const auto seen = (!creature.is_blind() && monster.get_monster_profile().ml);
    const auto maneable = floor.has_los_at({ monster.y, monster.x });
    return seen && maneable && (AngbandWorld::get_instance().timewalk_m_idx == 0);
}
