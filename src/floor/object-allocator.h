#pragma once

#include "floor/floor-allocation-types.h"
#include "system/angband.h"
#include "system/creature-entity.h"

enum dap_type : int;
bool alloc_stairs(CreatureEntity &creature, FEAT_IDX feat, int num, int walls);
void alloc_object(CreatureEntity &creature, dap_type set, dungeon_allocation_type typ, int num);
void alloc_specific_floor_items(CreatureEntity &creature);
