/*!
 * @file run-execution.cpp
 * @brief プレイヤーの走行処理実装
 */

#include "action/run-execution.h"
#include "action/movement-execution.h"
#include "core/disturbance.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "player-status/player-energy.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/terrain/terrain-definition.h"
#include "view/display-messages.h"

bool ignore_avoid_run;

/* The direction we are running */
static Direction find_current = Direction::none();

/* The direction we came from */
static Direction find_prevdir = Direction::none();

static bool find_openarea;

/* We are looking for a break */
static bool find_breakright;
static bool find_breakleft;

/*!
 * @brief ダッシュ移動処理中、移動先のマスが既知の壁かどうかを判定する /
 * Hack -- Check for a "known wall" (see below)
 * @param creature	クリーチャーへの参照
 * @param dir 想定する移動方向
 * @param pos_orig 移動元の座標
 * @return 移動先が既知の壁ならばTRUE
 */
static bool see_wall(CreatureEntity &creature, const Direction &dir, const Pos2D &pos_orig)
{
    const auto &floor = *creature.get_floor();
    const auto pos = pos_orig + dir.vec();
    if (!floor.contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
        return false;
    }

    const auto &grid = floor.get_grid(pos);
    if (!grid.is_mark()) {
        return false;
    }

    const auto terrain_id = grid.get_terrain_id(TerrainKind::MIMIC);
    const auto &terrain = grid.get_terrain(TerrainKind::MIMIC);
    if (!player_can_enter(creature, terrain_id, 0)) {
        return terrain.flags.has_not(TerrainCharacteristics::DOOR);
    }

    if (terrain.flags.has(TerrainCharacteristics::AVOID_RUN) && !ignore_avoid_run) {
        return true;
    }

    if (terrain.flags.has_none_of({ TerrainCharacteristics::MOVE, TerrainCharacteristics::CAN_FLY })) {
        return terrain.flags.has_not(TerrainCharacteristics::DOOR);
    }

    return false;
}

/*!
 * @brief ダメージ地形をダッシュ中に無視 (素通り) してよいか判定する
 * @param creature クリーチャーへの参照
 * @param terrain 判定対象の地形
 * @return 無視してよければ true (走行を止めない)
 */
