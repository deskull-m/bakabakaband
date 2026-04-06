#include "monster-floor/monster-remover.h"
#include "system/creature-entity.h"
#include "core/stuff-handler.h"
#include "floor/floor-object.h"
#include "grid/grid.h"
#include "monster-race/race-brightness-mask.h"
#include "monster/monster-status-setter.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-checker.h"
#include "tracking/health-bar-tracker.h"

/*!
 * @brief モンスター配列からモンスターを消去する
 * @param m_idx 消去するモンスターのフロア内インデックス
 * @details モンスターを削除するとそのモンスターが拾っていたアイテムも同時に削除される.
 */
void delete_monster_idx(CreatureEntity &creature, short m_idx)
{
    auto &floor = *creature.current_floor_ptr;
    auto &monster = floor.get_monster(m_idx);
    auto &monrace = monster.get_monrace();
    const auto m_pos = monster.get_position();
    monster.get_real_monrace().decrement_current_numbers();
    if (monrace.misc_flags.has(MonsterMiscType::MULTIPLY)) {
        floor.num_repro--;
    }

    if (monster.is_asleep()) {
        (void)set_monster_csleep(floor, m_idx, 0);
    }
    if (monster.is_accelerated()) {
        (void)set_monster_fast(floor, m_idx, 0);
    }
    if (monster.is_decelerated()) {
        (void)set_monster_slow(floor, m_idx, 0);
    }
    if (monster.is_stunned()) {
        (void)set_monster_stunned(floor, m_idx, 0);
    }
    if (monster.is_confused()) {
        (void)set_monster_confused(floor, m_idx, 0);
    }
    if (monster.is_fearful()) {
        (void)set_monster_monfear(floor, m_idx, 0);
    }
    if (monster.is_invulnerable()) {
        (void)set_monster_invulner(floor, m_idx, 0, false);
    }

    const auto target_m_idx = Target::get_last_target().get_m_idx();
    if (m_idx == target_m_idx) {
        Target::clear_last_target();
    }

    if (HealthBarTracker::get_instance().is_tracking(m_idx)) {
        health_track(creature, 0);
    }

    if (creature.pet_t_m_idx == m_idx) {
        creature.pet_t_m_idx = 0;
    }
    if (creature.riding_t_m_idx == m_idx) {
        creature.riding_t_m_idx = 0;
    }
    if (monster.is_riding()) { // creature.riding == m_idx のままの方がいい？
        static_cast<PlayerType &>(creature).ride_monster(0);
    }

    floor.get_grid(m_pos).m_idx = 0;
    delete_items(creature, monster.get_monster_profile().hold_o_idx_list);

    // 召喚元のモンスターが消滅した時は、召喚されたモンスターのparent_m_idxが
    // 召喚されたモンスター自身のm_idxを指すようにする
    for (MONSTER_IDX child_m_idx = 1; child_m_idx < floor.m_max; child_m_idx++) {
        auto &child_monster = floor.get_monster(child_m_idx);
        if (child_monster.is_valid() && child_monster.get_monster_profile().parent_m_idx == m_idx) {
            child_monster.get_monster_profile().parent_m_idx = child_m_idx;
        }
    }

    monster.wipe();
    floor.m_cnt--;
    lite_spot(creature, m_pos);
    if (monrace.brightness_flags.has_any_of(ld_mask)) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
    }
}

/*!
 * @brief プレイヤーのフロア離脱に伴う全モンスター配列の消去
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 視覚効果なしでdelete_monster() をフロア全体に対して呼び出す.
 */
void wipe_monsters_list(CreatureEntity &creature)
{
    auto &monraces = MonraceList::get_instance();
    auto &floor = *creature.current_floor_ptr;
    for (auto i = floor.m_max - 1; i >= 1; i--) {
        auto &monster = floor.get_monster(i);
        if (!monster.is_valid()) {
            continue;
        }

        floor.get_grid(monster.get_position()).m_idx = 0;
        monster.wipe();
    }

    monraces.reset_current_numbers();
    floor.m_max = 1;
    floor.m_cnt = 0;
    floor.reset_mproc_max();
    floor.num_repro = 0;
    Target::clear_last_target();
    creature.pet_t_m_idx = 0;
    creature.riding_t_m_idx = 0;
    health_track(creature, 0);
}

/*!
 * @brief 指定位置に存在するモンスターを削除する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 削除するモンスターの座標
 */
void delete_monster(CreatureEntity &creature, const Pos2D &pos)
{
    auto &floor = *creature.current_floor_ptr;
    if (!floor.contains(pos, FloorBoundary::OUTER_WALL_EXCLUSIVE)) {
        return;
    }

    const auto &grid = floor.get_grid(pos);
    if (grid.has_monster()) {
        delete_monster_idx(creature, grid.m_idx);
    }
}
