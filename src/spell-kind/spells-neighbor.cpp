#include "spell-kind/spells-neighbor.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/floor-util.h"
#include "grid/grid.h"
#include "spell-kind/earthquake.h"
#include "system/creature-entity.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief ドア生成処理(プレイヤー中心に周囲1マス) / Hooks -- affect adjacent grids (radius 1 ball attack)
 * @param creature クリーチャーへの参照
 * @return 作用が実際にあった場合TRUEを返す
 */
bool door_creation(CreatureEntity &creature, POSITION y, POSITION x)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
    return project(creature, 0, 1, y, x, 0, AttributeType::MAKE_DOOR, flg).notice;
}

/*!
 * @brief トラップ生成処理(起点から周囲1マス)
 * @param creature クリーチャーへの参照
 * @param y 起点Y座標
 * @param x 起点X座標
 * @return 作用が実際にあった場合TRUEを返す
 */
bool trap_creation(CreatureEntity &creature, POSITION y, POSITION x)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
    return project(creature, 0, 1, y, x, 0, AttributeType::MAKE_TRAP, flg).notice;
}

/*!
 * @brief 森林生成処理(プレイヤー中心に周囲1マス)
 * @param creature クリーチャーへの参照
 * @return 作用が実際にあった場合TRUEを返す
 */
bool tree_creation(CreatureEntity &creature, POSITION y, POSITION x)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
    return project(creature, 0, 1, y, x, 0, AttributeType::MAKE_TREE, flg).notice;
}

/*!
 * @brief 壁生成処理(プレイヤー中心に周囲1マス)
 * @param creature クリーチャーへの参照
 * @return 作用が実際にあった場合TRUEを返す
 */
bool wall_creation(CreatureEntity &creature, POSITION y, POSITION x)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
    return project(creature, 0, 1, y, x, 0, AttributeType::MAKE_WALL, flg).notice;
}

/*!
 * @brief 魔法のルーン生成処理(プレイヤー中心に周囲1マス)
 * @param creature クリーチャーへの参照
 * @return 作用が実際にあった場合TRUEを返す
 */
bool create_rune_protection_area(CreatureEntity &creature, POSITION y, POSITION x)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM;
    return project(creature, 0, 1, y, x, 0, AttributeType::MAKE_RUNE_PROTECTION, flg).notice;
}

/*!
 * @brief 壁生成処理(プレイヤー中心に周囲1マス)
 * @param creature クリーチャーへの参照
 * @return 作用が実際にあった場合TRUEを返す
 */
bool wall_stone(CreatureEntity &creature)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
    bool dummy = project(creature, 0, 1, creature.y, creature.x, 0, AttributeType::STONE_WALL, flg).notice;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::FLOW);
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    return dummy;
}

/*!
 * @brief ドア破壊処理(プレイヤー中心に周囲1マス)
 * @param creature クリーチャーへの参照
 * @return 作用が実際にあった場合TRUEを返す
 */
bool destroy_doors_touch(CreatureEntity &creature)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
    return project(creature, 0, 1, creature.y, creature.x, 0, AttributeType::KILL_DOOR, flg).notice;
}

/*!
 * @brief トラップ解除処理(プレイヤー中心に周囲1マス)
 * @param creature クリーチャーへの参照
 * @return 作用が実際にあった場合TRUEを返す
 */
bool disarm_traps_touch(CreatureEntity &creature)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
    return project(creature, 0, 1, creature.y, creature.x, 0, AttributeType::KILL_TRAP, flg).notice;
}

/*!
 * @brief スリープモンスター処理(プレイヤー中心に周囲1マス)
 * @param creature クリーチャーへの参照
 * @return 作用が実際にあった場合TRUEを返す
 */
bool sleep_monsters_touch(CreatureEntity &creature)
{
    BIT_FLAGS flg = PROJECT_KILL | PROJECT_HIDE;
    return project(creature, 0, 1, creature.y, creature.x, creature.level, AttributeType::OLD_SLEEP, flg).notice;
}

/*!
 * @brief 死者復活処理(起点より周囲5マス)
 * @param creature クリーチャーへの参照
 * @param src_idx 術者モンスターID(0ならばプレイヤー)
 * @param y 起点Y座標
 * @param x 起点X座標
 * @return 作用が実際にあった場合TRUEを返す
 */
bool animate_dead(CreatureEntity &creature, MONSTER_IDX src_idx, POSITION y, POSITION x)
{
    BIT_FLAGS flg = PROJECT_ITEM | PROJECT_HIDE;
    return project(creature, src_idx, 5, y, x, 0, AttributeType::ANIM_DEAD, flg).notice;
}

/*!
 * @brief 周辺破壊効果(プレイヤー中心)
 * @param creature クリーチャーへの参照
 */
void wall_breaker(CreatureEntity &creature)
{
    const auto p_pos = creature.get_position();
    auto attempts = 1000;
    if (randint1(80 + creature.level) < 70) {
        Pos2D pos(0, 0);
        while (attempts--) {
            pos = scatter(*creature.get_floor(), p_pos, 4, PROJECT_NONE);
            if (!creature.get_floor()->has_terrain_characteristics(pos, TerrainCharacteristics::PROJECTION)) {
                continue;
            }

            if (!creature.is_located_at(pos)) {
                break;
            }
        }

        constexpr auto flags = PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
        project(creature, 0, 0, pos.y, pos.x, 20 + randint1(30), AttributeType::KILL_WALL, flags);
        return;
    }

    if (randint1(100) > 30) {
        earthquake(creature, creature.get_position(), 1);
        return;
    }

    const auto num = Dice::roll(5, 3);
    for (auto i = 0; i < num; i++) {
        Pos2D pos(0, 0);
        while (true) {
            pos = scatter(*creature.get_floor(), p_pos, 10, PROJECT_NONE);
            if (!creature.is_located_at(pos)) {
                break;
            }
        }

        constexpr auto flags = PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
        project(creature, 0, 0, pos.y, pos.x, 20 + randint1(30), AttributeType::KILL_WALL, flags);
    }
}
