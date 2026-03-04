#pragma once

#include "object/tval-types.h"
#include "system/angband.h"
#include <tl/optional.hpp>
#include <vector>

class CreatureEntity;
class FloorType;
class ItemTester;
bool is_ring_slot(int i);
tl::optional<short> get_tag_floor(const FloorType &floor, char tag, const std::vector<short> &floor_item_index);
tl::optional<short> get_tag(CreatureEntity &creature, char tag, BIT_FLAGS mode, const ItemTester &item_tester);
bool get_item_okay(CreatureEntity &creature, OBJECT_IDX i, const ItemTester &item_tester);
bool get_item_allow(CreatureEntity &creature, INVENTORY_IDX i_idx);
INVENTORY_IDX label_to_equipment(CreatureEntity &creature, int c);
INVENTORY_IDX label_to_inventory(CreatureEntity &creature, int c);
bool verify(CreatureEntity &creature, concptr prompt, INVENTORY_IDX i_idx);
std::string prepare_label_string(CreatureEntity &creature, BIT_FLAGS mode, const ItemTester &item_tester);
