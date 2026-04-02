#pragma once

#include "system/angband.h"

class CreatureEntity;
struct turn_flags;
void update_object_by_monster_movement(CreatureEntity &creature, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION ny, POSITION nx);
void monster_drop_carried_objects(CreatureEntity &creature, CreatureEntity &target);
