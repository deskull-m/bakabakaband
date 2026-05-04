#pragma once

#include "object/item-tester-hooker.h"
#include "system/angband.h"
#include "system/baseitem/baseitem-allocation.h"
#include "util/point-2d.h"
#include <memory>
#include <string_view>
#include <tl/optional.hpp>
#include <utility>
#include <vector>

class ObjectIndexList;

class CreatureEntity;
class FloorType;
class ItemEntity;
class ItemTester;
tl::optional<ItemEntity> make_object(CreatureEntity &subject, BIT_FLAGS mode, BaseitemRestrict restrict = nullptr, tl::optional<int> rq_mon_level = tl::nullopt);
void delete_all_items_from_floor(CreatureEntity &creature, const Pos2D &pos);
void floor_item_increase(CreatureEntity &creature, INVENTORY_IDX i_idx, ITEM_NUMBER num);
void floor_item_optimize(CreatureEntity &creature, INVENTORY_IDX i_idx);
void delete_object_idx(CreatureEntity &creature, OBJECT_IDX o_idx);
void excise_object_idx(FloorType &floor, OBJECT_IDX o_idx);
void delete_items(CreatureEntity &creature, std::vector<OBJECT_IDX> delete_i_idx_list);
void delete_items(CreatureEntity &creature, ObjectIndexList &o_idx_list);
ObjectIndexList &get_o_idx_list_contains(FloorType &floor, OBJECT_IDX o_idx);
short drop_near(CreatureEntity &subject, ItemEntity &drop_item, const Pos2D &pos, bool show_drop_message = true);
void drop_ammo_near(CreatureEntity &creature, ItemEntity &drop_item, const Pos2D &pos, int destruction_chance);
void floor_item_charges(const FloorType &floor, INVENTORY_IDX i_idx);
void floor_item_describe(CreatureEntity &creature, INVENTORY_IDX i_idx);
std::pair<std::shared_ptr<ItemEntity>, short> choose_item(CreatureEntity &creature, std::string_view q, std::string_view s, BIT_FLAGS option, const ItemTester &item_tester = AllMatchItemTester());
