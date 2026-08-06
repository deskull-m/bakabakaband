#pragma once

#include "system/angband.h"

#include "object-enchant/tr-flags.h"

class CreatureEntity;
class ItemEntity;
bool is_active_torch(ItemEntity *o_ptr);
void torch_flags(ItemEntity *o_ptr, TrFlags &flags);
void torch_lost_fuel(ItemEntity *o_ptr);
void update_lite_radius(CreatureEntity &creature);
void update_lite(CreatureEntity &creature);
