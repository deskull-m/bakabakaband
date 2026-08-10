/*!
 * @brief effect_monster_type構造体の初期化処理
 * @date 2020/04/29
 * @author Hourier
 */

#include "effect/effect-monster-util.h"
#include "effect/attribute-types.h"
#include "floor/geometry.h"
#include "monster-floor/monster-death.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-util.h"
#include "object-enchant/tr-types.h"
#include "player-base/player-race.h"
#include "player-info/race-types.h"
#include "system/angband-system.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "term/z-rand.h"
#include <algorithm>

/*!
 * @brief EffectMonster構造体のコンストラクタ
 * @param creature クリーチャーへの参照
 * @param src_idx 魔法を発動したモンスター (0ならばプレイヤー)
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標y座標 / Target y location (or location to travel "towards")
 * @param x 目標x座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param attribute 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ
 * @param see_s_msg TRUEならばメッセージを表示する
 */
EffectMonster::EffectMonster(CreatureEntity &creature, MONSTER_IDX src_idx, POSITION r, POSITION y, POSITION x, int dam, AttributeType attribute, BIT_FLAGS flag, bool see_s_msg)
    : src_idx(src_idx)
    , r(r)
    , y(y)
    , x(x)
    , dam(dam)
    , attribute(attribute)
    , flag(flag)
    , see_s_msg(see_s_msg)
{
    auto &floor = *creature.get_floor();
    this->g_ptr = &floor.grid_array[this->y][this->x];
    this->m_ptr = &floor.m_list[this->g_ptr->m_idx];
    this->m_caster_ptr = this->is_monster() ? &floor.m_list[this->src_idx] : nullptr;
    this->monrace = this->m_ptr->get_monrace_shared();
    this->seen = this->m_ptr->is_visible_on_map();
    this->seen_msg = is_seen(creature, *this->m_ptr);
    this->slept = this->m_ptr->is_asleep();
    this->known = (Grid::calc_distance(creature.get_position(), this->m_ptr->get_position()) <= MAX_PLAYER_SIGHT) || AngbandSystem::get_instance().is_phase_out();
    this->note_dies = this->m_ptr->get_died_message();
    this->caster_lev = (this->is_monster() && (this->m_caster_ptr != nullptr)) ? this->m_caster_ptr->get_monrace().level : (creature.get_level() * 2);
}

bool EffectMonster::is_player() const
{
    return this->src_idx == 0;
}

bool EffectMonster::is_monster() const
{
    return this->src_idx > 0;
}

Pos2D EffectMonster::get_position() const
{
    return { this->y, this->x };
}

bool target_race_resists_element(EffectMonster *em_ptr, tr_type res_flag)
{
    if (!em_ptr->monrace->applies_player_race_resistances) {
        return false;
    }
    if (em_ptr->m_ptr->get_prace() == PlayerRaceType::NONE) {
        return false;
    }
    return CreatureRace(em_ptr->m_ptr).tr_flags().has(res_flag);
}

bool monster_saves_status_by_level(EffectMonster *em_ptr, int difficulty)
{
    // [提案C2第3弾・セーヴ] WIS + [提案C5・セーヴ] MAGIC_RES 突然変異のセーヴ補正を実効レベルへ加算 (いずれも opt-in・既定OFF)
    const auto effective_level = em_ptr->monrace->level + em_ptr->m_ptr->get_save_stat_bonus() + em_ptr->m_ptr->get_save_mutation_bonus();
    return effective_level > randint1(std::max(1, difficulty - 10)) + 10;
}
