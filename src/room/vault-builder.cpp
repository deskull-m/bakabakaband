#include "room/vault-builder.h"
#include "floor/floor-generator-util.h"
#include "floor/floor-util.h"
#include "game-option/cheat-options.h"
#include "grid/object-placer.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "system/creature-entity.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "view/display-messages.h"

/*
 * Grid based version of "creature_bold()"
 */
static bool player_grid(const CreatureEntity &creature, const Grid &grid)
{
    return &grid == &creature.get_floor()->grid_array[creature.y][creature.x];
}

/*
 * Grid based version of "cave_empty_bold()"
 */
static bool is_cave_empty_grid(const CreatureEntity &creature, const Grid &grid)
{
    bool is_empty_grid = grid.has(TerrainCharacteristics::PLACE);
    is_empty_grid &= !grid.has_monster();
    is_empty_grid &= !player_grid(creature, grid);
    return is_empty_grid;
}

/*!
 * @brief 特殊な部屋地形向けにモンスターを配置する
 * @param creature クリーチャーへの参照
 * @param pos_center 配置したい中心座標
 * @param num 配置したいモンスターの数
 */
void vault_monsters(CreatureEntity &creature, const Pos2D &pos_center, int num)
{
    auto &floor = *creature.get_floor();
    for (auto k = 0; k < num; k++) {
        for (auto i = 0; i < 9; i++) {
            const auto d = 1;
            const auto pos = scatter(floor, pos_center, d, 0);
            auto &grid = floor.get_grid(pos);
            if (!is_cave_empty_grid(creature, grid)) {
                continue;
            }

            floor.monster_level = floor.base_level + 2;
            const auto has_placed = place_random_monster(creature, pos.y, pos.x, PM_ALLOW_SLEEP | PM_ALLOW_GROUP);
            floor.monster_level = floor.base_level;
            if (has_placed) {
                break;
            }
        }
    }
}

/*!
 * @brief 特殊な部屋向けに各種アイテムを配置する
 * @param creature クリーチャーへの参照
 * @param pos_center 配置したい中心座標
 * @param num 配置したい数
 */
void vault_objects(CreatureEntity &creature, const Pos2D &pos_center, int num)
{
    auto &floor = *creature.get_floor();
    for (; num > 0; --num) {
        Pos2D pos = pos_center;
        int dummy = 0;
        for (int i = 0; i < 11; ++i) {
            while (dummy < SAFE_MAX_ATTEMPTS) {
                pos.y = rand_spread(pos_center.y, 2);
                pos.x = rand_spread(pos_center.x, 3);
                dummy++;
                if (!floor.contains(pos, FloorBoundary::OUTER_WALL_EXCLUSIVE)) {
                    continue;
                }

                break;
            }

            if (dummy >= SAFE_MAX_ATTEMPTS && cheat_room) {
                msg_print(_("警告！地下室のアイテムを配置できません！", "Warning! Could not place vault object!"));
            }

            const auto &grid = floor.get_grid(pos);
            if (!grid.is_floor() || !grid.o_idx_list.empty()) {
                continue;
            }

            if (evaluate_percent(75)) {
                place_object(creature, pos, 0);
            } else {
                place_gold(creature, pos);
            }

            break;
        }
    }
}

/*!
 * @brief 特殊な部屋向けに各種アイテムを配置する
 * @param pos_center トラップを配置したいマスの中心座標
 * @param distribution 配置分散
 */
static void vault_trap_aux(FloorType &floor, const Pos2D &pos_center, const Pos2DVec &distribution)
{
    Pos2D pos = pos_center;
    auto dummy = 0;
    for (auto count = 0; count <= 5; count++) {
        while (dummy < SAFE_MAX_ATTEMPTS) {
            pos.y = rand_spread(pos_center.y, distribution.y);
            pos.x = rand_spread(pos_center.x, distribution.x);
            dummy++;
            if (!floor.contains(pos, FloorBoundary::OUTER_WALL_EXCLUSIVE)) {
                continue;
            }
            break;
        }

        if (dummy >= SAFE_MAX_ATTEMPTS && cheat_room) {
            msg_print(_("警告！地下室のトラップを配置できません！", "Warning! Could not place vault trap!"));
        }

        const auto &grid = floor.get_grid(pos);
        if (!grid.is_floor() || !grid.o_idx_list.empty() || grid.has_monster()) {
            continue;
        }

        floor.place_trap_at(pos);
        break;
    }
}

/*!
 * @brief 特殊な部屋向けに各種アイテムを配置する
 * @param creature クリーチャーへの参照
 * @param pos_center トラップを配置したいマスの中心座標
 * @param distribution 配置分散
 * @param num 配置したいトラップの数
 * @todo rooms-normal からしか呼ばれていない、要調整
 */
void vault_traps(FloorType &floor, const Pos2D &pos_center, const Pos2DVec &distribution, int num)
{
    for (int i = 0; i < num; i++) {
        vault_trap_aux(floor, pos_center, distribution);
    }
}