static bool can_ignore_damaging_terrain(CreatureEntity &creature, const TerrainType &terrain)
{
    if (creature.is_invulnerable()) {
        return true;
    }

    const auto &flags = terrain.flags;
    if (flags.has(TerrainCharacteristics::LAVA) && !creature.has_immune_fire()) {
        return false;
    }

    // 虚空 (VOID) は無敵以外では無視不可 (bakabakaband 独自地形)
    if (flags.has(TerrainCharacteristics::VOID)) {
        return false;
    }

    if (flags.has(TerrainCharacteristics::COLD_PUDDLE) && !creature.has_immune_cold()) {
        return false;
    }

    if (flags.has(TerrainCharacteristics::ELEC_PUDDLE) && !creature.has_immune_elec()) {
        return false;
    }

    if (flags.has(TerrainCharacteristics::ACID_PUDDLE) && !creature.has_immune_acid()) {
        return false;
    }

    if (flags.has(TerrainCharacteristics::POISON_PUDDLE)) {
        return false;
    }

    if (flags.has_all_of({ TerrainCharacteristics::WATER, TerrainCharacteristics::DEEP })) {
        const auto can_ignore_water = creature.has_levitation() || creature.has_can_swim() || creature.has_resist_water() || (calc_inventory_weight(creature) <= calc_weight_limit(creature));
        if (!can_ignore_water) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief ダッシュ処理の導入 /
 * Initialize the running algorithm for a new direction.
 * @param creature	クリーチャーへの参照
 * @param dir 導入の移動先
 * @details
 * Diagonal Corridor -- allow diaginal entry into corridors.\n
 *\n
 * Blunt Corridor -- If there is a wall two spaces ahead and\n
 * we seem to be in a corridor, then force a turn into the side\n
 * corridor, must be moving straight into a corridor here. ???\n
 *\n
 * Diagonal Corridor    Blunt Corridor (?)\n
 *       \# \#                  \#\n
 *       \#x\#                  \@x\#\n
 *       \@\@p.                  p\n
 */
static void run_init(CreatureEntity &creature, const Direction &dir)
{
    find_current = dir;
    find_prevdir = dir;
    find_openarea = true;
    find_breakright = find_breakleft = false;
    const auto pos = creature.get_position();
    creature.set_run_py(pos.y);
    creature.set_run_px(pos.x);
    const auto pos_neighbor = creature.get_neighbor(dir);
    ignore_avoid_run = creature.get_floor()->has_terrain_characteristics(pos_neighbor, TerrainCharacteristics::AVOID_RUN);
    const auto dir_left45 = dir.rotated_45degree(1);
    const auto dir_right45 = dir.rotated_45degree(-1);
    auto deepleft = false;
    auto shortleft = false;
    if (see_wall(creature, dir_left45, pos)) {
        find_breakleft = true;
        shortleft = true;
    } else if (see_wall(creature, dir_left45, pos_neighbor)) {
        find_breakleft = true;
        deepleft = true;
    }

    auto deepright = false;
    auto shortright = false;
    if (see_wall(creature, dir_right45, pos)) {
        find_breakright = true;
        shortright = true;
    } else if (see_wall(creature, dir_right45, pos_neighbor)) {
        find_breakright = true;
        deepright = true;
    }

    if (!find_breakleft || !find_breakright) {
        return;
    }

    find_openarea = false;
    if (dir.is_diagonal()) {
        if (deepleft && !deepright) {
            find_prevdir = dir_right45;
        } else if (deepright && !deepleft) {
            find_prevdir = dir_left45;
        }

        return;
    }

    if (!see_wall(creature, dir, pos_neighbor)) {
        return;
    }

    if (shortleft && !shortright) {
        find_prevdir = dir.rotated_45degree(-2);
    } else if (shortright && !shortleft) {
        find_prevdir = dir.rotated_45degree(2);
    }
}

/*!
 * @brief ダッシュ移動処理中、移動先のマスか未知の地形かどうかを判定する /
 * Hack -- Check for an "unknown corner" (see below)
 * @param creature	クリーチャーへの参照
 * @param dir 想定する移動方向
 * @param pos_orig 移動元の座標
 * @return 移動先が未知の地形ならばTRUE
 */
static bool see_nothing(CreatureEntity &creature, const Direction &dir, const Pos2D &pos_orig)
{
    const auto pos = pos_orig + dir.vec();
    const auto &floor = *creature.get_floor();
    if (!floor.contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
        return true;
    }

    if (floor.get_grid(pos).is_mark()) {
        return false;
    }

    if (player_can_see_bold(creature, pos.y, pos.x)) {
        return false;
    }

    return true;
}

/*!
 * @brief ダッシュ移動が継続できるかどうかの判定 /
 * Update the current "run" path
 * @param creature	クリーチャーへの参照
 * @return 立ち止まるべき条件が満たされたらTRUE
 * ダッシュ移動が継続できるならばTRUEを返す。
 * Return TRUE if the running should be stopped
 */
static bool run_test(CreatureEntity &creature)
{
    const auto prev_dir = find_prevdir;
    const auto &floor = *creature.get_floor();
    const auto p_pos = creature.get_position();
    const auto &p_grid = floor.get_grid(p_pos);
    if ((disturb_trap_detect || alert_trap_detect) && creature.is_dtrap() && !(p_grid.info & CAVE_IN_DETECT)) {
        creature.set_dtrap(false);
        if (!(p_grid.info & CAVE_UNSAFE)) {
            if (alert_trap_detect) {
                msg_print(_("* 注意:この先はトラップの感知範囲外です！ *", "*Leaving trap detect region!*"));
            }

            if (disturb_trap_detect) {
                /* Break Run */
                return true;
            }
        }
    }

    Direction check_dir(5);
    auto option = Direction::none();
    auto option2 = Direction::none();
    const auto max = prev_dir.is_diagonal() ? 2 : 1;
    for (auto i = -max; i <= max; i++) {
        const auto new_dir = prev_dir.rotated_45degree(i);
        const auto pos = creature.get_neighbor(new_dir);
        const auto &grid = floor.get_grid(pos);
        if (grid.has_monster()) {
            const auto &monster = floor.get_monster(grid.m_idx);
            if (monster.is_visible_on_map()) {
                return true;
            }
        }

        for (const auto this_o_idx : grid.o_idx_list) {
            const auto &item = *floor.o_list[this_o_idx];
            if (item.marked.has(OmType::FOUND)) {
                return true;
            }
        }

        auto inv = true;
        if (grid.is_mark()) {
            const auto &terrain = grid.get_terrain(TerrainKind::MIMIC);
            const auto is_damaging = terrain.can_damage_player();
            auto notice = terrain.flags.has(TerrainCharacteristics::NOTICE) || is_damaging;
            if (notice && terrain.flags.has(TerrainCharacteristics::MOVE)) {
                if (find_ignore_doors && terrain.flags.has_all_of({ TerrainCharacteristics::DOOR, TerrainCharacteristics::CLOSE })) {
                    notice = false;
                } else if (find_ignore_stairs && terrain.flags.has(TerrainCharacteristics::STAIRS)) {
                    notice = false;
                } else if (is_damaging && can_ignore_damaging_terrain(creature, terrain)) {
                    notice = false;
                }
            }

            if (notice) {
                return true;
            }

            inv = false;
        }

        if (!inv && see_wall(creature, Direction::self(), pos)) {
            if (find_openarea) {
                if (i < 0) {
                    find_breakright = true;
                } else if (i > 0) {
                    find_breakleft = true;
                }
            }

            continue;
        }

        if (find_openarea) {
            continue;
        }

        if (!option) {
            option = new_dir;
            continue;
        }

        if (option2) {
            return true;
        }

        if (option != new_dir.rotated_45degree(-1)) {
            return true;
        }

        if (new_dir.is_diagonal()) {
            check_dir = new_dir.rotated_45degree(-2);
            option2 = new_dir;
            continue;
        }

        check_dir = new_dir.rotated_45degree(1);
        option2 = option;
        option = new_dir;
    }

    if (find_openarea) {
        for (int i = -max; i < 0; i++) {
            if (!see_wall(creature, prev_dir.rotated_45degree(i), p_pos)) {
                if (find_breakright) {
                    return true;
                }
            } else {
                if (find_breakleft) {
                    return true;
                }
            }
        }

        for (int i = max; i > 0; i--) {
            if (!see_wall(creature, prev_dir.rotated_45degree(i), p_pos)) {
                if (find_breakleft) {
                    return true;
                }
            } else {
                if (find_breakright) {
                    return true;
                }
            }
        }

        return see_wall(creature, find_current, p_pos);
    }

    if (!option) {
        return true;
    }

    if (!option2) {
        find_current = option;
        find_prevdir = option;
        return see_wall(creature, find_current, p_pos);
    } else if (!find_cut) {
        find_current = option;
        find_prevdir = option2;
        return see_wall(creature, find_current, p_pos);
    }

    const auto pos = creature.get_neighbor(option);
    if (!see_wall(creature, option, pos) || !see_wall(creature, check_dir, pos)) {
        if (see_nothing(creature, option, pos) && see_nothing(creature, option2, pos)) {
            find_current = option;
            find_prevdir = option2;
            return see_wall(creature, find_current, p_pos);
        }

        return true;
    }

    if (find_cut) {
        find_current = option2;
        find_prevdir = option2;
        return see_wall(creature, find_current, p_pos);
    }

    find_current = option;
    find_prevdir = option2;
    return see_wall(creature, find_current, p_pos);
}

/*!
 * @brief 継続的なダッシュ処理 /
 * Take one step along the current "run" path
 * @param creature	クリーチャーへの参照
 * @param dir 移動を試みる方向
 */
void run_step(CreatureEntity &creature, const Direction &dir)
{
    if (dir) {
        ignore_avoid_run = true;
        if (see_wall(creature, dir, creature.get_position())) {
            sound(SoundKind::HITWALL);
            msg_print(_("その方向には走れません。", "You cannot run in that direction."));
            disturb(creature, false, false);
            return;
        }

        run_init(creature, dir);
    } else {
        if (run_test(creature)) {
            disturb(creature, false, false);
            return;
        }
    }

    creature.set_running(creature.get_running() - 1);
    if (creature.get_running() <= 0) {
        return;
    }

    PlayerEnergy(creature).set_player_turn_energy(100);
    exe_movement(creature, find_current, false, false);
    if (creature.is_located_at_running_destination()) {
        creature.set_run_py(0);
        creature.set_run_px(0);
        disturb(creature, false, false);
    }
}
