﻿#pragma once

#include "system/angband.h"

class player_type;
typedef struct turn_flags turn_flags;
bool runaway_monster(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx);
