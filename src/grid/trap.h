#pragma once

#include "system/enums/terrain/trap.h"
#include "util/flag-group.h"
#include "util/point-2d.h"
#include <vector>

extern const std::vector<EnumClassFlagGroup<ChestTrapType>> chest_traps;

class CreatureEntity;
class FloorType;
void disclose_grid(CreatureEntity &creature, const Pos2D &pos);
void hit_trap(CreatureEntity &creature, bool break_trap);
