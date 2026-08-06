#pragma once

#include "system/angband.h"

class CreatureEntity;
struct saved_floor_type;
void init_saved_floors(bool force);
void clear_saved_floor_files(CreatureEntity &creature);
saved_floor_type *get_sf_ptr(FLOOR_IDX floor_id);
void kill_saved_floor(CreatureEntity &creature, saved_floor_type *sf_ptr);
FLOOR_IDX get_unused_floor_id(CreatureEntity &creature);
void precalc_cur_num_of_pet();
extern FLOOR_IDX max_floor_id;
