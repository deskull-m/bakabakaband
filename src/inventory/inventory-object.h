#pragma once

#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
class PlayerType;
void vary_item(CreatureEntity &creature, INVENTORY_IDX i_idx, ITEM_NUMBER num);
void inven_item_increase(CreatureEntity &creature, INVENTORY_IDX i_idx, ITEM_NUMBER num);
void inven_item_optimize(CreatureEntity &creature, INVENTORY_IDX i_idx);
void drop_from_inventory(CreatureEntity &creature, INVENTORY_IDX i_idx, ITEM_NUMBER amt);
void combine_pack(CreatureEntity &creature);
void reorder_pack(CreatureEntity &creature);
int16_t store_item_to_inventory(CreatureEntity &creature, ItemEntity *o_ptr);
bool check_store_item_to_inventory(const CreatureEntity &creature, const ItemEntity *o_ptr);
INVENTORY_IDX inven_takeoff(CreatureEntity &creature, INVENTORY_IDX i_idx, ITEM_NUMBER amt);
bool check_get_item(ItemEntity *o_ptr);
