#pragma once

#include "object-enchant/item-feeling.h"

class CreatureEntity;
class ItemEntity;
void sense_inventory1(CreatureEntity &creature);
void sense_inventory2(CreatureEntity &creature);
item_feel_type pseudo_value_check_heavy(const ItemEntity *o_ptr);
item_feel_type pseudo_value_check_light(const ItemEntity *o_ptr);
