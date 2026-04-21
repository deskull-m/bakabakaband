/*!
 * @brief モンスターの移動方向を決定する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster-floor/monster-direction.h"
#include "monster-floor/monster-sweep-grid.h"
#include "monster/monster-info.h"
#include "monster/monster-processor-util.h"
#include "monster/monster-status.h"
#include "pet/pet-util.h"
#include "player/player-status-flags.h"
#include "spell/range-calc.h"
#include "system/angband-system.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "target/projection-path-calculator.h"

/*!
 * @brief ペットが敵に接近するための方向を決定する
 * @param creature クリーチャーへの参照
 * @param monster_from 移動を試みているモンスターへの参照ポインタ
 * @param monster_to 移動先モンスターへの参照ポインタ
 * @param plus モンスターIDの増減 (1/2 の確率で+1、1/2の確率で-1)
 * @return ペットがモンスターに近づくならばTRUE
 */
static bool decide_pet_approch_direction(CreatureEntity &creature, const CreatureEntity &monster_from, const CreatureEntity &monster_to)
{
    if (!monster_from.is_pet()) {
        return false;
    }

    const auto p_pos = creature.get_position();
    const auto cdis_from = Grid::calc_distance(p_pos, monster_from.get_position());
    const auto cdis_to = Grid::calc_distance(p_pos, monster_to.get_position());
    if (creature.pet_follow_distance < 0) {
        if (cdis_to <= (0 - creature.pet_follow_distance)) {
            return true;
        }
    } else if ((cdis_from < cdis_to) && (cdis_to > creature.pet_follow_distance)) {
        return true;
    }

    const auto &monrace = monster_from.get_monrace();
    return monrace.aaf < cdis_to;
}

/*!
 * @brief モンスターが敵に接近するための方向を決定する
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @param start モンスターIDの開始
 * @param plus モンスターIDの増減 (1/2 の確率で+1、1/2の確率で-1)
 * @param y モンスターの移動方向Y
 * @param x モンスターの移動方向X
 */
static void decide_enemy_approch_direction(CreatureEntity &creature, MONSTER_IDX m_idx, int start, int plus, POSITION *y, POSITION *x)
{
    auto &floor = *creature.get_floor();
    const auto &monster_from = floor.get_monster(m_idx);
    const auto &monrace = monster_from.get_monrace();
    for (int i = start; ((i < start + floor.m_max) && (i > start - floor.m_max)); i += plus) {
        const auto dummy = (i % floor.m_max);
        if (dummy == 0) {
            continue;
        }

        const auto t_idx = dummy;
        const auto &monster_to = floor.get_monster(t_idx);
        if (&monster_to == &monster_from) {
            continue;
        }
        if (!monster_to.is_valid()) {
            continue;
        }
        if (decide_pet_approch_direction(creature, monster_from, monster_to)) {
            continue;
        }
        if (!monster_from.is_hostile_to_melee(monster_to)) {
            continue;
        }

        const auto can_pass_wall = monrace.feature_flags.has(MonsterFeatureType::PASS_WALL) && (!monster_from.is_riding() || has_pass_wall(creature));
        const auto can_kill_wall = monrace.feature_flags.has(MonsterFeatureType::KILL_WALL) && !monster_from.is_riding();
        const auto m_pos_from = monster_from.get_position();
        const auto m_pos_to = monster_to.get_position();
        if (can_pass_wall || can_kill_wall) {
            if (!in_disintegration_range(floor, m_pos_from, m_pos_to)) {
                continue;
            }
        } else {
            if (!projectable(floor, m_pos_from, m_pos_to)) {
                continue;
            }
        }

        *y = monster_to.y;
        *x = monster_to.x;
        return;
    }
}

/*!
 * @brief モンスターが敵に接近するための方向を計算するメインルーチン
 * Calculate the direction to the next enemy
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターの参照ID
 * @return 方向が確定した場合移動方向のリスト、接近する敵がそもそもいない場合tl::nullopt
 */
static tl::optional<MonsterMovementDirectionList> get_enemy_dir(CreatureEntity &creature, MONSTER_IDX m_idx)
{
    const auto &floor = *creature.get_floor();
    const auto &monster = floor.get_monster(m_idx);

    POSITION x = 0, y = 0;
    if (creature.riding_t_m_idx && creature.is_located_at({ monster.y, monster.x })) {
        y = floor.get_monster(creature.riding_t_m_idx).y;
        x = floor.get_monster(creature.riding_t_m_idx).x;
    } else if (monster.is_pet() && creature.pet_t_m_idx) {
        y = floor.get_monster(creature.pet_t_m_idx).y;
        x = floor.get_monster(creature.pet_t_m_idx).x;
    } else {
        int start;
        int plus = 1;
        if (AngbandSystem::get_instance().is_phase_out()) {
            start = randint1(floor.m_max - 1) + floor.m_max;
            if (randint0(2)) {
                plus = -1;
            }
        } else {
            start = floor.m_max + 1;
        }

        decide_enemy_approch_direction(creature, m_idx, start, plus, &y, &x);

        if ((x == 0) && (y == 0)) {
            return tl::nullopt;
        }
    }

    const Pos2D pos(y, x);
    const auto vec = pos - monster.get_position();

    return get_enemy_approch_direction(m_idx, vec);
}

