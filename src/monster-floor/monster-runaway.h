#pragma once

#include "system/angband.h"

class CreatureEntity;
struct turn_flags;
bool runaway_monster(CreatureEntity &creature, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx);
