#pragma once

#include "object-enchant/item-feeling.h"
#include "system/angband.h"

class CreatureEntity;
class ItemEntity;
void apply_item_feeling(CreatureEntity &creature, INVENTORY_IDX i_idx, item_feel_type feel);
void sense_inventory1(CreatureEntity &creature);
void sense_inventory2(CreatureEntity &creature);
item_feel_type pseudo_value_check_heavy(const ItemEntity *o_ptr);
item_feel_type pseudo_value_check_light(const ItemEntity *o_ptr);
