#pragma once

#include "system/creature-entity.h"
#include "util/point-2d.h"

class DungeonData;
struct dt_type;
bool build_tunnel(CreatureEntity &creature, DungeonData *dd_ptr, dt_type *dt_ptr, const Pos2D &pos_start, const Pos2D &pos_end);
bool build_tunnel2(CreatureEntity &creature, DungeonData *dd_ptr, const Pos2D &pos_start, const Pos2D &pos_end, int type, int cutoff);
