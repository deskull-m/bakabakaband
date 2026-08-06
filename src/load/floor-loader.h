#pragma once

#include "system/angband.h"

class CreatureEntity;
struct saved_floor_type;
errr rd_saved_floor(CreatureEntity &creature, saved_floor_type *sf_ptr);
bool load_floor(CreatureEntity &creature, saved_floor_type *sf_ptr, BIT_FLAGS mode);
