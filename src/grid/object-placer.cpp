#include "grid/object-placer.h"
#include "system/creature-entity.h"
#include "floor/floor-object.h"
#include "grid/grid.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"

/*!
 * @brief フロアの指定位置に生成階に応じた財宝オブジェクトの生成を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 配置したい座標
 * @return 生成に成功したらTRUEを返す。
 */
void place_gold(CreatureEntity &creature, const Pos2D &pos)
{
    auto &floor = *creature.current_floor_ptr;
    auto &grid = floor.get_grid(pos);
    if (!floor.contains(pos, FloorBoundary::OUTER_WALL_EXCLUSIVE)) {
        return;
    }
    if (!floor.can_drop_item_at(pos)) {
        return;
    }
    if (!grid.o_idx_list.empty()) {
        return;
    }

    auto item = floor.make_gold();
    const auto item_idx = floor.pop_empty_index_item();
    if (item_idx == 0) {
        return;
    }

    item.iy = pos.y;
    item.ix = pos.x;
    *floor.o_list[item_idx] = std::move(item);
    grid.o_idx_list.add(floor, item_idx);

    note_spot(creature, pos);
    lite_spot(creature, pos);
}

/*!
 * @brief フロアの指定位置に生成階に応じたベースアイテムの生成を行う
 * @param creature クリーチャーへの参照
 * @param pos 配置したい座標
 * @param mode オプションフラグ
 * @param restrict ベースアイテム制約関数。see BaseitemAllocationTable::set_restriction()
 */
void place_object(CreatureEntity &creature, const Pos2D &pos, uint32_t mode, BaseitemRestrict restrict)
{
    auto &floor = *creature.current_floor_ptr;
    auto &grid = floor.get_grid(pos);
    if (!floor.contains(pos, FloorBoundary::OUTER_WALL_EXCLUSIVE) || !floor.can_drop_item_at(pos) || !grid.o_idx_list.empty()) {
        return;
    }

    const auto item_idx = floor.pop_empty_index_item();
    if (item_idx == 0) {
        return;
    }

    auto item = make_object(creature, mode, restrict);
    if (!item) {
        return;
    }

    item->iy = pos.y;
    item->ix = pos.x;
    *floor.o_list[item_idx] = std::move(*item);
    grid.o_idx_list.add(floor, item_idx);

    note_spot(creature, pos);
    lite_spot(creature, pos);
}
