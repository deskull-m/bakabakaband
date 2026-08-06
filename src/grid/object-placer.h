#pragma once

#include "system/baseitem/baseitem-allocation.h"
#include "system/creature-entity.h"
#include "util/point-2d.h"
#include <cstdint>

void place_gold(CreatureEntity &creature, const Pos2D &pos);
void place_gold(CreatureEntity &creature, const Pos2D &pos, int drop_count);
void place_object(CreatureEntity &creature, const Pos2D &pos, uint32_t mode, BaseitemRestrict restrict = nullptr);
