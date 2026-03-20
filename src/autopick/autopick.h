#pragma once

#include "system/angband.h"

class CreatureEntity;
class Grid;
void autopick_alter_item(CreatureEntity &creature, INVENTORY_IDX i_idx, bool destroy);
void autopick_delayed_alter(CreatureEntity &creature);
void autopick_pickup_items(CreatureEntity &creature, const Grid &grid);