/*!
 * @brief 不規則歩行フラグを持つモンスターが不規則な方向に移動するかどうかをその確率に基づいて決定する
 * @param creature クリーチャーへの参照
 * @param mosnter モンスターへの参照
 * @return 不規則な方向へ移動するならtrue
 */
static bool random_walk(CreatureEntity &creature, const CreatureEntity &monster)
{
    auto &monrace = monster.get_monrace();
    if (monrace.behavior_flags.has_all_of({ MonsterBehaviorType::RAND_MOVE_50, MonsterBehaviorType::RAND_MOVE_25 }) && evaluate_percent(75)) {
        if (is_original_ap_and_seen(creature, monster)) {
            monrace.r_behavior_flags.set({ MonsterBehaviorType::RAND_MOVE_50, MonsterBehaviorType::RAND_MOVE_25 });
        }

        return true;
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::RAND_MOVE_50) && one_in_(2)) {
        if (is_original_ap_and_seen(creature, monster)) {
            monrace.r_behavior_flags.set(MonsterBehaviorType::RAND_MOVE_50);
        }

        return true;
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::RAND_MOVE_25) && one_in_(4)) {
        if (is_original_ap_and_seen(creature, monster)) {
            monrace.r_behavior_flags.set(MonsterBehaviorType::RAND_MOVE_25);
        }

        return true;
    }

    return false;
}

/*!
 * @brief ペットの移動方向のリストを決定する
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @return ペットであれば移動方向のリスト、そうでなければtl::nullopt
 */
static tl::optional<MonsterMovementDirectionList> decide_pet_movement_direction(CreatureEntity &creature, MONSTER_IDX m_idx)
{
    const auto &monster = creature.get_floor()->get_monster(m_idx);
    if (!monster.is_pet()) {
        return tl::nullopt;
    }

    if (const auto mmdl = get_enemy_dir(creature, m_idx)) {
        return mmdl;
    }

    const auto cdis = Grid::calc_distance(creature.get_position(), monster.get_position());
    auto &pet_follow_distance = creature.pet_follow_distance;
    const auto avoid = ((pet_follow_distance < 0) && (cdis <= (0 - pet_follow_distance)));
    const auto lonely = (!avoid && (cdis > pet_follow_distance));
    const auto distant = (cdis > PET_SEEK_DIST);
    if (!avoid && !lonely && !distant) {
        return MonsterMovementDirectionList::random_move(m_idx);
    }

    const auto distance_orig = pet_follow_distance;
    if (pet_follow_distance > PET_SEEK_DIST) {
        pet_follow_distance = PET_SEEK_DIST;
    }

    MonsterSweepGrid msd(&creature, m_idx);
    const auto mmdl = msd.get_movable_grid();
    pet_follow_distance = distance_orig;
    return mmdl;
}

/*!
 * @brief モンスターの移動方向のリストを決定する
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @param aware モンスターがプレイヤーに気付いているならばTRUE、超隠密状態ならばFALSE
 * @return 移動方向のリスト。移動しない場合はtl::nullopt
 */
tl::optional<MonsterMovementDirectionList> decide_monster_movement_direction(CreatureEntity &creature, MONSTER_IDX m_idx, bool aware)
{
    const auto &monster = creature.get_floor()->get_monster(m_idx);
    const auto &monrace = monster.get_monrace();

    if (monster.is_confused() || !aware) {
        return MonsterMovementDirectionList::random_move(m_idx);
    }

    if (random_walk(creature, monster)) {
        return MonsterMovementDirectionList::random_move(m_idx);
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::NEVER_MOVE) && (Grid::calc_distance(creature.get_position(), monster.get_position()) > 1)) {
        return MonsterMovementDirectionList::random_move(m_idx);
    }

    if (const auto mmdl = decide_pet_movement_direction(creature, m_idx)) {
        return mmdl;
    }

    if (!monster.is_hostile()) {
        const auto mmdl = get_enemy_dir(creature, m_idx);
        return mmdl ? mmdl : MonsterMovementDirectionList::random_move(m_idx);
    }

    MonsterSweepGrid msd(&creature, m_idx);
    return msd.get_movable_grid();
}
