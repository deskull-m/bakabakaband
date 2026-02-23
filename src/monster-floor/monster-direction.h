#pragma once

#include "monster-floor/monster-movement-direction-list.h"
#include "system/angband.h"
#include <tl/optional.hpp>

class Direction;
class CreatureEntity;
tl::optional<MonsterMovementDirectionList> decide_monster_movement_direction(CreatureEntity &creature, MONSTER_IDX m_idx, bool aware);
