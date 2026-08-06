#pragma once

#include "monster-floor/monster-movement-direction-list.h"
#include "system/angband.h"
#include "util/point-2d.h"
#include <tl/optional.hpp>
#include <utility>

class Direction;
class CreatureEntity;
class MonsterSweepGrid {
public:
    MonsterSweepGrid(CreatureEntity *creature_ptr, MONSTER_IDX m_idx);
    CreatureEntity *creature_ptr;
    MONSTER_IDX m_idx;
    tl::optional<MonsterMovementDirectionList> get_movable_grid();
};
